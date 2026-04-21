project(
    'gst-tutorials'
  , ['c', 'cpp']
  , version : '1.0'
  , meson_version : '>= 1.2'
  , default_options: {
        'cpp_std': 'gnu++23'
      , 'c_std':   'gnu18'
      }
  )

project_inc = include_directories('include')

if get_option('buildtype') == ('debug')
    add_project_arguments('-DDEBUG', language: ['c', 'cpp'])
endif
if get_option('buildtype') == 'release'
    add_project_arguments('-DNDEBUG', language: ['c', 'cpp'])
endif
add_project_arguments('-Wno-character-conversion', language:  'cpp')

if get_option('gtk4')
    boost_dep  = dependency('boost', modules: ['headers'])
endif
fmt_dep    = dependency('fmt')

subdir('gi')

subdir('01')
subdir('02')
subdir('03')
subdir('04')
if get_option('gtk4')
    subdir('05')
endif
subdir('06')
subdir('07')
