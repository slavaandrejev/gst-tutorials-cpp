#pragma once

#include <atomic>

#include <gtk/gtk.hpp>

#include <gst/gst.hpp>
#include <gstapp/gstapp.hpp>
#include <gstaudio/gstaudio.hpp>
#include <gstbase/gstbase.hpp>
#include <gstgl/gstgl.hpp>
#include <gstglegl/gstglegl.hpp>
#include <gstglx11/gstglx11.hpp>
#include <gstvideo/gstvideo.hpp>

#include "gnamespaces.h"
#include "videorenderer.h"

namespace Gtk {
    using namespace gi::repository::Gtk;
}

class MainWindow : public Gtk::impl::ApplicationWindowImpl
{
public:
    static gi::ref_ptr<MainWindow> new_();

    MainWindow(Gtk::ApplicationWindow cobj, Gtk::Builder builder);

private:
    void on_realize(Gtk::Widget);
    void on_unrealize(Gtk::Widget);

    void on_need_context(Gst::Bus, Gst::Message_Ref msg);
    void on_error(Gst::Bus, Gst::Message_Ref msg);
    void on_eos(Gst::Bus, Gst::Message_Ref msg);
    void on_state_change(Gst::Bus, Gst::Message_Ref msg);
    void on_application(Gst::Bus, Gst::Message_Ref msg);

    void on_tags(Gst::Element, gint stream);

    guint time_callback_id{};
    bool refresh_ui();

    void analyze_streams();

    Gst::FlowReturn new_preroll(Gst::AppSink);
    Gst::FlowReturn new_sample(Gst::AppSink);
    void store_new_frame(Gst::Buffer_Ref buffer);

    gint on_get_texture(GLib::Object);

    Gst::VideoInfo render_video_info;

    // Variables for lock-free frame synchronization between decoding and
    // displaying. We have one slot for writing and one for reading. The third
    // slot is a previous frame.
    GstVideoFrame     frames[3]{GST_VIDEO_FRAME_INIT};
    bool              mapped[3]{};
    std::atomic<int>  shared{2};
    std::atomic<bool> new_frame{false};
    int               write_slot{0};
    int               read_slot{1};

    guint slider_update_signal_id{};

    gi::ref_ptr<VideoRenderer> gl_area;

    Gtk::Button   play_button;
    Gtk::Button   pause_button;
    Gtk::Button   stop_button;
    Gtk::Scale    slider;
    Gtk::TextView streams_list;

    Gst::GLDisplay gst_gl_display;
    Gst::GLContext gst_gl_context;

    Gst::State state;

    gint64 duration = Gst::CLOCK_TIME_NONE_;

    Gst::Pipeline playbin{
        gi::object_cast<Gst::Pipeline>(
            Gst::ElementFactory::make("playbin", "playbin")
        )
    };
    Gst::GLBaseFilter glupload{
        gi::object_cast<Gst::GLBaseFilter>(
            Gst::ElementFactory::make("glupload", "glupload")
        )
    };
    Gst::GLBaseFilter glcolorconvert{
        gi::object_cast<Gst::GLBaseFilter>(
            Gst::ElementFactory::make("glcolorconvert", "glcolorconvert")
        )
    };
    Gst::AppSink video_sink{
        gi::object_cast<Gst::AppSink>(
            Gst::ElementFactory::make("appsink", "appsink")
        )
    };
    Gst::Bin video_sink_bin{Gst::Bin::new_("video_sink_bin")};
};
