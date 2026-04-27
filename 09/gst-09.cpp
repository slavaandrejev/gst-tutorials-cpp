#include <gst/gst.hpp>
#include <gstpbutils/gstpbutils.hpp>

#include <fmt/printf.h>

#include "fmtgststr.h"
#include "gnamespaces.h"

// Print information regarding a stream
void print_stream_info(Gst::DiscovererStreamInfo info, int depth) {
    auto caps = info.get_caps();

    auto desc = gi::cstring{};
    if (caps) {
        if (caps.is_fixed()) {
            desc = Gst::pb_utils_get_codec_description(caps);
        } else {
            desc = caps.to_string();
        }
    }
    fmt::print("{:{}}{}: {}\n", "", 2 * depth, info.get_stream_type_nick(), desc);

    auto tags = info.get_tags();
    if (tags) {
        fmt::print ("{:{}}Tags:\n", "", 2 * (depth + 1));
        tags.foreach([&](Gst::TagList_Ref tags, const gi::cstring_v tag) {
            auto [successs, value] = Gst::TagList::copy_value(tags, tag);
            auto str = gi::cstring{};
            if (G_VALUE_HOLDS_STRING(value.gobj_())) {
                str = value.get_value<gi::cstring>();
            } else {
                str = Gst::value_serialize(value);
            }
            fmt::print ("{:{}}{}: {}\n", "", 2 * (depth + 2), Gst::tag_get_nick(tag), str);
        });
    }
}

// Print information regarding a stream and its substreams, if any
void print_topology(Gst::DiscovererStreamInfo info, int depth) {
    if (!info) {
        return;
    }

    print_stream_info(info, depth);
    auto next = info.get_next();
    if (next) {
        print_topology(next, depth + 1);
    } else if (GST_IS_DISCOVERER_CONTAINER_INFO(info.gobj_())) {
        auto streams = gi::object_cast<Gst::DiscovererContainerInfo>(info).get_streams();
        for (auto &&s : streams) {
            print_topology(s, depth + 1);
        }
    }
}

int main(int argc, char *argv[]) {
    auto err = GLib::Error{};
    auto uri = gi::cstring{"https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm"};

    // if a URI was provided, use it instead of the default one
    if (1 < argc) {
        uri = argv[1];
    }

    gst_init(&argc, &argv);

    fmt::print("Discovering '{}'\n", uri);

    auto discoverer = Gst::Discoverer::new_(5 * Gst::SECOND_, &err);
    if (!discoverer) {
        fmt::print(stderr, "Error creating discoverer instance: {}\n", err.message_());
        return -1;
    }

    // This function is called every time the discoverer has information
    // regarding one of the URIs we provided.
    discoverer.signal_discovered().connect([&](Gst::Discoverer, Gst::DiscovererInfo info, GLib::Error_Ref err) {
        auto uri    = info.get_uri();
        auto result = info.get_result();

        switch (result)
        {
            case Gst::DiscovererResult::URI_INVALID_: {
                fmt::print("Invalid URI '{}'\n", uri);
                break;
            }
            case Gst::DiscovererResult::ERROR_: {
                fmt::print("Discoverer error: {}\n", err.message_());
                break;
            }
            case Gst::DiscovererResult::TIMEOUT_: {
                fmt::print("Timeout\n");
                break;
            }
            case Gst::DiscovererResult::BUSY_: {
                fmt::print("Busy\n");
                break;
            }
            case Gst::DiscovererResult::MISSING_PLUGINS_: {
                auto strs = info.get_missing_elements_installer_details();
                fmt::print("Missing plugins: ");
                auto first_print = true;
                for (auto &&s : strs) {
                    if (!first_print) {
                        fmt::print(", ");
                    } else {
                        first_print = false;
                    }
                    fmt::print("{}", s);
                }
                fmt::print("\n");
                break;
            }
            case Gst::DiscovererResult::OK_: {
                fmt::print("Discovered '{}'\n", uri);
                break;
            }
        }

        if (Gst::DiscovererResult::OK_ != result) {
            fmt::print(stderr, "This URI cannot be played\n");
            return;
        }

        // If we got no error, show the retrieved information
        fmt::print("\nDuration: {}:{:02}:{:02}.{:09}\n", GST_TIME_ARGS(info.get_duration()));
        fmt::print("Seekable: {}\n", info.get_seekable() ? "yes" : "no");
        fmt::print("\n");
        auto sinfo = info.get_stream_info();
        if (!sinfo) {
            return;
        }
        fmt::print("Stream information:\n");
        print_topology(sinfo, 1);
        fmt::print("\n");
    });

    auto loop = GLib::MainLoop::new_();

    // This function is called when the discoverer has finished examining all
    // the URIs we provided.
    discoverer.signal_finished().connect([&](Gst::Discoverer) {
        fmt::print("Finished discovering\n");
        loop.quit();
    });

    // Start the discoverer process (nothing to do yet)
    discoverer.start();

    // Add a request to process asynchronously the URI passed through the command line
    if (!discoverer.discover_uri_async(uri)) {
        fmt::print(stderr, "Failed to start discovering URI '{}'\n", uri);
        return -1;
    }

    // Set the loop to run, so we can wait for the signals
    loop.run();

    // Stop the discoverer process
    discoverer.stop();
}
