LD := $(CXX)
NASM := nasm

all: raytracer
bin:
	mkdir -p bin
bin/main.o: src/main.cpp
	$(CXX) -c -MMD -std=gnu++20 $(CXXFLAGS) src/main.cpp -o bin/main.o
bin/renderer.o: src/renderer.cpp
	$(CXX) -c -MMD -std=gnu++20 $(CXXFLAGS) src/renderer.cpp -o bin/renderer.o
DEPS := $(wildcard bin/*.d)
ifneq ($(DEPS),)
include $(DEPS)
endif
raytracer: bin bin/main.o bin/renderer.o
	$(LD) -oraytracer $(LD_FLAGS) bin/main.o bin/renderer.o -lm -lSDL2
clean:
	rm -rf bin/
	rm raytracer
