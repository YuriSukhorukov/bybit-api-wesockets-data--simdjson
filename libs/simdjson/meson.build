simdjson_lib = static_library(
  'simdjson',
  'simdjson.cpp',
  include_directories: include_directories('.'),
  install: false,
)

simdjson_dep = declare_dependency(link_with: simdjson_lib, include_directories: include_directories('.'))