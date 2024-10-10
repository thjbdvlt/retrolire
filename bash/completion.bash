#!/bin/bash

_retrolire(){
    local prev opts poss suff getter
    getter=
    poss=
    suff=' '
    opts='--last --tag --var --search --quote --show-tags --id --move'
    commands="edit open print quote refer add file list json cite update delete init"
    fileopts=

    prev="${COMP_WORDS[COMP_CWORD-1]}"

    case "$prev" in
        retrolire)
            poss="$commands"
            ;;
        -v | --var)
            # getter='_lfields'
            getter='_Fields'
            suff='='
            ;;
        -t | --tag)
            # getter="_ltags"
            getter="_Tags"
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
        u | up | upd | upda | updat | update)
            getter='_Fields'
            ;;
        -i | --id)
            poss=''
            ;;
        -s | --search | -q | --quote | -c | --concept)
            # TODO: -c completion concepts
            poss=""
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
