import itertools
import os
import string
import re


_re_nonword = re.compile(r"\W+")


def _re_remove_nonword(s):
    return _re_nonword.sub("", s)


def _generate_suffixes_generator(limit=10):
    """a generator that return (a, b, c ... ab, ac ... dza, dzb, eaa ...)

    used for unique id:
        - antin2008
        - antin2008a
        - antin2008b
        - ...
        - antin2008eh
        - ...

    Returns (Generator[str]):
    """

    letters = string.ascii_lowercase

    def infinite_product_incremented(limit=10):
        n = 2
        while n < limit:
            x = itertools.product(letters, repeat=n)
            for i in x:
                yield "".join(i)
            n += 1

    return itertools.chain.from_iterable(
        (letters, infinite_product_incremented(limit))
    )


def get_name(entry) -> str:
    """Récupère le nom d'une personne ou d'une entité quelconque.

    Args:
        entry (dict):  l'entry.

    Returns (str):  le nom d'une personne (ou d'un erstatz de nom).
    """

    person = None
    name = None
    for k in ("author", "editor", "translator"):
        if k in entry:
            person = entry[k]
            if len(person) == 0:
                person = None
            else:
                break
    if person:
        person = person[0]
        for k in ("family", "literal", "given"):
            if k in person:
                name = person[k]
                return name
    for k in ("publisher", "collection-title", "title"):
        if k in entry and entry[k]:
            name = entry[k]
            return name


def format_name(name):
    """format name to be used as citekey.
    args:
        name (str)

    returns (str)
    """

    name = _re_remove_nonword(name)
    name = name.lower()
    name = name[:15]
    return name


def get_year(entry) -> str:
    """récupère une année (de publication ou d'accès).

    args:
        entry (dict)

    returns (str):  the year
    """

    for k in ("issued", "accessed"):
        if k in entry:
            datepart = entry[k].get("date-parts")
            if datepart:
                year = datepart[0][0]
                return str(year)
    return ""


def make_unique_id(entry, ids, id_name_max_len=15):
    """make a unique id.

    args:
        entry (dict)
        ids (set)
        n_char_id_name (int):  the max character number of name.

    returns (str):  the id.
    """

    name = get_name(entry)
    if not name:
        return
    name = format_name(name)
    suffixes = _generate_suffixes_generator(limit=10)
    year = get_year(entry)
    id_ = name + year
    while id_ in ids:
        suffix = next(suffixes)
        id_ = name + year + suffix
    ids.add(id_)
    return id_


def replace_annote(entry) -> None:
    """if 'annote' is a filepath, replace by the content of this file.

    args:
        entry (dict)
    """

    if "annote" in entry:
        annote = entry["annote"]
        annote = annote.strip()
        annote = os.path.expanduser(annote)
        if os.path.isfile(annote):
            with open(annote, "r") as f:
                annote = f.read()
            entry["annote"] = annote


def update_csl(
    csl, ids, ignored_keys=["ID"], id_name_max_len=15
) -> None:
    """update every entry in a csl-json bibliography.

    args:
        csl (Iterable[dict]):  the parsed bibliography.
        ids (set[str]):  existing keys (not to be assigned to entries).
        ignored_keys (list[str]):  a list of keys to del.
    """

    for entry in csl:
        # del ignored_keys
        for k in ignored_keys:
            if k in entry:
                del entry[k]

        # generate unique id
        entry["id"] = make_unique_id(
            entry, ids, id_name_max_len=id_name_max_len
        )

        # replace filepath annote by content
        replace_annote(entry)
