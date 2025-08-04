__retrolire__ - command line bibliography manager

## Synopsis

```text
retrolire <command> [options] [filters]
```

## Commands

__edit__: Edit an entry's note
__cite__: Output an entry's ID
__add__: Add entries
__tag__: Edit an entry's tags
__list__: List entries matching criteria
__json__: Output entries matching criteria in JSON
__delete__: Delete an entry
__update__: Update a field of an entry
__init__: Initiate the database
__open__: Open an entry URL
__parse__: Parse notes and update database

Most commands requires no arguments, but some do:

```bash
retrolire add {doi|isbn|bibtex|json|template} <identifier|file>
retrolire update <field>
```

## Filters

All commands except *init* and *add* accept filters arguments.
There are three types of filters, parsed in following order:
· Key:Val1,Val2: *author:antin*, *title:fabulous,fantastic*.
· Tag: *.poetry*, *.philosophy*, *.unread*.
· Plain-Text search in note: anything else.
Filters are combined with logical operator *AND*.
Two keywords alter this:
· *or* replace the logical operator *AND* by *OR*.
· *not* negates the next filter.
In your configuration file (config.go), you can define key aliases,
so that (e.g.) *a:* is mapped to *author:* and *t:* to *title:*.

## EXAMPLES

```bash
retrolire init
retrolire edit --quote author:antin .read
retrolire open .unread .fiction
retrolire list author:wittgenstein,kripke .important
retrolire cite --concept .philosophy
retrolire update title
retrolire add json - < mycsl.json
```

## OPTIONS

|short|long|description|
|---|--------|---------|
|-q|--quote|Cite/Edit quotes instead of entries|
|-c|--concept|Cite/Edit concepts instead of entries|
|-i|--idea|Cite/Edit ideas instead of entries|
|-k|--keep-id|Don't generate new uniques entries IDs (command add)|
|-e|--exact|No fuzzy matching in fzf|
|-f|--force|Force note parsing no matter files modified time (command parse)|

## CONFIG

Configuration is done through config/config.go
Thus, yo need to compile the software in order to changes to apply.

## Notes parsing

When a reading note is modified (or added), its content is parsed to find and extract __quotes__ and __concepts__.

### Quotes

If some __quotes__ are found, they are indexed and put in another table: with the command `quote`, the user can get these quotes (formatted to be put in pandocs-markdown document).

