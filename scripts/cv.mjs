/**
 * Erzeugt public/cv.pdf.
 * Liegt inhalt/lebenslauf.pdf vor, wird dieses genommen. Sonst wird die Seite
 * /cv/ mit Playwright als A4-PDF gedruckt, damit es nur eine Quelle gibt.
 */
import { access, copyFile, mkdir, writeFile } from 'node:fs/promises';
import { chromium } from 'playwright';
import { serverBereitstellen } from './lib/server.mjs';

const EIGENES = 'inhalt/lebenslauf.pdf';
const ZIELE = ['public/cv.pdf', 'dist/cv.pdf'];

async function existiert(pfad) {
  try {
    await access(pfad);
    return true;
  } catch {
    return false;
  }
}

export async function cvErzeugen({ leise = false, basis: vorhandeneBasis } = {}) {
  await mkdir('public', { recursive: true });

  if (await existiert(EIGENES)) {
    await copyFile(EIGENES, ZIELE[0]);
    if (await existiert('dist')) await copyFile(EIGENES, ZIELE[1]);
    if (!leise) console.log(`${EIGENES} wurde nach public/cv.pdf kopiert.`);
    return;
  }

  const { basis, beenden } = vorhandeneBasis
    ? { basis: vorhandeneBasis, beenden: async () => {} }
    : await serverBereitstellen();
  const browser = await chromium.launch();

  try {
    const seite = await browser.newPage();
    await seite.goto(basis + '/cv/', { waitUntil: 'networkidle' });
    await seite.emulateMedia({ media: 'print' });
    await seite.evaluate(() => document.fonts.ready);

    const pdf = await seite.pdf({
      format: 'A4',
      printBackground: true,
      preferCSSPageSize: true,
    });

    await writeFile(ZIELE[0], pdf);
    if (await existiert('dist')) await writeFile(ZIELE[1], pdf);
    if (!leise) {
      console.log('public/cv.pdf wurde aus der Seite /cv/ erzeugt.');
      console.log('Eigenes PDF bevorzugt? Dann lege es als inhalt/lebenslauf.pdf ab.');
    }
  } finally {
    await browser.close();
    await beenden();
  }
}

if (import.meta.url === `file://${process.argv[1]}`) {
  await cvErzeugen();
}
