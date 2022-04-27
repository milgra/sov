project(
  'sov',
  'c',
  version: '0.61',
  license: 'ISC',
  default_options: ['c_std=c99']
)

cc = meson.get_compiler('c')

wayland_protos = dependency('wayland-protocols', version: '>=1.13')
wl_protocol_dir = wayland_protos.get_pkgconfig_variable('pkgdatadir')
wayland_scanner = find_program('wayland-scanner')
wayland_client = dependency('wayland-client')
freetype = dependency('freetype2')
math = cc.find_library('m')
rt = cc.find_library('rt')

sov_version = '"@0@"'.format(meson.project_version())
add_project_arguments('-DSOV_VERSION=@0@'.format(sov_version), language: 'c')

if get_option('buildtype').startswith('debug')
  add_project_arguments('-DDEBUG', language : 'c')
endif

wayland_scanner_code = generator(
  wayland_scanner,
  output: '@BASENAME@-protocol.c',
  arguments: ['private-code', '@INPUT@', '@OUTPUT@'],
)

wayland_scanner_client = generator(
  wayland_scanner,
  output: '@BASENAME@-client-protocol.h',
  arguments: ['client-header', '@INPUT@', '@OUTPUT@'],
)

client_protocols = [
  [wl_protocol_dir + '/stable/xdg-shell', 'xdg-shell.xml'],
  [wl_protocol_dir + '/unstable/xdg-output', 'xdg-output-unstable-v1.xml'],
  [meson.source_root() + '/protocols', 'wlr-layer-shell-unstable-v1.xml'],
]

foreach p : client_protocols
  xml = join_paths(p)
  src = wayland_scanner_code.process(xml)
  header = wayland_scanner_client.process(xml)

  name = p[1].split('.')[0].underscorify()

  lib = static_library(
    name,
    [src, header],
    dependencies: [wayland_client],
  )

  dep = declare_dependency(
    link_with: lib,
    sources: header,
  )

  set_variable(name, dep)
endforeach

sov_inc = include_directories('src/sov/config',
			      'src/sov/tree',
			      'src/modules/json',
			      'src/modules/storage',
			      'src/modules/zen_core',
			      'src/modules/zen_text')

com_sources = ['src/sov/config/config.c',
	       'src/sov/config/fontconfig.c',
	       'src/sov/tree/tree_drawer.c',
	       'src/sov/tree/tree_reader.c',	

	       'src/modules/json/json.c',
	       'src/modules/json/jsmn.c',
	       'src/modules/storage/kvlines.c',
	       'src/modules/zen_core/zc_bitmap.c',
	       'src/modules/zen_core/zc_channel.c',
	       'src/modules/zen_core/zc_cstring.c',
	       'src/modules/zen_core/zc_cstrpath.c',
	       'src/modules/zen_core/zc_graphics.c',
	       'src/modules/zen_core/zc_log.c',
	       'src/modules/zen_core/zc_map.c',
	       'src/modules/zen_core/zc_memory.c',
	       'src/modules/zen_core/zc_string.c',
	       'src/modules/zen_core/zc_vector.c',
	       'src/modules/zen_core/zc_wrapper.c',
	       'src/modules/zen_text/text.c']

sov_sources = com_sources
sov_sources += 'src/sov/main.c'
tst_sources = com_sources
tst_sources += 'src/sov/tree_test.c'

sov_dependencies = [xdg_output_unstable_v1, wayland_client, wlr_layer_shell_unstable_v1, xdg_shell, rt, freetype, math]

executable(
  'sov',
  sov_sources,
  include_directories: [sov_inc],
  dependencies: sov_dependencies,
  install: true,
)

install_data('config', install_dir : get_option('datadir') / 'sov')

executable(
  'test_tree',
  tst_sources,
  include_directories: [sov_inc],
  dependencies: sov_dependencies,
)