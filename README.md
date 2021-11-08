# tobsterlang

Tobsterlang is a simple imperative programming language, written in C++ with [LLVM](https://llvm.org). One of its
distinct features is the fact it uses [XML](https://en.wikipedia.org/wiki/XML) instead of a fancy, convenient for humans
to write language.

## Table of Contents

- [Background](#background)
- [Quick Start](#quick-start)
    - [Dependencies](#dependencies)
    - [Building](#building)

## Background

The idea for Tobsterlang came when I was extraordinarily bored. I thought it'd be a fun challenge to try and make my own
somewhat esoteric programming language. I have no concrete vision for the path this project will take or what the end
goal will be, but I'm gradually adding more and more stuff to it, and hopefully it'll become complete enough to be even
remotely useful some day.

## Quick Start

### Dependencies

- [LLVM](https://llvm.org)
- [Clang](https://clang.llvm.org)
- [Boost](https://www.boost.org)
    - [program_options](https://github.com/boostorg/program_options)
    - [property_tree](https://github.com/boostorg/property_tree)

### Building

Tobsterlang uses the [CMake](https://cmake.org) build system, and is supposed to be compiled using Clang. To build, run
the following commands:

```console
$ cmake \
  -DCMAKE_C_COMPILER=$(which clang) \
  -DCMAKE_CXX_COMPILER=$(which clang++) \
  -DCMAKE_BUILD_TYPE=Release -B ./build
$ cmake --build ./build --config Release
$ ./build/tobsterlang -h
```