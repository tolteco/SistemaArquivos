FILES =  NeoFS.h NeoFS.cpp program.cpp
CXXFLAGS = -std=gnu++0x -O2

program.x: ${FILES}
	g++ ${CXXFLAGS} ${FILES} -Wall -lrt -o $@
	
clean:
	rm *.o -f
