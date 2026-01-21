project(
  'PlayboxLib',
  ['c', 'cpp'],
  version: '1.0.0',
  meson_version: '>=0.60.0',
  default_options: [
    'c_std=c11',
    'cpp_std=c++17',
    'warning_level=2',
    'werror=false',
    'buildtype=release',
    'default_library=both',
    'b_lto=false',
    'b_ndebug=if-release',
  ]
)

fs = import('fs')

cc  = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')
host_os = host_machine.system()

opt_build_cpp      = get_option('playbox_cpp')
opt_build_examples = get_option('playbox_examples')
opt_install        = get_option('playbox_install')
opt_renderer       = get_option('playbox_renderer')
opt_input_impl     = get_option('playbox_input_impl')
opt_term_impl      = get_option('playbox_term_impl')
opt_replay         = get_option('playbox_replay')

playbox_inc = include_directories('include', 'src')
playbox_cpp_inc = include_directories('cpp')

if host_os == 'windows'
  error('PlayboxLib currently targets POSIX terminals only. (host: windows)')
endif

c_args = []
cpp_args = []

foreach a : ['-Wall', '-Wextra', '-Wpedantic']
  if cc.has_argument(a)
    c_args += [a]
  endif
  if cpp.has_argument(a)
    cpp_args += [a]
  endif
endforeach

core_sources = files(
  'src/pb_utf8.c',
  'src/pb_fb.c',
  'src/pb_time.c',
  'src/pb_app.c',
)

if opt_replay
  core_sources += files('src/pb_replay.c')
endif

if opt_renderer == 'ansi'
  if fs.exists('src/pb_renderer_ansi.c')
    core_sources += files('src/pb_renderer_ansi.c')
  else
    core_sources += files('src/pb_renderer.c')
  endif
else
  error('Unknown renderer: ' + opt_renderer + ' (expected: ansi)')
endif

if opt_term_impl == 'posix'
  core_sources += files('src/pb_term_posix.c')
else
  error('Unknown term impl: ' + opt_term_impl + ' (expected: posix)')
endif

if opt_input_impl == 'posix'
  core_sources += files('src/pb_input_posix.c')
elif opt_input_impl == 'default'
  if fs.exists('src/pb_input.c')
    core_sources += files('src/pb_input.c')
  else
    core_sources += files('src/pb_input_posix.c')
  endif
else
  error('Unknown input impl: ' + opt_input_impl + ' (expected: posix, default)')
endif

cpp_sources = []
if opt_build_cpp
  cpp_sources += files('cpp/playbox.cpp')
endif

libplaybox = library(
  'playbox',
  sources: core_sources + cpp_sources,
  include_directories: opt_build_cpp ? [playbox_inc, playbox_cpp_inc] : [playbox_inc],
  c_args: c_args,
  cpp_args: cpp_args,
  install: opt_install,
)

playbox_dep = declare_dependency(
  link_with: libplaybox,
  include_directories: opt_build_cpp ? [playbox_inc, playbox_cpp_inc] : [playbox_inc],
)

if opt_install
  install_subdir('include/playbox', install_dir: join_paths(get_option('includedir'), 'playbox'))
  if opt_build_cpp and fs.exists('cpp/playbox.hpp')
    install_headers('cpp/playbox.hpp', subdir: 'playbox')
  endif
endif

if opt_build_examples
  executable('pb_demo',    'examples/demo.c',     dependencies: playbox_dep, c_args: c_args, install: false)
  executable('pb_tetris',  'examples/tetris.c',   dependencies: playbox_dep, c_args: c_args, install: false)
  executable('pb_sandbox', 'examples/sandbox.c',  dependencies: playbox_dep, c_args: c_args, install: false)
  executable('pb_snake',   'examples/pb_snake.c', dependencies: playbox_dep, c_args: c_args, install: false)
endif

summary({
  'OS': host_os,
  'Replay enabled': opt_replay,
  'C++ wrapper': opt_build_cpp,
  'Examples': opt_build_examples,
  'Install': opt_install,
  'Renderer': opt_renderer,
  'Input impl': opt_input_impl,
  'Term impl': opt_term_impl,
  'Default library': get_option('default_library'),
}, section: 'PlayboxLib')
