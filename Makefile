FLAG_INCLUDE		:=		-I ./src/include
FLAG_CPP			:=		-Wall -std=c++17
FLAG_DEBUG			:=		-fsanitize=address -static-libasan -g
COMMON_FILES		:=		$(wildcard ./src/common/*.cpp)

FLAGS				:=		$(COMMON_FILES) $(FLAG_CPP) $(FLAG_INCLUDE)

#MAIN_FILE			:=		./src/test_device.cpp
#MAIN_FILE			:=		./src/test_format.cpp
MAIN_FILE			:=		./src/test_filesystem.cpp

target:	$(MAIN_FILE)
	g++ $< $(FLAGS) $(FLAG_DEBUG)

debug: $(MAIN_FILE)
	gdb ./a.out -tui -x gdb.txt

run:
	./a.out r

device: ./src/test_device.cpp
	g++ $< $(FLAGS) $(FLAG_DEBUG) -o $@

format: ./src/test_format.cpp
	g++ $< $(FLAGS) $(FLAG_DEBUG) -o $@

file_system: ./src/test_filesystem.cpp
	g++ $< $(FLAGS) $(FLAG_DEBUG) -o $@

debug_0322: format file_system
#	rm -f disk.img kernel
	./format disk.img
	make
	make run <cmd1.txt
	rm -f test.png
	make debug

.PHONY: clean
clean:
	rm -f a.out disk.img kernel device format file_system
