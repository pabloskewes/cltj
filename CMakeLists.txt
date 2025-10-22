cmake_minimum_required(VERSION 3.18)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")
include(AppendCompilerFlags)

project(CLTJ LANGUAGES C CXX)
set(CMAKE_PROJECT_NAME "CLTJ")

# Build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Options to control permissive mode and libc++ usage
option(CLTJ_PERMISSIVE "Add -fpermissive to mimic legacy builds" ON)
option(CLTJ_USE_LIBCXX "Force -stdlib=libc++ (needed mainly on macOS)" ON)
set(CLTJ_LEGACY_CXX_STANDARD 11 CACHE STRING "C++ standard for legacy targets")
set_property(CACHE CLTJ_LEGACY_CXX_STANDARD PROPERTY STRINGS 11 14 17)

add_library(cltj_core INTERFACE)
target_compile_features(cltj_core INTERFACE cxx_std_${CLTJ_LEGACY_CXX_STANDARD})
target_compile_options(cltj_core INTERFACE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra>
  $<$<AND:$<BOOL:${CLTJ_PERMISSIVE}>,$<CXX_COMPILER_ID:GNU,Clang>>:-fpermissive>
  $<$<CONFIG:Release>:-O3 -ffast-math -funroll-loops>
  $<$<AND:$<BOOL:${CLTJ_USE_LIBCXX}>,$<CXX_COMPILER_ID:Clang>,${APPLE}>:-stdlib=libc++>
)
target_include_directories(cltj_core INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/hybridBV/include
)
include(CheckSSE)
FindSSE ()
if( SSE4_2_FOUND )
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
add_executable(build-cltj src/bench/build-cltj.cpp)
target_link_libraries(build-cltj PRIVATE cltj_core sdsl divsufsort divsufsort64)
set_target_properties(build-cltj PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(build-cltj-dyn src/bench/build-cltj-dyn.cpp)
target_link_libraries(build-cltj-dyn PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(build-cltj-dyn PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(build-xcltj src/bench/build-xcltj.cpp)
target_link_libraries(build-xcltj PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(build-xcltj PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(build-xcltj-dyn src/bench/build-xcltj-dyn.cpp)
target_link_libraries(build-xcltj-dyn PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(build-xcltj-dyn PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(build-uncltj src/bench/build-uncltj.cpp)
target_link_libraries(build-uncltj PRIVATE cltj_core sdsl divsufsort divsufsort64)
set_target_properties(build-uncltj PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(build-cltj-rdf src/bench/build-cltj-rdf.cpp)
target_link_libraries(build-cltj-rdf PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(build-cltj-rdf PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(build-xcltj-rdf src/bench/build-xcltj-rdf.cpp)
target_link_libraries(build-xcltj-rdf PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(build-xcltj-rdf PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-query-cltj src/bench/bench-query-cltj.cpp)
target_compile_definitions(bench-query-cltj PRIVATE ADAPTIVE=1)
target_link_libraries(bench-query-cltj PRIVATE cltj_core sdsl divsufsort divsufsort64)
set_target_properties(bench-query-cltj PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-query-cltj-global src/bench/bench-query-cltj.cpp)
target_compile_definitions(bench-query-cltj-global PRIVATE ADAPTIVE=0)
target_link_libraries(bench-query-cltj-global PRIVATE cltj_core sdsl divsufsort divsufsort64)
set_target_properties(bench-query-cltj-global PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-query-uncltj src/bench/bench-query-uncltj.cpp)
target_compile_definitions(bench-query-uncltj PRIVATE ADAPTIVE=1)
target_link_libraries(bench-query-uncltj PRIVATE cltj_core sdsl divsufsort divsufsort64)
set_target_properties(bench-query-uncltj PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-query-uncltj-global src/bench/bench-query-uncltj.cpp)
target_compile_definitions(bench-query-uncltj-global PRIVATE ADAPTIVE=0)
target_link_libraries(bench-query-uncltj-global PRIVATE cltj_core sdsl divsufsort divsufsort64)
set_target_properties(bench-query-uncltj-global PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-query-xcltj src/bench/bench-query-xcltj.cpp)
target_compile_definitions(bench-query-xcltj PRIVATE ADAPTIVE=1)
target_link_libraries(bench-query-xcltj PRIVATE cltj_core sdsl divsufsort divsufsort64)
set_target_properties(bench-query-xcltj PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-query-xcltj-global src/bench/bench-query-xcltj.cpp)
target_compile_definitions(bench-query-xcltj-global PRIVATE ADAPTIVE=0)
target_link_libraries(bench-query-xcltj-global PRIVATE cltj_core sdsl divsufsort divsufsort64)
set_target_properties(bench-query-xcltj-global PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-query-cltj-rdf src/bench/bench-query-cltj-rdf.cpp)
target_compile_definitions(bench-query-cltj-rdf PRIVATE ADAPTIVE=1)
target_link_libraries(bench-query-cltj-rdf PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(bench-query-cltj-rdf PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-query-xcltj-rdf src/bench/bench-query-xcltj-rdf.cpp)
target_compile_definitions(bench-query-xcltj-rdf PRIVATE ADAPTIVE=1)
target_link_libraries(bench-query-xcltj-rdf PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(bench-query-xcltj-rdf PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-update-cltj src/bench/bench-update-cltj.cpp)
target_compile_definitions(bench-update-cltj PRIVATE ADAPTIVE=1)
target_link_libraries(bench-update-cltj PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(bench-update-cltj PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-update-xcltj src/bench/bench-update-xcltj.cpp)
target_compile_definitions(bench-update-xcltj PRIVATE ADAPTIVE=1)
target_link_libraries(bench-update-xcltj PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(bench-update-xcltj PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-indels-cltj src/bench/bench-indels-cltj.cpp)
target_link_libraries(bench-indels-cltj PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(bench-indels-cltj PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-indels-xcltj src/bench/bench-indels-xcltj.cpp)
target_link_libraries(bench-indels-xcltj PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(bench-indels-xcltj PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(stats-query-cltj src/bench/stats-query-cltj.cpp)
target_link_libraries(stats-query-cltj PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(stats-query-cltj PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

#EXAMPLE
add_executable(ex_dyn_rdf src/example/ex_dyn_rdf.cpp)
target_link_libraries(ex_dyn_rdf PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(ex_dyn_rdf PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/example)

add_executable(ex_dyn_ids src/example/ex_dyn_ids.cpp)
target_link_libraries(ex_dyn_ids PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(ex_dyn_ids PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/example)

add_executable(ex_static_ids src/example/ex_static_ids.cpp)
target_link_libraries(ex_static_ids PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(ex_static_ids PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/example)

#CMD
add_executable(cmd-cltj src/cmd/cmd-cltj.cpp)
target_link_libraries(cmd-cltj PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(cmd-cltj PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/cmd)

add_executable(cmd-cltj-rdf src/cmd/cmd-cltj-rdf.cpp)
target_link_libraries(cmd-cltj-rdf PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(cmd-cltj-rdf PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/cmd)

#TEST
add_executable(test-index-dyn src/test/test-index-dyn.cpp)
target_link_libraries(test-index-dyn PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(test-index-dyn PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(test-index-static src/test/test-index-static.cpp)
target_link_libraries(test-index-static PRIVATE cltj_core sdsl divsufsort divsufsort64)
set_target_properties(test-index-static PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(main src/test/main.cpp)
target_link_libraries(main PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(main PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)



add_executable(dyn-exp1 src/test/dyn-exp1.cpp)
target_compile_definitions(dyn-exp1 PRIVATE ADAPTIVE=1)
target_link_libraries(dyn-exp1 PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(dyn-exp1 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(dyn-exp1-metatrie src/test/dyn-exp1-metatrie.cpp)
target_compile_definitions(dyn-exp1-metatrie PRIVATE ADAPTIVE=1)
target_link_libraries(dyn-exp1-metatrie PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(dyn-exp1-metatrie PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(dyn-exp1-fixed src/test/dyn-exp1.cpp)
target_compile_definitions(dyn-exp1-fixed PRIVATE ADAPTIVE=0)
target_link_libraries(dyn-exp1-fixed PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(dyn-exp1-fixed PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(dyn-exp2 src/test/dyn-exp2.cpp)
target_compile_definitions(dyn-exp2 PRIVATE ADAPTIVE=1)
target_link_libraries(dyn-exp2 PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(dyn-exp2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(dyn-exp2-fixed src/test/dyn-exp2.cpp)
target_compile_definitions(dyn-exp2-fixed PRIVATE ADAPTIVE=0)
target_link_libraries(dyn-exp2-fixed PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(dyn-exp2-fixed PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(dyn-exp3 src/test/dyn-exp3.cpp)
target_compile_definitions(dyn-exp3 PRIVATE ADAPTIVE=1)
target_link_libraries(dyn-exp3 PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(dyn-exp3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(dyn-exp3-fixed src/test/dyn-exp3.cpp)
target_compile_definitions(dyn-exp3-fixed PRIVATE ADAPTIVE=0)
target_link_libraries(dyn-exp3-fixed PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(dyn-exp3-fixed PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(test-dict src/test/test-dict.cpp)
target_link_libraries(test-dict PRIVATE cltj_core sdsl divsufsort divsufsort64)
set_target_properties(test-dict PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(test-regex src/test/test-regex.cpp)
target_link_libraries(test-regex PRIVATE cltj_core sdsl divsufsort divsufsort64)
set_target_properties(test-regex PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(test-bulk-load src/test/test-bulk-load.cpp)
target_link_libraries(test-bulk-load PRIVATE cltj_core sdsl divsufsort divsufsort64)
set_target_properties(test-bulk-load PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(test-mt-dyn src/test/test-mt-dyn.cpp)
target_link_libraries(test-mt-dyn PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(test-mt-dyn PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(interactive-metatrie src/test/interactive-metatrie.cpp)
target_link_libraries(interactive-metatrie PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(interactive-metatrie PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(update-index-triples src/test/update-index-triples.cpp)
target_link_libraries(update-index-triples PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(update-index-triples PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(prepare-80 src/test/prepare-80.cpp)
target_link_libraries(prepare-80 PRIVATE cltj_core sdsl divsufsort divsufsort64)
set_target_properties(prepare-80 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(prepare-20 src/test/prepare-20.cpp)
target_link_libraries(prepare-20 PRIVATE cltj_core sdsl divsufsort divsufsort64)
set_target_properties(prepare-20 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(test-dyn src/test/test-dyn.cpp)
target_link_libraries(test-dyn PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(test-dyn PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(cltj-louds src/test/cltj_louds.cpp)
target_link_libraries(cltj-louds PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(cltj-louds PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(test-alternation-complexity src/test/test-alternation-complexity.cpp)
target_link_libraries(test-alternation-complexity PRIVATE cltj_core sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(test-alternation-complexity PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)
