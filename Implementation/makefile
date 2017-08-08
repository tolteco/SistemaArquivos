FILES =  NeoFS.h NeoFS.cpp teste.cpp
CXXFLAGS = -std=gnu++0x -O2

foo.x: ${FILES}
	g++ ${CXXFLAGS} ${FILES} -Wall -lrt -o $@
	
clean:
	rm *.o -f
