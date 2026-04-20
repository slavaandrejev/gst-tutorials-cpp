#include <gst/gst.hpp>

#include <fmt/printf.h>

#include "fmtgststr.h"
#include "gnamespaces.h"

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    auto source   = Gst::ElementFactory::make("uridecodebin", "source");
    auto convert  = Gst::ElementFactory::make("audioconvert", "convert");
    auto resample = Gst::ElementFactory::make("audioresample", "resample");
    auto sink     = Gst::ElementFactory::make("autoaudiosink", "sink");

    auto pipeline = Gst::Pipeline::new_("test-pipeline");

    if (!pipeline || !source || !convert || !resample || !sink) {
        fmt::print(stderr, "Not all elements could be created.\n");
        return -1;
    }

    pipeline.add(source);
    pipeline.add(convert);
    pipeline.add(resample);
    pipeline.add(sink);

    if (!convert.link(resample, sink)) {
        fmt::print(stderr, "Elements could not be linked.\n");
    }

    source.set_property("uri", "https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm");
    source.signal_pad_added().connect([&](Gst::Element src, Gst::Pad new_pad) {
        auto sink_pad = convert.get_static_pad("sink");

        fmt::print("Received new pad '{}' from '{}':\n", new_pad.get_name(), src.get_name());

        if (sink_pad.is_linked()) {
            fmt::print("We are already linked. Ignoring.\n");
            return;
        }

        auto new_pad_caps   = new_pad.get_current_caps();
        auto new_pad_struct = new_pad_caps.get_structure(0);
        auto new_pad_type   = new_pad_struct.get_name();
        if (!GLib::str_has_suffix(new_pad_type, "audio/x-raw")) {
            fmt::print("It has type '{}' which is not raw audio. Ignoring.\n", new_pad_type);
        }

        auto ret = new_pad.link(sink_pad);
        if (Gst::PadLinkReturn::OK_ > ret) {
            fmt::print("Type is '{}' but link failed.\n", new_pad_type);
        } else {
            fmt::print("Link succeeded (type '{}').\n", new_pad_type);
        }
    });

    auto ret = pipeline.set_state(Gst::State::PLAYING_);
    if (Gst::StateChangeReturn::FAILURE_ == ret) {
        fmt::print(stderr, "Unable to set the pipeline to the playing state.\n");
        return -1;
    }

    auto terminate = false;
    auto bus = pipeline.get_bus();
    do {
        auto msg = bus.timed_pop_filtered(
            Gst::CLOCK_TIME_NONE_
          , Gst::MessageType::STATE_CHANGED_ | Gst::MessageType::ERROR_ | Gst::MessageType::EOS_
          );

        if (msg) {
            switch (msg.type_()) {
                case Gst::MessageType::ERROR_: {
                    auto gerr = GLib::Error{};
                    auto dbg  = gi::cstring{};

                    msg.parse_error(&gerr, &dbg);
                    fmt::print(stderr, "Error received from element {}: {}\n", msg.src_().get_name(), gerr.what());
                    fmt::print(stderr, "Debugging information: {}\n", !dbg.empty() ? dbg.c_str() : "none");
                    terminate = true;
                    break;
                }
                case Gst::MessageType::EOS_: {
                    fmt::print("End-Of-Stream reached.\n");
                    terminate = true;
                    break;
                }
                case Gst::MessageType::STATE_CHANGED_: {
                    if (msg.src_() == pipeline) {
                        auto [old_state, new_state, pending_state] = msg.parse_state_changed();
                        fmt::print(
                            "Pipeline state changed from {} to {}:\n"
                          , Gst::StateNS_::get_name(old_state)
                          , Gst::StateNS_::get_name(new_state));
                    }
                    break;
                }
                default: {
                    fmt::print(stderr, "Unexpected message received.\n");
                    break;
                }
            }
        }
    } while (!terminate);

    pipeline.set_state(Gst::State::NULL_);
}
