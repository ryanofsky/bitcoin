#!/usr/bin/env bash

export LC_ALL=C

wtf() {
  "$@" &
  local pid="$!"
  echo "$pid -- $@" >&2
  wait "$pid"
  echo "$pid -- status $?" >&2
}

while true; do
  wtf tail -n0 -f /tmp/gdb.txt | while read pid path; do
      wtf screen -X screen sudo gdb -ex "handle SIGPIPE nostop noprint pass" -ex c -ex c -ex c "$path" "$pid"
  done
done
