# compiler and compiler options
CC = gcc
CCFLAGS = -I /usr/include/postgresql -L /usr/lib/ -lpq \
		  -Wall -Wextra -Wconversion \
		  -Wno-unused-variable -Wno-unused-parameter

# where the binary and the data will be installed
PREFIX?=/usr/local
BINDIR=$(PREFIX)/bin
DATADIR=/usr/share/retrolire

# the path to the binary (inside the project directory)
bin=./bin/retrolire

all:
	@make clean
	@make $(bin)

$(bin): config.h bin
	$(CC) src/*.c -o $(bin) $(CCFLAGS)

install: ./bin/retrolire
	@# create the directory for the binary if it does not exists
	mkdir -p $(BINDIR)
	@# copy the binary (retrolire) and the bash script (protolire)
	sudo cp $(bin) $(BINDIR)
	sudo cp ./bash/protolire $(BINDIR)
	@# create the directory for the data (mainly schema.sql)
	mkdir -p $(DATADIR)
	@# copy data
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
