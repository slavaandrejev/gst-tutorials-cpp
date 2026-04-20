#include <fmt/format.h>

#include <glbinding/gl/gl.h>

#include <gtk/gtk.hpp>

#include "videorenderer.h"
#include "gst-05-win.h"

using namespace gl;

gi::ref_ptr<MainWindow> MainWindow::new_() {
    gi::register_type<VideoRenderer>();
    auto builder = Gtk::Builder::new_();
    if (builder.add_from_resource("/gst-05-win.ui")) {
        return builder.get_object_derived<MainWindow>("main_window");
    }
    return gi::ref_ptr<MainWindow>();
}

MainWindow::MainWindow(Gtk::ApplicationWindow cobj, Gtk::Builder builder)
  : Gtk::impl::ApplicationWindowImpl(cobj, this)
{
    gl_area = builder.get_object_derived<VideoRenderer>("video_container");

    play_button  = builder.get_object<Gtk::Button>("play_button");
    pause_button = builder.get_object<Gtk::Button>("pause_button");
    stop_button  = builder.get_object<Gtk::Button>("stop_button");
    slider       = builder.get_object<Gtk::Scale>("slider");
    streams_list = builder.get_object<Gtk::TextView>("streams_list");

    signal_realize().connect(gi::mem_fun(&MainWindow::on_realize, this));
    signal_unrealize().connect(gi::mem_fun(&MainWindow::on_unrealize, this));

    play_button.signal_clicked().connect([&](Gtk::Button) {
        playbin.set_state(Gst::State::PLAYING_);
    });
    pause_button.signal_clicked().connect([&](Gtk::Button) {
        playbin.set_state(Gst::State::PAUSED_);
    });
    stop_button.signal_clicked().connect([&](Gtk::Button) {
        playbin.set_state(Gst::State::READY_);
    });
    slider_update_signal_id = slider.signal_value_changed().connect([&](Gtk::Range) {
        auto value = slider.get_value();
        playbin.seek_simple(
            Gst::Format::TIME_
          , Gst::SeekFlags::FLUSH_ | Gst::SeekFlags::KEY_UNIT_
          , (gint64)(value * Gst::SECOND_)
          );
    });

    playbin.set_property("uri", "https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm");
}

