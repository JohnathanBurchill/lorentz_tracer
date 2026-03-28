#!/usr/bin/env python3
"""Generate a 1024x1024 app icon for Lorentz Tracer."""
from PIL import Image, ImageDraw, ImageFilter
import math

SIZE = 1024
img = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
draw = ImageDraw.Draw(img)

cx, cy = SIZE // 2, SIZE // 2
r_bg = 480

# Dark circular background with subtle gradient
for r in range(r_bg, 0, -1):
    frac = r / r_bg
    shade = int(18 + 12 * (1 - frac))
    draw.ellipse([cx - r, cy - r, cx + r, cy + r],
                 fill=(shade, shade, shade + 10, 255))

# Helical orbit (projected 3D helix) — bigger radius, fewer tighter turns
n_pts = 800
helix_pts = []
for i in range(n_pts):
    t = i / n_pts * 5 * math.pi
    radius = 220
    x = radius * math.cos(t)
    y = radius * math.sin(t)
    drift = t / (5 * math.pi) * 350 - 175
    px = cx + x * 0.85 + y * 0.15
    py = cy + drift - y * 0.3
    helix_pts.append((px, py))

# Glow layer: draw thick blurred helix underneath
glow = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
glow_draw = ImageDraw.Draw(glow)
for i in range(1, len(helix_pts)):
    frac = i / len(helix_pts)
    alpha = int(30 + 100 * frac)
    glow_draw.line([helix_pts[i-1], helix_pts[i]],
                   fill=(80, 180, 255, alpha), width=28)
glow = glow.filter(ImageFilter.GaussianBlur(radius=12))
img = Image.alpha_composite(img, glow)
draw = ImageDraw.Draw(img)

# Main helix trail — thick, bright
for i in range(1, len(helix_pts)):
    frac = i / len(helix_pts)
    alpha = int(60 + 195 * frac)
    r_color = int(60 + 140 * (1 - frac))
    g_color = int(140 + 100 * frac)
    b_color = 255
    width = int(6 + 12 * frac)
    draw.line([helix_pts[i-1], helix_pts[i]],
              fill=(r_color, g_color, b_color, alpha), width=width)

# Bright core on the newest portion
for i in range(len(helix_pts) * 2 // 3, len(helix_pts)):
    frac = (i - len(helix_pts) * 2 // 3) / (len(helix_pts) // 3)
    alpha = int(100 + 155 * frac)
    width = int(2 + 4 * frac)
    draw.line([helix_pts[i-1], helix_pts[i]],
              fill=(200, 240, 255, alpha), width=width)

# Particle glow at the end — big and bright
px, py = helix_pts[-1]
for rr in range(50, 0, -1):
    alpha = int(255 * (1 - rr / 50) ** 1.5)
    draw.ellipse([px - rr, py - rr, px + rr, py + rr],
                 fill=(255, 255, 120, alpha))
draw.ellipse([px - 12, py - 12, px + 12, py + 12],
             fill=(255, 255, 220, 255))

# B arrow — more visible
arrow_x = cx + 300
arrow_y1 = cy + 220
arrow_y2 = cy - 220
draw.line([(arrow_x, arrow_y1), (arrow_x, arrow_y2)],
          fill=(255, 80, 255, 100), width=8)
draw.polygon([(arrow_x, arrow_y2 - 30),
              (arrow_x - 18, arrow_y2 + 10),
              (arrow_x + 18, arrow_y2 + 10)],
             fill=(255, 80, 255, 100))

# Thin ring around the icon for definition
draw.ellipse([cx - r_bg, cy - r_bg, cx + r_bg, cy + r_bg],
             outline=(80, 120, 200, 120), width=3)

img.save("resources/icon_1024.png")
print("Saved resources/icon_1024.png")
