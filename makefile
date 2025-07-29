# Retrolire - Command line bibliography manager

PREFIX := /usr/local/bin
bin := bin/retrolire
config := retrolire/internal/config/config.go

build: config.go
	@CGO_ENABLED=1 go build -C retrolire -o ../$(bin)

$(config): config.go
	cp -f $< $@

config.go:
	cp -n $(config) $@

install: retrolire
	cp $(bin) $(PREFIX)/retrolire

install-pipx:
	pipx install .

uninstall-pipx:
	pipx uninstall retrolire

uninstall:
	rm -f $(PREFIX)/retrolire

bin:
	mkdir -p $@

clean:
	rm -f $(bin)

.PHONY: build install uninstall clean install-pipx uninstall-pipx
