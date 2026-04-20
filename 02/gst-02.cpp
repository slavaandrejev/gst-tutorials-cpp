#include <gst/gst.hpp>

#include <fmt/printf.h>

#include "fmtgststr.h"
#include "gnamespaces.h"

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    auto source = Gst::ElementFactory::make("videotestsrc", "source");
    auto sink   = Gst::ElementFactory::make("autovideosink", "sink");

    auto pipeline = Gst::Pipeline::new_("test-pipeline");

    if (!pipeline || !source || !sink) {
        fmt::print(stderr, "Not all elements could be created.\n");
        return -1;
    }

    pipeline.add(source);
    pipeline.add(sink);

    if (!source.link(sink)) {
        fmt::print(stderr, "Elements could not be linked.\n");
    }

    source.set_property("pattern", 0);

    auto ret = pipeline.set_state(Gst::State::PLAYING_);
    if (Gst::StateChangeReturn::FAILURE_ == ret) {
        fmt::print(stderr, "Unable to set the pipeline to the playing state.\n");
    }

    auto bus = pipeline.get_bus();
    auto msg = bus.timed_pop_filtered(
        Gst::CLOCK_TIME_NONE_
      , Gst::MessageType::ERROR_ | Gst::MessageType::EOS_
      );
    if (msg) {
        switch (msg.type_()) {
            case Gst::MessageType::ERROR_: {
                auto gerr = GLib::Error{};
                auto dbg  = gi::cstring{};

                msg.parse_error(&gerr, &dbg);
                fmt::print(stderr, "Error received from element {}: {}\n", msg.src_().get_name(), gerr.what());
                fmt::print(stderr, "Debugging information: {}\n", !dbg.empty() ? dbg.c_str() : "none");
                break;
            }
            case Gst::MessageType::EOS_: {
                fmt::print("End-Of-Stream reached.\n");
                break;
            }
            default: {
                fmt::print(stderr, "Unexpected message received.\n");
                break;
            }
        }
    }

    pipeline.set_state(Gst::State::NULL_);
}
