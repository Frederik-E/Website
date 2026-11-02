/**
 * Erzeugt die Lebenslauf-PDFs: public/cv.pdf (Deutsch) und public/en/cv.pdf
 * (Englisch). Liegt in inhalt/lebenslauf-pdf/ ein eigenes PDF, wird dieses
 * genommen: eine Datei mit .en.pdf am Ende ist die englische, jede andere .pdf
 * die deutsche. Der Name davor ist egal. Fehlt eine Fassung, wird die Seite
 * /cv/ bzw. /en/cv/ mit Playwright als A4-PDF gedruckt.
 */
import { access, copyFile, mkdir, readdir, writeFile } from 'node:fs/promises';
import { dirname, join } from 'node:path';
import { chromium } from 'playwright';
import { serverBereitstellen } from './lib/server.mjs';

export const PDF_ORDNER = 'inhalt/lebenslauf-pdf';

const FASSUNGEN = [
  { sprache: 'de', seite: '/cv/', ziele: ['public/cv.pdf', 'dist/cv.pdf'] },
  { sprache: 'en', seite: '/en/cv/', ziele: ['public/en/cv.pdf', 'dist/en/cv.pdf'] },
];

/**
 * Sucht die eigenen PDFs in inhalt/lebenslauf-pdf/. Liefert je Sprache den
 * Pfad oder null. Liegen für eine Sprache mehrere PDFs dort, ist unklar,
 * welches gemeint ist; dann gibt es einen Fehler statt einer stillen Wahl.
 */
export async function eigenePdfs() {
  const namen = await readdir(PDF_ORDNER).catch(() => []);
  const pdfs = namen.filter((name) => name.toLowerCase().endsWith('.pdf')).sort();
  const englisch = pdfs.filter((name) => name.toLowerCase().endsWith('.en.pdf'));
  const deutsch = pdfs.filter((name) => !englisch.includes(name));

  const eines = (liste, sprache) => {
    if (liste.length > 1) {
      throw new Error(
        `In ${PDF_ORDNER}/ liegen mehrere ${sprache} PDFs: ${liste.join(', ')}. Lass nur eines darin.`,
      );
    }
    return liste.length === 1 ? join(PDF_ORDNER, liste[0]) : null;
  };

  return { de: eines(deutsch, 'deutsche'), en: eines(englisch, 'englische') };
}

async function existiert(pfad) {
  try {
    await access(pfad);
    return true;
  } catch {
    return false;
  }
}

/** Schreibt in public/ und, wenn schon gebaut wurde, auch in dist/. */
async function ablegen(ziele, schreiben) {
  for (const ziel of ziele) {
    if (ziel.startsWith('dist/') && !(await existiert('dist'))) continue;
    await mkdir(dirname(ziel), { recursive: true });
    await schreiben(ziel);
  }
}

export async function cvErzeugen({ leise = false, basis: vorhandeneBasis } = {}) {
  const eigene = await eigenePdfs();
  const zuDrucken = [];
  for (const fassung of FASSUNGEN) {
    const eigenes = eigene[fassung.sprache];
    if (eigenes) {
      await ablegen(fassung.ziele, (ziel) => copyFile(eigenes, ziel));
      if (!leise) console.log(`${eigenes} wurde nach ${fassung.ziele[0]} kopiert.`);
    } else {
      zuDrucken.push(fassung);
    }
  }
  if (zuDrucken.length === 0) return;

  const { basis, beenden } = vorhandeneBasis
    ? { basis: vorhandeneBasis, beenden: async () => {} }
    : await serverBereitstellen();
  const browser = await chromium.launch();

  try {
    const seite = await browser.newPage();
    for (const fassung of zuDrucken) {
      await seite.goto(basis + fassung.seite, { waitUntil: 'networkidle' });
      await seite.emulateMedia({ media: 'print' });
      await seite.evaluate(() => document.fonts.ready);

      const pdf = await seite.pdf({
        format: 'A4',
        printBackground: true,
        preferCSSPageSize: true,
      });

      await ablegen(fassung.ziele, (ziel) => writeFile(ziel, pdf));
      if (!leise) console.log(`${fassung.ziele[0]} wurde aus der Seite ${fassung.seite} erzeugt.`);
    }
    if (!leise) {
      console.log(`Eigenes PDF bevorzugt? Dann lege es in den Ordner ${PDF_ORDNER}/.`);
      console.log('Der Name ist egal; die englische Fassung endet auf .en.pdf.');
    }
  } finally {
    await browser.close();
    await beenden();
  }
}

if (import.meta.url === `file://${process.argv[1]}`) {
  await cvErzeugen();
}
