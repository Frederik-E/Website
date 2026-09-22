/**
 * Prueft die ganze Seite vor dem Veroeffentlichen und gibt alle Probleme
 * gesammelt aus, getrennt nach fehlenden Inhalten und technischen Fehlern.
 */
import { readdir, readFile, stat } from 'node:fs/promises';
import { spawnSync } from 'node:child_process';
import { chromium } from 'playwright';
import AxePaket from '@axe-core/playwright';
import { parseEinstellungen } from '../src/lib/einstellungen-schema.ts';
import { bauen, fremdeAnfrage, seitenFinden, serverBereitstellen } from './lib/server.mjs';
import { cvErzeugen } from './cv.mjs';

const AxeBuilder = AxePaket.default ?? AxePaket;

const GROSSES_BILD = 5 * 1024 * 1024;

const fehlendeInhalte = [];
const technischeFehler = [];
const warnungen = [];

const inhaltFehlt = (ort, meldung) => fehlendeInhalte.push({ ort, meldung });
const technischerFehler = (ort, meldung) => technischeFehler.push({ ort, meldung });
const warnung = (ort, meldung) => warnungen.push({ ort, meldung });

// 1. Einstellungen -----------------------------------------------------------
try {
  parseEinstellungen(await readFile('inhalt/einstellungen.yaml', 'utf8'), true);
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
for (const eintrag of await readdir('inhalt/seiten', { withFileTypes: true })) {
  if (eintrag.isFile() && eintrag.name.startsWith('beispiel-')) {
    warnung(
      `inhalt/seiten/${eintrag.name}`,
      'Beispielseite. Sie ist ein Entwurf und wird nicht veroeffentlicht. Du kannst sie loeschen.',
    );
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
  await cvErzeugen({ leise: true, basis });

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
