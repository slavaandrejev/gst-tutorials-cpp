gio_dep      = dependency('gio-2.0')
gio_unix_dep = dependency('gio-unix-2.0')
glib_dep     = dependency('glib-2.0')
gmodule_dep  = dependency('gmodule-2.0')
gobject_dep  = dependency('gobject-2.0')

giglib_lib = static_library(
    'giglib'
  , [
        gi_binding_src['gio']
      , gi_binding_src['giounix']
      , gi_binding_src['glib']
      , gi_binding_src['glibunix']
      , gi_binding_src['gmodule']
      , gi_binding_src['gobject']
      ]
  , dummy_headers
  , include_directories: [
        gi_binding_inc, gi_gen_inc
      ]
  , dependencies: [
        cppgir_dep
      , gio_dep
      , gio_unix_dep
      , glib_dep
      , gmodule_dep
      , gobject_dep
      ]
  )

giglib_dep = declare_dependency(
    sources: dummy_headers
  , link_with: giglib_lib
  , include_directories: gi_gen_inc
  , dependencies: [
        cppgir_dep
      , gio_dep
      , gio_unix_dep
      , glib_dep
      , gmodule_dep
      , gobject_dep
      ]
  )
