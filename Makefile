LD := $(CXX)
NASM := nasm
all: raytracer
bin:
	mkdir -p bin
bin/main.o: src/main.cpp
	$(CXX) -c $(CXXFLAGS) src/main.cpp -o bin/main.o
raytracer: bin bin/main.o
	$(LD) -oraytracer $(LD_FLAGS) bin/main.o -lm
clean:
	rm -rf bin/
	rm raytracer
