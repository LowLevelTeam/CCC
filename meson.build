project('ccc', 'cpp',
  version : '0.1.0',
  default_options : [
    'cpp_std=c++17',
    'warning_level=3',
    'buildtype=release'
  ]
)

# Dependencies
libcoil_dep = dependency('libcoil-dev')

# Include directories
inc_dirs = include_directories('include')

# Sources
sources = files(
  'src/main.cpp',
  'src/token.cpp',
  'src/lexer.cpp',
  'src/parser.cpp',
  'src/ast.cpp',
  'src/semantic.cpp',
  'src/codegen.cpp',
  'src/error.cpp',
  'src/utils.cpp'
)

# Main executable
executable('ccc',
  sources,
  include_directories : inc_dirs,
  dependencies : [libcoil_dep],
  install : true
)