project('cpuid', 'c',
    default_options: [
        'buildtype=minsize',
        'c_std=gnu99',
        'b_lto=true',
    ]
)

compiler = meson.get_compiler('c')
perl = find_program('perl')


gen_build_h = custom_target('build.h',
                            output : ['build.h'],
                            command : [perl, meson.current_source_dir() + '/tools/build.pl', '@OUTPUT@'])

gen_license_h = custom_target('license.h',
                              input : ['COPYING'],
                              output : ['license.h'],
                              command : [perl, meson.current_source_dir() + '/tools/license.pl', '@INPUT@', '@OUTPUT@'])

src = ['cache.c', 'clock.c', 'cpuid.c', 'feature.c', 'handlers.c', 'main.c', 'sanity.c', 'threads.c', 'util.c', 'version.c']

incdirs = include_directories('.')
lm = compiler.find_library('m')
threads = dependency('threads')

executable('cpuid',
           src,
           gen_build_h,
           gen_license_h,
           include_directories : incdirs,
           dependencies : [lm, threads])

# vim: set ts=4 sts=4 sw=4 et:
