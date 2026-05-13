"""
image_compare.py — shared ±10%-per-channel image diff for visual regression.

Pattern mirrors testingmap2png.py + testingmap4client.py: every channel
of every pixel must be within 10% of the reference value, otherwise the
test fails. PNG compression / quantisation jitter is absorbed by the
±10% tolerance; the per-image error budget is zero so a single rogue
pixel still trips the test.

Used by:
  - testingmap4client.py        — native Linux qtopengl --take-screenshot
  - testingcompilationwindows.py — wine64 qtopengl --take-screenshot
  - testingcompilationandroid.py — adb screencap of qtopengl on emulator

The tolerance (TOLERANCE_PER_CHANNEL = 0.10) is shared across all
callers; bumping it here loosens every visual-regression test in one
shot.
"""

import os

# Per-channel tolerance: |new - ref| <= 0.10 * ref → pass.
TOLERANCE_PER_CHANNEL = 0.10


def image_compare(image_new, image_reference, diff_image=None):
    """Per-pixel ±10%-per-channel match. Returns (ok, detail).

    On failure, when `diff_image` is non-None, writes a black-on-white
    mask of the failing pixels to that path so the operator can see
    *where* the divergence is.

    `ok=False` if:
      - PIL is missing
      - either file is unreadable
      - dimensions differ
      - any pixel's R/G/B/A diff exceeds 10% of the reference value"""
    try:
        from PIL import Image
    except ImportError:
        return False, "PIL (Pillow) not installed; required for image comparison"
    try:
        img_new = Image.open(image_new).convert("RGBA")
    except (OSError, ValueError) as e:
        return False, f"unreadable new image {image_new}: {e}"
    try:
        img_ref = Image.open(image_reference).convert("RGBA")
    except (OSError, ValueError) as e:
        return False, f"unreadable reference {image_reference}: {e}"
    if img_new.size != img_ref.size:
        return False, (f"size mismatch: new {img_new.size[0]}x{img_new.size[1]} "
                       f"vs ref {img_ref.size[0]}x{img_ref.size[1]}")
    w, h = img_ref.size
    px_new = img_new.load()
    px_ref = img_ref.load()
    diff_bytes = bytearray(b"\xff\xff\xff" * (w * h))
    fail_count = 0
    first_fail = None
    y = 0
    while y < h:
        x = 0
        while x < w:
            n = px_new[x, y]
            r = px_ref[x, y]
            ci = 0
            while ci < 4:
                if abs(n[ci] - r[ci]) > TOLERANCE_PER_CHANNEL * r[ci]:
                    if first_fail is None:
                        first_fail = (x, y, ci, n[ci], r[ci])
                    fail_count += 1
                    idx = (y * w + x) * 3
                    diff_bytes[idx] = 0
                    diff_bytes[idx + 1] = 0
                    diff_bytes[idx + 2] = 0
                    break
                ci += 1
            x += 1
        y += 1
    total = w * h
    if fail_count == 0:
        return True, f"all {total} pixels within +/-10% per channel ({w}x{h})"
    pct = (fail_count / total) * 100.0
    if diff_image:
        try:
            os.makedirs(os.path.dirname(diff_image), exist_ok=True)
        except OSError:
            pass
        try:
            Image.frombytes("RGB", (w, h), bytes(diff_bytes)).save(diff_image)
        except OSError:
            pass
    fx, fy, fc, fn, fr = first_fail
    chan = "RGBA"[fc]
    suffix = f"; diff mask saved to {diff_image}" if diff_image else ""
    return False, (f"{fail_count}/{total} pixels diff > 10% ({pct:.2f}%); "
                   f"first fail @ ({fx},{fy}) channel {chan}: "
                   f"new={fn} ref={fr}{suffix}")


def compare_with_reference(label, new_path, ref_path, diff_path=None):
    """Wrapper that returns (ok, detail) with a helpful "bless" message
    when the reference image doesn't exist yet (so the operator can
    accept the just-produced screenshot by copying it into the repo).

    `label` is just used in the bless message — pass something the
    operator can act on, e.g. "wine qtopengl autosolo"."""
    if not os.path.isfile(new_path):
        return False, f"screenshot not produced: {new_path}"
    if not os.path.isfile(ref_path):
        return False, (f"reference image missing for {label}: {ref_path}. "
                       f"Bless the just-produced screenshot with: "
                       f"cp {new_path} {ref_path}")
    return image_compare(new_path, ref_path, diff_image=diff_path)
