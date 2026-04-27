#include <gtk/gtk.hpp>

#include <fmt/printf.h>

#include "gst-05-win.h"

namespace Gtk {
    using namespace gi::repository::Gtk;
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    auto app = Gtk::Application::new_(Gio::ApplicationFlags::DEFAULT_FLAGS_);

    auto main_window = gi::ref_ptr<MainWindow>{};
    app.signal_activate().connect([&](Gio::Application app_) {
        main_window = MainWindow::new_();
        if (main_window) {
            app.add_window(*main_window);
            main_window->present();
        } else {
            app_.quit();
        }
    });
    app.signal_window_removed().connect([&](Gtk::Application, Gtk::Window window) {
        if (main_window && *main_window == window) {
            main_window.reset();
        }
    });

    return app.run({argv, size_t(argc)});
}