// The window is ready to be drawn. It means it has OpenGL context we can pass
// to GStreamer. It means we can create GStreamer components.
void MainWindow::on_realize(Gtk::Widget) {
    if (auto glx_context = Gst::GLContext::get_current_gl_context(Gst::GLPlatform::GLX_)) {
        gst_gl_display = Gst::GLDisplayX11::new_(get_display().get_name());
        auto gl_api = Gst::GLContext::get_current_gl_api(Gst::GLPlatform::GLX_, nullptr, nullptr);
        gst_gl_context = Gst::GLContext::new_wrapped(gst_gl_display, glx_context, Gst::GLPlatform::GLX_, gl_api);
    } else {
        auto egl_context = eglGetCurrentContext();
        auto egl_display = eglGetCurrentDisplay();

        gst_gl_display = Gst::GLDisplayEGL::new_with_egl_display(egl_display);
        auto gl_api = Gst::GLContext::get_current_gl_api(Gst::GLPlatform::EGL_, nullptr, nullptr);
        gst_gl_context = Gst::GLContext::new_wrapped(gst_gl_display, (guintptr)egl_context, Gst::GLPlatform::EGL_, gl_api);
    }

    // Memorize the current thread inside the context. Otherwise, `fill_info`
    // below won't work.
    gst_gl_context.activate(true);
    // Fill information from the wrapped GL context.
    gst_gl_context.fill_info();

    g_return_if_fail(playbin);
    g_return_if_fail(glupload);
    g_return_if_fail(glcolorconvert);
    g_return_if_fail(capsfilter);
    g_return_if_fail(video_sink);
    g_return_if_fail(video_sink_bin);

    video_sink_bin.add(capsfilter, glupload, glcolorconvert, video_sink);

    if (!glupload.link(glcolorconvert, capsfilter, video_sink)) {
        throw std::runtime_error("Elements couldn't be linked");
    }
    capsfilter.set_property("caps", Gst::Caps::from_string("video/x-raw(memory:GLMemory),format=RGBA"));
    auto sink_pad = Gst::GhostPad::new_("sink", glupload.get_static_pad("sink"));
    video_sink_bin.add_pad(sink_pad);
    playbin.set_property("video-sink", video_sink_bin);

    gi::signal_proxy<void(Gst::Element, gint stream)>(playbin, "video-tags-changed").connect(
        gi::mem_fun(&MainWindow::on_tags, this)
    );
    gi::signal_proxy<void(Gst::Element, gint stream)>(playbin, "audio-tags-changed").connect(
        gi::mem_fun(&MainWindow::on_tags, this)
    );
    gi::signal_proxy<void(Gst::Element, gint stream)>(playbin, "text-tags-changed").connect(
        gi::mem_fun(&MainWindow::on_tags, this)
    );

    auto bus = playbin.get_bus();
    bus.add_signal_watch();
    bus.enable_sync_message_emission();

    gi::signal_proxy<void(Gst::Bus, Gst::Message_Ref message)>(bus, "sync-message::need-context").connect(
        gi::mem_fun(&MainWindow::on_need_context, this)
    );
    gi::signal_proxy<void(Gst::Bus, Gst::Message_Ref message)>(bus, "message::error").connect(
        gi::mem_fun(&MainWindow::on_error, this)
    );
    gi::signal_proxy<void(Gst::Bus, Gst::Message_Ref message)>(bus, "message::eos").connect(
        gi::mem_fun(&MainWindow::on_eos, this)
    );
    gi::signal_proxy<void(Gst::Bus, Gst::Message_Ref message)>(bus, "message::state-changed").connect(
        gi::mem_fun(&MainWindow::on_state_change, this)
    );
    gi::signal_proxy<void(Gst::Bus, Gst::Message_Ref message)>(bus, "message::application").connect(
        gi::mem_fun(&MainWindow::on_application, this)
    );

    video_sink.set_emit_signals(true);
    video_sink.signal_new_preroll().connect(gi::mem_fun(&MainWindow::new_preroll, this));
    video_sink.signal_new_sample().connect(gi::mem_fun(&MainWindow::new_sample, this));

    // Update video info after capabilities negotiation
    gi::signal_proxy<void(GLib::Object, GLib::ParamSpec)>(sink_pad, "notify::caps").connect([&](GLib::Object p, GLib::ParamSpec param_spec) {
        auto sink_pad = video_sink.get_static_pad("sink");
        auto caps     = sink_pad.get_current_caps();
        if (caps) {
            render_video_info = Gst::VideoInfo::new_from_caps(caps);
        }
    });

    gl_area->signal_get_texture().connect(gi::mem_fun(&MainWindow::on_get_texture, this));

    auto ret = playbin.set_state(Gst::State::PLAYING_);
    if (Gst::StateChangeReturn::FAILURE_ == ret) {
        throw std::runtime_error("Unable to set the pipeline to the playing state.");
    }

    time_callback_id = GLib::timeout_add(
        GLib::PRIORITY_DEFAULT_
      , 100
      , gi::mem_fun(&MainWindow::refresh_ui, this));
}

void MainWindow::on_unrealize(Gtk::Widget) {
    if (0 != time_callback_id) {
        GLib::Source::remove(time_callback_id);
        time_callback_id = 0;
    }
    if (playbin) {
        playbin.set_state(Gst::State::NULL_);
    }
}

