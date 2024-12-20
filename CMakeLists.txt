cmake_minimum_required(VERSION 2.8.7)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")
include(AppendCompilerFlags)
set(CMAKE_OSX_ARCHITECTURES x86_64)

project(CLTJ)
set(CMAKE_PROJECT_NAME "CLTJ")

#Change to "Debug" when you need it
set(CMAKE_BUILD_TYPE "Debug")

# C++11 compiler Check
if(NOT CMAKE_CXX_COMPILER_VERSION) # work around for cmake versions smaller than 2.8.10
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE CMAKE_CXX_COMPILER_VERSION)
endif()
if(CMAKE_CXX_COMPILER MATCHES ".*clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_COMPILER_IS_CLANGXX 1)
endif()
if( (CMAKE_COMPILER_IS_GNUCXX AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 4.7) OR
(CMAKE_COMPILER_IS_CLANGXX AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 3.2))
    message(FATAL_ERROR "Your C++ compiler does not support C++11. Please install g++ 4.7 (or greater) or clang 3.2 (or greater)")
else()
    message(STATUS "Compiler is recent enough to support C++11.")
endif()
if( CMAKE_COMPILER_IS_GNUCXX )
    append_cxx_compiler_flags("-fpermissive -std=c++11 -Wall -Wextra " "GCC" CMAKE_CXX_FLAGS)
    append_cxx_compiler_flags("-msse4.2 -O3 -ffast-math -funroll-loops -fno-omit-frame-pointer -g" "GCC" CMAKE_CXX_FLAGS_RELEASE)
else()
    append_cxx_compiler_flags("-fpermissive -std=c++11" "CLANG" CMAKE_CXX_FLAGS)
    append_cxx_compiler_flags("-stdlib=libc++" "CLANG" CMAKE_CXX_FLAGS)
    append_cxx_compiler_flags("-msse4.2 -O3  -ffast-math -funroll-loops -DNDEBUG" "CLANG" CMAKE_CXX_FLAGS_RELEASE)
endif()
include(CheckSSE)
FindSSE ()
if( SSE4_2_FOUND )
    if( CMAKE_COMPILER_IS_GNUCXX )
        append_cxx_compiler_flags("-msse4.2" "GCC" CMAKE_CXX_FLAGS)
    else()
        append_cxx_compiler_flags("-msse4.2" "CLANG" CMAKE_CXX_FLAGS)
    endif()
    message(STATUS "CPU does support SSE4.2.")
else()
    message(STATUS "CPU does NOT support SSE4.2")
endif()

include_directories(~/include
                    ${CMAKE_HOME_DIRECTORY}/lib/hybridBV/include
                    ${CMAKE_HOME_DIRECTORY}/include)

link_directories(~/lib)
add_subdirectory(lib/hybridBV)

#BENCH EXECUTABLES
add_executable(build-index src/bench/build-index.cpp)
target_link_libraries(build-index sdsl divsufsort divsufsort64)
set_target_properties(build-index PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(build-index-dyn src/bench/build-index-dyn.cpp)
target_link_libraries(build-index-dyn sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(build-index-dyn PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(build-metatrie-index src/bench/build-metatrie-index.cpp)
target_link_libraries(build-metatrie-index sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(build-metatrie-index PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(build-metatrie-index-dyn src/bench/build-metatrie-index-dyn.cpp)
target_link_libraries(build-metatrie-index-dyn sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(build-metatrie-index-dyn PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(build-uncltj-index src/bench/build-uncltj-index.cpp)
target_link_libraries(build-uncltj-index sdsl divsufsort divsufsort64)
set_target_properties(build-uncltj-index PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(build-index-triples src/bench/build-index-triples.cpp)
target_link_libraries(build-index-triples sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(build-index-triples PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(build-index-bulk-triples src/bench/build-index-bulk-triples.cpp)
target_link_libraries(build-index-bulk-triples sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(build-index-bulk-triples PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-query-index src/bench/bench-query-index.cpp)
target_compile_definitions(bench-query-index PUBLIC ADAPTIVE=1)
target_link_libraries(bench-query-index sdsl divsufsort divsufsort64)
set_target_properties(bench-query-index PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-query-index-global src/bench/bench-query-index.cpp)
target_compile_definitions(bench-query-index-global PUBLIC ADAPTIVE=0)
target_link_libraries(bench-query-index-global sdsl divsufsort divsufsort64)
set_target_properties(bench-query-index-global PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-query-uncltj-index src/bench/bench-query-uncltj-index.cpp)
target_compile_definitions(bench-query-uncltj-index PUBLIC ADAPTIVE=1)
target_link_libraries(bench-query-uncltj-index sdsl divsufsort divsufsort64)
set_target_properties(bench-query-uncltj-index PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-query-uncltj-index-global src/bench/bench-query-uncltj-index.cpp)
target_compile_definitions(bench-query-uncltj-index-global PUBLIC ADAPTIVE=0)
target_link_libraries(bench-query-uncltj-index-global sdsl divsufsort divsufsort64)
set_target_properties(bench-query-uncltj-index-global PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-query-metatrie-index src/bench/bench-query-metatrie-index.cpp)
target_compile_definitions(bench-query-metatrie-index PUBLIC ADAPTIVE=1)
target_link_libraries(bench-query-metatrie-index sdsl divsufsort divsufsort64)
set_target_properties(bench-query-metatrie-index PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-query-metatrie-index-global src/bench/bench-query-metatrie-index.cpp)
target_compile_definitions(bench-query-metatrie-index-global PUBLIC ADAPTIVE=0)
target_link_libraries(bench-query-metatrie-index-global sdsl divsufsort divsufsort64)
set_target_properties(bench-query-metatrie-index-global PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-query-index-triples src/bench/bench-query-index-triples.cpp)
target_link_libraries(bench-query-index-triples sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(bench-query-index-triples PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-update-ratio src/bench/bench-update-ratio.cpp)
target_compile_definitions(bench-update-ratio PUBLIC ADAPTIVE=1)
target_link_libraries(bench-update-ratio sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(bench-update-ratio PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-update-metatrie-ratio src/bench/bench-update-metatrie-ratio.cpp)
target_compile_definitions(bench-update-metatrie-ratio PUBLIC ADAPTIVE=1)
target_link_libraries(bench-update-metatrie-ratio sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(bench-update-metatrie-ratio PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)


#EXAMPLE

add_executable(ex_dyn_rdf src/example/ex_dyn_rdf.cpp)
target_link_libraries(ex_dyn_rdf sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(ex_dyn_rdf PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/example)

add_executable(ex_dyn_ids src/example/ex_dyn_ids.cpp)
target_link_libraries(ex_dyn_ids sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(ex_dyn_ids PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/example)

add_executable(ex_static_ids src/example/ex_static_ids.cpp)
target_link_libraries(ex_static_ids sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(ex_static_ids PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/example)



add_executable(test-index-dyn src/test-index-dyn.cpp)
target_link_libraries(test-index-dyn sdsl divsufsort divsufsort64 hybridbv_gn)

add_executable(test-index-static src/test-index-static.cpp)
target_link_libraries(test-index-static sdsl divsufsort divsufsort64)

add_executable(main src/main.cpp)
target_link_libraries(main sdsl divsufsort divsufsort64 hybridbv_gn)










#TODO: activar ADAPTIVE


add_executable(dyn-exp1 src/dyn-exp1.cpp)
target_compile_definitions(dyn-exp1 PUBLIC ADAPTIVE=1)
target_link_libraries(dyn-exp1 sdsl divsufsort divsufsort64 hybridbv_gn)

add_executable(dyn-exp1-metatrie src/dyn-exp1-metatrie.cpp)
target_compile_definitions(dyn-exp1-metatrie PUBLIC ADAPTIVE=1)
target_link_libraries(dyn-exp1-metatrie sdsl divsufsort divsufsort64 hybridbv_gn)

add_executable(dyn-exp1-fixed src/dyn-exp1.cpp)
target_compile_definitions(dyn-exp1-fixed PUBLIC ADAPTIVE=0)
target_link_libraries(dyn-exp1-fixed sdsl divsufsort divsufsort64 hybridbv_gn)

add_executable(dyn-exp2 src/dyn-exp2.cpp)
target_compile_definitions(dyn-exp2 PUBLIC ADAPTIVE=1)
target_link_libraries(dyn-exp2 sdsl divsufsort divsufsort64 hybridbv_gn)

add_executable(dyn-exp2-fixed src/dyn-exp2.cpp)
target_compile_definitions(dyn-exp2-fixed PUBLIC ADAPTIVE=0)
target_link_libraries(dyn-exp2-fixed sdsl divsufsort divsufsort64 hybridbv_gn)

add_executable(dyn-exp3 src/dyn-exp3.cpp)
target_compile_definitions(dyn-exp3 PUBLIC ADAPTIVE=1)
target_link_libraries(dyn-exp3 sdsl divsufsort divsufsort64 hybridbv_gn)

add_executable(dyn-exp3-fixed src/dyn-exp3.cpp)
target_compile_definitions(dyn-exp3-fixed PUBLIC ADAPTIVE=0)
target_link_libraries(dyn-exp3-fixed sdsl divsufsort divsufsort64 hybridbv_gn)

add_executable(test-dict src/test-dict.cpp)
target_link_libraries(test-dict sdsl divsufsort divsufsort64)

add_executable(test-regex src/test-regex.cpp)
target_link_libraries(test-regex sdsl divsufsort divsufsort64)

add_executable(test-bulk-load src/test-bulk-load.cpp)
target_link_libraries(test-bulk-load sdsl divsufsort divsufsort64)

add_executable(test-mt-dyn src/test-mt-dyn.cpp)
target_link_libraries(test-mt-dyn sdsl divsufsort divsufsort64 hybridbv_gn)






add_executable(interactive-metatrie src/interactive-metatrie.cpp)
target_link_libraries(interactive-metatrie sdsl divsufsort divsufsort64 hybridbv_gn)

add_executable(update-index-triples src/update-index-triples.cpp)
target_link_libraries(update-index-triples sdsl divsufsort divsufsort64 hybridbv_gn)



add_executable(prepare-80 src/prepare-80.cpp)
target_link_libraries(prepare-80 sdsl divsufsort divsufsort64)

add_executable(test-dyn src/test-dyn.cpp)
target_link_libraries(test-dyn sdsl divsufsort divsufsort64 hybridbv_gn)