# hscompile
This repository contains a compiler and runtime for MNRL files
using Hyperscan as a CPU back-end.  There is also support for compiling PCRE to MNRL files.

*Note:* Only hStates are currently supported.

## Required Packages

- g++-5, gcc-5 !!(Newer versions of gcc/g++ currently aren't supported)!!
- cmake >= 2.6
- Hyperscan 4.4 source
    - Boost >= 1.57
- MNRL

## Building
To successfully build `hscompile`, you must first build Hyperscan and the MNRL C++ API:

```bash
git clone https://github.com/tjt7a/hscompile.git
```

```bash
cd hscompile
mkdir lib
cd lib
git clone https://github.com/01org/hyperscan
cd hyperscan
git checkout v4.4.1
mkdir build
cd build
cmake ..
make
```

```bash
cd ..
git clone https://github.com/kevinaangstadt/mnrl
git checkout v1.0
cd mnrl/C++
make
```

Next, clone the `hscompile` repository and create a build directory inside the repo.  Then, you can use `cmake` to generate a Makefile and build.  You must provide paths to Hyperscan (`HS_SOURCE_DIR`) and MNRL (`MNRL_SOURCE_DIR`), and you can override the default Hyperscan build path with `HS_BUILD_DIR`.

```bash
cd .. 
mkdir build
cd build

cmake -DCMAKE_CXX_COMPILER=g++-5 -DCMAKE_C_COMPILER=gcc-5 -DHS_SOURCE_DIR=/home/tjt7a/src/hscompile/lib/hyperscan -DMNRL_SOURCE_DIR=/home/tjt7a/src/hscompile/lib/mnrl/C++ ..


make
```
