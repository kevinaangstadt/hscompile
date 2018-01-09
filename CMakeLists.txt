cmake_minimum_required(VERSION 2.6)

project(hscompile C CXX)

# deal with Boost
set(BOOST_MINVERSION 1.57.0)

find_package(Boost ${BOOST_MINVERSION} QUIET COMPONENTS regex)
if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost ${BOOST_MINVERSION} or later not found. Either install system packages if available or set the CMake BOOST_ROOT variable.")
endif()

message(STATUS "Boost version: ${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}")

# set up Hyperscan sources
set(HS_SOURCE_DIR "" CACHE PATH "Path to Hyperscan source")
message(STATUS "Hyperscan source: ${HS_SOURCE_DIR}")
if((${HS_SOURCE_DIR} STREQUAL "") OR (NOT EXISTS "${HS_SOURCE_DIR}/CMakeLists.txt"))
    message(FATAL_ERROR "HS_SOURCE_DIR isn't set properly! Be sure to set -DHS_SOURCE_DIR when you call cmake.")
endif()

# set up Hyperscan libraries
set(HS_BUILD_DIR "${HS_SOURCE_DIR}/build" CACHE PATH "Path to Hyperscan build directory")
message(STATUS "Hyperscan build: ${HS_BUILD_DIR}")
if(NOT EXISTS "${HS_BUILD_DIR}/lib/libhs.a")
    message(FATAL_ERROR "HS_BUILD_DIR isn't set properly! Be sure that Hyperscan is build or specify location on command line with -DHS_BUILD_DIR.")
endif()

# set up MNRL sources
set(MNRL_SOURCE_DIR "" CACHE PATH "Path to MNRL C++ source")
message(STATUS "MNRL source: ${MNRL_SOURCE_DIR}")
if((${MNRL_SOURCE_DIR} STREQUAL "") OR (NOT EXISTS "${MNRL_SOURCE_DIR}/libmnrl.a"))
    message(FATAL_ERROR "MNRL_SOURCE_DIR isn't set properly! Be sure to set -DMNRL_SOURCE_DIR when you call cmake and that you have built MNRL.")
endif()

# we use OpenMP
find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

# all of the include directories
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(SYSTEM include)

include_directories(${HS_SOURCE_DIR})
include_directories(${HS_SOURCE_DIR}/src)
include_directories(${HS_BUILD_DIR})

include_directories(${Boost_INCLUDE_DIR})

include_directories(${MNRL_SOURCE_DIR}/include)

set(hscompile_SRC
    include/hs_compile_mnrl.h
    include/hs_compile_mnrl_int.h
    include/ht.h
    include/parse_symbol_set.h
    include/uthash.h
    src/hscompile.c
    src/hs_compile_mnrl.cpp
    src/parse_symbol_set.cpp
)

set(hsrun_SRC
    include/hs_compile_mnrl.h
    include/ht.h
    include/read_input.h
    include/uthash.h
    src/hsrun.c
)

set(pcre2mnrl_SRC
    include/hs_pcre_mnrl.h
    include/hs_pcre_mnrl_int.h
    src/dump_charclass.cpp
    src/hs_pcre_mnrl.cpp
    src/pcre2mnrl.cpp
)

add_executable(hscompile ${hscompile_SRC})
add_executable(hsrun ${hsrun_SRC})
add_executable(pcre2mnrl ${pcre2mnrl_SRC})

# we always want to link with CXX
set_target_properties(hscompile hsrun pcre2mnrl
    PROPERTIES LINKER_LANGUAGE CXX
    C_STANDARD 99
)

target_compile_features(hscompile PRIVATE cxx_range_for)
target_compile_features(hsrun PRIVATE cxx_range_for)
target_compile_features(pcre2mnrl PRIVATE cxx_range_for)

target_link_libraries(hscompile ${HS_BUILD_DIR}/lib/libhs.a ${MNRL_SOURCE_DIR}/libmnrl.a)
target_link_libraries(hsrun ${HS_BUILD_DIR}/lib/libhs.a)
target_link_libraries(pcre2mnrl ${HS_BUILD_DIR}/lib/libhs.a ${MNRL_SOURCE_DIR}/libmnrl.a ${Boost_LIBRARIES})
