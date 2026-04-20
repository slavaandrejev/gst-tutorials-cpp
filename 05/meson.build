fs    = import('fs')
gnome = import('gnome')

glbinding_dep = dependency('glbinding', method: 'cmake')
glm_dep       = dependency('glm', method: 'cmake')

bpc = find_program('blueprint-compiler', required: false)
if not bpc.found()
    subproject('blueprint-compiler')
    bpc = find_program('blueprint-compiler')
endif

blp_files = ['gst-05-win.blp']
ui_files = []
foreach f : blp_files
    ui_files += [fs.replace_suffix(f, '.ui')]
endforeach
blueprints = custom_target('blueprints',
    input:   blp_files
  , output:  ui_files
  , command: [bpc, 'batch-compile', '@OUTDIR@', '@CURRENT_SOURCE_DIR@', '@INPUT@']
  )

gtk_files = gnome.compile_resources(
    'gst-05'
  , 'gst-05.gresource.xml'
  , c_name: 'gst_05'
  , dependencies: blueprints
  )

executable(
    'gst-05'
  , 'gst-05.cpp'
  , 'gst-05-win.cpp'
  , 'videorenderer.cpp'
  , gtk_files
  , include_directories: [project_inc]
  , dependencies : [
        boost_dep
      , fmt_dep
      , gigtk_dep
      , gigst_dep
      , glbinding_dep
      , glm_dep
      ]
  )
