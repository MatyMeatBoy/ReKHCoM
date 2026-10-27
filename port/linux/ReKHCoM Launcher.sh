#!/bin/sh
DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P) || exit 1
if ! command -v python3 >/dev/null 2>&1; then
    echo 'The optional native importer needs Python3.9+. Play directly with ReKHCoM.sh; no dependencies were installed.' >&2
    command -v kdialog >/dev/null 2>&1 && kdialog --error 'Native import needs Python3.9+. Play directly with ReKHCoM.sh.'
    exit 2
fi
if [ "$#" -eq 0 ]; then set -- --gui; fi
exec python3 "$DIR/launcher/rekhcom_launcher.py" --data "$DIR" "$@"
