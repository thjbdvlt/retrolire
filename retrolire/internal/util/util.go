package util

import (
	"fmt"
	_ "github.com/mattn/go-sqlite3"
	"log"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"bytes"

	"retrolire/internal/config"
)

// FileExists - Check if a file exists
func FileExists(fp string) bool {
	_, err := os.Stat(fp)
	if err == nil {
		return true
	}
	if os.IsNotExist(err) {
		return false
	}
	return true
}

// Dir - Get the directory from config, expanding home tilde
func Dir() string {
	dir := config.Directory
	if strings.HasPrefix(dir, "~/") {
		home, _ := os.UserHomeDir()
		dir = filepath.Join(home, dir[2:])
	}
	return dir
}

// DbPath - Return database full path
func DbPath() string {
	return filepath.Join(Dir(), ".retrolire.db?mode=rwc&cache=shared")
}

// EditTemp - Edit value in temporary file
func EditTemp(b []byte) []byte {
	var err error
	f, err := os.CreateTemp("", "retrolire.edit")
	Check(err)
	_, err = f.Write(b)
	Check(err)
	_ = f.Close()
	fname := f.Name()
	EditFile(fname)
	b, err = os.ReadFile(fname)
	Check(err)
	_ = os.Remove(f.Name())
	return b
}

// EditFile - Edit a file with configured Editor
func EditFile(fname string) {
	editCmd := exec.Command(config.Editor, fname)
	editCmd.Dir = Dir()
	editCmd.Stdin = os.Stdin
	editCmd.Stdout = os.Stdout
	editCmd.Stderr = os.Stderr
	err := editCmd.Run()
	Check(err)
}

// EditFileLine - Edit a file with configured Editor and go at a specific line
func EditFileLine(fname string, linenr string) {
	editCmd := exec.Command(config.Editor, fname, config.EditorFlagLineNr, linenr)
	editCmd.Dir = Dir()
	editCmd.Stdin = os.Stdin
	editCmd.Stdout = os.Stdout
	editCmd.Stderr = os.Stderr
	err := editCmd.Run()
	Check(err)
}

// Check - Stop program and show error if any
func Check(err error) {
	if err != nil {
		log.Fatal(err)
	}
}

// Abort - Exit program and print error
func Abort(args ...any) {
	fmt.Fprintln(os.Stderr, args...)
	os.Exit(1)
}

// ConfirmUser - Ask user for confirmation (i.e. y/n)
func ConfirmUser(msg string) bool {
	fmt.Println(msg, "(y/N)")
	var input string
	_, err := fmt.Scanln(&input)
	if err != nil {
		return false
	}
	return strings.TrimSpace(input) == "y"
}

// Popen2 - Read/Write to a shell command
func Popen2(in []byte, command []string) []byte {
	var bufIn *bytes.Buffer
	var bufOut bytes.Buffer
	bufIn = bytes.NewBuffer(in)
	sh := exec.Command(command[0], command[1:]...)
	sh.Stdin = bufIn
	sh.Stdout = &bufOut
	sh.Stderr = os.Stderr
	err := sh.Run()
	Check(err)
	return bufOut.Bytes()
}
