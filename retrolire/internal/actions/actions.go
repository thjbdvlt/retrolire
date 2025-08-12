// Package actions - Do something on entries
package actions

import (
	"bytes"
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
	"retrolire/internal/edit"
)

// EditEntryLine - Edit an entry's note at a specific line (line can be integer/string)
func EditEntryLine(t state.State, id string, line any) error {
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
	edit.FileLine(fname, lineString)
	return note.Parse(t, []string{id}, []int64{0})
}

// EditEntry - Edit an entry's note
func EditEntry(t state.State, id string) error {
	return EditEntryLine(t, id, "1")
}

// EditPerson - Edit a person's note
func EditPerson(t state.State, name string) error {
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
	edit.File(filepath + config.Ext)
	return note.Parse(t, []string{filepath}, []int64{0})
}

// DeleteEntry - Delete an entry. This function don't ask for confirmation.
func DeleteEntry(t state.State, id string) error {
	stmts := []string{
		`DELETE FROM entry WHERE id = ?`,
		`DELETE FROM tag WHERE entry = ?`,
		`DELETE FROM textobj WHERE entry = ?`,
	}
	db := t.DB()
	for _, i := range stmts {
		_, err := db.Exec(i, id)
		if err != nil {
			return err
		}
	}
	return nil
}

// UpdateEntryField - Update an entry's field value
func UpdateEntryField(t state.State, id string, field string) error {
	var err error
	db := t.DB()
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
	value, err = edit.Temp(value)
	state.NoErr(t, err)
	db = t.DB()
	_, err = db.Exec(stmtTo, path, strings.TrimSpace(string(value)), id)
	if err != nil {
		return err
	}
	return nil
}

// GetEntryValue - Get the value of a field for an entry
func GetEntryValue(t state.State, id string, field string) (string, error) {
	var err error
	db := t.DB()
	row := db.QueryRow(`SELECT coalesce(csl ->> ?, '') FROM entry WHERE id = ?`, field, id)
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
func Head(t state.State, id string) (string, error) {
	var err error
	db := t.DB()
	row := db.QueryRow(`SELECT head FROM entry WHERE id = ?`, id)
	var head string
	err = row.Scan(&head)
	if err != nil {
		return "", err
	}
	return head, nil
}

// OpenEntryURL - Open the URL of an Entry
func OpenEntryURL(t state.State, id string) error {
	url, err := GetEntryValue(t, id, "URL")
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
func UpdateEntryTags(t state.State, id string, tags []string, deleteTags bool) error {
	db := t.DB()
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
	return nil
}

// EditEntryTags - Edit entry's tag in config.Editor
func EditEntryTags(t state.State, id string) error {
	db := t.DB()
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
	_ = db.Close() // Close database while editing
	tags, err = edit.Temp(tags)
	state.NoErr(t, err)
	newTags := strings.Split(string(tags), "\n")
	for i, tag := range newTags {
		newTags[i] = strings.TrimSpace(tag)
	}
	newTags = slices.DeleteFunc(newTags, func(s string) bool {
		return s == ""
	})
	return UpdateEntryTags(t, id, newTags, true)
}

// GetTags - Get a list of tags
func GetTags(t state.State) []string {
	tags, _ := util.GetSomethingAsSlice(t.DB(), `SELECT DISTINCT tag FROM tag`)
	return tags
}

// GetFields - Get a list of fields (CSL variables)
func GetFields(t state.State) []string {
	fields, _ := util.GetSomethingAsSlice(t.DB(), `SELECT DISTINCT x.key FROM entry, json_each(csl) as x`)
	return fields
}
