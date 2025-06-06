project(
  'libgepub', 'c',
  version: '0.7.3',
  license: 'LGPL2+',
  default_options: 'buildtype=debugoptimized',
  meson_version: '>= 0.50.0'
)

gepub_version = meson.project_version()
version_array = gepub_version.split('.')
gepub_major_version = version_array[0].to_int()
gepub_minor_version = version_array[1].to_int()
gepub_version_micro = version_array[2].to_int()
gepub_api_version = '@0@.@1@'.format(gepub_major_version, gepub_minor_version)
gepub_lib_name = '@0@-@1@'.format(meson.project_name(), gepub_api_version)

gepub_gir_ns = 'Gepub'
gepub_gir_version = '1.0'

gepub_prefix = get_option('prefix')
gepub_datadir = join_paths(gepub_prefix, get_option('datadir'))
gepub_libdir = join_paths(gepub_prefix, get_option('libdir'))
gepub_libexecdir = join_paths(gepub_prefix, get_option('libexecdir'))
gepub_includedir = join_paths(gepub_prefix, get_option('includedir'))

soversion = 0
current = 0
revision = 0
libversion = '@0@.@1@.@2@'.format(soversion, current, revision)

cc = meson.get_compiler('c')

gepub_deps = [
  dependency('libsoup-3.0'),
  dependency('glib-2.0'),
  dependency('gobject-2.0'),
  dependency('gio-2.0'),
  dependency('libxml-2.0'),
  dependency('libarchive')
]

if get_option('widget')
  webkit2gtk = dependency('webkit2gtk-4.1')
  gepub_deps += [webkit2gtk]
endif

gnome = import('gnome')
pkg = import('pkgconfig')

top_inc = include_directories('.')

subdir('libgepub')

if get_option('widget')
  subdir('tests')
endif

config_h = configuration_data()

if get_option('widget')
  config_h.set('GEPUB_WIDGET_ENABLED', 1)
endif

configure_file(
  output: 'config.h',
  configuration: config_h,
  install: true,
  install_dir: gepub_includedir / gepub_lib_name,
)
