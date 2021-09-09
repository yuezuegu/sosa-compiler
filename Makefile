define \newline


endef

CFLAGS = -std=c++11 -g -Wall
LDFLAGS = -DBOOST_LOG_DYN_LINK -lpthread -lboost_log -lboost_thread -lboost_system

PROG = main
SRC = main layer ops compiler tiles bank array interconnect post_processor
HDR = nlohmann/json logger_setup layer ops compiler tiles bank array interconnect post_processor

INC = /home/yuezuegu/boost_1_61_0
LIB = /home/yuezuegu/boost_1_61_0/stage/lib



CMD = g++ ${CFLAGS}  ${LDFLAGS} $(INC:%=-I%) -c compiler/*.cpp -o compiler/*.o ${\newline}

all: 
	$(foreach i,${SRC},${subst *,$i,${CMD}})
	g++ $(LIB:%=-L%) ${LDFLAGS} -o ${PROG} ${SRC:%=compiler/%.o} 

clean:
	rm main ${SRC:%=compiler/%.o}

