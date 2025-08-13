# otlet -- commande line bibliography manager.
# Copyright (C) 2024,2025  thjbdvlt
#
# otlet is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# otlet is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with otlet.  If not, see <https://www.gnu.org/licenses/>.
from isbnlib import meta, doi2tex, registry
import isbnlib
import argparse
import sys


def from_isbn(isbn, services) -> str:
    """fetch a reference from an ISBN.

    args:
        isbn (str):  the ISBN.
        services (list[str]):  list of services to use.

    returns (str, None):  bibtex describing the reference (or None).
    """

    bibtex = registry.bibformatters["bibtex"]
    ref = None
    for s in services:
        try:
            ref = meta(isbn, s)
            if ref and len(ref) > 0:
                return bibtex(ref)
        except isbnlib.NotValidISBNError:
            print("not a valid ISBN:", isbn, file=sys.stderr)
            exit(1)
    return None


def fetch_bibtex(method, identifier, isbn_services=None) -> str:
    """fetch a reference from a DOI or an ISBN.

    args:
        doi (str): the DOI.
        isbn (str): the isbn.
        isbn_services (list[str]): list of isbn services.

    returns (str, None): the reference in bibtex format.

    only one parameter in `doi` and `isbn` must be set.
    """

    if method == "doi":
        identifier = identifier.replace('https://doi.org/', '')
        return doi2tex(identifier)

    elif method == "isbn":
        return from_isbn(identifier, isbn_services)


def parse_args():
    """command line argument parser."""

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "method",
        choices=["doi", "isbn"],
        type=str,
        help="the method to get the reference.",
    )
    parser.add_argument(
        "identifier",
        type=str,
        help="the identifier of the reference.",
    )
    parser.add_argument(
        "--services",
        type=str,
        default="openl wiki goob",
        help="list of services to use for the `isbn` method (separated by space).",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=str,
        help="output file (default to stdout).",
        default=None,
    )
    args = parser.parse_args()
    return args


def main():
    """main function for command line."""

    args = parse_args()
    method = args.method
    id = args.identifier
    output = args.output
    services = args.services.split(" ")
    ref = None
    if method == "doi":
        try:
            ref = doi2tex(id)
        except isbnlib.dev._exceptions.ISBNLibHTTPError:
            print(
                "no reference found for ",
                method,
                id,
                "\nmaybe it's not a doi..?",
                file=sys.stderr,
            )
            exit(1)
    else:
        ref = from_isbn(id, services)
    if not ref:
        print("no reference found for ", method, id, file=sys.stderr)
        exit(1)
    if not output:
        print(ref)
    else:
        with open(output, "w") as f:
            f.write(ref)
    exit(0)


if __name__ == "__main__":
    main()
