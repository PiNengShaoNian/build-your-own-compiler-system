EXE=lit
CC=g++
OBJ=main.o elf_file.o linker.o
CFLAGS=-g -Wno-deprecated
CXXFLAGS=-g -Wno-deprecated -DDEBUG	#传入参数DEBUG
$(EXE):$(OBJ)
	$(CC) $(OBJ) -o $(EXE) $(CFLAGS)
	rm $(OBJ) *~ -f
clean:
	rm $(EXE) $(OBJ) *~ -f
