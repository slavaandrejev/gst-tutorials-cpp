gtk_dep       = dependency('gtk4')
# we need this dependency because of a cppgir defect
gtk_print_dep = dependency('gtk4-unix-print')

gigtk_lib = static_library(
    'gigtk'
  , [
        gi_binding_src['cairo']
      , gi_binding_src['freetype2']
      , gi_binding_src['gdk']
      , gi_binding_src['gdkpixbuf']
      , gi_binding_src['graphene']
      , gi_binding_src['gsk']
      , gi_binding_src['gtk']
      , gi_binding_src['harfbuzz']
      , gi_binding_src['pango']
      , gi_binding_src['pangocairo']
      ]
  , dummy_headers
  , include_directories: [
        gi_binding_inc, gi_gen_inc
      ]
  , dependencies: [
        cppgir_dep
      , giglib_dep
      , gtk_dep
      , gtk_print_dep
      ]
  )

gigtk_dep = declare_dependency(
    sources: dummy_headers
  , link_with: gigtk_lib
  , dependencies: [
        cppgir_dep
      , giglib_dep
      , gtk_dep
      , gtk_print_dep
      ]
  )
