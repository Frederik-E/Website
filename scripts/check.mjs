/**
 * Prueft die ganze Seite vor dem Veroeffentlichen und gibt alle Probleme
 * gesammelt aus, getrennt nach fehlenden Inhalten und technischen Fehlern.
 */
import { readdir, readFile, stat } from 'node:fs/promises';
import { spawnSync } from 'node:child_process';
import { chromium } from 'playwright';
import AxePaket from '@axe-core/playwright';
import { parse as parseYaml } from 'yaml';
import { parseEinstellungen } from '../src/lib/einstellungen-schema.ts';
import { teileAuf } from './lib/frontmatter.mjs';
import { bauen, fremdeAnfrage, seitenFinden, serverBereitstellen } from './lib/server.mjs';
import { cvErzeugen, eigenePdfs, PDF_ORDNER } from './cv.mjs';

const AxeBuilder = AxePaket.default ?? AxePaket;

const GROSSES_BILD = 5 * 1024 * 1024;

const fehlendeInhalte = [];
const technischeFehler = [];
const warnungen = [];

const inhaltFehlt = (ort, meldung) => fehlendeInhalte.push({ ort, meldung });
const technischerFehler = (ort, meldung) => technischeFehler.push({ ort, meldung });
const warnung = (ort, meldung) => warnungen.push({ ort, meldung });

// 1. Einstellungen -----------------------------------------------------------
let einstellungen;
try {
  einstellungen = parseEinstellungen(await readFile('inhalt/einstellungen.yaml', 'utf8'), true);
} catch (fehler) {
  for (const zeile of String(fehler.message).split('\n').slice(1)) {
    inhaltFehlt('inhalt/einstellungen.yaml', zeile.replace(/^\s*-\s*/, ''));
  }
}

// 2. Dateien in inhalt/ ------------------------------------------------------
async function dateienSammeln(ordner) {
  const gefunden = [];
  for (const eintrag of await readdir(ordner, { withFileTypes: true })) {
    const pfad = `${ordner}/${eintrag.name}`;
    if (eintrag.isDirectory()) gefunden.push(...(await dateienSammeln(pfad)));
    else gefunden.push(pfad);
  }
  return gefunden;
}

const inhaltsdateien = await dateienSammeln('inhalt');

for (const pfad of inhaltsdateien) {
  if (/\.heic$/i.test(pfad)) {
    warnung(pfad, 'HEIC kann der Browser nicht anzeigen. Speichere das Foto als JPG.');
  }
  if (/\.gif$/i.test(pfad)) {
    warnung(pfad, 'GIFs sind gross und ruckeln. Nimm ein kurzes MP4:  npm run video');
  }
  if (/\.(jpe?g|png|webp)$/i.test(pfad)) {
    const groesse = (await stat(pfad)).size;
    if (groesse > GROSSES_BILD) {
      warnung(pfad, `Das Bild ist ${(groesse / 1024 / 1024).toFixed(1)} MB gross. Unter 5 MB baut die Seite schneller.`);
    }
  }
}

// 3. Platzhalter in den Quelldateien ----------------------------------------
for (const pfad of inhaltsdateien) {
  // einstellungen.yaml wird oben schon Feld fuer Feld geprueft.
  if (!/\.(md|ya?ml)$/i.test(pfad) || pfad === 'inhalt/einstellungen.yaml') continue;
  const text = await readFile(pfad, 'utf8');
  const stellen = [];
  text.split('\n').forEach((zeile, nummer) => {
    if (zeile.includes('[TODO')) stellen.push(`Zeile ${nummer + 1}: ${zeile.trim().slice(0, 90)}`);
  });
  if (stellen.length > 0) {
    inhaltFehlt(pfad, stellen.slice(0, 6).join('\n') + (stellen.length > 6 ? `\n... und ${stellen.length - 6} weitere` : ''));
  }
}

// 4. Beispiele ---------------------------------------------------------------
for (const eintrag of await readdir('inhalt/projekte', { withFileTypes: true })) {
  if (eintrag.isDirectory() && eintrag.name.startsWith('beispiel-')) {
    inhaltFehlt(
      `inhalt/projekte/${eintrag.name}/`,
      'Das ist noch ein Beispielprojekt. Ersetze es durch ein eigenes oder loesche den Ordner.',
    );
  }
}

