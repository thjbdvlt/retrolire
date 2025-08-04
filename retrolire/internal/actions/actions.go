// Package actions - Do something on entries
package actions

import (
	"bytes"
	"database/sql"
	"errors"
	"os/exec"
	"path/filepath"
	"slices"
	"strconv"
	"strings"

	"retrolire/internal/config"
	"retrolire/internal/fs"
	"retrolire/internal/note"
	"retrolire/internal/state"
	"retrolire/internal/util"
)

type connector = state.Connector

// EditEntryLine - Edit an entry's note at a specific line (line can be integer/string)
func EditEntryLine(cn connector, id string, line any) error {
	if id == "" {
		return errors.New("no id")
	}
	if line == "" {
		line = "1"
	}
	fname := id + config.Ext
	var lineString string
	switch v := line.(type) {
	case int:
		lineString = strconv.Itoa(v)
	case string:
		lineString = v
	default:
		return errors.New("unsupported type for parameter line")
	}
	util.EditFileLine(fname, lineString)
	note.Parse([]string{id}, []int64{0}, cn)
	return nil
}

// EditEntry - Edit an entry's note
func EditEntry(cn connector, id string) error {
	return EditEntryLine(cn, id, "1")
}

// EditPerson - Edit a person's note
func EditPerson(cn connector, name string) error {
	var err error
	root := fs.Root()
	err = fs.CreateDirectory(root, fs.PeopleDirectoryName)
	if err != nil {
		return err
	}
	err = root.Close()
	if err != nil {
		return err
	}
	filepath := filepath.Join(fs.PeopleDirectoryName, fs.ToFilename(name))
	util.EditFile(filepath + config.Ext)
	note.Parse([]string{filepath}, []int64{0}, cn)
	return nil
}

// DeleteEntry - Delete an entry. This function don't ask for confirmation.
func DeleteEntry(cn connector, id string) error {
	stmts := []string{
		`DELETE FROM entry WHERE id = ?`,
		`DELETE FROM tag WHERE entry = ?`,
		`DELETE FROM textobj WHERE entry = ?`,
	}
	db := cn.Conn()
	for _, i := range stmts {
		_, err := db.Exec(i, id)
		if err != nil {
			return err
		}
	}
	return db.Close()
}

// UpdateEntryField - Update an entry's field value
func UpdateEntryField(cn connector, id string, field string) error {
	var err error
	db := cn.Conn()
	path := "$." + field
	row := db.QueryRow(`SELECT json_type(csl -> ?) = 'text' FROM entry WHERE id = ?`, path, id)
	var isText bool
	if row.Err() != nil {
		return row.Err()
	}
	if err = row.Scan(&isText); err != nil {
		return err
	}
	var stmtFrom, stmtTo string
	if isText {
		stmtFrom = `SELECT csl ->> ? FROM entry WHERE id = ?`
		stmtTo = `UPDATE entry SET csl = json_set(csl, ?, json_quote(?)) WHERE id = ?`
	} else {
		stmtFrom = `SELECT csl -> ? from entry where id = ?`
		stmtTo = `UPDATE entry SET csl = json_set(csl, ?, json(?)) WHERE id = ?`
	}
	row = db.QueryRow(stmtFrom, field, id)
	if err = row.Err(); err != nil {
		return err
	}
	var value []byte
	if err = row.Scan(&value); err != nil {
		return err
	}
	value = bytes.TrimSpace(value)
	if err = db.Close(); err != nil {
		return err
	}
	value = util.EditTemp(value)
	db = cn.Conn()
	_, err = db.Exec(stmtTo, path, strings.TrimSpace(string(value)), id)
	if err != nil {
		return err
	}
	return db.Close()
}

// GetEntryValue - Get the value of a field for an entry
func GetEntryValue(cn connector, id string, field string) (string, error) {
	var err error
	db := cn.Conn()
	row := db.QueryRow(`SELECT coalesce(csl ->> ?, '') FROM entry WHERE id = ?`, field, id)
	if err = db.Close(); err != nil {
		return "", err
	}
	var data string
	if err = row.Err(); err != nil {
		return "", err
	}
	if err = row.Scan(&data); err != nil {
		return "", err
	}
	return data, nil
}

