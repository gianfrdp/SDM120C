CC = gcc
CFLAGS  = -Wall -g `pkg-config --cflags libmodbus`
LDFLAGS = -Wall -g `pkg-config --libs libmodbus`

SDM = sdm120c
%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

${SDM}: sdm120c.o 
	$(CC) -o $@ sdm120c.o $(LDFLAGS)
	chmod 4711 ${SDM}

clean:
	rm -f *.o ${SDM}

install: ${SDM}
	install -m 4711 $(SDM) /usr/local/bin

uninstall:
	rm -f /usr/local/$(SDM)