// Beispielseiten sind Entwuerfe und werden nie veroeffentlicht, deshalb nur ein Hinweis.
// Der Ordner kann fehlen, wenn es keine eigenen Seiten gibt.
for (const eintrag of await readdir('inhalt/seiten', { withFileTypes: true }).catch(() => [])) {
  if (eintrag.isFile() && eintrag.name.startsWith('beispiel-')) {
    warnung(
      `inhalt/seiten/${eintrag.name}`,
      'Beispielseite. Sie ist ein Entwurf und wird nicht veroeffentlicht. Du kannst sie loeschen.',
    );
  }
}

// Das Foto fuer Startseite und Lebenslauf.
const fotos = (await readdir('inhalt/foto').catch(() => [])).filter((name) => /\.(jpe?g|png|webp|avif)$/i.test(name));
if (fotos.length === 0) {
  inhaltFehlt('inhalt/foto/', 'Noch kein Foto. Leg ein Bild als JPG, PNG oder WebP hier ab, im Hochformat.');
} else if (fotos.length > 1) {
  warnung('inhalt/foto/', `Mehrere Bilder. Benutzt wird nur ${fotos.sort()[0]}.`);
}

// Eigene Lebenslauf-PDFs: mehrere je Sprache sind ein Fehler, ein deutsches
// ohne englisches nur ein Hinweis (auf /en/ wird dann eines aus der Seite gedruckt).
try {
  const pdfs = await eigenePdfs();
  if (pdfs.de && !pdfs.en) {
    warnung(
      `${PDF_ORDNER}/`,
      'Nur ein deutsches PDF. Auf der englischen Seite wird ein PDF aus /en/cv/ erzeugt. Lege die englische Fassung als <name>.en.pdf daneben.',
    );
  } else if (pdfs.en && !pdfs.de) {
    warnung(`${PDF_ORDNER}/`, 'Nur ein englisches PDF. Auf der deutschen Seite wird ein PDF aus /cv/ erzeugt.');
  }
} catch (fehler) {
  inhaltFehlt(`${PDF_ORDNER}/`, fehler.message);
}

// 4b. Englische Fassungen ----------------------------------------------------
// Fehlt eine Übersetzung, steht auf /en/ der deutsche Text. Das blockiert die
// Veröffentlichung nicht, wird aber gemeldet.
const ohneEnglisch = 'Keine englische Fassung. Auf der englischen Seite steht der deutsche Text.';

async function lesen(pfad) {
  return readFile(pfad, 'utf8').catch(() => undefined);
}

function kopfdaten(text) {
  try {
    const { kopf, rest } = teileAuf(text);
    return { daten: parseYaml(kopf) ?? {}, text: rest.trim() };
  } catch {
    return { daten: {}, text: '' };
  }
}

if (einstellungen) {
  if (einstellungen.untertitel_en === undefined) {
    warnung('inhalt/einstellungen.yaml', 'Feld "untertitel_en" fehlt. Auf der englischen Seite steht der deutsche Untertitel.');
  }
  if (einstellungen.einleitung !== '' && einstellungen.einleitung_en === undefined) {
    warnung('inhalt/einstellungen.yaml', 'Feld "einleitung_en" fehlt. Auf der englischen Startseite steht die deutsche Einleitung.');
  }
}

for (const eintrag of await readdir('inhalt/projekte', { withFileTypes: true })) {
  if (!eintrag.isDirectory()) continue;
  const ordner = `inhalt/projekte/${eintrag.name}`;
  const deutsch = await lesen(`${ordner}/index.md`);
  if (deutsch === undefined) continue;
  const de = kopfdaten(deutsch);
  if (de.daten.entwurf === true) continue;
  const englisch = await lesen(`${ordner}/index.en.md`);
  if (englisch === undefined) {
    warnung(`${ordner}/index.en.md`, ohneEnglisch);
    continue;
  }
  const en = kopfdaten(englisch);
  const fehlend = ['kurz', 'stichpunkte', 'rolle', 'tags', 'titelbild_alt'].filter(
    (feld) => de.daten[feld] !== undefined && en.daten[feld] === undefined,
  );
  if (fehlend.length > 0) {
    warnung(`${ordner}/index.en.md`, `Nicht übersetzt: ${fehlend.join(', ')}. Dort steht auf Englisch der deutsche Text.`);
  }
  if (de.text !== '' && en.text === '') {
    warnung(`${ordner}/index.en.md`, 'Der Text unter den Kopfdaten fehlt. Die englische Detailseite zeigt den deutschen Text.');
  }
}

