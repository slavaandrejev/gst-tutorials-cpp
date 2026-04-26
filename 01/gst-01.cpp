#include <gst/gst.hpp>

#include <fmt/printf.h>

#include "gnamespaces.h"

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    auto pipeline = Gst::parse_launch("playbin uri=https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm");
    pipeline.set_state(Gst::State::PLAYING_);

    auto bus = pipeline.get_bus();
    auto msg = bus.timed_pop_filtered(
        Gst::CLOCK_TIME_NONE_
      , Gst::MessageType::ERROR_ | Gst::MessageType::EOS_
      );

    if (Gst::MessageType::ERROR_ == msg.type_()) {
        fmt::print(stderr,
            "An error occurred! Re-run with the GST_DEBUG=*:WARN "
            "environment variable set for more details.\n");
    }

    pipeline.set_state(Gst::State::NULL_);
}