// Head - Get the head representation of an entry (title / author)
func Head(cn connector, id string) string {
	db := cn.Conn()
	row := db.QueryRow(`SELECT head FROM entry WHERE id = ?`, id)
	var head string
	util.Check(row.Scan(&head), db.Close())
	return head
}

// OpenEntryURL - Open the URL of an Entry
func OpenEntryURL(cn connector, id string) error {
	url, err := GetEntryValue(cn, id, "URL")
	if err != nil {
		return err
	}
	if url != "" {
		sh := exec.Command(config.Opener, url)
		if err = sh.Run(); err != nil {
			return err
		}
	}
	return nil
}

// UpdateEntryTags - Update an entry's set of tags
func UpdateEntryTags(cn connector, id string, tags []string, deleteTags bool) error {
	db := cn.Conn()
	tx, err := db.Begin()
	if err != nil {
		return err
	}
	if deleteTags {
		_, err = tx.Exec(`DELETE FROM tag WHERE entry = ?`, id)
		if err != nil {
			return err
		}
	}
	stmt, err := tx.Prepare(`INSERT INTO tag (entry, tag, implicit)
VALUES (?, ?, false)
ON CONFLICT DO NOTHING`)
	if err != nil {
		return err
	}
	for _, i := range tags {
		_, err = stmt.Exec(id, i)
		if err != nil {
			return err
		}
	}
	_, err = tx.Exec(`INSERT INTO tag (entry, tag, implicit)
SELECT distinct ?, td.isA, true
FROM tag t
JOIN tagDef td ON t.tag = td.tag
WHERE t.entry = ?`, id, id)
	if err != nil {
		return err
	}
	// FIXME: This seems to update tags for evrey entries
	_, err = tx.Exec(`UPDATE entry SET tags = coalesce((
SELECT json_group_object(tag, 1) FROM tag WHERE entry = ?), '{}') where id = ?`, id, id)
	if err != nil {
		return err
	}
	err = tx.Commit()
	if err != nil {
		return err
	}
	return db.Close()
}

// EditEntryTags - Edit entry's tag in config.Editor
func EditEntryTags(cn connector, id string) error {
	db := cn.Conn()
	var tags []byte
	var err error
	row := db.QueryRow(`SELECT
coalesce(group_concat(tag, char(10)), '')
FROM tag
WHERE entry = $1`, id)
	if err = row.Err(); err != nil {
		return err
	}
	if err = row.Scan(&tags); err != nil {
		return err
	}
	if err = db.Close(); err != nil {
		return err
	}
	tags = util.EditTemp(tags)
	newTags := strings.Split(string(tags), "\n")
	for i, tag := range newTags {
		newTags[i] = strings.TrimSpace(tag)
	}
	newTags = slices.DeleteFunc(newTags, func(s string) bool {
		return s == ""
	})
	return UpdateEntryTags(cn, id, newTags, true)
}

func getSomethingAsSlice(db *sql.DB, stmt string) ([]string, error) {
	rows, err := db.Query(stmt)
	if err != nil {
		return []string{}, err
	} else if err = rows.Err(); err != nil {
		return []string{}, err
	}
	var things []string
	for rows.Next() {
		var t string
		util.Check(rows.Scan(&t))
		things = append(things, t)
	}
	return things, nil
}

// GetTags - Get a list of tags
func GetTags(db *sql.DB) []string {
	tags, _ := getSomethingAsSlice(db, `SELECT DISTINCT tag FROM tag`)
	return tags
}

// GetFields - Get a list of fields (CSL variables)
func GetFields(db *sql.DB) []string {
	fields, _ := getSomethingAsSlice(db, `SELECT DISTINCT x.key FROM entry, json_each(csl) as x`)
	return fields
}