for (const name of await readdir('inhalt/seiten').catch(() => [])) {
  if (!name.endsWith('.md') || name.endsWith('.en.md') || name.startsWith('beispiel-')) continue;
  const de = kopfdaten((await lesen(`inhalt/seiten/${name}`)) ?? '');
  if (de.daten.entwurf === true) continue;
  const englisch = `inhalt/seiten/${name.replace(/\.md$/, '.en.md')}`;
  if ((await lesen(englisch)) === undefined) warnung(englisch, ohneEnglisch);
}

for (const datei of ['inhalt/impressum.en.md', 'inhalt/datenschutz.en.md']) {
  if ((await lesen(datei)) === undefined) warnung(datei, ohneEnglisch);
}

// Der englische Lebenslauf muss denselben Aufbau haben: dieselben Abschnitte,
// Einträge und Zeitangaben in derselben Reihenfolge.
{
  const englisch = await lesen('inhalt/lebenslauf.en.yaml');
  if (englisch === undefined) {
    warnung('inhalt/lebenslauf.en.yaml', ohneEnglisch);
  } else {
    const monat = (wert) =>
      wert instanceof Date
        ? `${wert.getUTCFullYear()}-${String(wert.getUTCMonth() + 1).padStart(2, '0')}`
        : String(wert ?? '').trim().replace(/\s*-\s*/, '-');
    const aufbau = (text) => {
      try {
        return (parseYaml(text)?.abschnitte ?? []).map((abschnitt) => ({
          startseite: Boolean(abschnitt.startseite),
          eintraege: (abschnitt.eintraege ?? []).map((e) =>
            'bereich' in e ? `Liste mit ${(e.liste ?? []).length} Angaben` : `${monat(e.von)} bis ${monat(e.bis) || 'heute'}, ${(e.punkte ?? []).length} Punkte`,
          ),
        }));
      } catch {
        return undefined;
      }
    };
    const de = aufbau((await lesen('inhalt/lebenslauf.yaml')) ?? '');
    const en = aufbau(englisch);
    const unterschiede = [];
    if (de && en) {
      if (de.length !== en.length) unterschiede.push(`${de.length} Abschnitte auf Deutsch, ${en.length} auf Englisch.`);
      de.forEach((abschnitt, i) => {
        const gegenstueck = en[i];
        if (!gegenstueck) return;
        if (abschnitt.startseite !== gegenstueck.startseite) {
          unterschiede.push(`Abschnitt ${i + 1}: "startseite" ist verschieden.`);
        }
        const laenge = Math.max(abschnitt.eintraege.length, gegenstueck.eintraege.length);
        for (let j = 0; j < laenge; j++) {
          if (abschnitt.eintraege[j] !== gegenstueck.eintraege[j]) {
            unterschiede.push(
              `Abschnitt ${i + 1}, Eintrag ${j + 1}: Deutsch ${abschnitt.eintraege[j] ?? 'fehlt'}, Englisch ${gegenstueck.eintraege[j] ?? 'fehlt'}.`,
            );
          }
        }
      });
    }
    if (unterschiede.length > 0) {
      warnung(
        'inhalt/lebenslauf.en.yaml',
        ['Weicht im Aufbau von lebenslauf.yaml ab:', ...unterschiede.slice(0, 6)].join('\n'),
      );
    }
  }
}

// 5. Typen -------------------------------------------------------------------
console.log('Typen werden geprueft ...');
const typen = spawnSync('npx', ['astro', 'check'], { encoding: 'utf8' });
if (typen.status !== 0) {
  technischerFehler('astro check', (typen.stdout + typen.stderr).trim().split('\n').slice(-25).join('\n'));
}

// 6. Bauen -------------------------------------------------------------------
console.log('Seite wird gebaut ...');
try {
  await bauen();
} catch (fehler) {
  technischerFehler('astro build', String(fehler.message));
  bericht();
  process.exit(1);
}

// 7. Server, PDF und Seitenpruefung -----------------------------------------
const { basis, beenden } = await serverBereitstellen({ ohneBauen: true });
const browser = await chromium.launch();

