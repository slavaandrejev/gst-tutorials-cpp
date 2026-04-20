#include <gst/gst.hpp>

#include <fmt/printf.h>

#include "fmtgststr.h"
#include "gnamespaces.h"

void print_caps(const Gst::Caps &caps, const char *pfx) {
    g_return_if_fail(caps);

    if (caps.is_any()) {
        fmt::print("{}ANY\n", pfx);
        return;
    }

    if (caps.is_empty()) {
        fmt::print("{}EMPTY\n", pfx);
        return;
    }

    for (auto i = guint{}; caps.get_size() > i; ++i) {
        auto s = caps.get_structure(i);
        fmt::print("{}{}\n", pfx, s.get_name());
        s.foreach_id_str([&](auto &&name, auto &&value) {
            fmt::print("{}  {:15}: {}\n", pfx, name.as_str(), Gst::value_serialize(value));
            return true;
        });
    }
}

// Prints information about a Pad Template, including its Capabilities
void print_pad_templates_information(Gst::ElementFactory factory) {
    fmt::print("Pad Templates for {}:\n", factory.get_metadata(GST_ELEMENT_METADATA_LONGNAME));
    if (0 == factory.get_num_pad_templates()) {
        fmt::print("  none\n");
        return;
    }

    auto pads = factory.get_static_pad_templates();
    for (auto &&pad : pads) {
        auto name_template = pad.name_template_();
        if (Gst::PadDirection::SRC_ == pad.direction_()) {
            fmt::print("  SRC template: '{}'\n", name_template);
        } else if (Gst::PadDirection::SINK_ == pad.direction_()) {
            fmt::print("  SINK template: '{}'\n", name_template);
        } else {
            fmt::print("  UNKNOWN!!! template: '{}'\n", name_template);
        }

        auto presence = pad.presence_();
        if (Gst::PadPresence::ALWAYS_ == presence) {
            fmt::print("    Availability: Always\n");
        } else if (Gst::PadPresence::SOMETIMES_ == presence) {
            fmt::print("    Availability: Sometimes\n");
        } else if (Gst::PadPresence::REQUEST_ == presence) {
            fmt::print("    Availability: On request\n");
        } else {
            fmt::print("    Availability: UNKNOWN!!!\n");
        }

        // cppgir defect: pad is const, so we cannot call `pad.get_caps()`
        if (nullptr != pad.gobj_()->static_caps.string) {
            fmt::print("    Capabilities:\n");
            auto static_caps = gi::wrap(&pad.gobj_()->static_caps, gi::transfer_none);
            auto caps = static_caps.get();
            print_caps(caps, "      ");
        }
        fmt::print("\n");
    }
}

// Shows the CURRENT capabilities of the requested pad in the given element
void print_pad_capabilities(Gst::Element element, const gi::cstring_v pad_name) {
    // Retrieve pad
    auto pad = element.get_static_pad(pad_name);
    if (!pad) {
        fmt::print(stderr, "Could not retrieve pad '{}'\n", pad_name);
        return;
    }

    // Retrieve negotiated caps (or acceptable caps if negotiation is not finished yet)
    auto caps = pad.get_current_caps();
    if (!caps) {
        caps = pad.query_caps();
    }

    fmt::print("Caps for the {} pad:\n", pad_name);
    print_caps(caps, "      ");
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    // Create the element factories
    auto source_factory = Gst::ElementFactory::find("audiotestsrc");
    auto sink_factory   = Gst::ElementFactory::find("autoaudiosink");
    if (!source_factory || !sink_factory) {
        fmt::print(stderr, "Not all element factories could be created.\n");
        return -1;
    }

    // Print information about the pad templates of these factories
    print_pad_templates_information(source_factory);
    print_pad_templates_information(sink_factory);

    // Ask the factories to instantiate actual elements
    auto source = source_factory.create("source");
    auto sink   = source_factory.create("sink");

    // Create the empty pipeline
    auto pipeline = Gst::Pipeline::new_("test-pipeline");

    if (!pipeline || !source || !sink) {
        fmt::print(stderr, "Not all elements could be created.\n");
        return -1;
    }

    // Build the pipeline
    pipeline.add(source, sink);
    if (!source.link(sink)) {
        fmt::print(stderr, "Elements could not be linked.\n");
        return -1;
    }
    fmt::print("In NULL state:\n");
    print_pad_capabilities(sink, "sink");
}
