vfilter_sources = [
  'plugin.c',
  'gstsimpleoverlay.c',
]

gstsimpleoverlay = library('gstsimpleoverlay',
  vfilter_sources,
  c_args : gst_plugins_bad_args,
  include_directories : [configinc],
  dependencies : [gstbase_dep, gstvideo_dep, libm],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstsimpleoverlay, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstsimpleoverlay]