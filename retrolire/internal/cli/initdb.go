// Package cli - Command line usage
package cli

import (
	"database/sql"

	"retrolire/internal/fs"
	"retrolire/internal/nlp"
)

func initDB(*CliState) {
	var err error
	var db *sql.DB
	fs.CD()
	db, err = sql.Open("sqlite3", fs.DBNAME)
	check(err)
	tx, err := db.Begin()
	check(err)
	for _, i := range []string{
		// Entry is the main table
		// The CSL variable holds bibliographic informations (author, title, publisher, ...)
		// As "author", in CSL-JSON format, looks like: [{"given": "George", "family": "Perec"}],
		// we make another column to store authors in a human-readable format: "George Perec".
		// It's also easier to search on. Else, "author:given" would returns almost all entries.
		// (At least if we search with "LIKE" operator, which is the simpliest method.)
		// Tags are kept in another column, "tags".
		// (Tests with a separate table showed that a JSON column is faster.)
		// Its format is '{"tag1": 1, "tag2": 1, "tag3": 1}'.
		// Every tag is a key that has 1 has value.
		// Column "title" is a shortcut to "csl ->> 'title'", because it's very frequently used.
		// And column "head" is a shortcut for a frequently used one-string representation.
		// It's used for FZF (and some other stuff).
		`CREATE TABLE IF NOT EXISTS entry (
  id        text  PRIMARY KEY NOT NULL,
  csl      jsonb  NOT NULL,
	tags     jsonb  NOT NULL DEFAULT '{}',
  author    text  NOT NULL DEFAULT '',
  lastedit   int  NOT NULL DEFAULT 0, -- Not (unixepoch('now'))!
  lastpick   int  NOT NULL DEFAULT 0,
  title     text  GENERATED ALWAYS AS (coalesce(csl ->> 'title', '')),
	vec       blob,
  head      text  GENERATED ALWAYS AS (title || CHAR(10) || '    ' || author || '  @' || id)
)`,
		// Textobj is that table that holds information about quotes, concepts, ideas, ...,
		// i.e. everything that is parsed from the notes.
		// Every textobj is a line (or a part of a line) of the notes file.
		// The column "line" refers to the line number in this file.
		// Every textobj has a "class", e.g. "concept", "quote", ...
		// The column "head" exists for the same reason as "entry.head".
		`CREATE TABLE IF NOT EXISTS textobj (
	-- 2025-08-08: Textobj can now be parsed not only in entries notes but also people
  -- entry  text   NOT NULL REFERENCES entry(id),
  entry  text   NOT NULL,
  text   text   NOT NULL DEFAULT '',
	least  text   NOT NULL DEFAULT '',
  linenr  int   NOT NULL DEFAULT 1,
  class   int   NOT NULL DEFAULT 0,
  page   text   NOT NULL DEFAULT '',
	vec    blob,
  head   text   GENERATED ALWAS AS (text || '  @' || entry || ',' || linenr || ',' || page)
)`,
		`CREATE TABLE IF NOT EXISTS entry_person (entry text NOT NULL, person text NOT NULL)`,
		`CREATE TABLE IF NOT EXISTS person (name text PRIMARY KEY NOT NULL)`,
		// Textobjs and Entries are printed through a similar interface.
		// So it makes sense to have a view that gives access to both table as a same structure.
		// UNION ALL is required over simple UNION to avoid DISTINCT, which affect performance.
		`CREATE VIEW IF NOT EXISTS obj AS
	WITH _tag as (SELECT DISTINCT tag FROM tag)
	SELECT
	e.id     AS    id,
	e.title  AS  main,
	e.author AS least,
	e.author AS author,
	e.tags   AS  tags,
	e.csl    AS   csl,
	-1       AS  line,
	1        AS class,
	e.vec    AS   vec
	FROM entry e
	UNION ALL
	SELECT
	o.entry  AS    id,
	o.text   AS  main,
	o.least  AS least,
	e.author AS author,
	e.tags   AS  tags,
	e.csl    AS   csl,
	o.linenr AS  line,
	o.class  AS class,
	o.vec    AS   vec
	FROM textobj o
	LEFT JOIN entry e ON e.id = o.entry
	UNION ALL
	SELECT
	''       AS id,
	tag      AS main,
	''       AS least,
	''       AS author,
	'{}'     AS csl,
	'{}'     AS tags,
	0        AS line,
	2        AS class,
	NULL     AS   vec
	FROM _tag
	UNION ALL
	SELECT
	''       AS id,
	name     AS main,
	''       AS least,
	''       AS author,
	'{}'     AS csl,
	'{}'     AS tags,
	0        AS line,
	3        AS class,
	NULL     AS   vec
	FROM person
	`,
		// Textobjs and Entries are printed through a similar interface.
		// So it makes sense to have a view that gives access to both table as a same structure
		`CREATE VIEW IF NOT EXISTS entry_obj AS
	SELECT
	e.id     AS id,
	e.title  AS main,
	e.author AS least,
	e.tags   AS tags,
	e.csl    AS csl,
	1        AS line,
	1        AS class
	FROM entry e`,
		// Tables for tags will probably be replaced, or dropped, soon.
		`CREATE TABLE IF NOT EXISTS tag (
  entry    text   NOT NULL REFERENCES entry(id),
  tag      text   NOT NULL,
  implicit bool   NOT NULL DEFAULT false
)`,
		`CREATE TABLE IF NOT EXISTS tagDef (
  tag      text   NOT NULL,
  isA      text   NOT NULL
)`,
		// Indexes on entries and textobj, which are usually joins
		`CREATE INDEX entry_id      ON entry(id)`,
		`CREATE INDEX textobj_entry ON textobj(entry)`,
		`CREATE INDEX textobj_class ON textobj(class)`,
		`CREATE INDEX tag_entry     ON tag(entry)`,
		`CREATE INDEX tag_tag       ON tag(tag)`,
		`CREATE INDEX tagDef_tag    ON tagDef(tag)`,
		`CREATE INDEX tagDef_isA    ON tagDef(isA)`,
		// Triggers (INSERT/UPDATE) to have an "author" field without "given"/"family"/"literal"
		`CREATE TRIGGER entryAuthorUpdate
AFTER UPDATE ON entry
BEGIN
    UPDATE entry SET author = (
        SELECT coalesce(group_concat(y.value, ' '), '')
        FROM
					json_each(NEW.csl -> 'author') AS x,
					json_each(x.value) AS y
    ) WHERE id = NEW.id;
END`,
		`CREATE TRIGGER entryAuthorInsert
AFTER INSERT ON entry
BEGIN
    UPDATE entry SET author = (
        SELECT coalesce(group_concat(y.value, ' '), '')
        FROM
					json_each(NEW.csl -> 'author') AS x,
					json_each(x.value) AS y
    ) WHERE id = NEW.id;
END`,
		// Create a FTS5 table
		`CREATE VIRTUAL TABLE fts USING fts5(id, line, main)`,
		// Create a virtual table for word vectors. (New vectors training will replace it.)
		`CREATE TABLE vec_word (word text, vec blob)`,
		// Authors, translator and editors are put in the table person
		`CREATE TRIGGER personInsert
AFTER INSERT ON entry
BEGIN
  INSERT INTO entry_person (entry, person)
	SELECT NEW.id, coalesce(x.value ->> 'literal', coalesce(x.value ->> 'given', '')
	    || ' '
      || coalesce(x.value ->> 'dropping-particle' || ' ', '')
			|| coalesce(x.value ->> 'family', ''))
			FROM entry, json_each(json_extract(csl,
				'$.author',
				'$.editor',
				'$.translator',
				'$.container-author'
			)) AS p, json_each(p.value) AS x
	WHERE entry.id = NEW.id
	ON CONFLICT DO NOTHING;
	INSERT INTO person (name)
	SELECT DISTINCT person
	FROM entry_person
	WHERE entry = NEW.id
	ON CONFLICT DO NOTHING;
END`,
		`CREATE TRIGGER tagInsert
AFTER INSERT on tag
BEGIN
	UPDATE entry SET tags = coalesce((
	SELECT json_group_object(tag, 1) FROM tag WHERE entry = NEW.entry), '{}') where id = NEW.entry;
END`,
	} {
		check2(tx.Exec(i))
	}
	check(tx.Commit(), nlp.UpdateStopWords(db), db.Close())
}
