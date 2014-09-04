
tinylisp : *.c *.h
	cc -std=c99 -Wall main.c value.c mpc.c -ledit -lm -o tinylisp

