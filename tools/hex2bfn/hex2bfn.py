import sys

hexfont_path = sys.argv[1]
binfont_path = sys.argv[2]

max_codepoint = 0
with open(hexfont_path, "r") as hexfont:
    for line in hexfont:
        codepoint, _ = line.split(":")
        codepoint = int(codepoint, 16)
        max_codepoint = max(max_codepoint, codepoint)
        
file_header = bytes([
    *"bfnt".encode("ascii"),
    *max_codepoint.to_bytes(4, "little"),
    *int(16).to_bytes(4, "little"),
    0,
    0,
    0,
    0,
])

with open(hexfont_path, "r") as hexfont, open(binfont_path, "wb") as binfont:
    binfont.write(file_header)
    
    glyph_header_offset = len(file_header) + (max_codepoint + 1) * 4
    
    for line in hexfont:
        # parse hex data
        codepoint, glyph = line.split(":")
        if len(glyph) == 0:
            continue
        codepoint = int(codepoint, 16)
        glyph = bytes.fromhex(glyph)
        is_full_width = len(glyph) == 32
        
        # make glyph header
        glyph_header = int(1 if is_full_width else 0).to_bytes(4, "little")
        
        # write glyph data
        binfont.seek(glyph_header_offset)
        binfont.write(glyph_header + glyph)
        
        # write glyph offset
        binfont.seek(len(file_header) + codepoint * 4)
        binfont.write(glyph_header_offset.to_bytes(4, "little"))
        
        glyph_header_offset += len(glyph_header + glyph)
        
        