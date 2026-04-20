#pragma once

#include <gtk/gtk.hpp>

#include <gst/video/video-frame.h>

#include <glbinding/gl/gl.h>

#include "gnamespaces.h"
#include "shader.h"

namespace Gtk {
    using namespace gi::repository::Gtk;
}
namespace Gdk {
    using namespace gi::repository::Gdk;
}

class VideoRenderer : public Gtk::impl::GLAreaImpl {
    friend struct WidgetClassDef::TypeInitData;
    friend struct GLAreaClassDef::TypeInitData;
public:
    // We need this constructor only to satisfy the requirement of
    // `Gtk::Builder::get_object_derived`. This class is constructed by
    // GtkBuilder's C side and this constructor is never called.
    VideoRenderer(Gtk::GLArea cobj, Gtk::Builder builder)
      : Gtk::impl::GLAreaImpl(cobj, this)
    {}

    VideoRenderer(const InitData &id)
      : Gtk::impl::GLAreaImpl(this, id, "VideoRenderer")
    {}

    static GType get_type_() {
        return register_type_<VideoRenderer>("VideoRenderer", 0, {}, {}, {});
    }

    gi::signal<gint(GLib::Object)> get_texture{this, "get-texture"};

    gi::signal_proxy<gint(GLib::Object)> signal_get_texture() {
        return get_texture;
    }

private:
    bool render_(Gdk::GLContext context) noexcept override;
    void realize_() noexcept override;
    void unrealize_() noexcept override;

    guint tick_callback_id{};
    bool timer_event(Gtk::Widget, Gdk::FrameClock frame_clock);

    std::unique_ptr<Shader> video_shader;
    gl::GLuint empty_vao{};
};
