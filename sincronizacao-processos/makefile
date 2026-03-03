CC = gcc
CFLAGS = -Wall -Wextra -Iheaders/
SRC = sources/main.c sources/disk.c sources/superblock.c sources/inode.c sources/bitmap.c sources/dir.c sources/interativo.c sources/script.c
OBJ = $(SRC:.c=.o)
TARGET = fs_simulator

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET) fs.bin

.PHONY: all clean