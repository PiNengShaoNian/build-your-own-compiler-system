EXE=ass
CC=g++
OBJ=main.o lexial.o parser.o semantic.o elf_file.o
CFLAGS=-g -Wno-deprecated
CXXFLAGS=-g -Wno-deprecated -DDEBUG	#传入参数DEBUG
$(EXE):$(OBJ)
	$(CC) $(OBJ) -o $(EXE) $(CFLAGS)
	rm $(OBJ) *~ -f
clean:
	rm $(EXE) $(OBJ) *~ -f
