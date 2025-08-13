// Package util - Utilities functions
package util

import (
	"database/sql"
)

// GetSomethingAsSlice - Get some data from the data as a string slice
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
		err = rows.Scan(&t)
		if err != nil {
			_ = rows.Close()
			return nil, err
		}
		things = append(things, t)
	}
	return things, nil
}
