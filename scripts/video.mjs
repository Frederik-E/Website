/**
 * Komprimiert ein Video fuer die Website:
 *   npm run video -- <datei> <projektordner>
 * Ergebnis: inhalt/projekte/<projektordner>/demo.mp4 (H.264, hoechstens 1280px,
 * 30 fps, ohne Ton). Fehlt ein Titelbild, wird der erste Frame genommen.
 */
import { access, readFile, rm, stat, writeFile } from 'node:fs/promises';
import { spawnSync } from 'node:child_process';
import { createInterface } from 'node:readline/promises';
import { readdirSync } from 'node:fs';
import { setzeFeld, hatFeld } from './lib/frontmatter.mjs';

const MB = 1024 * 1024;
const WARNUNG_AB = 10 * MB;
const ABBRUCH_AB = 25 * MB;

const [quelle, projektordner] = process.argv.slice(2);

if (!quelle || !projektordner) {
  console.error('So wird das Skript benutzt:');
  console.error('  npm run video -- pfad/zum/video.mp4 name-des-projektordners');
  process.exit(1);
}

function ffmpegVorhanden() {
  const ergebnis = spawnSync('ffmpeg', ['-version'], { stdio: 'ignore' });
  return ergebnis.status === 0;
}

if (!ffmpegVorhanden()) {
  console.error('ffmpeg ist nicht installiert. Es komprimiert das Video, damit die Seite schnell bleibt.');
  console.error('');
  console.error('So installierst du es:');
  console.error('  Windows:  winget install Gyan.FFmpeg      (danach Terminal neu oeffnen)');
  console.error('  macOS:    brew install ffmpeg');
  console.error('  Linux:    sudo apt install ffmpeg         (Ubuntu, Debian)');
  console.error('            sudo dnf install ffmpeg         (Fedora)');
  console.error('');
  console.error('Danach diesen Befehl noch einmal ausfuehren.');
  process.exit(1);
}

const ordner = `inhalt/projekte/${projektordner}`;
const kopfdatei = `${ordner}/index.md`;

try {
  await access(kopfdatei);
} catch {
  console.error(`Die Datei ${kopfdatei} gibt es nicht.`);
  console.error('Lege das Projekt zuerst an:  npm run new -- "Titel des Projekts"');
  process.exit(1);
}

try {
  await access(quelle);
} catch {
  console.error(`Die Datei ${quelle} gibt es nicht.`);
  process.exit(1);
}

const ziel = `${ordner}/demo.mp4`;

async function existiert(pfad) {
  try {
    await access(pfad);
    return true;
  } catch {
    return false;
  }
}

async function frage(text) {
  const leser = createInterface({ input: process.stdin, output: process.stdout });
  const antwort = await leser.question(text);
  leser.close();
  return antwort.trim().toLowerCase();
}

if (await existiert(ziel)) {
  const antwort = await frage(`${ziel} gibt es schon. Ueberschreiben? [j/N] `);
  if (antwort !== 'j' && antwort !== 'ja') {
    console.log('Abgebrochen. Es wurde nichts geaendert.');
    process.exit(0);
  }
}

console.log('Video wird komprimiert. Das dauert je nach Laenge eine Weile ...');

const ergebnis = spawnSync(
  'ffmpeg',
  [
    '-y',
    '-i', quelle,
    '-vf', "scale='min(1280,iw)':-2,fps=30",
    '-c:v', 'libx264',
    '-preset', 'slow',
    '-crf', '26',
    '-pix_fmt', 'yuv420p',
    '-movflags', '+faststart',
    '-an',
    ziel,
  ],
  { stdio: ['ignore', 'inherit', 'inherit'] },
);

if (ergebnis.status !== 0) {
  console.error('ffmpeg konnte das Video nicht umwandeln.');
  process.exit(1);
}

const groesse = (await stat(ziel)).size;
const inMb = (groesse / MB).toFixed(1);

if (groesse > ABBRUCH_AB) {
  await rm(ziel, { force: true });
  console.error(`Das Ergebnis ist ${inMb} MB gross. Cloudflare nimmt hoechstens 25 MB pro Datei.`);
  console.error('Kuerze das Video und versuche es noch einmal. Zehn Sekunden reichen meistens.');
  process.exit(1);
}

let kopf = await readFile(kopfdatei, 'utf8');
kopf = setzeFeld(kopf, 'video', 'demo.mp4');

// Fehlt ein Titelbild, wird der erste Frame genommen. Es dient auch als Posterbild.
const bilder = readdirSync(ordner).filter((datei) => /^titelbild\.(jpg|jpeg|png|webp)$/i.test(datei));

if (!hatFeld(kopf, 'titelbild') || bilder.length === 0) {
  const titelbild = `${ordner}/titelbild.jpg`;
  if (bilder.length === 0) {
    const frame = spawnSync(
      'ffmpeg',
      ['-y', '-i', ziel, '-frames:v', '1', '-q:v', '3', titelbild],
      { stdio: ['ignore', 'ignore', 'inherit'] },
    );
    if (frame.status !== 0) {
      console.error('Der erste Frame konnte nicht als Titelbild gespeichert werden.');
      process.exit(1);
    }
    console.log(`Titelbild aus dem ersten Frame: ${titelbild}`);
  }
  kopf = setzeFeld(kopf, 'titelbild', './titelbild.jpg');
  if (!hatFeld(kopf, 'titelbild_alt')) {
    kopf = setzeFeld(kopf, 'titelbild_alt', '"[TODO: Was ist auf dem Bild zu sehen?]"');
  }
}

await writeFile(kopfdatei, kopf);

console.log('');
console.log(`Fertig: ${ziel} (${inMb} MB)`);
console.log(`Eingetragen in ${kopfdatei}: video, titelbild, titelbild_alt`);

if (groesse > WARNUNG_AB) {
  console.log('');
  console.log(`Hinweis: ${inMb} MB ist gross fuer eine Website. Ein kuerzeres Video laedt schneller.`);
}

console.log('');
console.log('Naechster Schritt: titelbild_alt in den Kopfdaten ausfuellen, dann npm run dev.');
