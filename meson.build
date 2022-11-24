project(
  'sov',
  'c',
  version: '0.61b',
  license: 'MIT',
  default_options: ['c_std=gnu99']
)

cc = meson.get_compiler('c')

png = cc.find_library('png',static:false)
freetype = cc.find_library('freetype',static:false)
math = cc.find_library('m',static:false)
z = cc.find_library('z',static:false)
bz2 = cc.find_library('bz2',static:false)
rt = cc.find_library('rt',static:false)
gl = cc.find_library('GL',static:false)
glew = cc.find_library('GLEW',static:false)
wegl = cc.find_library('wayland-egl',static:false)
pthread = cc.find_library('pthread',static:false)
egl = cc.find_library('EGL',static:false)

xkbcommon = dependency('xkbcommon')

wayland_client      = dependency('wayland-client')
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


sov_dependencies = [
		  wayland_client,
		  wayland_cursor,
		  png,
		  rt,
		  freetype,
		  math,
		  gl,
		  glew,
		  pthread,
		  z,
		  bz2,
		  xkbcommon,
		  egl,
		  wegl]

sov_version = '"@0@"'.format(meson.project_version())
pkg_datadir = join_paths(get_option('prefix'), get_option('datadir')) / 'sov'
add_project_arguments('-DPKG_DATADIR="' + pkg_datadir + '"',
		      '-DSOV_VERSION=@0@'.format(sov_version),
		      language: 'c')
add_project_arguments('-Wno-unused-but-set-variable', language: 'c')

if get_option('buildtype') == 'debug'
    add_project_arguments('-DDEBUG',language: 'c')
endif

sov_inc = include_directories(
	'/usr/include',
	'/usr/include/GLES2',
	'/usr/include/EGL',
	'/usr/include/freetype2',
	'src/mt_math',
	'src/mt_core',
	'src/kinetic_ui',
	'src/kinetic_ui/egl',
	'src/kinetic_ui/handler',
	'src/kinetic_ui/texture',
	'src/sov',
	'src/mt_core_ext')


com_sources = ['src/sov/config.c',
	       'src/sov/jsmn.c',
	       'src/sov/json.c',
	       'src/sov/kvlines.c',
	       'src/sov/tree_reader.c',

	       'src/mt_core_ext/mt_bitmap_ext.c',
	       'src/mt_core_ext/mt_string_ext.c',
	       'src/mt_core_ext/mt_map_ext.c',

	       'src/mt_core/mt_channel.c',
	       'src/mt_core/mt_string.c',
	       'src/mt_core/mt_path.c',
	       'src/mt_core/mt_log.c',
	       'src/mt_core/mt_map.c',
	       'src/mt_core/mt_memory.c',
	       'src/mt_core/mt_number.c',
	       'src/mt_core/mt_time.c',
	       'src/mt_core/mt_vector.c',
	       'src/mt_core/mt_wrapper.c',

	       'src/mt_math/mt_matrix_3d.c',
	       'src/mt_math/mt_matrix_4d.c',
	       'src/mt_math/mt_vector_2d.c',
	       'src/mt_math/mt_vector_3d.c',
	       'src/mt_math/mt_vector_4d.c',
	       'src/mt_math/mt_math_2d.c',
	       'src/mt_math/mt_math_3d.c',

	       'src/kinetic_ui/ku_event.c',
	       'src/kinetic_ui/ku_html.c',
	       'src/kinetic_ui/ku_css.c',
	       'src/kinetic_ui/egl/ku_gl.c',
	       'src/kinetic_ui/ku_rect.c',
	       'src/kinetic_ui/ku_bitmap.c',
	       'src/kinetic_ui/texture/tg_css.c',
	       'src/kinetic_ui/texture/tg_knob.c',
	       'src/kinetic_ui/texture/tg_text.c',
	       'src/kinetic_ui/texture/tg_scaledimg.c',
	       'src/kinetic_ui/handler/vh_anim.c',
	       'src/kinetic_ui/handler/vh_button.c',
	       'src/kinetic_ui/handler/vh_drag.c',
	       'src/kinetic_ui/handler/vh_key.c',
	       'src/kinetic_ui/handler/vh_knob.c',
	       'src/kinetic_ui/handler/vh_roll.c',
	       'src/kinetic_ui/handler/vh_slider.c',
	       'src/kinetic_ui/handler/vh_textinput.c',
	       'src/kinetic_ui/handler/vh_touch.c',
	       'src/kinetic_ui/handler/vh_cv_body.c',
	       'src/kinetic_ui/handler/vh_cv_scrl.c',
	       'src/kinetic_ui/handler/vh_cv_evnt.c',
	       'src/kinetic_ui/handler/vh_tbl_body.c',
	       'src/kinetic_ui/handler/vh_tbl_evnt.c',
	       'src/kinetic_ui/handler/vh_tbl_scrl.c',
	       'src/kinetic_ui/handler/vh_tbl_head.c',
	       'src/kinetic_ui/handler/vh_table.c',
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

sov_sources = [com_sources, 'src/sov/main.c']

executable(
  'sov',
  sov_sources,
  include_directories: [sov_inc],
  dependencies: sov_dependencies,
  install: true,
)

install_data('config', install_dir : pkg_datadir )
install_data('contrib/systemd/sov.service', install_dir : get_option('libdir') / 'systemd' / 'user' )
install_data('contrib/systemd/sov.socket', install_dir : get_option('libdir') / 'systemd' / 'user' )

e = executable('maintest', 'src/tests/test.c')

test('MAIN TEST', e)
