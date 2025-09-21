# Generate a version.h from version.h.in

conf = configuration_data()
conf.set('SPARK_VERSION_MAJOR', version_major)
conf.set('SPARK_VERSION_MINOR', version_minor)
conf.set('SPARK_VERSION_PATCH', version_patch)
conf.set_quoted('SPARK_VERSION_STR', version_full)
conf.set_quoted('SPARK_BUILD_TYPE', get_option('buildtype'))

# Generate version.h and install to <includedir>/spark
spark_version_h = configure_file(
  input: 'version.h.in',
  output: 'version.h',
  configuration: conf,
  install: true,
  install_dir: join_paths(get_option('includedir'), 'spark')
)
