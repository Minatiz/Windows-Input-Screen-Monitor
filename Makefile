CC = x86_64-w64-mingw32-gcc

CFLAGS = -lgdi32

TARGET = main.exe
TAGET_TEST = test.exe
SRCS = main.c
OUTPUT = output



$(TARGET): $(SRCS)
	$(CC) -o $(TARGET) $(SRCS) $(CFLAGS)

all: test $(TARGET)

test:
	$(CC) -o $(TAGET_TEST) $(SRCS) $(CFLAGS) -DRUN_TESTS

clean:
	rm -f $(TARGET) $(TAGET_TEST)
	rm -rf $(OUTPUT)