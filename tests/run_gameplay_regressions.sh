#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
build=$(mktemp -d)
trap 'rm -rf "$build"' EXIT
cc tools/textconv.c tools/utf8.c tools/hashtable.c -o "$build/textconv"
"$build/textconv" charmap.txt include/text_strings.h.in "$build/text_strings.h"
"$build/textconv" charmap_menu.txt include/text_menu_strings.h.in "$build/text_menu_strings.h"
flags=(-std=gnu11 -ffunction-sections -fdata-sections -Werror=implicit-function-declaration
    -D__ANDROID__ -DNON_MATCHING -DAVOID_UB -DTARGET_LINUX -DF3DEX_GBI_2E
    -D_LANGUAGE_C -DNO_SEGMENTED_MEMORY -DVERSION_US -I"$build" -I. -Iinclude -Isrc)
for test in netplay_free_travel completed_save netplay_avatar netplay_contact_world; do
    cc "${flags[@]}" "tests/$test.c" -Wl,--gc-sections -lm -o "$build/$test"
    "$build/$test"
done
