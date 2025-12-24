
test/util/strmap-1.o: test/util/strmap-1.c strmap.h
	$(CC) -c $(CFLAGS) -I . -o $(@) $(<)

test/util/strmap-1.x: test/util/strmap-1.o util.o strmap.o
	$(CC) $(<) strmap.o util.o -o $(@)
