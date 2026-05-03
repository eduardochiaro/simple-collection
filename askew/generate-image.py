from PIL import Image, ImageDraw, ImageFont
import math, os

OUT = "resources/images"

VARIANTS = [
    ("",       14, 100, 24),   # normal  → text_pebble_{angle}.png
    ("_large", 20, 140, 32),   # large   → text_pebble_{angle}_large.png
]

for angle in [0, 10, 15, 20, 25, 30]:
    for suffix, font_size, W, H in VARIANTS:
        base = Image.new("RGBA", (W, H), (0, 0, 0, 0))
        draw = ImageDraw.Draw(base)
        try:
            font = ImageFont.truetype("/System/Library/Fonts/Helvetica.ttc", font_size)
        except:
            font = ImageFont.load_default()
        bbox = draw.textbbox((0, 0), "pebble", font=font)
        tw, th = bbox[2] - bbox[0], bbox[3] - bbox[1]
        x = (W - tw) // 2 - bbox[0]
        y = (H - th) // 2 - bbox[1]
        draw.text((x, y), "pebble", fill=(85, 85, 85, 255), font=font)

        rotated = base.rotate(-angle, expand=True, resample=Image.BICUBIC)
        name = f"text_pebble_{angle}{suffix}.png"
        rotated.save(os.path.join(OUT, name))
        print(f"Saved {name}  {rotated.size}")

print("Done")