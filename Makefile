yuic: yuic.c

test: yuic
	./test.sh

clean:
	rm -f yuic *.o *~ tmp*