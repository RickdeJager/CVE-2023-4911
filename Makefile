.PHONY: exploit
exploit: main.c suid_lib.so
	gcc main.c -o exploit -g
.PHONY: debug
debug: exploit
	gdb ./exploit -ex 'set follow-fork-mode parent' -ex r


suid_lib.so: suid_lib.c
	gcc -shared -fPIC suid_lib.c -o suid_lib.so -g -ldl

.PHONY: clean
clean:
	rm -f exploit suid_lib.so
