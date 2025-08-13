#!/bin/bash

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

_otlet(){
    local cur prev opts poss suff getter
    getter=
    poss=
    suff=' '
    commands='edit open add file list json cite update delete init tag-pick parse'
    keywords='or not'

    prev="${COMP_WORDS[COMP_CWORD-1]}";
    case "$prev" in
        add) poss='doi isbn json bibtex template';;
        update) getter=_field;;
        *)
        cur="${COMP_WORDS[COMP_CWORD]}";
        case "$cur" in
            @* | author* | author:*) getter=_author; suff=" ";;
            .*) getter=_tag; suff=" ";;
            *) getter=_field; suff=":";;
        esac
    esac

    if [ "$getter" ]
    then poss="$(otlet "$getter")" || poss=
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

complete -o nospace -F _otlet otlet
