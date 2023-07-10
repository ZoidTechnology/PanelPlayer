TARGET = extension.so

.PHONY: clean

$(TARGET): main.c makefile
	gcc -Wall -Werror -fPIC -O3 -shared $< -o $@

clean:
	rm $(TARGET)