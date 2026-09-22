/**
 * Legt eine neue Seite an:  npm run new-seite -- "Titel der Seite"
 * Ergebnis: inhalt/seiten/<name>.md, erreichbar unter /<name>/
 */
import { mkdir, readFile, writeFile, access } from 'node:fs/promises';
import { parseEinstellungen } from '../src/lib/einstellungen-schema.ts';
import { slug } from '../src/lib/text.ts';
import { setzeFeld } from './lib/frontmatter.mjs';

const VERGEBEN = new Set(['about', 'cv', 'projects', 'impressum', 'datenschutz', '404', 'robots', 'favicon', 'index']);

const titel = process.argv.slice(2).join(' ').trim();

if (titel === '') {
  console.error('So wird das Skript benutzt:\n  npm run new-seite -- "Titel der Seite"');
  process.exit(1);
}

const einstellungen = parseEinstellungen(await readFile('inhalt/einstellungen.yaml', 'utf8'));
const name = slug(titel);

if (name === '') {
  console.error(`Aus dem Titel "${titel}" laesst sich kein Dateiname bilden.`);
  process.exit(1);
}

if (VERGEBEN.has(name)) {
  console.error(`Die Adresse /${name}/ ist schon vergeben. Waehle einen anderen Titel.`);
  process.exit(1);
}

const datei = `inhalt/seiten/${name}.md`;

try {
  await access(datei);
  console.error(`${datei} gibt es schon. Waehle einen anderen Titel oder loesche die Datei.`);
  process.exit(1);
} catch {
  // Datei gibt es noch nicht, alles gut.
}

const vorlage = await readFile(`vorlagen/seite.${einstellungen.sprache}.md`, 'utf8');
const text = setzeFeld(vorlage, 'titel', JSON.stringify(titel));

await mkdir('inhalt/seiten', { recursive: true });
await writeFile(datei, text);

console.log(`Angelegt: ${datei}`);
console.log(`Adresse:  /${name}/`);
console.log('');
console.log('Naechste Schritte:');
console.log(`  1. ${datei} oeffnen und die [TODO]-Stellen ersetzen.`);
console.log('  2. Soll die Seite in die Kopfzeile, navigation auf true setzen.');
console.log('  3. Wenn sie fertig ist, entwurf auf false setzen.');
