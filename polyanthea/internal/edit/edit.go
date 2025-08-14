// Package edit - Let user edit files or values in their editor
package edit

import (
	"os"
	"os/exec"

	"polyanthea/internal/config"
)

// Temp - Edit value in temporary file
func Temp(b []byte) ([]byte, error) {
	var err error
	f, err := os.CreateTemp("", "polyanthea.edit")
	if err != nil {
		return nil, err
	}
	_, err = f.Write(b)
	if err != nil {
		return nil, err
	}
	_ = f.Close()
	fname := f.Name()
	File(fname)
	b, err = os.ReadFile(fname)
	if err != nil {
		return nil, err
	}
	_ = os.Remove(f.Name())
	return b, nil
}

// File - Edit a file with configured Editor
func File(fname string) error {
	// TODO: Return error
	editCmd := exec.Command(config.Editor, fname)
	editCmd.Dir = config.Directory
	editCmd.Stdin = os.Stdin
	editCmd.Stdout = os.Stdout
	editCmd.Stderr = os.Stderr
	return editCmd.Run()
}

// FileLine - Edit a file with configured Editor and go at a specific line
func FileLine(fname string, linenr string) error {
	// TODO: Edit a constant-defined file name / temporary file?
	// And then put it's edited content in the destination file
	// So it's possible to use Root.OpenFile()
	// And Root.Stat() and everything as Root.
	editCmd := exec.Command(config.Editor, fname, config.EditorFlagLineNr, linenr)
	editCmd.Dir = config.Directory
	editCmd.Stdin = os.Stdin
	editCmd.Stdout = os.Stdout
	editCmd.Stderr = os.Stderr
	return editCmd.Run()
}
