CC = gcc
CFLAGS = `pkg-config --cflags libmodbus`
LDFLAGS = `pkg-config --libs libmodbus`

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

sdm120c: sdm120c.o 
	$(CC) -o sdm120c sdm120c.o $(LDFLAGS)

clean:
	rm -f *.o sdm120c
