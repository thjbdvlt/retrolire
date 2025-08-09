// Package main - command line bibliography manager
package main

import (
	"os"

	sqlite_vec "github.com/asg017/sqlite-vec-go-bindings/cgo"

	"retrolire/internal/cli"
	"retrolire/internal/fs"
)

func main() {
	sqlite_vec.Auto()
	fs.CD() // Change to the retrolire directory. This avoid building paths laters.
	cli.Call(os.Args[1:])
}
