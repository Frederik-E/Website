"""
Hellt den Hintergrund eines Portraitfotos auf und gibt ihm den kühlen Ton der
Seite (Lavender Mist). Die Person bleibt unverändert.

    python3 scripts/foto-aufhellen.py inhalt/foto/original/IMG_6101.JPG inhalt/foto/portrait.jpg

Gedacht für Fotos vor einem einfarbigen Studiohintergrund. Ablauf:
1. Der Hintergrundverlauf wird aus Rand und oberen Ecken geschätzt
   (Polynom zweiten Grades in x und y, im Lab-Farbraum).
2. GrabCut trennt Person und Hintergrund, Gesicht, Hals und helles Hemd sind
   als sicher Vordergrund vorgegeben.
3. Nur der Anteil des Hintergrunds wird verschoben: neu = alt + (1 - alpha) * delta.
   So bleiben Körnung und Verlauf erhalten, und an Haaren und Kanten entsteht
   kein dunkler Saum.

Braucht Python 3 mit numpy, scipy und opencv (cv2).
"""

import sys

import cv2
import numpy as np
from scipy import ndimage

# Um so viel wird der Hintergrund heller (L im OpenCV-Lab, 0 bis 255).
AUFHELLEN = 38
# Farbton des Hintergrunds: Lavender Mist, die Grundfarbe der Seite.
ZIELFARBE_RGB = (252, 247, 255)


def hintergrund_modell(lab, h, w):
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    probe = np.zeros((h, w), bool)
    probe[:30, :] = True
    probe[: int(h * 0.58), :18] = True
    probe[: int(h * 0.62), -18:] = True
    probe[: int(h * 0.42), : int(w * 0.24)] = True
    probe[: int(h * 0.42), int(w * 0.75) :] = True

    def merkmale(x, y):
        x, y = x / w, y / h
        return np.stack([np.ones_like(x), x, y, x * x, y * y, x * y], axis=-1)

    a = merkmale(xx[probe], yy[probe])
    koeff = np.stack([np.linalg.lstsq(a, lab[..., k][probe], rcond=None)[0] for k in range(3)], axis=1)
    return merkmale(xx, yy) @ koeff, xx, yy


def maske_berechnen(bild, lab, modell, xx, yy):
    h, w = lab.shape[:2]
    abstand = np.linalg.norm(lab - modell, axis=2)

    nah = abstand < 7
    markiert, _ = ndimage.label(nah)
    saat = set(np.unique(markiert[0])) | set(np.unique(markiert[: int(h * 0.55), 0])) | set(
        np.unique(markiert[: int(h * 0.6), -1])
    )
    saat.discard(0)
    hg = np.isin(markiert, list(saat))

    maske = np.full((h, w), cv2.GC_PR_FGD, np.uint8)
    maske[hg] = cv2.GC_PR_BGD
    maske[ndimage.binary_erosion(hg, iterations=10)] = cv2.GC_BGD
    gesicht = ((xx - w * 0.495) / (w * 0.15)) ** 2 + ((yy - h * 0.30) / (h * 0.17)) ** 2 < 1
    maske[gesicht] = cv2.GC_FGD
    maske[int(h * 0.44) : int(h * 0.55), int(w * 0.37) : int(w * 0.60)] = cv2.GC_FGD
    hell = (lab[..., 0] > 214) & (yy > h * 0.5)
    hl, _ = ndimage.label(hell)
    unten = set(np.unique(hl[-1]))
    unten.discard(0)
    maske[np.isin(hl, list(unten))] = cv2.GC_FGD
    maske[(abstand > 20) & (yy > h * 0.5)] = cv2.GC_FGD

    bgd, fgd = np.zeros((1, 65)), np.zeros((1, 65))
    cv2.grabCut(bild, maske, None, bgd, fgd, 8, cv2.GC_INIT_WITH_MASK)
    vorder = (maske == cv2.GC_FGD) | (maske == cv2.GC_PR_FGD)

    lv, n = ndimage.label(vorder)
    if n > 1:
        groessen = ndimage.sum(np.ones_like(lv), lv, range(1, n + 1))
        vorder = lv == (np.argmax(groessen) + 1)
    vorder = ndimage.binary_fill_holes(vorder)
    vorder = ndimage.binary_opening(vorder, iterations=2)
    weich = cv2.GaussianBlur(vorder.astype(np.float32), (0, 0), 2.0)
    return np.clip((weich - 0.5) * 2.2 + 0.5, 0, 1)


def main(quelle, ziel):
    bild = cv2.imread(quelle)
    if bild is None:
        sys.exit(f"Kann {quelle} nicht lesen.")
    h, w = bild.shape[:2]
    lab = cv2.cvtColor(bild, cv2.COLOR_BGR2LAB).astype(np.float32)

    modell, xx, yy = hintergrund_modell(lab, h, w)
    alpha = maske_berechnen(bild, lab, modell, xx, yy)

    # Neuer Hintergrund: Helligkeit angehoben, Farbton von Lavender Mist.
    ziel_bgr = np.uint8([[ZIELFARBE_RGB[::-1]]])
    ziel_lab = cv2.cvtColor(ziel_bgr, cv2.COLOR_BGR2LAB).astype(np.float32)[0, 0]
    neu = modell.copy()
    neu[..., 0] = np.minimum(modell[..., 0] + AUFHELLEN, 250)
    neu[..., 1] = ziel_lab[1]
    neu[..., 2] = ziel_lab[2]

    ergebnis = lab + (1 - alpha)[..., None] * (neu - modell)
    ergebnis = np.clip(ergebnis, 0, 255).astype(np.uint8)
    cv2.imwrite(ziel, cv2.cvtColor(ergebnis, cv2.COLOR_LAB2BGR), [cv2.IMWRITE_JPEG_QUALITY, 93])
    print(f"{ziel} geschrieben ({w} x {h} px).")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit("Aufruf: python3 scripts/foto-aufhellen.py <original> <ziel.jpg>")
    main(sys.argv[1], sys.argv[2])
