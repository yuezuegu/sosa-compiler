
CC = g++
CFLAGS = -std=c++11 -g

SRC_FILES = compiler/main.cpp compiler/layer.cpp compiler/ops.cpp compiler/compiler.cpp compiler/tiles.cpp compiler/bank.cpp compiler/array.cpp compiler/interconnect.cpp compiler/post_processor.cpp
HDR_FILES = compiler/nlohmann/json.hpp compiler/layer.hpp compiler/ops.hpp compiler/compiler.hpp compiler/tiles.hpp compiler/bank.hpp compiler/array.hpp compiler/interconnect.hpp compiler/post_processor.hpp 
TARGET_EXEC = compiler/main



all:
	${CC} ${CFLAGS} ${SRC_FILES} -o ${TARGET_EXEC} -Wall