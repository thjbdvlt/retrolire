# Retrolire - Command line bibliography manager

PREFIX := /usr/local/bin
bin := bin/retrolire
config := retrolire/internal/config/config.go


build: $(config) | bin
	@# With --tags fts5 the first build will be long. Nexts will be faster.
	@CGO_ENABLED=1 go build -C retrolire --tags fts5 -o ../$(bin) #--tags sqlite_vtable

$(config): config.go
	mkdir -p $(@D)
	cp -f $< $@

config.go:
	cp -n config.def.go $@

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
	rm -f $(bin) $(config)

.PHONY: build install uninstall clean install-pipx uninstall-pipx
