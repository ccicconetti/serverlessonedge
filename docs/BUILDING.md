# Building instructions

First, make sure you have [CMake](https://cmake.org/) (version >= 3.2), a recent C++ compiler and all the dependencies installed:

- [glog](https://github.com/google/glog)
- [Boost](https://www.boost.org/)
- [gRPC](https://grpc.io/)
- [protobuf](https://developers.google.com/protocol-buffers/)
- [C++ REST SDK](https://github.com/microsoft/cpprestsdk)
- [OpenCV](https://opencv.org/)

Very likely you can find packaged versions in your system's package repository.
Make sure you are installing the development version of the packages, which also include header files.
gRPC is better built from source, and it may automatically download and compile protobuf too (recommended).

Note that [gmock](https://github.com/google/googlemock) is also needed to compile the unit tests but, if everything goes right, it will be downloaded automatically by CMake (needless to say: you do need a working Internet connection for this step).

```
git clone https://github.com/ccicconetti/serverlessonedge.git
git submodule update --init --recursive
```

Once everything is ready (assuming `clang++` is your compiler):

```
cd build/debug
../build.sh clang++
make
```

This will compile the full build tree. The executables will be in `build/debug/Executables` from the repo root.

The unit tests can be executed by running `build/debug/Test/testedge` or, if you want to execute _all_ the unit tests (including those of the sub-modules), run `NOBUILD=1 etsimec/rest/support/jenkins/run_tests.sh`. Note that without setting the environment variable `NOBUILD` the script will remove the full content of the build directory.

If you want to compile with compiler optimisations and no assertions:

```
cd build/release
../build.sh clang++
make
```

In this case the unit tests will not be compiled (gmock will not be even downloaded).

## Mac OS X

Tested with clang-11.0.0 and with the following dependencies installed via [Homebrew](https://brew.sh/):

- glog: 0.4.0
- boost: 1.72.0
- gRPC: 1.27.2
- protobuf: 3.11.4
- cpprestsdk: 2.10.14_1
- opencv: 4.2.0_1

## Linux

Tested with gcc-7.4.0 and clang-10.0.0 on  Ubuntu 18.04 (Bionic).

The following depencies have been installed via `apt`:

```
apt install libgoogle-glog-dev libboost-all-dev
```

On the other hand, the dependencies below have been compiled and installed from source:

- gRPC at tag v1.27.2 (with protobuf included as third-party software), see GitHub repo [here](https://github.com/grpc/grpc)
- C++ REST SDK at tag v2.10.14, see GitHub repo [here](https://github.com/microsoft/cpprestsdk)
- OpenCV at tag v4.1.1, see GitHub repo [here](https://github.com/opencv/opencv)
