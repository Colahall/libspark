project('libspark', ['c'],
  version : '0.1.0',
  license: 'MIT',
  default_options : ['c_std=gnu11',
                     'buildtype=debugoptimized', 'warning_level=3']
)

# Split version safely
version_full = meson.project_version()
version = version_full.split('.')
version_major = version[0]
version_minor = version[1]
version_patch = version[2]

# Add cargs based on platform
cargs = []

if host_machine.system() != 'windows'
  # Export only what we mark
  cargs += ['-fvisibility=hidden']
  # Add POSIX feature flag
  cargs += ['-D_POSIX_C_SOURCE=200809L']
endif

# Dependencies
# catch2_dep = dependency('catch2-with-main', required : true)

# Include directories
inc = include_directories('include')

# Library sources
lib_sources = [
  'lib/block.c',
  'lib/version.c',
  'lib/iir-filter/iir_sosfilt_f32.c',
]

# Generate version.h
subdir('include')

lib_headers = [
  'include/spark/iir_filter.h',
  'include/spark/block.h',
  'include/spark/libspark_api.h',
  spark_version_h,
]

# Create the library
libspark = library('spark',
  lib_sources,
  include_directories : inc,
  c_args: cargs,
  install : true
)

# Create dependency object for the library
libspark_dep = declare_dependency(
  include_directories : inc,
  link_with : libspark
)

# Install the header files
install_headers (
  lib_headers,
  subdir: 'spark'
)

# Build tests first to generate comparison plots
#subdir('test')

# Then build documentation
if not meson.is_subproject()
  subdir('doc')
endif