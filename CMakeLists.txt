cmake_minimum_required (VERSION 3.11)

project(sosa-compiler LANGUAGES C CXX)

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
    compiler/interconnect/entry.hpp
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