The parsing/extraction requires that the reading note is written in [markdown](https://pandoc.org/chunkedhtml-demo/8-pandocs-markdown.html). Only blockquotes are indexed as quotes. Page number can be precised at the end of the quote, in parentheses or in brackets,:

```markdown
a normal paragraph

> a quote [32]

> another quote (p. 32)

> here too, another quote (32-33)

another normal paragraph
```

### Concepts

Reading notes are also parsed in order to find concept definitions (that user can get using `concept` command).
The syntax used is the syntax for [definition list](https://pandoc.org/MANUAL.html#definition-lists) in pandoc markdown, with one difference: a concept may only have one definition.

```markdown

concept
: definition

<!-- There's also a not-markdown syntax: -->
concept = definition

```

## completion (bash)

The completion script (bash) allows for automatic completion of __actions__, __options__, tags, and fields (variables). To use it, source it e.g. in your `.bashrc`.

## installation

```bash
git clone https://github.com/thjbdvlt/retrolire retrolire

# Here, you should configure retrolire through "config.go" file.
make # Build retrolire
sudo make install # Install retrolire
pipx install . # Install small command line python programs for "add" command

retrolire init # After you have configured retrolire!
retrolire add bibtex - < yourbibliography.bib
retrolire add json - < yourbibliography.json
```

In addition to the executable `retrolire` (installed in /usr/bin), four other executables (python) are installed using [pipx](https://pipx.pypa.io/stable/installation/):

- `csljson-update`: Builds unique _ids_ for a csl-json.
- `fetchref`: Get a bibtex reference from a DOI or ISBN.

### cite

Command `cite` is used to get the __ID__ (citation key) of an entry, for example, to be inserted in a [pandoc](https://pandoc.org/)-markdown document for which footnotes and bibliography will be automatically produced with [citeproc](https://github.com/jgm/citeproc).

```bash
retrolire cite | xclip -selection clipboard
```

### edit

Edit the reading note of an entry with the program defined as the `editor` (in `config.go`).

### open

The `open` action opens an URL associated with an entry. By default, [xdg-open](https://linux.die.net/man/1/xdg-open) to define the software to use (depending on the extension), but it can be changed in the config file.

```bash
retrolire open author:quintane,cadiot publisher=p.o.l
retrolire open '.unread'
```

![](./img/open.gif)

<!--
### file

The `file` action adds a __file__ to an entry. It takes the file path as an argument.

```bash
# Associate the file './la_maison_de_wittgenstein.pdf' with the entry 'commetti2017'
retrolire add file './la_maison_de_wittgenstein.pdf' -i 'cometti2017'
```
-->

### tag

The `tag` action edits (in the `$EDITOR`) the tags associated to an entry. It takes an optional argument `pick` that allows selecting (with fzf) tags from the tags already used.

### add

The `add` action adds bibliographic entries from a [bibtex](https://www.bibtex.org/) or [csl-json](https://citeproc-js.readthedocs.io/en/last/csl-json/markup.html) file, or a single one from a [doi](https://dx.doi.org/), an [isbn](https://en.wikipedia.org/wiki/International_Standard_Book_Number), or a template to fill.

It requires two arguments:

- The method: `bibtex` or `json` to import multiple entries from a file, `doi` or `isbn` to retrieve information of an entry from an identifier, or `template` to manually fill values from a file already containing the fields.
- The file (if method is `bibtex` or `json`), or the identifier (if `doi` or `isbn`). It can be left empty if the method is `template`.

```bash
# Add an entry from DOI
retrolire add doi 10.58282/lht.3619
```

```bash
# Import a bibliography in CSL-JSON format
retrolire add json - < ../found_bibliography.json
```

```bash
# Create a bibtex file from a template
retrolire add template book
```

(The filter options are ignored.)


### list

The `list` action shows the information of the chosen bibliography entries. (If no filter option is selected, it displays the entire bibliography. Unlike other options that use filters, there is no _selection_ of an entry with fzf: the `list` command displays all entries that match the filters.)

```bash
retrolire list author:antin
```

### json

The `json` action works like `list`, but the entries are displayed in [csl-json](https://citeproc-js.readthedocs.io/en/last/csl-json/markup.html) format.

```bash
retrolire json author:rédaction .inquiry
```
```json
[
    {
        "id": "larédaction2012",
        "type": "book",
        "title": "Les Berthier: portraits statistiques",
        "author": [{"literal": "La Rédaction"}],
        "issued": {"date-parts": [[2012]]},
        "publisher": "Questions théoriques",
        "title-short": "Les Berthier"
    }
]
```

### update

The `update` action modifies the value of a field (_title_, _publisher_, etc.). It requires an argument (_field_): the field whose value needs to be updated. The second argument (_value_) is optional: if absent, the current value will be opened in the `$EDITOR` to be modified directly; if provided, it is used as the new value.

```bash
retrolire update container-title id:becker2013
```

## DOI / ISBN

Retrieving bibliographic references from a [doi](https://dx.doi.org/) or an [isbn](https://en.wikipedia.org/wiki/International_Standard_Book_Number) is done using the [isbnlib](https://pypi.org/project/isbntools/) library.

## dependencies

- Go
- SQLite3
- [fzf](https://github.com/junegunn/fzf).

For the importation to the database in the python command line tools:
 
- [orjson](https://github.com/ijl/orjson)

To get references from doi/isbn:

- [isbntools](https://pypi.org/project/isbntools/)
- [isbnlib](https://pypi.org/project/isbnlib/)

Correction, structuring, formatting, and conversion from bibtex to csl-json:

- [pandoc](https://pandoc.org/)
