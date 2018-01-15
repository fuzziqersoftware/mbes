OBJECTS=main.o util.o level.o gl_text.o level_completion.o audio.o
CXXFLAGS=-O0 -g -Wall -DMACOSX -Wno-deprecated-declarations -std=c++11 -I/usr/local/include
LDFLAGS=-framework OpenAL -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -g -std=c++11 -L/usr/local/lib -lglfw3 -lphosg
EXECUTABLES=mbes

all: mbes.app/Contents/MacOS/mbes

mbes: $(OBJECTS)
	g++ $(LDFLAGS) -o mbes $^

mbes.app/Contents/MacOS/mbes: mbes mbes.icns levels.mbl
	./make_bundle.sh mbes "Move Blocks and Eat Stuff" com.fuzziqersoftware.mbes mbes
	cp levels.mbl mbes.app/Contents/Resources/
	rm -rf "Move Blocks and Eat Stuff.app"
	cp -r mbes.app "Move Blocks and Eat Stuff.app"

clean:
	-rm -rf *.o $(EXECUTABLES) mbes.app "Move Blocks and Eat Stuff.app"

.PHONY: clean
