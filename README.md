<img width="983" height="580" alt="player screenshot" src="https://github.com/user-attachments/assets/bb10e502-d5aa-44bd-a1c7-1e8ab9a3e6cb" />

# GStreamer C++ Tutorials

This project helps use GStreamer with modern C++. It is a translation of the
[GStreamer Tutorials](https://gstreamer.freedesktop.org/documentation/tutorials/basic/index.html?gi-language=c)
to C++ using bindings auto-generated from [GObject Introspection](https://gi.readthedocs.io/en/latest/) type libraries.
[cppgir](https://gitlab.com/mnauw/cppgir) is used as the binding generator.

Special attention was given to Tutorial 5. The original is aimed at the outdated
GTK+ and is not an idiomatic example of modern GTK usage. This project uses
GTK 4 and demonstrates contemporary, idiomatic use of the framework. It includes:
  * Declarative GUI construction using the [Blueprint](https://gnome.pages.gitlab.gnome.org/blueprint-compiler/) language.
  * Subclassing GTK widgets in C++ and declaring them directly in Blueprint.
  * GLib resources for storing the GUI description and OpenGL shaders.
  * OpenGL context sharing between GTK and GStreamer.
  * Video rendering to a GtkGLArea widget.

Anyone trying to do GStreamer + GTK 4 + OpenGL properly in C++ may find this invaluable.
