#!/bin/bash

# retrolire -- commande line bibliography manager.
# Copyright (C) 2024,2025  thjbdvlt
#
# retrolire is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# retrolire is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with retrolire.  If not, see <https://www.gnu.org/licenses/>.

_retrolire(){
    local prev opts poss suff getter
    getter=
    poss=
    suff=' '
    filteropts="-t -v -s -q -c --tag --var --search --quote --concept"
    opts="$filteropts -l -i -r -e -o -O -p --last --id --recent --exact --or --not --output --pager -a -A"
    commands="edit open print quote refer add file list json cite update delete init"
    fileopts=

    prev="${COMP_WORDS[COMP_CWORD-1]}"

    case "$prev" in
        retrolire)
            poss="$commands $opts"
            ;;
        -o | -n | --or | --not)
            poss="$filteropts"
            ;;
        -v | --var)
            getter='_fields'
            suff='='
            ;;
        -t | --tag)
            getter="_tags"
            ;;
        d | de | del | dele | delet | delete)
            poss="entry file"
            ;;
        p | pr | pri | prin | print)
            poss="json tags $opts"
            ;;
        a | ad | add)
            poss='doi isbn template json bibtex'
            ;;
        doi | isbn)
            poss=""
            ;;
        -a)
            getter=_authors
            ;;
        template)
            poss='article book inproceeding inbook misc software'
            ;;
        u | up | upd | upda | updat | update)
            getter='_fields'
            ;;
        -i | --id)
            poss=''
            ;;
        -r | --regex | -q | --quote | -c | --concept)
            poss=""
            ;;
        -s | --search)
            getter=_lemmes
            ;;
        -l | --last | -e | --exact)
            poss="$opts"
            ;;
        f | fi | fil | file | json | bibtex)
            fileopts='-o filenames -A file'
            poss=""
            ;;
        *) poss="$opts"
            ;;
    esac

    if [ "$getter" ]
    then {
            dbname="$RETROLIRE_DBNAME"
            if [ "$dbname" ]
            then
            poss="$(retrolire "$getter")" || poss=
            fi
        }
    fi

    readarray -t poss < <(compgen -W "$poss" $fileopts -- "$2")

    readarray -t COMPREPLY < <(
        for i in "${poss[@]}"
        do
            echo "$i$suff"
        done
    )
    return 0
}

complete -o nospace -F _retrolire retrolire
