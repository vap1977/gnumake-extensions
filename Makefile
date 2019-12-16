
all: gnumake_include_once_extension.so gnumake_subsystems_extension.so

clean:
	rm -f gnumake_include_once_extension.so
	rm -f gnumake_subsystems_extension.so

gnumake_include_once_extension.so: gnumake_include_once_extension.c
	$(CC) -shared -fPIC -o $@ $<

gnumake_subsystems_extension.so: gnumake_subsystems_extension.c
	$(CC) -shared -fPIC -o $@ $<
