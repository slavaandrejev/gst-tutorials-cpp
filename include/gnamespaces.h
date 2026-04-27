#pragma once

#include <gio/gio.hpp>
#include <glib/glib.hpp>
#include <glibunix/glibunix.hpp>
#include <gobject/gobject.hpp>
#include <gstapp/gstapp.hpp>
#include <gstaudio/gstaudio.hpp>
#include <gstgl/gstgl.hpp>
#include <gstglegl/gstglegl.hpp>
#include <gstglx11/gstglx11.hpp>
#include <gstpbutils/gstpbutils.hpp>
#include <gstvideo/gstvideo.hpp>

namespace GLib {
    using namespace gi::repository::GLib;
    using namespace gi::repository::GLibUnix;
    using namespace gi::repository::GObject;
}
namespace Gio {
    using namespace gi::repository::Gio;
}
namespace Gst {
    using namespace gi::repository::Gst;
    using namespace gi::repository::GstApp;
    using namespace gi::repository::GstAudio;
    using namespace gi::repository::GstBase;
    using namespace gi::repository::GstGL;
    using namespace gi::repository::GstGLEGL;
    using namespace gi::repository::GstGLX11;
    using namespace gi::repository::GstPbutils;
    using namespace gi::repository::GstVideo;
}
