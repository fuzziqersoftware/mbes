OBJECTS=main.o util.o level.o gl_text.o
CXXFLAGS=-O0 -g -Wall -DMACOSX -Wno-deprecated-declarations -std=c++11 -I/usr/local/include
LDFLAGS=-framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -g -std=c++11 -L/usr/local/lib -lglfw3
EXECUTABLES=mbes

all: mbes

mbes: $(OBJECTS)
	g++ $(LDFLAGS) -o mbes $^

clean:
	-rm -f *.o $(EXECUTABLES)

.PHONY: clean
