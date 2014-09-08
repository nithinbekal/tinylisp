
tinylisp : *.c *.h
	cc -std=c99 -Wall main.c mpc.c builtins.c value.c -ledit -lm -o tinylisp