try {
  console.log('Lebenslauf-PDF wird erzeugt ...');
  // Mehrere PDFs je Sprache sind oben schon gemeldet; hier nicht abbrechen.
  await cvErzeugen({ leise: true, basis }).catch((fehler) => technischerFehler('npm run cv', fehler.message));

  const seiten = await seitenFinden(basis);
  const fremde = new Map();
  const kontext = await browser.newContext({ viewport: { width: 1280, height: 900 } });

  kontext.on('request', (anfrage) => {
    const host = fremdeAnfrage(anfrage.url(), basis);
    if (host) fremde.set(host, anfrage.url());
  });

  const seite = await kontext.newPage();
  const interneZiele = new Map();
  const seitenMitPlatzhalter = [];
  const seitenMitBeispieldomain = [];

  for (const pfad of seiten) {
    console.log(`  ${pfad}`);
    const antwort = await seite.goto(basis + pfad, { waitUntil: 'networkidle' });

    if (!antwort || (!antwort.ok() && !pfad.startsWith('/404'))) {
      technischerFehler(pfad, `Die Seite antwortet mit Status ${antwort?.status() ?? 'unbekannt'}.`);
      continue;
    }

    const html = await seite.content();

    if (html.includes('[TODO')) seitenMitPlatzhalter.push(pfad);
    if (html.includes('example.com')) seitenMitBeispieldomain.push(pfad);

    for (const treffer of html.matchAll(/(?:href|src)="(\/[^"#?]*)"/g)) {
      if (!interneZiele.has(treffer[1])) interneZiele.set(treffer[1], pfad);
    }

    const axe = await new AxeBuilder({ page: seite })
      .withTags(['wcag2a', 'wcag2aa', 'wcag21a', 'wcag21aa'])
      .analyze();

    for (const verstoss of axe.violations) {
      technischerFehler(pfad, `axe: ${verstoss.id} (${verstoss.impact}) ${verstoss.help}`);
    }
  }

  if (seitenMitPlatzhalter.length > 0) {
    inhaltFehlt(
      'Veroeffentlichte Seiten mit Platzhaltern',
      `${seitenMitPlatzhalter.join(', ')}\nSolange dort [TODO steht, wird nicht veroeffentlicht.`,
    );
  }

  if (seitenMitBeispieldomain.length > 0) {
    inhaltFehlt(
      'Veroeffentlichte Seiten mit example.com',
      `${seitenMitBeispieldomain.join(', ')}\nTrage deine Domain in inhalt/einstellungen.yaml ein.`,
    );
  }

  console.log('Interne Links werden geprueft ...');
  for (const [ziel, herkunft] of interneZiele) {
    const antwort = await fetch(basis + ziel, { method: 'GET' });
    if (!antwort.ok) {
      technischerFehler(herkunft, `Der Link ${ziel} fuehrt ins Leere (Status ${antwort.status}).`);
    }
  }

  for (const [host, adresse] of fremde) {
    technischerFehler('Fremde Anfrage', `${host} wurde geladen (${adresse}). Alles muss von der eigenen Domain kommen.`);
  }

  await kontext.close();
} finally {
  await browser.close();
  await beenden();
}

bericht();

function abschnitt(titel, eintraege) {
  if (eintraege.length === 0) return;
  console.log('');
  console.log(titel);
  console.log('-'.repeat(titel.length));
  for (const { ort, meldung } of eintraege) {
    console.log(`  ${ort}`);
    for (const zeile of String(meldung).split('\n')) console.log(`    ${zeile}`);
  }
}

function bericht() {
  console.log('');
  console.log('='.repeat(60));

  abschnitt('Fehlende Inhalte', fehlendeInhalte);
  abschnitt('Technische Fehler', technischeFehler);
  abschnitt('Hinweise', warnungen);

  console.log('');
  if (fehlendeInhalte.length === 0 && technischeFehler.length === 0) {
    console.log('Alles in Ordnung. Die Seite kann veroeffentlicht werden:  npm run deploy');
  } else {
    console.log(
      `${fehlendeInhalte.length} fehlende Inhalte, ${technischeFehler.length} technische Fehler. ` +
        'Die Seite wird nicht veroeffentlicht, solange etwas davon offen ist.',
    );
    process.exitCode = 1;
  }
}
