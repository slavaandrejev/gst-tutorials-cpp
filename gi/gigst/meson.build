gst_dep         = dependency('gstreamer-1.0')
gst_app_dep     = dependency('gstreamer-app-1.0')
gst_audio_dep   = dependency('gstreamer-audio-1.0')
gst_base_dep    = dependency('gstreamer-base-1.0')
gst_gl_dep      = dependency('gstreamer-gl-1.0')
gst_gl_egl_dep  = dependency('gstreamer-gl-egl-1.0')
gst_gl_glx_dep  = dependency('gstreamer-gl-x11-1.0')
gst_video_dep   = dependency('gstreamer-video-1.0')

gigst_lib = static_library(
    'gigst'
  , [
        gi_binding_src['gst']
      , gi_binding_src['gstapp']
      , gi_binding_src['gstaudio']
      , gi_binding_src['gstbase']
      , gi_binding_src['gstgl']
      , gi_binding_src['gstglegl']
      , gi_binding_src['gstglx11']
      , gi_binding_src['gstvideo']
      ]
  , dummy_headers
  , include_directories: [
        gi_binding_inc, gi_gen_inc
      ]
  , dependencies: [
        cppgir_dep
      , giglib_dep
      , gst_dep
      , gst_app_dep
      , gst_audio_dep
      , gst_base_dep
      , gst_gl_dep
      , gst_gl_egl_dep
      , gst_gl_glx_dep
      , gst_video_dep
      ]
  )

gigst_dep = declare_dependency(
    sources: dummy_headers
  , link_with: gigst_lib
  , dependencies: [
        cppgir_dep
      , giglib_dep
      , gst_dep
      , gst_app_dep
      , gst_audio_dep
      , gst_base_dep
      , gst_gl_dep
      , gst_gl_egl_dep
      , gst_gl_glx_dep
      , gst_video_dep
      ]
  )
