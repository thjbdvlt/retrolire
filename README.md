__Polyanthea__ - Bibliography manager named after [Paul Polyanthea](https://en.wikipedia.org/wiki/Paul_Polyanthea).

Before doing anything, copy `config.def.g` to `config.go` and configure Polyanthea. Then, create the database:

```bash
polyanthea init
```

Now you want to add references, from BibTeX, CSl-JSON, ISBN, DOI...

```bash
polyanthea add bibtex mybibliography.bib
polyanthea add json alotofbooks.json
polyanthea add isbn 978-2-840066-234-1
polyanthea add doi 10.3406/item.2003.1247
```

Of course, the data reach by these method will likely be incorrect, so you can add by writing it by yourself, or correct some fields afterwards:

```bash
polyanthea add template book
polyanthea update title id:antin2002
```

Here you'll likely want to read things and takes notes to edit an entry's note file, you can:

```bash
polyanthea # use the TUI and press <Enter> on an entry (recommended)
polyanthea edit # pick an entry with FZF
```

Each time you edit a note, its content is *indexed* and *parsed*: some lines will be classified as *concepts*, other as *quotes*, other as *heading* -- and everything else as *commentary*.

```text
a concept = and its definition, with a locator p.32
> a wonderful quote, with a page number in parenthesis (32)
(i hate this book -- lines in parenthesis are not indexed)
# a heading, this is chapter 3 about cool stuff
this is a simple commentary of the text you're reading, e.g. a summary

> a quote, with *tags*, because line starting with comma are tags
,coolquotes ,sociology ,philosophy-of-science
```

The parsing is 100% *line-based*. Every line becomes an object in the database, with a specific class. Empty lines are ignored.

Now, let's say you've some hundred of books and articles. You probable want to *search* in all these notes, all these ideas, all these cool quotes. For this, you can use plain-text search and filters. Plain-text search searches in entries titles, and in notes content. Filters are more powerfull, you can filters using CSL variables (*author*, *publisher-place*, etc.), or tags, or even *class*, using `AND`, `OR` and `NOT` boolean operators:

```text
author:antin,perec,quintane,pireyre
author:kaplan or publisher:pol
publisher:pol and not ,poetry ,unread
art world =entry
=quote sky
=concept lexical
```

There's also a shortcut for authors, which is `@`:

```text
@becker art world
```

Searching will not only show you *entries*, but all objects. You will see a list of entries, people (authors, translators, editors), of quotes, of concepts and definitions, of heading and commentary, and even *tags*, and *classes*.
If you press `<enter>` when an entry is selected, you'll edit its note. If you're on a *line object* (concept, quote...), you'll edit the note at this specific line. If you're on a person, you'll edit the attached notes, because there's also notes for people. If you're on a tag or a class, this will trigger a new search with this tag or class added as a condition.

There is no *collection* in __Polyanthea__, only tags. But tags are actually powerfull enough, and __Polyanthea__ tags are a bit like classes: a *tag* can be a subtag of another tag. You define *tags tree* (hierarchy) into a file named `.polyanthea.tags`:

```text
fiction
 science-fiction = sf
 romance
 fantasy
humanities
 digital-humanities = dh
 philosophy = philo
  philosophy-of-mind
  philosophy-of-language
 social-sciences
  sociology
  anthropology
computing
 minimal-computing
 linux
 databases = db
  sql
   sqlite
```

With this file, tag *sf* will behave just like *science-fiction*, and if you add tag *sf* to an entry, it will considered to have the tag *fiction* as well.

,, ## word vectors, FTS5
,,
,, __Polyanthea__ not only tries to offer ways to find what you want, but also to make new links 

<!-- TODO: Document word vectors and FTS5 -->

## Installation

```bash
git clone https://github.com/thjbdvlt/polyanthea polyanthea

# Here, you should configure polyanthea through "config.go" file.
make # Build polyanthea
sudo make install # Install polyanthea
pipx install . # Install small command line python programs for "add" command
```

In addition to the executable `polyanthea` (installed in /usr/bin), four other executables (python) are installed using [pipx](https://pipx.pypa.io/stable/installation/):

- `csljson-update`: Builds unique _ids_ for a csl-json.
- `fetchref`: Get a bibtex reference from a DOI or ISBN.

Some commands are not available from the TUI but only from the command line:

The `list` command shows (pretty-print) the information of the chosen bibliography entries. (If no filter option is selected, it displays the entire bibliography. Unlike other options that use filters, there is no _selection_ of an entry with fzf: the `list` command displays all entries that match the filters.)

```bash
polyanthea list author:antin
```

The `json` command works like `list`, but the entries are displayed in [csl-json](https://citeproc-js.readthedocs.io/en/last/csl-json/markup.html) format.

```bash
polyanthea json author:rédaction .inquiry
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

## Config

Configuration is done through config/config.go
Thus, yo need to compile the software in order to changes to apply.

## Completion (Bash)

The completion script (bash) allows for automatic completion of __actions__, __options__, tags, and fields (variables). To use it, source it e.g. in your `.bashrc`.

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