// This function is called when GStreamer needs various contexts. We use it to
// pass OpenGL context from GTK to GStreamer.
void MainWindow::on_need_context(Gst::Bus, Gst::Message_Ref msg) {
    auto context_type = gi::cstring_v{};
    msg.parse_context_type(&context_type);
    if (GST_GL_DISPLAY_CONTEXT_TYPE == context_type) {
        auto display_context = Gst::Context::new_(GST_GL_DISPLAY_CONTEXT_TYPE, true);
        Gst::context_set_gl_display(display_context, gst_gl_display);
        auto src = gi::object_cast<Gst::Element>(msg.src_());
        src.set_context(display_context);
    } else if ("gst.gl.app_context" == context_type) {
        auto app_context = Gst::Context::new_("gst.gl.app_context", true);
        auto s = gst_context_writable_structure(app_context.gobj_());
        gst_structure_set(s, "context", GST_TYPE_GL_CONTEXT, gst_gl_context.gobj_(), nullptr);
        auto src = gi::object_cast<Gst::Element>(msg.src_());
        src.set_context(app_context);
    }
}

// This function is called when an error message is posted on the bus
void MainWindow::on_error(Gst::Bus, Gst::Message_Ref msg)
{
    auto gerr = GLib::Error{};
    auto dbg  = gi::cstring{};

    msg.parse_error(&gerr, &dbg);
    fmt::print(stderr, "Error received from element {}: {}\n", msg.src_().get_name(), gerr.what());
    fmt::print(stderr, "Debugging information: {}\n", !dbg.empty() ? dbg.c_str() : "none");

    playbin.set_state(Gst::State::READY_);
}

// This function is called when an End-Of-Stream message is posted on the bus.
// We just set the pipeline to READY (which stops playback)
void MainWindow::on_eos(Gst::Bus, Gst::Message_Ref msg) {
    fmt::print("End-Of-Stream reached.\n");

    playbin.set_state(Gst::State::READY_);
}

// This function is called when the pipeline changes states. We use it to keep
// track of the current state.
void MainWindow::on_state_change(Gst::Bus, Gst::Message_Ref msg) {
    if (msg.src_() == playbin) {
        auto [old_state, new_state, pending_state] = msg.parse_state_changed();
        state = new_state;
        fmt::print(
            "State set to {}\n"
          , Gst::StateNS_::get_name(new_state));
        if (Gst::State::READY_ == old_state && Gst::State::PAUSED_ == new_state) {
            refresh_ui();
        }
    }
}

// This function is called when an "application" message is posted on the bus.
// Here we retrieve the message posted by the tags_cb callback
void MainWindow::on_application(Gst::Bus, Gst::Message_Ref msg) {
    if (msg.get_structure().get_name() == "tags-changed") {
        analyze_streams();
    }
}

// This function is called when new metadata is discovered in the stream
void MainWindow::on_tags(Gst::Element, gint stream) {
    // We are possibly in a GStreamer working thread, so we notify the main
    // thread of this event through a message in the bus.
    playbin.post_message(
        Gst::Message::new_application(
            playbin, Gst::Structure::new_empty("tags-changed")));
}

// This function is called periodically to refresh the GUI
bool MainWindow::refresh_ui() {
    auto current = gint64{-1};

    // We do not want to update anything unless we are in the PAUSED or PLAYING states
    if (state < Gst::State::PAUSED_) {
        return true;
    }

    // If we didn't know it yet, query the stream duration
    if (!GST_CLOCK_TIME_IS_VALID(duration)) {
        if (!playbin.query_duration(Gst::Format::TIME_, &duration)) {
            fmt::print(stderr, "Could not query current duration.\n");
        } else {
            // Set the range of the slider to the clip duration, in SECONDS
            slider.set_range(0, duration / double(Gst::SECOND_));
        }
    }

    if (playbin.query_position(Gst::Format::TIME_, &current)) {
        // Block the "value-changed" signal, so the slider_cb function is not
        // called (which would trigger a seek the user has not requested)
        GLib::signal_handler_block(slider, slider_update_signal_id);
        // Set the position of the slider to the current pipeline position, in SECONDS
        slider.set_value(current / double(Gst::SECOND_));
        // Re-enable the signal
        GLib::signal_handler_unblock(slider, slider_update_signal_id);
    }

    return true;
}

