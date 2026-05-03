from PIL import Image, ImageDraw, ImageFont
import math, os

OUT = "resources/images"

for angle in [10, 15, 20, 25, 30]:
    # Draw text on a wide transparent canvas, then rotate + crop tight
    W, H = 100, 24
    base = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(base)
    # Use a proportional font close to Gothic14 size
    try:
        font = ImageFont.truetype("/System/Library/Fonts/Helvetica.ttc", 13)
    except:
        font = ImageFont.load_default()
    bbox = draw.textbbox((0, 0), "pebble", font=font)
    tw, th = bbox[2] - bbox[0], bbox[3] - bbox[1]
    x = (W - tw) // 2 - bbox[0]
    y = (H - th) // 2 - bbox[1]
    draw.text((x, y), "pebble", fill=(200, 200, 200, 255), font=font)

    # Rotate (expand=True keeps all pixels)
    rotated = base.rotate(-angle, expand=True, resample=Image.BICUBIC)
    rotated.save(os.path.join(OUT, f"text_pebble_{angle}.png"))
    print(f"Saved text_pebble_{angle}.png  {rotated.size}")

print("Done")