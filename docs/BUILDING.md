# Building instructions

## Quick start

Create a VM with [multipass](https://multipass.run/), download ServerlessOnEdge, do a minimal build it with debug options (without OpenCV / proxygen / mvfst), and run the unit test:

```
sudo multipass launch bionic -c 5 -m 10G -d 10G -n soe-bionic
sudo multipass shell soe-bionic
git clone https://github.com/ccicconetti/serverlessonedge.git
cd serverlessonedge/
git submodule update --init --recursive
sudo etsimec/utils/build_deps.sh
cd build/debug/
../build.sh g++
ctest
```

## Longer version

1. Clone the git repository:

```
git clone https://github.com/ccicconetti/serverlessonedge.git
cd serverlessonedge
git submodule update --init --recursive
```

2. Install dependencies (see below)

3. Compile (assuming `g++` is your preferred compiler):

```
cd build/debug
../build.sh g++
make
```

4. [optional] Run unit tests:

```
ctest
```

Note that [gmock](https://github.com/google/googlemock) is also needed to compile the unit tests but, if everything goes right, it will be downloaded automatically by CMake (needless to say: you do need a working Internet connection for this step).

This will compile the full build tree without optional components. The executables will be in `build/debug/Executables` from the repo root.

If you want to compile with compiler optimisations and no assertions / unit tests:

```
cd build/release
../build.sh g++
make
```

### Optional components

#### Compile with OpenCV

If you want to compile also the executables and libraries depending OpenCV, first install the related dependencies (see below) and then run build with `-DWITH_OPENCV=1`.

#### Compile with QUIC/proxygen (Linux only)

If you want to compile with QUIC support via proxygen, then first compile [proxygen](https://github.com/facebook/proxygen) with [mvfst](https://github.com/facebookincubator/mvfst) support with (tested successfully only on Linux/Ubuntu 18.04):

```
cd thirdparty
./build_deps.sh
cd ..
```

Then build with `-DWITH_QUIC=1`.

#### Compile with QUIC/mvfst (Linux only)

If you want to compile with QUIC support via proxygen, then first compile [proxygen](https://github.com/facebook/proxygen) with [mvfst](https://github.com/facebookincubator/mvfst) support with (tested successfully only on Linux/Ubuntu 18.04):

```
cd thirdparty
./build_mvfst.sh
cd ..
```

Then build with `-DWITH_MVFST=1`.

## Dependencies

Compiling the software requires the following:

- recent C++ compiler, tested with clang-15
- [CMake](https://cmake.org/) >= 3.18
- [glog](https://github.com/google/glog), tested with 0.3.5
- [Boost](https://www.boost.org/), tested with 1.65
- [gRPC](https://grpc.io/), tested with 1.44.0
- [protobuf](https://developers.google.com/protocol-buffers/), tested with 3.19.2
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

On Linux this is a bit more complicated. A script that downloads and installs all dependencies can be found in the repo, which assumes that you are using `Ubuntu 18.04 (Bionic)`. Note that the script requires root privileges, it will change the system default CMake version and it will install headers and libraries in the system path `/usr/local`. To run it just hit (from within the cloned `serverlessonedge` repository):

```
[sudo] etsimec/utils/build_deps.sh
```

from the same directory where you have cloned the repository. This will require about 2 GB of disk space, most of which can be freed after installation by removing the local repositories with compilation artifacts.

If you want to compile OpenCV, run also:

```
[sudo] utils/build_opencv.sh
```

Note that this requires about 12 GB of free space (!).