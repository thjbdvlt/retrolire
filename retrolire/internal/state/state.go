// Package state - Store program state and manage database connection
package state

import (
	"database/sql"
	"fmt"
	"log"
	"os"
	"strings"

	sqlite_vec "github.com/asg017/sqlite-vec-go-bindings/cgo"
	_ "github.com/mattn/go-sqlite3"
	"golang.design/x/clipboard"

	"retrolire/internal/config"
	"retrolire/internal/fs"
)

// State - Hold and gives program state informationso
type State interface {
	DB() *sql.DB
	Log(err ...error)
	IsTui() bool
	Exit()
}

// State - Main program state, mostly used to (re)connect to the database
type MainState struct {
	db             *sql.DB
	dbChecked      bool
	logger         *log.Logger
	startDirectory string
	clip           bool
	clipInit       bool
}

// RunDirectory - The directory when the program start
func (s *MainState) RunDirectory() string {
	return s.startDirectory
}

// IsTui - MainState by default is not a TUI.
func (MainState) IsTui() bool { return false }

// DB - Get a database connection, either existing or new.
func (s *MainState) DB() *sql.DB {
	if s.db == nil {
		s.db = s.conn()
		return s.db
	}
	err := s.db.Ping()
	if err == nil {
		return s.db
	}
	s.db = s.conn()
	return s.db
}

// NewState initialize the program state, holding connection and global informations
func NewState(connect bool) *MainState {
	sqlite_vec.Auto()
	// Keep track on initial directory.
	// This is required for command such as "add json" where argument is a file.
	dir, err := os.Getwd()
	if err != nil {
		panic(err)
	}
	// Change to the retrolire directory.
	// This avoid building paths laters, i.e. to the database.
	fs.CD()
	t := &MainState{startDirectory: dir}
	// We usually will connect to the database just once.
	// In few case, we don't want to connect, e.g. because the database needs to be create.
	if connect {
		t.db = t.conn()
	}
	return t
}

// WriteClipBoard - Write something to the clipboard, if clipboard available.
func (s *MainState) WriteClipBoard(text string) {
	if !s.clipInit {
		s.clip = clipboard.Init() == nil
		s.clipInit = true
	}
	if s.clip {
		clipboard.Write(clipboard.FmtText, []byte(text))
	}
}

// Conn - Connect to the database. This function calls log.Fatal if connection fails.
func (s *MainState) conn() *sql.DB {
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

// Exit - Exit main program state, closing database.
func (s *MainState) Exit() {}

func (s *MainState) initLogger() {
	f, err := os.OpenFile(fs.LogFile, os.O_WRONLY|os.O_CREATE|os.O_APPEND, 0600)
	if err != nil {
		panic(err)
	}
	s.logger = log.New(f, "", log.Ldate|log.Ltime)
}

func (s *MainState) log(errs []error) {
	if s.logger == nil {
		s.initLogger()
	}
	for _, e := range errs {
		if e != nil {
			s.logger.Println(e)
		}
	}
}

// Log - Log non-nil things to the log file.
func (s *MainState) Log(err ...error) {
	s.log(err)
}

// CloseDB - Close the database, if any.
// This usually will be called only at the end of the program.
func (s *MainState) CloseDB() {
	if s.db != nil {
		s.Log(s.db.Close())
	}
}

// NoErr - Ensure that an error is nil.
// If the error is not nil, log the error to the log file, exit the state, print a message.
func NoErr(s State, err error, message ...string) {
	if err != nil {
		s.Log(err)
		db := s.DB()
		if db != nil && db.Ping() == nil {
			s.Log(db.Close())
		}
		s.Exit() // Before printing message to stderr, to ensure a clean screen
		if len(message) == 0 {
			message = []string{err.Error()}
		}
		fmt.Fprintln(os.Stderr, strings.Join(message, " "))
		os.Exit(1)
	}
}
