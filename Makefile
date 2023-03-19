FLAG_INCLUDE		:=		-I ./src/include
FLAG_CPP			:=		-Wall -std=c++17
FLAG_DEBUG			:=		-fsanitize=address -static-libasan -g
COMMON_FILES		:=		$(wildcard *.cpp ./src/common/*)

FLAGS				:=		$(COMMON_FILES) $(FLAG_CPP) $(FLAG_INCLUDE)

#MAIN_FILE			:=		./src/test_device.cpp
MAIN_FILE			:=		./src/test_format.cpp
#MAIN_FILE			:=		./src/test_filesystem.cpp

target:	$(MAIN_FILE)
	g++ $< $(FLAGS) $(FLAG_DEBUG)

debug: $(MAIN_FILE)
	gdb ./a.out -tui -x gdb.txt

run:
	./a.out r

tmp: ./src/test_device.cpp
	g++ $< $(FLAGS) $(FLAG_DEBUG) -o tmp

.PHONY: clean
clean:
	rm -f a.out disk.img kernel tmp
