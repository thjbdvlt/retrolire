// Package util - Utilities functions
package util

import (
	"bytes"
	"database/sql"
	"fmt"
	"log"
	"os"
	"os/exec"
	"strings"

	"retrolire/internal/config"
)

func GetSomethingAsSlice(db *sql.DB, stmt string) ([]string, error) {
	rows, err := db.Query(stmt)
	if err != nil {
		return []string{}, err
	} else if err = rows.Err(); err != nil {
		return []string{}, err
	}
	var things []string
	for rows.Next() {
		var t string
		Check(rows.Scan(&t))
		things = append(things, t)
	}
	return things, nil
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
	// TODO: Return error
	editCmd := exec.Command(config.Editor, fname)
	editCmd.Dir = config.Directory
	editCmd.Stdin = os.Stdin
	editCmd.Stdout = os.Stdout
	editCmd.Stderr = os.Stderr
	err := editCmd.Run()
	Check(err)
}

// EditFileLine - Edit a file with configured Editor and go at a specific line
func EditFileLine(fname string, linenr string) {
	// TODO: Edit a constant-defined file name / temporary file
	// And then put it's edited content in the destination file
	// So it's possible to use Root.OpenFile()
	// And Root.Stat() and everything as Root.
	editCmd := exec.Command(config.Editor, fname, config.EditorFlagLineNr, linenr)
	editCmd.Dir = config.Directory
	editCmd.Stdin = os.Stdin
	editCmd.Stdout = os.Stdout
	editCmd.Stderr = os.Stderr
	err := editCmd.Run()
	Check(err)
}

// Check - Stop program and show error if any
func Check(errs ...error) {
	for _, e := range errs {
		if e != nil {
			log.Fatal(e)
		}
	}
}

// Check2 - Check the second parameter
func Check2(_ any, err error) {
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
