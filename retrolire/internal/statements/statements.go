// package statements - SQL statements used in many places
package statements

// Sep - Separator between title/author and ID
const Sep = `  @`

// LocatorSep - Separator between entry ID, line number and page number
const LocatorSep = ","

// Entry - Select entries
const Entry = `select e.asline from entry e`

// OrderBy - Order by clause
const OrderBy = "order by lastpick desc"

// List - List entries, pretty printing
const List = `select group_concat(
  ? || c.key || ? || ': ' || c.value, char(10)
) || char(10) || '---' as record
from entry e, json_each(json(e.csl)) as c`

// GroupID - Group entry by ID
const GroupID = `group by e.id`

// CreateEntry - Create the Entry Table
const CreateEntry = `CREATE TABLE IF NOT EXISTS entry (
  id text PRIMARY KEY NOT NULL,
  csl jsonb NOT NULL,
  lastedit int DEFAULT (unixepoch('now')),
  author text GENERATED ALWAYS AS (
    COALESCE(csl ->> '$.author[0].family', '')
  ),
  asline text GENERATED ALWAYS AS (
    COALESCE(csl ->> 'title', '')
    || CHAR(10)
    || '    '
    || COALESCE(csl ->> '$.author[0].given', '')
    || ' '
    || COALESCE(csl ->> '$.author[0].family', '')
    || '  @'
    || COALESCE(csl ->> 'id', '')
  ),
  lastpick int DEFAULT 0
)`

// CreateTextObj - Create the TextObj Table
const CreateTextObj = `CREATE TABLE IF NOT EXISTS textobj (
  entry text REFERENCES entry(id),
  text text,
  linenr int DEFAULT 1,
  class text DEFAULT 'idea',
  asline text GENERATED ALWAS AS (
    COALESCE(text, '')
	|| '  @'
	|| entry
	|| ','
	|| linenr
	|| ','
	|| page
  ),
  page text
)`

// CreateTag - Create tag table
const CreateTag = `create table if not exists tag (
  entry text references entry(id),
  tag text not null
)`

// CreatTagDef - Create the tagdef table, used to define relations between tags
const CreatTagDef = `create table if not exists tagDef (
  tag text,
  isA text
)`

// SelectTagOrderByUse - Get tags, most used first
const SelectTagOrderByUse = `with x as (
  select tag, count(distinct entry) as count
  from tag
  group by tag
)
select x.tag
from x
order by x.count desc
`

// JSON - Select entries as a single JSON array
const JSON = "select json_group_array(json(csl)) from entry e"
