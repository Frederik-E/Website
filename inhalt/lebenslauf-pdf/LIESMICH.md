# Lebenslauf als PDF

Leg hier dein eigenes Lebenslauf-PDF ab. Es wird dann statt des automatisch
erzeugten PDFs zum Herunterladen angeboten.

- **Deutsch:** ein PDF mit beliebigem Namen, z. B. `Lebenslauf.pdf`.
  Herunterladen auf der deutschen Seite (`/cv.pdf`).
- **Englisch:** dasselbe mit `.en.pdf` am Ende, z. B. `Lebenslauf.en.pdf`.
  Herunterladen auf der englischen Seite (`/en/cv.pdf`).

Je Sprache nur ein PDF. Fehlt eine Sprache, wird das PDF dafür wie bisher aus
der Seite `/cv/` bzw. `/en/cv/` gedruckt.

Die englische Fassung des deutschen PDFs erzeugt
`python3 scripts/lebenslauf-englisch.py` (nur der Text wird übersetzt).

Danach `npm run cv` ausführen (oder `npm run check`, das macht es mit).

Achtung: Das PDF ist öffentlich herunterladbar. Adresse, Telefonnummer und
Geburtsdatum gehören nicht hinein.
