# import sys
# from PIL import Image

# W, H = 128, 128
# src, out = sys.argv[1], sys.argv[2]

# img = Image.open(src).convert("RGB")
# img.thumbnail((W, H), Image.LANCZOS)          # keeps aspect ratio
# canvas = Image.new("RGB", (W, H), (0, 0, 0))
# canvas.paste(img, ((W - img.width) // 2, (H - img.height) // 2))

# words = [((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
#          for r, g, b in canvas.getdata()]

# with open(out, "w") as f:
#     f.write("#pragma once\n#include <stdint.h>\n#include <pgmspace.h>\n")
#     f.write(f"constexpr int IMG_W = {W}, IMG_H = {H};\n")
#     f.write("const uint16_t image_data[] PROGMEM = {\n")
#     for i in range(0, len(words), 12):
#         f.write("  " + ", ".join(f"0x{w:04X}" for w in words[i:i+12]) + ",\n")
#     f.write("};\n")

import sys, os, re
from PIL import Image, ImageOps

W, H = 128, 128
MODE = "fill"   # "fill" = crop to cover the screen, "fit" = letterbox with black bars

out, folder = sys.argv[1], sys.argv[2]
exts = (".png", ".jpg", ".jpeg")
files = sorted(f for f in os.listdir(folder) if f.lower().endswith(exts))
if not files:
    sys.exit("No images found in " + folder)

def convert(path):
    img = Image.open(path).convert("RGB")
    if MODE == "fill":
        canvas = ImageOps.fit(img, (W, H), Image.LANCZOS)
    else:
        img.thumbnail((W, H), Image.LANCZOS)
        canvas = Image.new("RGB", (W, H), (0, 0, 0))
        canvas.paste(img, ((W - img.width) // 2, (H - img.height) // 2))
    raw = canvas.tobytes()
    return [((raw[i] & 0xF8) << 8) | ((raw[i+1] & 0xFC) << 3) | (raw[i+2] >> 3)
            for i in range(0, len(raw), 3)]

with open(out, "w") as f:
    f.write("#pragma once\n#include <stdint.h>\n#include <pgmspace.h>\n")
    f.write(f"constexpr int IMG_W = {W}, IMG_H = {H};\n")
    f.write(f"constexpr int IMG_COUNT = {len(files)};\n\n")
    for n, name in enumerate(files):
        words = convert(os.path.join(folder, name))
        f.write(f"// {re.sub(r'[^A-Za-z0-9_. -]', '_', name)}\n")
        f.write(f"const uint16_t img_{n}[] PROGMEM = {{\n")
        for i in range(0, len(words), 12):
            f.write("  " + ", ".join(f"0x{w:04X}" for w in words[i:i+12]) + ",\n")
        f.write("};\n\n")
    f.write("const uint16_t* const images[] = { "
            + ", ".join(f"img_{n}" for n in range(len(files))) + " };\n")

print(f"Wrote {len(files)} images to {out}")