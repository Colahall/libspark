fs = import('fs')

# Find doxygen
doxygen = find_program('doxygen', required : false)

if doxygen.found()
    # Provide custom config
    cdata = configuration_data()
    cdata.set('VERSION', meson.project_version())
    cdata.set('PROJECT_NAME', meson.project_name())
    cdata.set('SOURCE_DIR', meson.global_source_root())
    cdata.set('BUILD_DIR', meson.global_build_root())

    # Output of Doxygen configuration
    doxyfile = configure_file(
        input: 'Doxygen.in',
        output: 'Doxygen',
        configuration: cdata,
    )

    # Generate documentation
    docs = custom_target('docs',
        input: [doxyfile, libspark],
        output: 'html',
        command: [doxygen, doxyfile],
        install: true,
        install_dir: join_paths(get_option('datadir'), 'doc', meson.project_name())
    )
endif