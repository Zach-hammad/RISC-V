SOURCE	:= Main.c Trace.c Cache.c
CC	:= gcc
TARGET	:= Main
LINK	:= -lm

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) -o $(TARGET) $(SOURCE) $(LINK)

clean:
	rm -f $(TARGET)
