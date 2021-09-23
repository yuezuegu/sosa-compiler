define \newline


endef

LDFLAGS = -DBOOST_LOG_DYN_LINK -lpthread -lboost_log -lboost_thread -lboost_system -lboost_filesystem -lboost_atomic -lboost_regex -lboost_chrono -lboost_program_options

PROG = main
SRC = main layer ops compiler tiles bank array interconnect post_processor
HDR = nlohmann/json logger_setup layer ops compiler tiles bank array interconnect post_processor

INC = /home/yuezuegu/boost/include
LIB = /home/yuezuegu/boost/lib

CMD_DEBUG = g++ -std=c++11 -g -Wall  ${LDFLAGS} $(INC:%=-I%) -c compiler/*.cpp -o compiler/*.o ${\newline}
CMD_FINAL = g++ -std=c++11 -O3 -Wall  ${LDFLAGS} $(INC:%=-I%) -c compiler/*.cpp -o compiler/*.o ${\newline}

all: 
	$(foreach i,${SRC},${subst *,$i,${CMD_DEBUG}})
	g++ $(LIB:%=-L%) ${LDFLAGS} -o ${PROG} ${SRC:%=compiler/%.o} 

final:
	$(foreach i,${SRC},${subst *,$i,${CMD_FINAL}})
	g++ $(LIB:%=-L%) ${LDFLAGS} -o ${PROG} ${SRC:%=compiler/%.o} 	

clean:
	rm main ${SRC:%=compiler/%.o}

