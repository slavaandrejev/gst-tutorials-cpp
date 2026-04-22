#include <gst/gst.hpp>

#include <fmt/printf.h>

#include "fmtgststr.h"
#include "gnamespaces.h"

auto constexpr chunk_size  = 1024;
auto constexpr num_samples = chunk_size / 2;
auto constexpr sample_rate = 48'000;

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    auto app_source     = gi::object_cast<Gst::AppSrc>(Gst::ElementFactory::make("appsrc", "audio_source"));
    auto tee            = Gst::ElementFactory::make("tee", "tee");
    auto audio_queue    = Gst::ElementFactory::make("queue", "audio_queue");
    auto audio_convert1 = gi::object_cast<Gst::BaseTransform>(Gst::ElementFactory::make("audioconvert", "audio_convert1"));
    auto audio_resample = gi::object_cast<Gst::BaseTransform>(Gst::ElementFactory::make("audioresample", "audio_resample"));
    auto audio_sink     = gi::object_cast<Gst::Bin>(Gst::ElementFactory::make("autoaudiosink", "audio_sink"));
    auto video_queue    = Gst::ElementFactory::make("queue", "video_queue");
    auto audio_convert2 = gi::object_cast<Gst::BaseTransform>(Gst::ElementFactory::make("audioconvert", "audio_convert2"));
    auto visual         = Gst::ElementFactory::make("wavescope", "visual");
    auto video_convert  = gi::object_cast<Gst::BaseTransform>(Gst::ElementFactory::make("videoconvert", "video_convert"));
    auto video_sink     = gi::object_cast<Gst::Bin>(Gst::ElementFactory::make("autovideosink", "video_sink"));
    auto app_queue      = Gst::ElementFactory::make("queue", "app_queue");
    auto app_sink       = gi::object_cast<Gst::AppSink>(Gst::ElementFactory::make("appsink", "app_sink"));

    auto pipeline = Gst::Pipeline::new_();

    if (!pipeline || !app_source || !tee || !audio_queue || !audio_convert1 || !audio_resample ||
        !audio_sink || !video_queue || !audio_convert2 || !visual || !video_convert || !video_sink ||
        !app_queue || !app_sink)
    {
        fmt::print(stderr, "Not all elements could be created.\n");
        return -1;
    }

    // Configure wavescope
    visual.set_properties("shader", 0, "style", 0);

    auto info = Gst::AudioInfo::new_();
    info.set_format(Gst::AudioFormat::S16_, sample_rate, 1, nullptr);
    app_source.set_properties("caps", info.to_caps(), "format", Gst::Format::TIME_);

    // Parameters for waveform generation
    auto a = 0.0f;
    auto b = 1.0f;
    auto c = 0.0f;
    auto d = 1.0f;
    auto generated_samples = guint64{};

    // Configure appsrc
    auto sourceid = guint{};
    // This signal callback triggers when appsrc needs data. Here, we add an
    // idle handler to the mainloop to start pushing data into the appsrc.
    app_source.signal_need_data().connect([&](Gst::AppSrc, guint length) {
        if (0 == sourceid) {
            fmt::print("\nStart feeding.\n");
            // This method is called by the idle GSource in the mainloop, to
            // feed CHUNK_SIZE bytes into appsrc. The idle handler is added to
            // the mainloop when appsrc requests us to start sending data
            // (need-data signal) and is removed when appsrc has enough data
            // (enough-data signal).
            sourceid = GLib::idle_add(GLib::PRIORITY_DEFAULT_IDLE_, [&]() {
                auto buffer = Gst::Buffer::new_allocate(chunk_size);
                auto timestamp = Gst::util_uint64_scale(generated_samples, Gst::SECOND_, sample_rate);
                buffer.pts_(timestamp);
                buffer.duration_(Gst::util_uint64_scale(num_samples, Gst::SECOND_, sample_rate));

                auto [success, map] = buffer.map(Gst::MapFlags::WRITE_);
                auto data = map.get_data();
                auto raw  = reinterpret_cast<gint16 *>(const_cast<guint8 *>(&data[0]));

                c += d;
                d -= c / 1000;
                auto freq = 1100 + 1000 * d;
                for (auto i = 0; num_samples > i; ++i) {
                    a += b;
                    b -= a / freq;
                    raw[i] = static_cast<gint16>(500 * a);
                }
                buffer.unmap(map);
                generated_samples += num_samples;
                auto ret = app_source.push_buffer(std::move(buffer));

                return Gst::FlowReturn::OK_ == ret;
            });
        }
    });
    // This callback triggers when appsrc has enough data and we can stop
    // sending. We remove the idle handler from the mainloop
    app_source.signal_enough_data().connect([&](Gst::AppSrc) {
        if (0 != sourceid) {
            fmt::print("Stop feeding.\n");
            GLib::Source::remove(sourceid);
            sourceid = 0;
        }
    });

    // Configure appsink
    app_sink.set_emit_signals(true);
    app_sink.set_caps(info.to_caps());
    app_sink.signal_new_sample().connect([&](Gst::AppSink) {
        auto sample = app_sink.pull_sample();
        if (sample) {
            // The only thing we do in this example is print a * to indicate a received buffer
            fmt::print("*");
            fflush(stdout);
        }
        return Gst::FlowReturn::OK_;
    });

    // Link all elements that can be automatically linked because they have "Always" pads
    pipeline.add(
        app_source, tee, audio_queue, audio_convert1, audio_resample
      , audio_sink, video_queue, audio_convert2, visual, video_convert
      , video_sink, app_queue, app_sink);
    if (
        !app_source.link(tee)
     || !audio_queue.link(audio_convert1, audio_resample, audio_sink)
     || !video_queue.link(audio_convert2, visual, video_convert, video_sink)
     || !app_queue.link(app_sink)
    )
    {
        fmt::print(stderr, "Elements could not be linked.\n");
        return -1;
    }

    // Manually link the Tee, which has "Request" pads
    auto tee_audio_pad = tee.request_pad_simple("src_%u");
    fmt::print("Obtained request pad {} for audio branch.\n", tee_audio_pad.get_name());
    auto queue_audio_pad =  audio_queue.get_static_pad("sink");
    auto tee_video_pad = tee.request_pad_simple("src_%u");
    fmt::print("Obtained request pad {} for video branch.\n", tee_video_pad.get_name());
    auto queue_video_pad =  video_queue.get_static_pad("sink");
    auto tee_app_pad = tee.request_pad_simple("src_%u");
    fmt::print("Obtained request pad {} for app branch.\n", tee_app_pad.get_name());
    auto queue_app_pad =  app_queue.get_static_pad("sink");
    if (
        Gst::PadLinkReturn::OK_ != tee_audio_pad.link(queue_audio_pad)
     || Gst::PadLinkReturn::OK_ != tee_video_pad.link(queue_video_pad)
     || Gst::PadLinkReturn::OK_ != tee_app_pad.link(queue_app_pad)
    )
    {
        fmt::print("Tee could not be linked\n");
        return -1;
    }

    auto main_loop = GLib::MainLoop::new_();
    auto bus  = pipeline.get_bus();
    bus.add_signal_watch();
    gi::signal_proxy<void(Gst::Bus, Gst::Message_Ref message)>(bus, "message::error").connect(
        [&](Gst::Bus, Gst::Message_Ref msg) {
            auto gerr = GLib::Error{};
            auto dbg  = gi::cstring{};
            msg.parse_error(&gerr, &dbg);
            fmt::print("Error received from element {}: {}\n", msg.src_().get_name(), gerr.what());
            fmt::print("Debugging information: {}\n", !dbg.empty() ? dbg.c_str() : "none");
            main_loop.quit();
        }
    );
    gi::signal_proxy<void(Gst::Bus, Gst::Message_Ref message)>(bus, "message::eos").connect(
        [&](Gst::Bus, Gst::Message_Ref msg) {
            fmt::print("\nEnd-Of-Stream reached.\n");
            main_loop.quit();
        }
    );

    GLib::signal_add(GLib::PRIORITY_HIGH_, SIGINT, [&]() {
        fmt::print("\nCaught SIGINT, sending EOS...\n");
        pipeline.send_event(Gst::Event::new_eos());
        return false;  // remove the source
    });

    pipeline.set_state(Gst::State::PLAYING_);
    main_loop.run();
    pipeline.set_state(Gst::State::NULL_);
}
