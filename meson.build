project(
  'sov',
  'c',
  version: '0.12',
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

sov_inc = include_directories('include',
			      'src/overview/config',
			      'src/overview/tree',
			      'src/modules/json',
			      'src/modules/storage',
			      'src/modules/zen_core',
			      'src/modules/zen_text')

sov_sources = ['src/overview/overview.c',
	       'src/overview/buffer.c',
	       'src/overview/log.c',

	       'src/overview/config/config.c',
	       'src/overview/config/fontconfig.c',
	       'src/overview/tree/tree_drawer.c',
	       'src/overview/tree/tree_reader.c',	

	       'src/modules/json/json.c',
	       'src/modules/json/jsmn.c',
	       'src/modules/storage/kvlines.c',
	       'src/modules/zen_core/zc_bitmap.c',
	       'src/modules/zen_core/zc_channel.c',
	       'src/modules/zen_core/zc_cstring.c',
	       'src/modules/zen_core/zc_cstrpath.c',
	       'src/modules/zen_core/zc_graphics.c',
	       'src/modules/zen_core/zc_map.c',
	       'src/modules/zen_core/zc_memory.c',
	       'src/modules/zen_core/zc_string.c',
	       'src/modules/zen_core/zc_vector.c',
	       'src/modules/zen_core/zc_wrapper.c',
	       'src/modules/zen_text/text.c'
]
sov_dependencies = [xdg_output_unstable_v1, wayland_client, wlr_layer_shell_unstable_v1, xdg_shell, rt, freetype, math]

executable(
  'sov',
  sov_sources,
  include_directories: [sov_inc],
  dependencies: sov_dependencies,
  install: true
)

test('parse-input', executable(
  'test-parse-input',
  ['tests/sov_parse_input.c'],
  include_directories: [sov_inc]
))

scdoc = dependency('scdoc', version: '>=1.9.2', native: true, required: get_option('man-pages'))
if scdoc.found()
  scdoc = find_program(scdoc.get_pkgconfig_variable('scdoc'), native: true)
  sh = find_program('sh', native: true)
  mandir = get_option('mandir')
  scdfile = 'sov.1.scd'
  manfile = scdfile.split('.scd')[0]

  custom_target(
    manfile,
    input: scdfile,
    output: manfile,
    command: [
      sh, '-c', '@0@ < @INPUT@ > @1@'.format(scdoc.path(), manfile)
    ],
    install: true,
    install_dir: join_paths(mandir, 'man1')
  )
endif
