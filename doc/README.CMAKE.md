Updated 2025-04

## Building TbaMUD with the cmake tool

# Building with CMake

This document describes how to configure, build and install tbamud
from source code using the CMake build tool. To build with CMake, you of
course first have to install CMake. The minimum required version of CMake is
specified in the file `CMakeLists.txt` found in the top of the tbamud source
tree. Once the correct version of CMake is installed you can follow the
instructions below for the platform you are building on.

CMake builds can be configured either from the command line, or from one of
CMake's GUIs. 

NOTE: The current CMakeLists.txt only supports linux.

# Configuring

A CMake configuration of tbamud is similar to the autotools build of curl.
It consists of the following steps after you have unpacked the source.

We recommend building with CMake on Windows. 

## Using `cmake`

You can configure for in source tree builds or for a build tree
that is apart from the source tree.

- Build in a separate directory (parallel to the source tree in this
  example). The build directory is created for you. This is recommended over
  building in the source tree to separate source and build artifacts.

```shell
$ cmake -B build -S .
```

- Build in the source tree. Not recommended.

```shell
$ cmake -B .
```

The examples below will assume you have created a build folder.

The above commands will generate the build files. If you need to regenerate
the files, you can delete the cmake cache file, and rerun the above command:

```shell
$ rm build/CMakeCache.txt
```

Once the build files are generated, the build is run with cmake

```shell
$ cmake --build build
```

This will generate the object files in a subdirectory under the specified
build folder and link the executable. The resulting binaries will be in the
bin/ folder.

### Utilities

It is possible to build only single tools, none or all of them,
by specifying the target in the build command:

```shell
# only build the mud
$ cmake --build build --target circle

# only build tools
$ cmake --build build --target utils

# only build one tool
$ cmake --build build --target wld2html
```

### Debugging memory

In case you want to run the mud with memory debugging turned on, you
can set the MEMORY_DEBUG flag during configuration by specifying the 
flag:

```shell
$ cmake -B build -S . -DMEMORY_DEBUG:int=1
$ cmake --build build
```

When the mud is shut down, the zmalloc code will identify any leaks in your code.
Note that memory debugging may consume quite a lot of memory and take some time
to be handled on shutdown.