project('app', 'cpp', default_options: ['cpp_std=c++23'])
add_project_arguments(
  # '-O3', # максимальная оптимизация
  '-Ofast', # максимальная оптимизация
  '-march=native', # использовать все возможности текущего процессора
  '-flto', # link-time optimization
  '-funroll-loops',
  '-fomit-frame-pointer',
  '-ftree-vectorize',
  '-mpopcnt',
  '-DNDEBUG', # отключить assert (важно для -O3)
  language: 'cpp',
)

boost_dep = dependency('boost', modules: ['system', 'thread'])
ssl_dep = dependency('openssl')

# simdjson_src = files('libs/simdjson/simdjson.cpp')
# simdjson_dep = declare_dependency(
#   include_directories: include_directories('libs/simdjson'),
#   sources: simdjson_src,
# )

subdir('libs/simdjson')

executable(
  'app',
  ['src/main.cpp', 'src/bybit/ws/data.cpp'],
  dependencies: [boost_dep, ssl_dep, simdjson_dep],
)