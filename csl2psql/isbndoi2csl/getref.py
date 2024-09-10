from isbnlib import meta, doi2tex, registry
import sys


def from_isbn(identifier, services):
    """get reference from isbn.

    args:
        identifier (str): the ISBN.
        services: a list of services.

    returns (dict, None): the reference, if found.
    """

    bibtex = registry.bibformatters["bibtex"]
    ref = None
    for s in services:
        ref = meta(isbn=identifier, service=s)
        if ref and len(ref) > 1:
            break
        else:
            ref = None
    if ref:
        return bibtex(ref)


def main():
    """main function, for command line usage."""

    if len(sys.argv) < 2:
        raise ValueError("not enough arguments.")

    method = sys.argv[1]
    identifier = sys.argv[2]

    if method == "isbn":
        services = sys.argv[3]
        ref = from_isbn(identifier, services)
        if ref:
            print(ref)
            exit(0)
        else:
            exit(1)

    elif method == "doi":
        ref = doi2tex(identifier)
        if ref:
            print(ref)
            exit(0)
        else:
            exit(1)


if __name__ == "__main__":
    main()
