OBJECTS=main.o util.o level.o gl_text.o level_completion.o
CXXFLAGS=-O0 -g -Wall -DMACOSX -Wno-deprecated-declarations -std=c++11 -I/usr/local/include
LDFLAGS=-framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -g -std=c++11 -L/usr/local/lib -lglfw3
EXECUTABLES=mbes
BUNDLES=mbes.app

all: mbes.app/Contents/MacOS/mbes

mbes: $(OBJECTS)
	g++ $(LDFLAGS) -o mbes $^

mbes.app/Contents/MacOS/mbes: mbes mbes.icns levels.dat
	./make_bundle.sh mbes com.fuzziqersoftware.mbes mbes
	cp levels.dat mbes.app/Contents/Resources/

clean:
	-rm -rf *.o $(EXECUTABLES) $(BUNDLES)

.PHONY: clean
