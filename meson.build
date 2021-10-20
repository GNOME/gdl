project('gdl', 'c',
  version: '3.40.0',
  meson_version: '>= 0.57.0',
  default_options: [
    'c_std=c99',
    'warning_level=0',
  ]
)

cc = meson.get_compiler('c')

xml_dep = dependency('libxml-2.0', version: '>= 2.2.8')
gtk_dep = dependency('gtk+-3.0')

if cc.get_id() == 'msvc'
  add_project_arguments('-mms-bitfields', language: 'c')
endif

gdl_header_subdir = 'libgdl-3.0' / 'gdl'
gdl_includedir = get_option('prefix') / get_option('includedir') / gdl_header_subdir

conf = configuration_data()
conf.set('VERSION', meson.project_version())
conf.set('prefix', get_option('prefix'))
conf.set('exec_prefix', get_option('prefix'))
conf.set('libdir', get_option('prefix') / get_option('libdir'))
conf.set('includedir', get_option('prefix') / get_option('includedir'))
conf.set('datarootdir', get_option('prefix') / get_option('datadir'))

configure_file(
  input: 'gdl-3.0.pc.in',
  output: 'gdl-3.0.pc',
  install: true,
  install_dir: get_option('libdir') / 'pkgconfig',
  configuration: conf,
)

subdir('gdl')