CC = gcc
CCFLAGS = -I /usr/include/postgresql -L /usr/lib/ -lpq \
		  -Wall -Wextra -Wconversion \
		  -Wno-unused-variable -Wno-unused-parameter
PREFIX?=/usr/local
BINDIR=$(PREFIX)/bin
DATADIR=/usr/share/retrolire
bin=./bin/retrolire

all:
	@make clean
	@make $(bin)

$(bin): config.h bin
	$(CC) src/*.c -o $(bin) $(CCFLAGS)

$(BINDIR):
	mkdir $(BINDIR)

$(DATADIR):
	mkdir $(DATADIR)

install: ./bin/retrolire $(BINDIR) $(DATADIR)
	sudo cp $(bin) $(BINDIR)
	sudo cp ./bash/protolire $(BINDIR)
	cp ./bash/completion.bash ./schema.sql -r templates $(DATADIR)/

uninstall:
	sudo rm -rf $(BINDIR)/retrolire $(BINDIR)/protolire $(DATADIR)

install-pipx:
	pipx install .

uninstall-pipx:
	pipx uninstall retrolire

bin:
	mkdir bin

config.h:
	cp config.def.h config.h

.PHONY: clean run

run:
	./bin/retrolire

clean:
	rm -f src/*.o src/*.gch
	rm -f $(bin)
