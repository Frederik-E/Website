/**
 * Schneidet den einfarbigen Rand eines Bildes ab und legt einen schmalen Rand
 * wieder an. Gedacht fuer technische Zeichnungen, die viel Weissraum
 * mitbringen: in kleinen Vorschauen bleibt sonst kaum Zeichnung uebrig.
 *
 *   npm run bild -- inhalt/projekte/<ordner>/titelbild.png
 *
 * Das Bild wird an Ort und Stelle ersetzt. Die Originaldatei ist danach weg,
 * also vorher aufheben, wenn du sie noch brauchst.
 */
import { readFile, writeFile } from 'node:fs/promises';
import sharp from 'sharp';

const datei = process.argv[2];

if (!datei) {
  console.error('Bitte eine Datei angeben, zum Beispiel:');
  console.error('  npm run bild -- inhalt/projekte/mein-projekt/titelbild.png');
  process.exit(1);
}

// Anteil der Breite, der als Rand stehen bleibt.
const RAND = 0.02;

let roh;
try {
  roh = await readFile(datei);
} catch {
  console.error(`Die Datei ${datei} gibt es nicht.`);
  process.exit(1);
}

const vorher = await sharp(roh).metadata();

// Die Randfarbe steht im Pixel oben links.
const ecke = await sharp(roh).extract({ left: 0, top: 0, width: 1, height: 1 }).raw().toBuffer();
const hintergrund = { r: ecke[0], g: ecke[1], b: ecke[2], alpha: (ecke[3] ?? 255) / 255 };

const { info } = await sharp(roh).trim({ threshold: 5 }).toBuffer({ resolveWithObject: true });

if (info.width >= vorher.width - 2 && info.height >= vorher.height - 2) {
  console.log(`${datei} hat keinen einfarbigen Rand. Es bleibt unveraendert.`);
  process.exit(0);
}

if (info.width < vorher.width * 0.1 || info.height < vorher.height * 0.1) {
  console.error('Der Beschnitt wuerde fast alles wegnehmen. Bitte das Bild pruefen.');
  process.exit(1);
}

const rand = Math.round(info.width * RAND);
const ergebnis = await sharp(roh)
  .trim({ threshold: 5 })
  .extend({ top: rand, bottom: rand, left: rand, right: rand, background: hintergrund })
  .toBuffer();

await writeFile(datei, ergebnis);

const nachher = await sharp(ergebnis).metadata();
console.log(`${datei}`);
console.log(`  vorher:  ${vorher.width} x ${vorher.height} Pixel, ${Math.round(roh.length / 1024)} KB`);
console.log(`  nachher: ${nachher.width} x ${nachher.height} Pixel, ${Math.round(ergebnis.length / 1024)} KB`);
console.log(`  Rand: ${rand} Pixel in der Farbe des vorherigen Rands`);
