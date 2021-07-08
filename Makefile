

all: compiler/main.cpp compiler/nlohmann/json.hpp
	g++ -std=c++11 compiler/main.cpp -o compiler/sosa_compiler