# compiler and compiler options
CC = gcc
CCFLAGS = -I /usr/include/postgresql -L /usr/lib/ -lpq \
		  -Wall -Wextra -Wconversion \
		  -Wno-unused-variable -Wno-unused-parameter

# where the binary and the data will be installed
DEST_DIR=/usr/bin
DATA_DIR=/usr/share/retrolire

# the path to the binary (inside the project directory)
bin=./bin/retrolire

all:
	@make clean
	@make $(bin)

$(bin): config.h bin
	$(CC) src/*.c -o $(bin) $(CCFLAGS)

install: ./bin/retrolire
	# create the directory for the binary if it does not exists
	mkdir -p $(DEST_DIR)
	# copy the binary (retrolire) and the bash script (protolire)
	sudo cp $(bin) $(DEST_DIR)
	sudo cp ./bash/protolire $(DEST_DIR)
	# create the directory for the data (mainly schema.sql)
	mkdir -p $(DATA_DIR)
	# copy data
	cp ./bash/completion.bash ./schema.sql -r templates $(DATA_DIR)/
	# install python command line programs
	pipx install .

uninstall:
	# remove the binary and the bash script
	sudo rm -rf $(DEST_DIR)/retrolire $(DEST_DIR)/protolire $(DATA_DIR)
	# uninstall python command line programs
	pipx uninstall retrolire

bin:
	# the directory containing the binary in the project directory
	mkdir bin

config.h:
	# generate the config file from config.def.h
	cp config.def.h config.h

.PHONY: clean
.PHONY: run

run:
	./bin/retrolire

clean:
	rm -f src/*.o src/*.gch
	rm -f $(bin)
