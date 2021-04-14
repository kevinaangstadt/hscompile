# hscompile
This repository contains a compiler and runtime for MNRL files
using Hyperscan as a CPU back-end.  There is also support for compiling PCRE to MNRL files.

*Note:* Only hStates are currently supported.

## Required Packages

- cmake >= 2.6
- Hyperscan 4.4 source
    - Boost >= 1.57
- MNRL 1.0

## Building
To successfully build `hscompile`, you must first build Hyperscan and the MNRL C++ API:

```bash
git clone https://github.com/01org/hyperscan
cd hyperscan
git checkout v4.4.1
mkdir build
cd build
cmake ..
make
```

```bash
git clone https://github.com/kevinaangstadt/mnrl
cd mnrl
git checkout v1.0
cd C++
git submodule init
git submodule update   
cd lib/valijson
git checkout v0.3
cd ../..
```

edit the Makefile and change 
```
JSON = ./lib/valijson/thirdparty/nlohmann-json-1.1.0
```
with
```
JSON = ./lib/valijson/thirdparty/nlohmann-json-3.1.2
```
then
```
make
```

Next, clone the `hscompile` repository and create a build directory inside the repo.  Then, you can use `cmake` to generate a Makefile and build.  You must provide paths to Hyperscan (`HS_SOURCE_DIR`) and MNRL (`MNRL_SOURCE_DIR`), and you can override the default Hyperscan build path with `HS_BUILD_DIR`.

```bash
git clone https://github.com/kevinaangstadt/hscompile
cd hscompile
mkdir build
cd build
cmake -DHS_SOURCE_DIR=/path/to/hyperscan -DMNRL_SOURCE_DIR=/path/to/mnrl/C++ ..
make
```
