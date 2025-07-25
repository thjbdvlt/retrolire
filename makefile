# retrolire -- commande line bibliography manager.
# Copyright (C) 2024,2025  thjbdvlt
#
# retrolire is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# retrolire is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with retrolire.  If not, see <https://www.gnu.org/licenses/>.

PREFIX? = /usr/local
BINDIR = $(PREFIX)/bin
DATADIR = /usr/share/retrolire
bin = ./bin/retrolire

$(bin): | bin
	$(MAKE) -C src

clean:
	$(MAKE) -C src clean

$(BINDIR):
	@mkdir $(BINDIR)

$(DATADIR):
	mkdir $(DATADIR)

install: ./bin/retrolire $(BINDIR) $(DATADIR)
	sudo cp $(bin) $(BINDIR)
	cp ./bash/completion.bash ./schema.sql -r templates $(DATADIR)/

uninstall:
	sudo rm -rf $(BINDIR)/retrolire

install-pipx:
	pipx install .

uninstall-pipx:
	pipx uninstall retrolire

bin:
	mkdir -p bin

.PHONY: run install uninstall install-pipx uninstall-pipx clean

run:
	./bin/retrolire
