
tinylisp : main.c
	cc -std=c99 -Wall main.c mpc.c -ledit -lm -o tinylisp

