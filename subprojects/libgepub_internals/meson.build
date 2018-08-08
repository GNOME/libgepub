project(
  'libgepub_internals', 'rust',
  version: '3.29.6',
  license: 'GPLv3',
)

libgepub_internals_version = meson.project_version()
version_array = libgepub_internals_version.split('.')
libgepub_internals_major_version = version_array[0].to_int()
libgepub_internals_minor_version = version_array[1].to_int()
libgepub_internals_version_micro = version_array[2].to_int()

libgepub_internals_prefix = get_option('prefix')

cargo = find_program('cargo', required: true)
cargo_vendor = find_program('cargo-vendor', required: false)
cargo_script = find_program('scripts/cargo.sh')
grabber = find_program('scripts/grabber.sh')
cargo_release = find_program('scripts/release.sh')

c = run_command(grabber)
sources = c.stdout().strip().split('\n')

cargo_build = custom_target('cargo-build',
                        build_by_default: true,
                        input: sources,
                        output: ['libgepub_internals'],
                        install: false,
                        command: [cargo_script, '@CURRENT_SOURCE_DIR@', '@OUTPUT@'])

#gen = generator(cargo_script,
#                        arguments: ['@CURRENT_SOURCE_DIR@', '@OUTPUT@'],
#                        output: '@BASENAME@.a')
#libfiles = gen.process('libgepub_internals')
libgepub_internals_lib = static_library('gepub_internals', cargo_build)

libgepub_internals_dep = declare_dependency(
  link_with: libgepub_internals_lib,
  sources: cargo_build,
)
