#!/usr/bin/env python3

# Use this script to generate a table with mappings
# of HD44780 code to UTF8-strings


hdmap1 = "→←"
hdmap2 = "。「」、・ヲァィゥェォャュョッ"\
    "ーアイウエオカキクケコサシスセソ"\
    "タチツテトナニヌネノハヒフヘホマ"\
    "ミムメモヤユヨラリルレロワン゛゜"\
    "αäßεμσρg√⁻¹j˟₵₤ñö"\
    "pqΘ∞ΩüΣπx̅y不万円÷ █"

ind = 0x20
for i in range(32, 126):
    print(hex(i), chr(i), chr(i).encode())

print()

ind = 126
for i in range(2):
    print(hex(ind), hdmap1[i], hdmap1[i].encode())
    ind += 1

print()

ind = 0xa1
skp = False
for c in hdmap2:
    if skp is False:
        print(hex(ind), c, c.encode())
    else:
        print("     ", c, c.encode())
        skp = False
        ind += 1
        continue
    if ind != 0xe9 and ind != 0xf8:
        ind += 1
    else:
        skp = True
