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

# Logging interface target wired through cltj_core
add_library(cltj_logging INTERFACE)
target_include_directories(cltj_logging INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# CMake options to allow overriding defaults
set(CLTJ_ENABLE_LOGGING "" CACHE STRING "Override ENABLE_LOGGING (0 or 1). Empty uses config defaults")
set(CLTJ_MIN_LOG_LEVEL "" CACHE STRING "Override MIN_LOG_LEVEL numeric (10=DEBUG,20=INFO,30=WARN,40=ERROR)")

target_compile_definitions(cltj_logging INTERFACE
  # ENABLE_LOGGING: explicit override takes precedence
  $<$<BOOL:${CLTJ_ENABLE_LOGGING}>:ENABLE_LOGGING=${CLTJ_ENABLE_LOGGING}>
  # Defaults by config when not overridden
  $<$<AND:$<NOT:$<BOOL:${CLTJ_ENABLE_LOGGING}>>,$<CONFIG:Debug>>:ENABLE_LOGGING=1>
  $<$<AND:$<NOT:$<BOOL:${CLTJ_ENABLE_LOGGING}>>,$<CONFIG:Release>>:ENABLE_LOGGING=0>
  $<$<AND:$<NOT:$<BOOL:${CLTJ_ENABLE_LOGGING}>>,$<CONFIG:RelWithDebInfo>>:ENABLE_LOGGING=0>
  $<$<AND:$<NOT:$<BOOL:${CLTJ_ENABLE_LOGGING}>>,$<CONFIG:MinSizeRel>>:ENABLE_LOGGING=0>
  # MIN_LOG_LEVEL: explicit override or Debug default to DEBUG(10)
  $<$<BOOL:${CLTJ_MIN_LOG_LEVEL}>:MIN_LOG_LEVEL=${CLTJ_MIN_LOG_LEVEL}>
  $<$<AND:$<NOT:$<BOOL:${CLTJ_MIN_LOG_LEVEL}>>,$<CONFIG:Debug>>:MIN_LOG_LEVEL=10>
)

target_link_libraries(cltj_core INTERFACE cltj_logging)
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

# Function to add executables with common linking and output dir
function(add_cltj_executable name source_file output_dir)
  add_executable(${name} ${source_file})
  target_link_libraries(${name} PRIVATE cltj_core sdsl divsufsort divsufsort64 ${ARGN})
  set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${output_dir})
endfunction()

# Suppress warnings from external C library (hybridBV)
if(TARGET hybridbv_gn)
  target_compile_options(hybridbv_gn PRIVATE -w)
endif()

#BENCH EXECUTABLES
add_cltj_executable(build-cltj src/bench/build-cltj.cpp bench)

add_cltj_executable(build-cltj-dyn src/bench/build-cltj-dyn.cpp bench hybridbv_gn)

add_cltj_executable(build-xcltj src/bench/build-xcltj.cpp bench hybridbv_gn)

add_cltj_executable(build-xcltj-dyn src/bench/build-xcltj-dyn.cpp bench hybridbv_gn)

add_cltj_executable(build-uncltj src/bench/build-uncltj.cpp bench)

add_cltj_executable(build-cltj-rdf src/bench/build-cltj-rdf.cpp bench hybridbv_gn)

add_cltj_executable(build-xcltj-rdf src/bench/build-xcltj-rdf.cpp bench hybridbv_gn)

add_cltj_executable(bench-query-cltj src/bench/bench-query-cltj.cpp bench)
target_compile_definitions(bench-query-cltj PRIVATE ADAPTIVE=1)

add_cltj_executable(bench-query-cltj-global src/bench/bench-query-cltj.cpp bench)
target_compile_definitions(bench-query-cltj-global PRIVATE ADAPTIVE=0)

add_cltj_executable(bench-query-uncltj src/bench/bench-query-uncltj.cpp bench)
target_compile_definitions(bench-query-uncltj PRIVATE ADAPTIVE=1)

add_cltj_executable(bench-query-uncltj-global src/bench/bench-query-uncltj.cpp bench)
target_compile_definitions(bench-query-uncltj-global PRIVATE ADAPTIVE=0)

add_cltj_executable(bench-query-xcltj src/bench/bench-query-xcltj.cpp bench)
target_compile_definitions(bench-query-xcltj PRIVATE ADAPTIVE=1)

add_cltj_executable(bench-query-xcltj-global src/bench/bench-query-xcltj.cpp bench)
target_compile_definitions(bench-query-xcltj-global PRIVATE ADAPTIVE=0)

add_cltj_executable(bench-query-cltj-rdf src/bench/bench-query-cltj-rdf.cpp bench hybridbv_gn)
target_compile_definitions(bench-query-cltj-rdf PRIVATE ADAPTIVE=1)

add_cltj_executable(bench-query-xcltj-rdf src/bench/bench-query-xcltj-rdf.cpp bench hybridbv_gn)
target_compile_definitions(bench-query-xcltj-rdf PRIVATE ADAPTIVE=1)

