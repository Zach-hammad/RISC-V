SOURCE	:= main.c parser.c registers.c core.c
CC	:= gcc
CCFLAGS := -std=gnu99 -Wall -O3 
TARGET	:= RISCV_core

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) -o $(TARGET) $(SOURCE)

clean:
	rm -f $(TARGET)
