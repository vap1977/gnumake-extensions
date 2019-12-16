
all: gnumake_include_once_extension.so

clean:
	rm -f gnumake_include_once_extension.so

gnumake_include_once_extension.so: gnumake_include_once_extension.c
	$(CC) -shared -fPIC -o $@ $<