// Extract metadata from all the streams and write it to the text widget in the GUI
void MainWindow::analyze_streams() {
    auto text = streams_list.get_buffer();
    text.set_text("", -1);

    const auto n_video = playbin.get_property("n-video").get_value<gint>();
    const auto n_audio = playbin.get_property("n-audio").get_value<gint>();
    const auto n_text  = playbin.get_property("n-text").get_value<gint>();

    for (auto i = 0; n_video > i; ++i) {
        auto tags = gi::signal_proxy<Gst::TagList(Gst::Element, gint stream)>(playbin, "get-video-tags").emit(gint(i));
        if (tags) {
            text.insert_at_cursor(fmt::format("video stream {}:\n", i), -1);
            auto [success, str] = tags.get_string(Gst::TAG_VIDEO_CODEC_);
            if (str.empty()) {
                str = "unknown";
            }
            text.insert_at_cursor(fmt::format("  codec: {}\n", str), -1);
        }
    }

    for (auto i = 0; n_audio > i; ++i) {
        auto tags = gi::signal_proxy<Gst::TagList(Gst::Element, gint stream)>(playbin, "get-audio-tags").emit(gint(i));
        if (tags) {
            text.insert_at_cursor(fmt::format("\naudio stream {}:\n", i), -1);
            auto [success, str] = tags.get_string(Gst::TAG_AUDIO_CODEC_);
            if (success) {
                text.insert_at_cursor(fmt::format("  codec: {}\n", str), -1);
            }
            std::tie(success, str) = tags.get_string(Gst::TAG_LANGUAGE_CODE_);
            if (success) {
                text.insert_at_cursor(fmt::format("  language: {}\n", str), -1);
            }
            auto rate = guint{};
            std::tie(success, rate) = tags.get_uint(Gst::TAG_BITRATE_);
            if (success) {
                text.insert_at_cursor(fmt::format("  bitrate: {}\n", rate), -1);
            }
        }
    }

    for (auto i = 0; n_text > i; ++i) {
        auto tags = gi::signal_proxy<Gst::TagList(Gst::Element, gint stream)>(playbin, "get-text-tags").emit(gint(i));
        if (tags) {
            text.insert_at_cursor(fmt::format("\nsubtitle stream {}:\n", i), -1);
            auto [success, str] = tags.get_string(Gst::TAG_LANGUAGE_CODE_);
            if (success) {
                text.insert_at_cursor(fmt::format("  language: {}\n", str), -1);
            }
        }
    }
}

Gst::FlowReturn MainWindow::new_preroll(Gst::AppSink) {
    auto sample = video_sink.pull_preroll();
    auto buffer = sample.get_buffer();

    store_new_frame(buffer);

    return Gst::FlowReturn::OK_;
}

Gst::FlowReturn MainWindow::new_sample(Gst::AppSink) {
    auto sample = video_sink.pull_sample();
    auto buffer = sample.get_buffer();

    store_new_frame(buffer);

    return Gst::FlowReturn::OK_;
}

void MainWindow::store_new_frame(Gst::Buffer_Ref buffer) {
    if (mapped[write_slot]) {
        gst_video_frame_unmap(&frames[write_slot]);
        mapped[write_slot] = false;
    }
    gst_video_frame_map(&frames[write_slot], render_video_info.gobj_(), buffer.gobj_(), GstMapFlags(GST_MAP_READ | GST_MAP_GL));
    mapped[write_slot] = true;

    write_slot = shared.exchange(write_slot);
    new_frame.store(true, std::memory_order_release);
}

gint MainWindow::on_get_texture(GLib::Object) {
    if (new_frame.exchange(false, std::memory_order_acquire)) {
        read_slot = shared.exchange(read_slot);
    }
    if (mapped[read_slot]) {
        return *(guint *)frames[read_slot].data[0];
    }

    return -1;
}
