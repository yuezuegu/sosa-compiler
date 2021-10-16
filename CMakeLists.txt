cmake_minimum_required (VERSION 3.11)

project(sosa-compiler LANGUAGES C CXX)

if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /WX)
else()
    # lots of warnings and all warnings as errors
    add_compile_options(-Wall -Wextra -pedantic)
endif()

set(CMAKE_CXX_STANDARD 17)

find_package(Boost 1.68 REQUIRED log program_options)
# find_package(nlohmann_json 3.2.0 REQUIRED)

add_executable(
    compiler
    # sources
    compiler/main.cpp
    compiler/layer.cpp
    compiler/ops.cpp
    compiler/compiler.cpp
    compiler/tiles.cpp
    compiler/bank.cpp
    compiler/array.cpp
    compiler/interconnect.cpp
    compiler/post_processor.cpp
)

target_include_directories(compiler PRIVATE compiler/)
target_link_libraries(
    compiler
    PRIVATE
    # nlohmann_json::nlohmann_json
    Boost::boost
    Boost::log
    Boost::program_options
)

add_executable(
    test_pls
    # sources
    compiler/parallel_linear_search.cpp
    compiler/ostream_mt.cpp
)

target_include_directories(test_pls PRIVATE compiler/)
target_link_libraries(
    test_pls
    PRIVATE
    # nlohmann_json::nlohmann_json
    Boost::boost
    Boost::log
    Boost::program_options
)

add_test(NAME test_pls COMMAND test_pls)

# Uncomment the following line to help debug the PLS.
# target_compile_definitions(parallel_linear_search PRIVATE $<$<CONFIG:DEBUG>:PARALLEL_LINEAR_SEARCH_DEBUG>)
