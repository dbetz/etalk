SRC=src
OBJ=obj
BIN=bin

OBJS=\
$(OBJ)/compile.o \
$(OBJ)/decompile.o \
$(OBJ)/error.o \
$(OBJ)/interpret.o \
$(OBJ)/memory.o \
$(OBJ)/methods.o \
$(OBJ)/myapp.o \
$(OBJ)/scan.o

DIRS=\
obj \
bin

CFLAGS=-Wall

all:	$(BIN)/etalk

$(BIN)/etalk:	$(DIRS) $(OBJS)
	cc -o $@ $(OBJS)

$(OBJ)/%.o:	$(SRC)/%.c
	cc -c -o $@ $<

$(DIRS):
	mkdir -p $@

clean:
	rm -rf $(OBJ) $(BIN)
