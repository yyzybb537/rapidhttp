cmake_minimum_required(VERSION 2.8)

###################################################################################
project(RapidHttpTest)

if (CMAKE_BUILD_TYPE)
else()
    #set(CMAKE_BUILD_TYPE DEBUG)
    set(CMAKE_BUILD_TYPE RELEASE)
endif()

set(CMAKE_CXX_FLAGS "-std=c++11 -g -Wall")

option(WITH_PROFILE "link benchmark with profiler" OFF)
option(USE_PICO "based picohttpparser" OFF)
message("------------ Options -------------")
message("  CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message("  CMAKE_CXX_FLAGS_FINAL: ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}}")
message("  WITH_PROFILE: ${WITH_PROFILE}")

if (USE_PICO)
    message("  USE_PICO: ON")
    set(USE_PICO 1)
    execute_process(COMMAND ${PROJECT_SOURCE_DIR}/scripts/extract_pico.sh "${PROJECT_SOURCE_DIR}")
else()
    message("  USE_PICO: OFF")
    set(USE_PICO 0)
    execute_process(COMMAND ${PROJECT_SOURCE_DIR}/scripts/extract_http_parser.sh "${PROJECT_SOURCE_DIR}")
endif()
message("----------------------------------")
configure_file(${PROJECT_SOURCE_DIR}/include/rapidhttp/cmake_config.h.in ${PROJECT_SOURCE_DIR}/include/rapidhttp/cmake_config.h)

include_directories("${PROJECT_SOURCE_DIR}/include")

aux_source_directory(${PROJECT_SOURCE_DIR}/test TEST_SRC_LIST)
add_executable(unittest ${TEST_SRC_LIST})
target_link_libraries(unittest -lgtest -lgtest_main -pthread)

add_executable(tutorial_parse ${PROJECT_SOURCE_DIR}/tutorial/parse.cpp)
add_executable(tutorial_serialize ${PROJECT_SOURCE_DIR}/tutorial/serialize.cpp)

aux_source_directory(${PROJECT_SOURCE_DIR}/benchmark BM_SRC_LIST)
add_executable(benchmark ${BM_SRC_LIST})
if (WITH_PROFILE)
    message("link benchmark with profile")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPROFILE=1")
    target_link_libraries(benchmark -lprofiler -lunwind)
endif()
target_link_libraries(benchmark -lbenchmark -ltcmalloc_minimal -pthread)
