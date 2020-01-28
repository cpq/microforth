all:
	cc test/unit_test.c -o /tmp/x
	echo 1.23 4.56 + | /tmp/x
