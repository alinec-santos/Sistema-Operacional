CC = gcc
CFLAGS = -Wall -g

SRC = sources
INC = headers

OBJ = $(SRC)/main.o $(SRC)/sistema.o $(SRC)/gerenciador.o

all: simulador

simulador: $(OBJ)
	$(CC) $(CFLAGS) -o simulador $(OBJ)

$(SRC)/main.o: $(SRC)/main.c $(INC)/sistema.h
	$(CC) $(CFLAGS) -I$(INC) -c $(SRC)/main.c -o $(SRC)/main.o

$(SRC)/sistema.o: $(SRC)/sistema.c $(INC)/sistema.h
	$(CC) $(CFLAGS) -I$(INC) -c $(SRC)/sistema.c -o $(SRC)/sistema.o

$(SRC)/gerenciador.o: $(SRC)/gerenciador.c $(INC)/sistema.h
	$(CC) $(CFLAGS) -I$(INC) -c $(SRC)/gerenciador.c -o $(SRC)/gerenciador.o

clean:
	rm -f $(SRC)/*.o simulador
