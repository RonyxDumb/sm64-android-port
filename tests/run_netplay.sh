#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
build=$(mktemp -d)
trap 'rm -rf "$build"' EXIT
cc -std=gnu11 -Wall -Wextra -Werror -DNETPLAY_SOCKET_TEST -Isrc/pc \
    tests/netplay_session.c src/pc/netplay_lan.c -lm -o "$build/session"
"$build/session" host > "$build/host.log" 2>&1 &
host_pid=$!
"$build/session" guest > "$build/guest.log" 2>&1 &
guest_pid=$!
result=0
wait "$host_pid" || result=1
wait "$guest_pid" || result=1
cat "$build/host.log" "$build/guest.log"
cc -std=gnu11 -Wall -Wextra -Werror -DNETPLAY_SOCKET_TEST -Isrc/pc \
    tests/netplay_packets.c -lm -o "$build/packets"
"$build/packets"
bash tests/run_gameplay_regressions.sh
exit "$result"