add_cltj_executable(bench-update-cltj src/bench/bench-update-cltj.cpp bench hybridbv_gn)
target_compile_definitions(bench-update-cltj PRIVATE ADAPTIVE=1)

add_cltj_executable(bench-update-xcltj src/bench/bench-update-xcltj.cpp bench hybridbv_gn)
target_compile_definitions(bench-update-xcltj PRIVATE ADAPTIVE=1)

add_cltj_executable(bench-indels-cltj src/bench/bench-indels-cltj.cpp bench hybridbv_gn)

add_cltj_executable(bench-indels-xcltj src/bench/bench-indels-xcltj.cpp bench hybridbv_gn)

add_cltj_executable(stats-query-cltj src/bench/stats-query-cltj.cpp bench hybridbv_gn)

#EXAMPLE
add_cltj_executable(ex_dyn_rdf src/example/ex_dyn_rdf.cpp example hybridbv_gn)

add_cltj_executable(ex_dyn_ids src/example/ex_dyn_ids.cpp example hybridbv_gn)

add_cltj_executable(ex_static_ids src/example/ex_static_ids.cpp example hybridbv_gn)

#CMD
add_cltj_executable(cmd-cltj src/cmd/cmd-cltj.cpp cmd hybridbv_gn)

add_cltj_executable(cmd-cltj-rdf src/cmd/cmd-cltj-rdf.cpp cmd hybridbv_gn)

#TEST
add_cltj_executable(test-index-dyn src/test/test-index-dyn.cpp test hybridbv_gn)

add_cltj_executable(test-index-static src/test/test-index-static.cpp test)

add_cltj_executable(main src/test/main.cpp test hybridbv_gn)

add_cltj_executable(dyn-exp1 src/test/dyn-exp1.cpp test hybridbv_gn)
target_compile_definitions(dyn-exp1 PRIVATE ADAPTIVE=1)

add_cltj_executable(dyn-exp1-metatrie src/test/dyn-exp1-metatrie.cpp test hybridbv_gn)
target_compile_definitions(dyn-exp1-metatrie PRIVATE ADAPTIVE=1)

add_cltj_executable(dyn-exp1-fixed src/test/dyn-exp1.cpp test hybridbv_gn)
target_compile_definitions(dyn-exp1-fixed PRIVATE ADAPTIVE=0)

add_cltj_executable(dyn-exp2 src/test/dyn-exp2.cpp test hybridbv_gn)
target_compile_definitions(dyn-exp2 PRIVATE ADAPTIVE=1)

add_cltj_executable(dyn-exp2-fixed src/test/dyn-exp2.cpp test hybridbv_gn)
target_compile_definitions(dyn-exp2-fixed PRIVATE ADAPTIVE=0)

add_cltj_executable(dyn-exp3 src/test/dyn-exp3.cpp test hybridbv_gn)
target_compile_definitions(dyn-exp3 PRIVATE ADAPTIVE=1)

add_cltj_executable(dyn-exp3-fixed src/test/dyn-exp3.cpp test hybridbv_gn)
target_compile_definitions(dyn-exp3-fixed PRIVATE ADAPTIVE=0)

add_cltj_executable(test-dict src/test/test-dict.cpp test)

add_cltj_executable(test-regex src/test/test-regex.cpp test)

add_cltj_executable(test-bulk-load src/test/test-bulk-load.cpp test)

add_cltj_executable(test-mt-dyn src/test/test-mt-dyn.cpp test hybridbv_gn)

add_cltj_executable(interactive-metatrie src/test/interactive-metatrie.cpp test hybridbv_gn)

add_cltj_executable(update-index-triples src/test/update-index-triples.cpp test hybridbv_gn)

add_cltj_executable(prepare-80 src/test/prepare-80.cpp test)

add_cltj_executable(prepare-20 src/test/prepare-20.cpp test)

add_cltj_executable(test-dyn src/test/test-dyn.cpp test hybridbv_gn)

add_cltj_executable(cltj-louds src/test/cltj_louds.cpp test hybridbv_gn)

add_cltj_executable(test-alternation-complexity src/test/test-alternation-complexity.cpp test hybridbv_gn)

add_cltj_executable(test-mphf-utils src/test/hashing/test_mphf_utils.cpp test)

add_cltj_executable(test-mphf src/test/hashing/test_mphf.cpp test)
set_target_properties(test-mphf PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

add_cltj_executable(test-mphf-size-breakdown src/test/hashing/test_mphf_size_breakdown.cpp test)
set_target_properties(test-mphf-size-breakdown PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

add_cltj_executable(test-trit-packed-array src/test/hashing/test_trit_packed_array.cpp test)
set_target_properties(test-trit-packed-array PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

add_cltj_executable(test-rank-support-glgh src/test/hashing/test_rank_support_glgh.cpp test)
set_target_properties(test-rank-support-glgh PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

add_cltj_executable(analyze-trie-structure src/test/hashing/analyze_trie_structure.cpp analysis)

add_cltj_executable(count-distinct-spo src/analysis/count_distinct_spo.cpp analysis)
