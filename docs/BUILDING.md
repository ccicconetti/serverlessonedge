# Building instructions

1. Clone the git repository:

```
git clone https://github.com/ccicconetti/serverlessonedge.git
git submodule update --init --recursive
```

2. Install dependencies (see below)

3. Compile (assuming `g++` is your preferred compiler):

```
cd etsimec/build/debug
../build.sh g++
make
```

Note that [gmock](https://github.com/google/googlemock) is also needed to compile the unit tests but, if everything goes right, it will be downloaded automatically by CMake (needless to say: you do need a working Internet connection for this step).

This will compile the full build tree *except OpenCV*. The executables will be in `build/debug/Executables` from the repo root.

The unit tests can be executed by running `build/debug/Test/testedge` or, if you want to execute _all_ the unit tests (including those of the sub-modules), run `NOBUILD=1 etsimec/rest/support/jenkins/run_tests.sh`. Note that without setting the environment variable `NOBUILD` the script will remove the full content of the build directory.

If you want to compile with compiler optimisations and no assertions:

```
cd build/release
../build.sh g++
make
```

The unit tests will not be compiled (gmock will not be even downloaded).

If you want to compile *also the executables and libraries depending OpenCV* then run (e.g., in debug mode):

```
cd etsimec/build/debug
../build.sh g++ -DWITH_OPENCV=1
make
```

Note that this requires extra dependencies, as explained below.

## Dependencies

Compiling the software requires the following:

- recent C++ compiler, tested with clang-10 and cc-7
- [CMake](https://cmake.org/) >= 3.2
- [glog](https://github.com/google/glog), tested with 0.3.5
- [Boost](https://www.boost.org/), tested with 1.65
- [gRPC](https://grpc.io/), tested with 1.27.1
- [protobuf](https://developers.google.com/protocol-buffers/), tested with version shipped with gRPC 1.27.1
- [OpenCV](https://opencv.org/) (_optional_), tested with 4.1.1 and 4.2.0

### Mac OS X

With Mac OS X this should be as simple as installing everything via [Homebrew](https://brew.sh/):

```
brew install grpc protobuf cpprestsdk boost glog opencv
```

If you also want to compile OpenCV stuff:

```
brew install opencv
```

### Linux

On Linux this is a bit more complicated. A script that downloads and installs all dependencies can be found in the repo, which assumes that you are using `Ubuntu 18.04 (Bionic)`. Note that the script requires root privileges, it will change the system default CMake version to 3.16.1, and it will install headers and libraries in the system path `/usr/local`. To run it just hit:

```
[sudo] serverlessonedge/etsimec/utils/build_deps.sh
```

from the same directory where you have cloned the repository. This will require about 2 GB of disk space, most of which can be freed after installation by removing the local repositories with compilation artifacts.

If you want to compile OpenCV, run also:

```
[sudo] serverlessonedge/utils/build_opencv.sh
```

Note that this requires about 12 GB of free space (!).

### Supported versions

Tested on Mac OS X 10.14.5  with clang-11.0.0 and with the following dependencies installed via [Homebrew](https://brew.sh/):

- glog: 0.4.0
- boost: 1.72.0
- gRPC: 1.27.2
- protobuf: 3.11.4
- cpprestsdk: 2.10.14_1
- opencv: 4.2.0_1

Tested on Linux Ubuntu 18.04 (Bionic)  with gcc-7.4.0 and clang-10.0.0, with the following dependencies:

- glog from `apt` at version 0.3.5-1
- boost from `apt` at version 1.65.1
- gRPC at tag v1.27.2 (with protobuf included as third-party software), see GitHub repo [here](https://github.com/grpc/grpc)
- C++ REST SDK at tag v2.10.14, see GitHub repo [here](https://github.com/microsoft/cpprestsdk)
- OpenCV at tag v4.1.1, see GitHub repo [here](https://github.com/opencv/opencv)
