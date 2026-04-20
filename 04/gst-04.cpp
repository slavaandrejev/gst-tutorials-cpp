#include <gst/gst.hpp>

#include <fmt/printf.h>

#include "fmtgststr.h"
#include "gnamespaces.h"

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    auto playing      = false;
    auto terminate    = false;
    auto seek_enabled = false;
    auto seek_done    = false;
    auto duration     = gint64{};

    auto playbin = Gst::ElementFactory::make("playbin", "playbin");

    if (!playbin) {
        fmt::print(stderr, "Not all elements could be created.\n");
        return -1;
    }

    playbin.set_property("uri", "https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm");
    auto ret = playbin.set_state(Gst::State::PLAYING_);
    if (Gst::StateChangeReturn::FAILURE_ == ret) {
        fmt::print(stderr, "Unable to set the pipeline to the playing state.\n");
        return -1;
    }

    auto bus = playbin.get_bus();
    do {
        auto msg = bus.timed_pop_filtered(
            100 * Gst::MSECOND_
          , Gst::MessageType::STATE_CHANGED_ |
            Gst::MessageType::ERROR_ |
            Gst::MessageType::EOS_ |
            Gst::MessageType::DURATION_CHANGED_
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
                    fmt::print("\nEnd-Of-Stream reached.\n");
                    terminate = true;
                    break;
                }
                case Gst::MessageType::DURATION_CHANGED_: {
                    // The duration has changed, mark the current one as invalid
                    duration = Gst::CLOCK_TIME_NONE_;
                    break;
                }
                case Gst::MessageType::STATE_CHANGED_: {
                    if (msg.src_() == playbin) {
                        auto [old_state, new_state, pending_state] = msg.parse_state_changed();
                        fmt::print(
                            "Pipeline state changed from {} to {}:\n"
                          , Gst::StateNS_::get_name(old_state)
                          , Gst::StateNS_::get_name(new_state));
                        // Remember whether we are in the PLAYING state or not
                        playing = Gst::State::PLAYING_ == new_state;
                        if (playing) {
                            // We just moved to PLAYING. Check if seeking is possible
                            auto start = gint64{};
                            auto end   = gint64{};
                            auto query = Gst::Query::new_seeking(Gst::Format::TIME_);
                            if (playbin.query(query)) {
                                query.parse_seeking(nullptr, &seek_enabled, &start, &end);
                                if (seek_enabled) {
                                    fmt::print(
                                        "Seeking is ENABLED from {}:{:02}:{:02}.{:09} to {}:{:02}:{:02}.{:09}\n"
                                      , GST_TIME_ARGS(start), GST_TIME_ARGS(end));
                                } else {
                                    fmt::print("Seeking is DISABLED for this stream.\n");
                                }
                            } else {
                                fmt::print(stderr, "Seeking query failed.");
                            }
                        }
                    }
                    break;
                }
                default: {
                    fmt::print(stderr, "Unexpected message received.\n");
                    break;
                }
            }
        } else {
            // We got no message, this means the timeout expired
            if (playing) {
                auto current = gint64{-1};
                if (!playbin.query_position(Gst::Format::TIME_, &current)) {
                    fmt::print(stderr, "Could not query current position.\n");
                }
                // If we didn't know it yet, query the stream duration
                if (!GST_CLOCK_TIME_IS_VALID(duration)) {
                    if (!playbin.query_duration(Gst::Format::TIME_, &duration)) {
                        fmt::print(stderr, "Could not query current duration.\n");
                    }
                }
                // Print current position and total duration
                fmt::print(
                    "Position {}:{:02}:{:02}.{:09} / {}:{:02}:{:02}.{:09}\r"
                  , GST_TIME_ARGS(current), GST_TIME_ARGS(duration));
                fflush(stdout);
                // If seeking is enabled, we have not done it yet, and the time is right, seek
                if (seek_enabled && !seek_done  && 10 * Gst::SECOND_ < current) {
                    fmt::print("\nReached 10s, performing seek...\n");
                    playbin.seek_simple(
                        Gst::Format::TIME_
                      , Gst::SeekFlags::FLUSH_ |
                        Gst::SeekFlags::KEY_UNIT_
                      , 30 * Gst::SECOND_);
                    seek_done = true;
                }
            }
        }
    } while (!terminate);

    playbin.set_state(Gst::State::NULL_);
}
