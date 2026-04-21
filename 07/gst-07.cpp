#include <gst/gst.hpp>

#include <fmt/printf.h>

#include "fmtgststr.h"
#include "gnamespaces.h"

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    auto audio_source   = Gst::ElementFactory::make("audiotestsrc", "audio_source");
    auto tee            = Gst::ElementFactory::make("tee", "tee");
    auto audio_queue    = Gst::ElementFactory::make("queue", "audio_queue");
    auto audio_convert  = gi::object_cast<Gst::BaseTransform>(Gst::ElementFactory::make("audioconvert", "audio_convert"));
    auto audio_resample = gi::object_cast<Gst::BaseTransform>(Gst::ElementFactory::make("audioresample", "audio_resample"));
    auto audio_sink     = gi::object_cast<Gst::Bin>(Gst::ElementFactory::make("autoaudiosink", "audio_sink"));
    auto video_queue    = Gst::ElementFactory::make("queue", "video_queue");
    auto visual         = Gst::ElementFactory::make("wavescope", "visual");
    auto video_convert  = gi::object_cast<Gst::BaseTransform>(Gst::ElementFactory::make("videoconvert", "csp"));
    auto video_sink     = gi::object_cast<Gst::Bin>(Gst::ElementFactory::make("autovideosink", "video_sink"));

    auto pipeline = Gst::Pipeline::new_("test-pipeline");

    if (!pipeline || !audio_source || !tee || !audio_queue || !audio_convert || !audio_resample ||
        !audio_sink || !video_queue || !visual || !video_convert || !video_sink)
    {
        fmt::print(stderr, "Not all elements could be created.\n");
        return -1;
    }

    audio_source.set_property("freq", 215.0f);
    visual.set_properties("shader", 0, "style", 1);

    // Link all elements that can be automatically linked because they have "Always" pads
    pipeline.add(audio_source, tee, audio_queue, audio_convert, audio_resample, audio_sink,
      video_queue, visual, video_convert, video_sink);
    if (
        !audio_source.link(tee)
     || !audio_queue.link(audio_convert, audio_resample, audio_sink)
     || !video_queue.link(visual, video_convert, video_sink))
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
    if (
        Gst::PadLinkReturn::OK_ != tee_audio_pad.link(queue_audio_pad)
     || Gst::PadLinkReturn::OK_ != tee_video_pad.link(queue_video_pad)
    )
    {
        fmt::print("Tee could not be linked\n");
        return -1;
    }

    pipeline.set_state(Gst::State::PLAYING_);

    auto bus  = pipeline.get_bus();
    auto msg = bus.timed_pop_filtered(
        Gst::CLOCK_TIME_NONE_
      , Gst::MessageType::ERROR_ | Gst::MessageType::EOS_
      );
    pipeline.set_state(Gst::State::NULL_);
}
