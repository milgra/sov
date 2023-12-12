project(
  'sov',
  'c',
  version: '0.94',
  license: 'MIT',
  default_options: ['c_std=gnu99',
  		    'warning_level=3']
)

cc = meson.get_compiler('c')

egl = dependency('egl')
png = dependency('libpng')
math = cc.find_library('m')
wegl = dependency('wayland-egl')
glesv2 = dependency('glesv2')
freetype = dependency('freetype2')
xkbcommon = dependency('xkbcommon')

wayland_client      = dependency('wayland-client',version : '>=1.21.0')
wayland_cursor      = dependency('wayland-cursor')
wayland_protos      = dependency('wayland-protocols')
wayland_scanner_dep = dependency('wayland-scanner')
wayland_scanner     = find_program(
  wayland_scanner_dep.get_pkgconfig_variable('wayland_scanner')
)

wl_protocol_dir = wayland_protos.get_pkgconfig_variable('pkgdatadir')

protocols = [
  [wl_protocol_dir, 'stable/xdg-shell/xdg-shell.xml'],
  [wl_protocol_dir, 'unstable/xdg-output/xdg-output-unstable-v1.xml'],
  [wl_protocol_dir, 'unstable/pointer-gestures/pointer-gestures-unstable-v1.xml'],
  'wlr-layer-shell-unstable-v1.xml'
]

protos_src = []
protos_headers = []

foreach p : protocols
  xml = join_paths(p)
  protos_src += custom_target(
    xml.underscorify() + '_client_c',
    input: xml,
    output: '@BASENAME@-protocol.c',
    command: [wayland_scanner, 'public-code', '@INPUT@', '@OUTPUT@'],
  )
  protos_headers += custom_target(
    xml.underscorify() + '_client_h',
    input: xml,
    output: '@BASENAME@-client-protocol.h',
    command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
  )
endforeach


sov_dependencies = [wayland_client,
		    wayland_cursor,
		    png,
		    freetype,
		    math,
		    glesv2,
		    xkbcommon,
		    egl,
		    wegl]

sov_version = '"@0@"'.format(meson.project_version())
pkg_datadir = join_paths(get_option('prefix'), get_option('datadir')) / 'sov'
add_project_arguments('-DPKG_DATADIR="' + pkg_datadir + '"',
		      '-DSOV_VERSION=@0@'.format(sov_version),
		      '-Wno-unused-parameter',
		      language: 'c')

if get_option('buildtype') == 'debug'
    add_project_arguments('-DDEBUG',language: 'c')
endif

sov_inc = include_directories(
	'src/mt_math',
	'src/mt_core',
	'src/kinetic_ui',
	'src/kinetic_ui/egl',
	'src/kinetic_ui/handler',
	'src/kinetic_ui/texture',
	'src/sov',
	'src/mt_core_ext')

if build_machine.system() == 'freebsd'
   epoll = dependency('epoll-shim')
   pthread = dependency('threads')
   sov_dependencies += pthread
   sov_dependencies += epoll
endif

com_sources = ['src/sov/jsmn.c',
	       'src/sov/json.c',
	       'src/sov/tree.c',
	       'src/sov/gen.c',
	       'src/sov/ku_bitmap_ext.c',
	       
	       'src/mt_core_ext/mt_string_ext.c',
	       'src/mt_core/mt_string.c',
	       'src/mt_core/mt_path.c',
	       'src/mt_core/mt_log.c',
	       'src/mt_core/mt_map.c',
	       'src/mt_core/mt_memory.c',
	       'src/mt_core/mt_number.c',
	       'src/mt_core/mt_time.c',
	       'src/mt_core/mt_vector.c',
	       'src/mt_core/mt_wrapper.c',
	       'src/mt_math/mt_matrix_4d.c',

	       'src/kinetic_ui/ku_event.c',
	       'src/kinetic_ui/ku_html.c',
	       'src/kinetic_ui/ku_css.c',
	       'src/kinetic_ui/egl/ku_gl.c',
	       'src/kinetic_ui/ku_rect.c',
	       'src/kinetic_ui/ku_bitmap.c',
	       'src/kinetic_ui/texture/tg_css.c',
	       'src/kinetic_ui/texture/tg_text.c',
	       'src/kinetic_ui/handler/vh_anim.c',
	       'src/kinetic_ui/handler/vh_button.c',
	       'src/kinetic_ui/handler/vh_drag.c',
	       'src/kinetic_ui/handler/vh_key.c',
	       'src/kinetic_ui/handler/vh_slider.c',
	       'src/kinetic_ui/ku_view.c',
	       'src/kinetic_ui/egl/ku_gl_atlas.c',
	       'src/kinetic_ui/egl/ku_gl_floatbuffer.c',
	       'src/kinetic_ui/egl/ku_gl_shader.c',
	       'src/kinetic_ui/ku_gen_html.c',
	       'src/kinetic_ui/ku_gen_css.c',
	       'src/kinetic_ui/ku_gen_type.c',
	       'src/kinetic_ui/ku_renderer_egl.c',
	       'src/kinetic_ui/ku_renderer_soft.c',
	       'src/kinetic_ui/ku_fontconfig.c',
	       'src/kinetic_ui/ku_connector_wayland.c',
	       'src/kinetic_ui/ku_window.c',
	       'src/kinetic_ui/ku_png.c',
	       'src/kinetic_ui/ku_draw.c',
	       'src/kinetic_ui/ku_text.c',
	       'src/kinetic_ui/ku_gen_textstyle.c'] + protos_src + protos_headers

sov_sources = [com_sources, 'src/sov/sov.c']
draw_sources = [com_sources, 'src/sov/draw.c']

executable(
  'sov',
  sov_sources,
  include_directories: [sov_inc],
  dependencies: sov_dependencies,
  install: true,
)

executable(
  'draw',
  draw_sources,
  include_directories: [sov_inc],
  dependencies: sov_dependencies,
  install: false,
)

pkg_datadir = join_paths(get_option('prefix'), get_option('datadir')) / 'sov'

install_data('res/html/main.html', install_dir : pkg_datadir / 'html' )
install_data('res/html/main.css', install_dir : pkg_datadir / 'html' )

install_data('contrib/systemd/sov.service', install_dir : get_option('libdir') / 'systemd' / 'user' )
install_data('contrib/systemd/sov.socket', install_dir : get_option('libdir') / 'systemd' / 'user' )

mt_core_test = executable(
	   'mt_core_test',
	   ['src/mt_core/mt_log.c',
	    'src/mt_core/mt_memory.c',
	    'src/mt_core/mt_vector.c',
	    'src/mt_core/mt_map.c',
	    'src/mt_core/mt_string.c',
	    'src/mt_core/mt_channel.c',
	    'src/mt_core/mt_path.c',
	    'src/mt_core_test/mt_core_test.c',
	    'src/mt_core_test/mt_memory_test.c',
	    'src/mt_core_test/mt_vector_test.c',
	    'src/mt_core_test/mt_map_test.c',
	    'src/mt_core_test/mt_string_test.c',
	    'src/mt_core_test/mt_channel_test.c',
	    'src/mt_core_test/mt_path_test.c'
	    ],
	    include_directories: ['src/mt_core'],
	    dependencies: sov_dependencies)

test('MT_CORE_TEST', mt_core_test)

scripttests = find_program('tst/runtests.sh')

test('SCRIPTED TESTS', scripttests, timeout: 0, workdir : meson.current_source_dir(), args : [meson.current_build_dir(),meson.current_source_dir()])
