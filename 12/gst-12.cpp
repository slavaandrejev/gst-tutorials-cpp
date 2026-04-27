#include <gst/gst.hpp>
#include <gstpbutils/gstpbutils.hpp>

#include <fmt/printf.h>

#include "fmtgststr.h"
#include "gnamespaces.h"

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    auto pipeline = Gst::parse_launch(
        "playbin uri=https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm");
    auto bus = pipeline.get_bus();

    auto is_live = false;

    // Start playing
    auto ret = pipeline.set_state(Gst::State::PLAYING_);
    if (Gst::StateChangeReturn::FAILURE_ == ret) {
        fmt::print(stderr, "Unable to set the pipeline to the playing state.\n");
        return -1;
    } else if (Gst::StateChangeReturn::NO_PREROLL_ == ret) {
        is_live = true;
    }

    auto loop = GLib::MainLoop::new_(nullptr, false);

    bus.add_signal_watch();
    bus.signal_message().connect([&](Gst::Bus, Gst::Message_Ref msg) {
        switch (msg.type_()) {
            case Gst::MessageType::ERROR_: {
                auto err = GLib::Error{};
                auto dbg = gi::cstring{};
                msg.parse_error(&err, &dbg);
                fmt::print(stderr, "Error: {}\n", err.what());

                pipeline.set_state(Gst::State::READY_);
                loop.quit();
                break;
            }
            case Gst::MessageType::EOS_: {
                pipeline.set_state(Gst::State::READY_);
                loop.quit();
                break;
            }
            case Gst::MessageType::BUFFERING_: {
                // If the stream is live, we do not care about buffering.
                if (is_live) {
                    break;
                }
                auto percent = msg.parse_buffering();
                fmt::print("Buffering ({:3}%)\r", percent);
                fflush(stdout);
                // Wait until buffering is complete before start/resume playing
                if (100 > percent) {
                    pipeline.set_state(Gst::State::PAUSED_);
                } else {
                    pipeline.set_state(Gst::State::PLAYING_);
                    fmt::print("\n");
                }
                break;
            }
            case Gst::MessageType::CLOCK_LOST_: {
                // Get a new clock
                pipeline.set_state(Gst::State::PAUSED_);
                pipeline.set_state(Gst::State::PLAYING_);
                break;
            }
            default: {
                break;
            }
        }
    });

    loop.run();
    pipeline.set_state(Gst::State::NULL_);
}
