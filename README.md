## CPUID

[![Build Status](https://travis-ci.org/tycho/cpuid.svg?branch=master)](https://travis-ci.org/tycho/cpuid)

"cpuid" is a very simple C program, designed to dump and extract information
from the x86 CPUID instruction.

cpuid is capable of dumping all CPUID leaves (except any unknown leaves which
require special ECX values to dump all information). cpuid can only decode
certain leaves, but this functionality will be expanded as the CPUID
specifications provided by AMD and Intel change.


### Building

The build process is simplified compared to a plethora of other open source
projects out there. You don't need autoconf/automake or any of the headaches
that go along with those tools.

Required:
- C compiler (GNU C Compiler, LLVM/Clang, Microsoft Visual C++, and the Intel
  C++ Compiler are known to work)
- Perl 5.8 or later

And one of:
- GNU Make 3.80 or later
- Meson 0.50 or later

Depending on whether you have GNU Make or Meson, do one of:

```
$ meson . build
$ ninja -C build
```

or

```
$ make
```

And you should have a new executable called 'cpuid' in a few seconds.


### Usage

Since the usage will likely change over time, I recommend that you take a look
at the output of:

```
$ ./cpuid --help
```


### Reporting Bugs

If you find a bug in CPUID, please submit details about it to the [issue
tracker](https://github.com/tycho/cpuid/issues) on GitHub.

If the bug is regarding the decoding or dumping of CPUID details, then you
should include the dump.txt and decode.txt generated with these commands:

```
$ ./cpuid -d -c -1 > dump.txt
$ ./cpuid -c -1 > decode.txt
```

You should also specify what revision of CPUID you are running. If you don't
know, you can find out with:

```
$ ./cpuid --version
```


### Reference Documentation

You can find current Intel and AMD CPUID specifications at these locations:

- [Intel Software Developer Manual volume 2A](https://www.intel.com/content/www/us/en/architecture-and-technology/64-ia-32-architectures-software-developer-vol-2a-manual.html)
- [Intel Architecture Instruction Set Extensions Programming Reference](https://software.intel.com/en-us/download/intel-architecture-instruction-set-extensions-programming-reference)
- [AMD Processor Programming Reference](http://developer.amd.com/resources/developer-guides-manuals/)

I try to keep up with these as they change, but sometimes I'm slow on the
uptake. Please notify me if you notice any inconsistencies or if features you
find relevant are not being decoded.


### Contact

I'm contactable via email and respond frequently.

[Steven Noonan \<steven@uplinklabs.net>](mailto:steven@uplinklabs.net)
