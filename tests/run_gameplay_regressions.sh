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
python3 - "$build/menu_font_fixture.h" <<'PY'
import pathlib, re, sys
source = pathlib.Path("bin/segment2.c").read_text()
table = source.split("const u8 *const main_font_lut[] = {", 1)[1]
table = table.split("#elif defined(VERSION_US)", 1)[1].split("#elif", 1)[0]
table = re.sub(r"//[^\n]*|/\*.*?\*/", "", table, flags=re.S)
entries = re.findall(r"texture_font_char_us_\w+|0x0", table)
assert len(entries) == 256, len(entries)
with pathlib.Path(sys.argv[1]).open("w") as out:
    out.write("void *main_font_lut[256] = {\n")
    for i, entry in enumerate(entries):
        if entry != "0x0":
            out.write(f"    [{i}] = texture,\n")
    out.write("};\n")
PY
for test in netplay_free_travel completed_save netplay_avatar netplay_contact_world netplay_menu_render; do
    cc "${flags[@]}" "tests/$test.c" -Wl,--gc-sections -lm -o "$build/$test"
    "$build/$test"
done
