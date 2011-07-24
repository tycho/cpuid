env = Environment()

debug = ARGUMENTS.get('debug', 0)

# Optimization levels
if int(debug):
	env.Append(CFLAGS='-O0 -ggdb')
else:
	env.Append(CFLAGS='-O2')

# Basic CFLAGS for correctness
env.Append(CFLAGS='-std=gnu89 -fno-strict-aliasing')

# Warning flags
env.Append(CFLAGS='-Wall -Wextra -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wmissing-declarations -Werror=declaration-after-statement')

# pthreads
env.Append(CFLAGS='-pthread')
env.Append(LINKFLAGS='-pthread')

sources = [
	'cache.c',
	'cpuid.c',
	'feature.c',
	'handlers.c',
	'main.c',
	'sanity.c',
	'threads.c',
	'util.c',
	'version.c',
]

env.Command('build.h', '', 'perl tools/build.pl build.h')
env.Command('license.h', '#COPYING', 'perl tools/license.pl COPYING license.h')

env.Program(target='cpuid', source=sources)
