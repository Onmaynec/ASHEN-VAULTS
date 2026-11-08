#!/usr/bin/env python3
"""SHA-pinned Kenney Tiny Dungeon CC0 sprites adapted into ASHEN VAULTS palettes.
Dependencies: Python 3.11, Pillow (free). The original ZIP remains unmodified in transit;
only four sampled 16x16 sprites per biome are exported as a compact 4x4 atlas.
"""
from __future__ import annotations
import hashlib
import io
from pathlib import Path
from urllib.request import urlopen, Request
from zipfile import ZipFile
from PIL import Image

URL = "https://raw.githubusercontent.com/bill2432/kenney-2d-mirror/main/packs/tiny-dungeon.zip"
SOURCE = "https://kenney.nl/assets/tiny-dungeon"
SHA256 = "c109438ab06f65fd80f9b2686a4cf9c7c11dc64444b47333ec71d602f8bb5fc7"
PALETTES = (
    ((29,39,51),(91,107,115),(167,186,189)),  # crypt
    ((25,46,37),(78,127,83),(156,197,143)),   # spores
    ((43,27,36),(151,86,68),(229,164,111)),   # foundry
    ((27,29,37),(97,87,93),(179,118,115)),    # bastion
)
# Chosen Kenney base props; no user-facing claim about their in-game semantics.
INDICES = (64, 80, 96, 112)


def main() -> None:
    req = Request(URL, headers={"User-Agent": "AshenVaults-CC0-asset-import/1.0"})
    with urlopen(req, timeout=45) as response:
        zip_data = response.read(2_000_000)
    if hashlib.sha256(zip_data).hexdigest() != SHA256:
        raise RuntimeError("Kenney archive hash mismatch — refusing unknown artwork")

    with ZipFile(io.BytesIO(zip_data)) as archive:
        files = {Path(n).name: n for n in archive.namelist()
                 if n.lower().endswith(".png") and "/tiles/" in n.lower()}
        atlas = Image.new("RGBA", (64,64), (0,0,0,0))
        for biome, palette in enumerate(PALETTES):
            for column, idx in enumerate(INDICES):
                name = f"tile_{idx:04d}.png"
                if name not in files:
                    raise FileNotFoundError("Missing Kenney CC0 tile " + name)
                art = Image.open(io.BytesIO(archive.read(files[name]))).convert("RGBA")
                art = art.resize((16,16), Image.Resampling.NEAREST)
                pixels = art.load()
                for y in range(16):
                    for x in range(16):
                        r,g,b,a = pixels[x,y]
                        if a == 0: continue
                        luminance = (r*54 + g*183 + b*19)//256
                        base = palette[0] if luminance < 80 else (
                            palette[1] if luminance < 160 else palette[2])
                        pixels[x,y] = (*base,a)
                atlas.alpha_composite(art, (column*16,biome*16))

    directory = Path("Content/AVVisuals/Kenney")
    directory.mkdir(parents=True, exist_ok=True)
    atlas.save(directory / "TinyDungeonDecor.png", optimize=True)
    (directory / "SOURCE_CC0.txt").write_text(
        "KENNEY TINY DUNGEON (CC0 1.0 Universal)\n"
        f"Official source: {SOURCE}\n"
        f"Mirror used for reproducibility: {URL}\n"
        f"Mirror ZIP SHA256: {SHA256}\n"
        "Original art: Kenney. Pixel recoloring: ASHEN VAULTS.\n"
        "Attribution is not legally required by CC0; voluntarily given here.\n",
        encoding="utf-8",
    )
    print("Wrote 4x4 recolored CC0 Kenney object atlas.")


if __name__ == "__main__":
    main()
