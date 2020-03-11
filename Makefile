$(CC)	:= gcc

all:
	${CC} -O3 -Wextra -Wall csv_parse.c -o csv_parse
