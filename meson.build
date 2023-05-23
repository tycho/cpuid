project('cpuid', 'c',
    default_options: [
        'buildtype=minsize',
        'c_std=gnu99',
        'b_lto=true',
    ]
)

compiler = meson.get_compiler('c')
perl = find_program('perl')
is_sanitize = get_option('b_sanitize')

gen_build_h = custom_target('build.h',
                            output : ['build.h'],
                            command : [perl, meson.current_source_dir() + '/tools/build.pl', '@OUTPUT@'])

gen_license_h = custom_target('license.h',
                              input : ['LICENSE'],
                              output : ['license.h'],
                              command : [perl, meson.current_source_dir() + '/tools/license.pl', '@INPUT@', '@OUTPUT@'])

src = ['cache.c', 'clock.c', 'cpuid.c', 'feature.c', 'handlers.c', 'main.c', 'sanity.c', 'threads.c', 'util.c', 'version.c']

c_flags = []
if is_sanitize != 'none'
    c_flags += [
        compiler.first_supported_argument([
            '-fno-sanitize-recover=all'
        ])
    ]
endif

incdirs = [include_directories('.')]

dependencies = [
    dependency('threads'),
]

if host_machine.system() != 'windows'
    dependencies += compiler.find_library('m')
endif

if target_machine.system() == 'windows' or target_machine.system() == 'cygwin'
    dependencies += compiler.find_library('winmm')
endif

if not compiler.has_header('getopt.h')
    incdirs += include_directories('getopt')
    src += 'getopt/getopt_long.c'
endif

executable('cpuid',
           src,
           gen_build_h,
           gen_license_h,
           c_args : c_flags,
           include_directories : incdirs,
           dependencies : dependencies)

# vim: set ts=4 sts=4 sw=4 et:
