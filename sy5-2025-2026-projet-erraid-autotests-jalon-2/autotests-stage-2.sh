set +e

function nop() {
    :
}
trap nop SIGINT # So that bash does lauch the needed kill commands even if the
                # tests were SIGINT'd

dokill=true
if [ "$#" -ge 1 -a "$1" = "-n" ]; then
  dokill=false
  shift
fi

ran_daemon=false
ran_client=false

contains () {
  local e match="$1"
  shift
  for e; do [ "$e" = "$match" ] && return 0; done
  return 1
}

if ! command -v python3 >/dev/null 2>/dev/null; then
  printf "Abandon : la commande `python3` est introuvable. Veuillez installer Python.\n" >&2
  exit 1
fi

if [ -z "${GRADE_PROJECT}" ] || ! "${GRADE_PROJECT}"; then
  printf 'Compilation du projet avec `make`...\n\n' >&2
fi
(
  cd ..
  make >/dev/null 2>&1 </dev/null
)
RES=$?
if [ "$RES" -ne 0 ]; then
  printf "Erreur : la compilation s'est terminée avec le code de retour %s. Abandon.\n" "$RES"
  exit 1
fi

if contains "1" "$@"; then
  python3 run-erraid-tests-jalon-1.py
  ran_daemon=true
fi

if [ "$#" -eq 0 ] || contains "2" "$@"  || contains "2c" "$@" ; then
  python3 run-tadmor-tests-jalon-2.py
  ran_client=true
fi

if [ "$#" -eq 0 ] || contains "2" "$@"  || contains "2d" "$@" ; then
  python3 run-erraid-tests-jalon-2.py
  ran_daemon=true
fi

if $dokill && $ran_daemon ; then
  killall -q -u "$USER" erraid >/dev/null 2>/dev/null
fi
if $dokill && $ran_client ; then
  killall -q -u "$USER" tadmor >/dev/null 2>/dev/null
fi
if $dokill ; then
  killall -q -u "$USER" memcheck-amd64- >/dev/null 2>/dev/null
fi

sleep 1

if $dokill && $ran_daemon ; then
  killall -q -9 -u "$USER" erraid >/dev/null 2>/dev/null
fi
if $dokill && $ran_client ; then
  killall -q -9 -u "$USER" tadmor >/dev/null 2>/dev/null
fi
if $dokill ; then
  killall -q -9 -u "$USER" memcheck-amd64- >/dev/null 2>/dev/null
fi

true

