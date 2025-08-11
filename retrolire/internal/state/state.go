// Package state - Store program state and manage database connection
package state

import (
	"database/sql"
	"fmt"
	"log"
	"os"

	_ "github.com/mattn/go-sqlite3"

	"retrolire/internal/config"
	"retrolire/internal/fs"
)

// Connector - Something that can connect or reconnect to the database
type Connector interface{ Conn() *sql.DB }

// State - Main program state, mostly used to (re)connect to the database
type State struct {
	dbChecked    bool
	logger       *log.Logger
	RunDirectory string
	isTui        bool // TODO
}

func NewState() *State {
	dir, err := os.Getwd()
	if err != nil {
		panic(err)
	}
	return &State{RunDirectory: dir}
}

// Conn - Connect to the database. This function calls log.Fatal if connection fails.
func (s *State) Conn() *sql.DB {
	err := os.Chdir(config.Directory)
	if err != nil {
		fmt.Println("Cannot open directory:", config.Directory)
		os.Exit(1)
	}
	db, err := sql.Open("sqlite3", fs.DBNAME)
	if err != nil {
		fmt.Println("Error opening database in directory", config.Directory)
		os.Exit(1)
	}
	err = db.Ping()
	if err != nil {
		log.Fatal(err)
	}
	// Fix slow note parsing and 'database is locked'
	_, err = db.Exec("PRAGMA synchronous = OFF")
	if err != nil {
		log.Fatal(err)
	}
	// See https://github.com/mattn/go-sqlite3/issues/569
	_, err = db.Exec("PRAGMA journal_mode = WAL")
	s.Log(err, err)
	if err != nil {
		log.Fatal(err)
	}
	if !s.dbChecked {
		s.dbChecked = true
		// Check that the table ENTRY exists.
		// If it doesn't, it's likely that the database doesn't exist at all.
		_, err := db.Exec("select id, csl, lastedit from entry limit 0")
		if err != nil {
			fmt.Println(
				"Looks like there's no retrolire database in this directory: ",
				config.Directory,
				"\nYou should call `retrolire init` to create the database.",
			)
			os.Exit(1)
		}
	}
	return db
}

func (s *State) initLogger() {
	f, err := os.OpenFile(fs.LogFile, os.O_WRONLY|os.O_CREATE|os.O_APPEND, 0600)
	if err != nil {
		panic(err)
	}
	s.logger = log.New(f, "", log.Ldate|log.Ltime)
}

func (s *State) log(errs []error) {
	if s.logger == nil {
		s.initLogger()
	}
	for _, e := range errs {
		if e != nil {
			s.logger.Println(e)
		}
	}
}

// Log - Log things to the log file
func (s *State) Log(err ...error) {
	s.log(err)
}
