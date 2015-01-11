OBJECTS=main.o gl_text.o
CXXFLAGS=-O0 -g -Wall -DMACOSX -Wno-deprecated-declarations -std=c++11
LDFLAGS=-framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -g -std=c++11 -lglfw3
EXECUTABLES=mbes

all: mbes

mbes: $(OBJECTS)
	g++ $(LDFLAGS) -o mbes $^

clean:
	-rm -f *.o $(EXECUTABLES)

.PHONY: clean
