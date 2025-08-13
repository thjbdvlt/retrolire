# otlet - Command line bibliography manager

PREFIX := /usr/local/bin
bin := bin/otlet
config := otlet/internal/config/config.go


build: $(config) | bin
	@# With --tags fts5 the first build will be long. Nexts will be faster.
	@CGO_ENABLED=1 go build -C otlet --tags fts5 -o ../$(bin) #--tags sqlite_vtable

$(config): config.go
	mkdir -p $(@D)
	cp -f $< $@

config.go:
	cp -n config.def.go $@

install: otlet
	cp $(bin) $(PREFIX)/otlet

install-pipx:
	pipx install .

uninstall-pipx:
	pipx uninstall otlet

uninstall:
	rm -f $(PREFIX)/otlet

bin:
	mkdir -p $@

clean:
	rm -f $(bin) $(config)

.PHONY: build install uninstall clean install-pipx uninstall-pipx
