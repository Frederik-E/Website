/**
 * Legt ein neues Projekt an:  npm run new -- "Titel des Projekts"
 */
import { mkdir, readFile, writeFile, access } from 'node:fs/promises';
import { parseEinstellungen } from '../src/lib/einstellungen-schema.ts';
import { slug } from '../src/lib/text.ts';
import { setzeFeld } from './lib/frontmatter.mjs';

const titel = process.argv.slice(2).join(' ').trim();

if (titel === '') {
  console.error('So wird das Skript benutzt:\n  npm run new -- "Titel des Projekts"');
  process.exit(1);
}

const einstellungen = parseEinstellungen(await readFile('inhalt/einstellungen.yaml', 'utf8'));
const ordnername = slug(titel);

if (ordnername === '') {
  console.error(`Aus dem Titel "${titel}" laesst sich kein Ordnername bilden.`);
  process.exit(1);
}

const ordner = `inhalt/projekte/${ordnername}`;

try {
  await access(ordner);
  console.error(`Der Ordner ${ordner} gibt es schon. Waehle einen anderen Titel oder loesche ihn.`);
  process.exit(1);
} catch {
  // Ordner gibt es noch nicht, alles gut.
}

const vorlage = await readFile(`vorlagen/projekt.${einstellungen.sprache}.md`, 'utf8');
const jetzt = new Date();
const monat = `${jetzt.getFullYear()}-${String(jetzt.getMonth() + 1).padStart(2, '0')}`;

let text = setzeFeld(vorlage, 'titel', JSON.stringify(titel));
text = setzeFeld(text, 'datum', `"${monat}"`);
text = setzeFeld(text, 'entwurf', 'true');

await mkdir(ordner, { recursive: true });
await writeFile(`${ordner}/index.md`, text);

console.log(`Angelegt: ${ordner}/index.md`);
console.log('');
console.log('Naechste Schritte:');
console.log(`  1. ${ordner}/index.md oeffnen und die [TODO]-Stellen ersetzen.`);
console.log(`  2. Bild ablegen:  ${ordner}/titelbild.jpg  und in den Kopfdaten eintragen.`);
console.log(`  3. Video hinzufuegen:  npm run video -- pfad/zum/video.mp4 ${ordnername}`);
console.log('  4. Wenn du keine Detailseite willst, alles unter den Kopfdaten loeschen.');
console.log('  5. Fertig? Dann in den Kopfdaten entwurf auf false setzen.');
