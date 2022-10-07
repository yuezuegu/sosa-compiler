cmake_minimum_required (VERSION 3.10)

set(CMAKE_C_COMPILER "/usr/bin/gcc")
set(CMAKE_CXX_COMPILER "/usr/bin/g++")

project(sosa-compiler LANGUAGES C CXX)

if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /WX)
else()
    # lots of warnings and all warnings as errors
    add_compile_options(-Wall -Wextra -pedantic -Wno-unused-parameter)
endif()

set(CMAKE_CXX_STANDARD 17)

find_package(Boost 1.68 REQUIRED log program_options serialization thread)
# find_package(nlohmann_json 3.2.0 REQUIRED)

set(
    COMPILER_SRCS
    compiler/ostream_mt.cpp
    compiler/main.cpp
    compiler/layer.cpp
    compiler/ops.cpp
    compiler/compiler.cpp
    compiler/tiles.cpp
    compiler/bank.cpp
    compiler/array.cpp
    compiler/interconnect.cpp
    compiler/post_processor.cpp
    compiler/dram.cpp
)

set(
    COMPILER_LIBS
    # nlohmann_json::nlohmann_json
    Boost::boost
    Boost::log
    Boost::program_options
    Boost::serialization
)

# multi-threaded compiler target
add_executable(compiler_mt ${COMPILER_SRCS})
target_include_directories(compiler_mt PRIVATE compiler/)
target_link_libraries(compiler_mt PRIVATE ${COMPILER_LIBS})
# target_compile_definitions(compiler_mt PRIVATE COMPILER_MULTITHREADING)

# single-threaded compiler target
add_executable(compiler_st ${COMPILER_SRCS})
target_include_directories(compiler_st PRIVATE compiler/)
target_link_libraries(compiler_st PRIVATE ${COMPILER_LIBS})

# Parallel Linear Search tests

add_executable(
    test_pls
    # sources
    compiler/ostream_mt.cpp # should be initialized first (or else segfault)
    compiler/parallel_linear_search.cpp
)
target_include_directories(test_pls PRIVATE compiler/)
target_link_libraries(
    test_pls
    PRIVATE
    # nlohmann_json::nlohmann_json
    Boost::boost
    Boost::log
    Boost::program_options
    Boost::serialization
)

# Tests for compiler

add_executable(
    compiler_tests
    compiler/tests.cpp
    compiler/ostream_mt.cpp
    compiler/layer.cpp
    compiler/ops.cpp
    compiler/compiler.cpp
    compiler/tiles.cpp
    compiler/bank.cpp
    compiler/array.cpp
    compiler/interconnect.cpp
    compiler/post_processor.cpp
    compiler/dram.cpp
)
target_include_directories(compiler_tests PRIVATE compiler/)
target_link_libraries(
    compiler_tests
    PRIVATE
    Boost::boost
    Boost::log
    Boost::program_options
    Boost::serialization
)

add_executable(
    run_cycle_model
    #sources
    compiler/cycle_model.cpp
    compiler/layer.cpp
    compiler/ops.cpp
    compiler/compiler.cpp
    compiler/tiles.cpp
    compiler/bank.cpp
    compiler/array.cpp
    compiler/interconnect.cpp
    compiler/post_processor.cpp
    compiler/dram.cpp    
)
target_include_directories(run_cycle_model PRIVATE compiler/)
target_link_libraries(
    run_cycle_model
    PRIVATE
    # nlohmann_json::nlohmann_json
    Boost::boost
    Boost::log
    Boost::program_options
    Boost::serialization
)

add_subdirectory(compiler/pybind11)
pybind11_add_module(
    pythonbinder 
    compiler/pythonbinder.cpp 
    compiler/layer.cpp
    compiler/ops.cpp
    compiler/compiler.cpp
    compiler/tiles.cpp
    compiler/bank.cpp
    compiler/array.cpp
    compiler/interconnect.cpp
    compiler/post_processor.cpp
    compiler/dram.cpp   
    )

target_include_directories(pythonbinder PRIVATE compiler/)
target_link_libraries(
    pythonbinder
    PRIVATE
    # nlohmann_json::nlohmann_json
    Boost::thread 
    Boost::boost
    Boost::log
    Boost::program_options
    Boost::serialization
    Boost::filesystem
)



add_test(NAME test_pls COMMAND test_pls)

# Uncomment the following line to help debug the PLS.
# target_compile_definitions(test_pls PRIVATE $<$<CONFIG:DEBUG>:PARALLEL_LINEAR_SEARCH_DEBUG_MSGS>)
