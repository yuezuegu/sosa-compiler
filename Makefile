
CC = g++
CFLAGS = -std=c++11 -g

SRC_FILES = compiler/main.cpp compiler/layer.cpp
HDR_FILES = compiler/nlohmann/json.hpp compiler/layer.hpp
TARGET_EXEC = compiler/main
all:
	${CC} ${CFLAGS} ${SRC_FILES} -o ${TARGET_EXEC} -Wall