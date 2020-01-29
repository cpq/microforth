all:
	cc -W -Wall -g -O0 test/unit_test.c -o /tmp/x
#	$(DEBUGGER) /tmp/x 1.23 4.56 + words
