// Package main - command line bibliography manager
package main

import (
	"os"

	"retrolire/internal/cli"
	"retrolire/internal/fs"
)

func main() {
	fs.CD() // Change to the retrolire directory. This avoid building paths laters.
	cli.Call(os.Args[1:])
}
