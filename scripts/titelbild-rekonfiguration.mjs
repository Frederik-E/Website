// Zeichnet das Titelbild für inhalt/projekte/rl-kreislauffabrik/:
//
//   node scripts/titelbild-rekonfiguration.mjs [ordner]
//
//   rekonfiguration.png  eine Produktionsstation als Werkzeugmaschine, darunter
//                        die Prozesse, auf die sie umgerüstet werden kann. Der
//                        gewählte Prozess ist grün umrandet, der Weg dorthin grün.
//
// Die Prozesse stammen aus dem Projekttext: eine Station kann prüfen, aber
// auch Demontage- oder Montagearbeiten übernehmen.
//
// Das Bild entsteht zweimal: auf Deutsch und auf Englisch
// (rekonfiguration.en.png). Nur die Namen der Prozesse sind übersetzt.
//
// Schrift: die Etikettenschrift der Seite (Barlow Semi Condensed) aus
// .astro/fonts; sie liegt dort nach einem Bau.

import { chromium } from 'playwright';
import { readdirSync, readFileSync } from 'node:fs';
import { join } from 'node:path';

const ordnerZiel = process.argv[2] ?? 'inhalt/projekte/rl-kreislauffabrik';
const zielPng = (sprache) =>
  join(ordnerZiel, sprache === 'de' ? 'rekonfiguration.png' : 'rekonfiguration.en.png');

const PROZESSE = ['Demontage', 'Prüfung', 'Montage'];

// Beschriftung der Prozesse je Sprache
const NAMEN = {
  de: { Demontage: 'Demontage', Prüfung: 'Prüfung', Montage: 'Montage' },
  en: { Demontage: 'Disassembly', Prüfung: 'Inspection', Montage: 'Assembly' },
};
const GEWAEHLT = 1; // Index in PROZESSE

const B = 1520;
const H = 900;
const MITTE = B / 2;

const farbe = {
  grund: '#fcf7ff',
  linie: '#394648', // Iron Grey
  text: '#394648',
  koerper: '#ece9f0', // 8 % Iron Grey in Mist
  blende: '#d5d4da', // 20 % Iron Grey in Mist
  metall: '#8a9296',
  glas: '#dcebff',
  akzent: '#65afff', // Cool Sky
  schirm: '#b8d7ff',
  notaus: '#b98389', // Dusty Rose
  teil: '#f7ddc6', // wie die Pylonen im Korridor-Bild
  teilRand: '#c9703a',
  leise: '#b2b4ba',
  feld: '#f4f1f8',
  gruen: '#3f9f68',
  gruenFlaeche: '#e8f5ec',
};

// Maschine ------------------------------------------------------------------
// Gehäuse mittig über dem gewählten Weg, oben abgeschrägt; das Bedienpult
// hängt rechts am Gehäuse.
const gB = 440;
const gx = MITTE - gB / 2;
const oben = 64;
const unten = 440;
const fase = 28;

function maschine(werkzeug) {
  const t = [];
  // Spindelmotor über dem Gehäuse
  t.push(`<rect x="${MITTE - 84}" y="${oben - 34}" width="168" height="44" rx="6" fill="${farbe.blende}" stroke="${farbe.linie}" stroke-width="3"/>`);
  // Gehäuse mit abgeschrägten oberen Ecken
  t.push(
    `<path d="M ${gx} ${unten} V ${oben + fase} L ${gx + fase} ${oben} H ${gx + gB - fase} L ${gx + gB} ${oben + fase} V ${unten} Z" ` +
      `fill="${farbe.koerper}" stroke="${farbe.linie}" stroke-width="3" stroke-linejoin="round"/>`,
  );
  // Streifen in Cool Sky
  t.push(`<rect x="${gx + 1.5}" y="${oben + 32}" width="${gB - 3}" height="10" fill="${farbe.akzent}"/>`);
  // Tür mit Sichtfenster
  t.push(`<rect x="${gx + 20}" y="${oben + 56}" width="${gB - 40}" height="252" rx="6" fill="${farbe.koerper}" stroke="${farbe.linie}" stroke-width="2.5"/>`);
  const fx = gx + 38;
  const fy = oben + 74;
  const fB = gB - 76;
  const fH = 216;
  t.push(`<rect x="${fx}" y="${fy}" width="${fB}" height="${fH}" rx="4" fill="${farbe.glas}" stroke="${farbe.linie}" stroke-width="2"/>`);
  // Griff
  t.push(`<rect x="${gx + gB - 32}" y="${oben + 150}" width="8" height="70" rx="4" fill="${farbe.linie}"/>`);

  // Innenraum: Führungen, Tisch, Werkstück, Spindel mit Werkzeug
  for (const dx of [-74, 74]) {
    t.push(`<line x1="${MITTE + dx}" y1="${fy}" x2="${MITTE + dx}" y2="${fy + 120}" stroke="${farbe.metall}" stroke-width="5"/>`);
  }
  const tischY = fy + fH - 34;
  t.push(`<rect x="${fx + 30}" y="${tischY}" width="${fB - 60}" height="16" fill="${farbe.metall}" stroke="${farbe.linie}" stroke-width="2"/>`);
  for (let i = 1; i < 6; i++) {
    const x = fx + 30 + ((fB - 60) * i) / 6;
    t.push(`<line x1="${x}" y1="${tischY + 3}" x2="${x}" y2="${tischY + 13}" stroke="${farbe.linie}" stroke-width="2"/>`);
  }
  t.push(`<rect x="${fx + 30}" y="${tischY + 16}" width="${fB - 60}" height="18" fill="${farbe.blende}" stroke="${farbe.linie}" stroke-width="2"/>`);
  const wsB = 120;
  const wsH = 44;
  const wsY = tischY - wsH;
  t.push(`<rect x="${MITTE - wsB / 2}" y="${wsY}" width="${wsB}" height="${wsH}" fill="${farbe.teil}" stroke="${farbe.teilRand}" stroke-width="2.5"/>`);
  // Schlitten und Spindel
  t.push(`<rect x="${MITTE - 64}" y="${fy}" width="128" height="40" fill="${farbe.blende}" stroke="${farbe.linie}" stroke-width="2"/>`);
  const spB = 40;
  const spUnten = fy + 64;
  t.push(`<rect x="${MITTE - spB / 2}" y="${fy + 40}" width="${spB}" height="${spUnten - fy - 40}" fill="${farbe.metall}" stroke="${farbe.linie}" stroke-width="2"/>`);
  t.push(`<polygon points="${MITTE - spB / 2},${spUnten} ${MITTE + spB / 2},${spUnten} ${MITTE + 11},${spUnten + 18} ${MITTE - 11},${spUnten + 18}" fill="${farbe.blende}" stroke="${farbe.linie}" stroke-width="2"/>`);
  t.push(werkzeug(spUnten + 18, wsY));
  // Spiegelung auf der Scheibe
  t.push(`<line x1="${fx + 22}" y1="${fy + 70}" x2="${fx + 82}" y2="${fy + 10}" stroke="#ffffff" stroke-width="6" stroke-linecap="round" opacity="0.8"/>`);
  t.push(`<line x1="${fx + 22}" y1="${fy + 100}" x2="${fx + 112}" y2="${fy + 10}" stroke="#ffffff" stroke-width="3" stroke-linecap="round" opacity="0.8"/>`);

  // Späneschublade unter der Tür
  t.push(`<rect x="${gx + 30}" y="${oben + 324}" width="${gB - 60}" height="${unten - oben - 340}" rx="4" fill="${farbe.blende}" stroke="${farbe.linie}" stroke-width="2"/>`);
  t.push(`<rect x="${MITTE - 44}" y="${oben + 338}" width="88" height="9" rx="4.5" fill="${farbe.linie}"/>`);
  // Sockel und Füße
  t.push(`<rect x="${gx - 12}" y="${unten}" width="${gB + 24}" height="22" rx="3" fill="${farbe.linie}"/>`);
  t.push(`<rect x="${gx + 10}" y="${unten + 22}" width="34" height="10" fill="${farbe.linie}"/>`);
  t.push(`<rect x="${gx + gB - 44}" y="${unten + 22}" width="34" height="10" fill="${farbe.linie}"/>`);

  // Bedienpult rechts
  const px = gx + gB + 18;
  t.push(`<rect x="${gx + gB}" y="${oben + 110}" width="18" height="14" fill="${farbe.linie}"/>`);
  t.push(`<rect x="${px}" y="${oben + 70}" width="86" height="220" rx="8" fill="${farbe.koerper}" stroke="${farbe.linie}" stroke-width="3"/>`);
  t.push(`<rect x="${px + 12}" y="${oben + 84}" width="62" height="54" rx="3" fill="${farbe.schirm}" stroke="${farbe.linie}" stroke-width="2"/>`);
  for (let r = 0; r < 3; r++) {
    for (let s = 0; s < 3; s++) {
      t.push(`<circle cx="${px + 23 + s * 20}" cy="${oben + 160 + r * 20}" r="5.5" fill="${farbe.linie}"/>`);
    }
  }
  t.push(`<circle cx="${px + 43}" cy="${oben + 256}" r="12" fill="${farbe.notaus}" stroke="${farbe.linie}" stroke-width="2"/>`);
  return t.join('\n');
}

// Werkzeuge in der Spindel, passend zum gewählten Prozess
const werkzeuge = {
  // Messtaster: Schaft, Taststift, Kugel, die das Werkstück berührt
  Prüfung: (y0, wsY) =>
    `<rect x="${MITTE - 10}" y="${y0}" width="20" height="18" fill="${farbe.metall}" stroke="${farbe.linie}" stroke-width="2"/>` +
    `<line x1="${MITTE}" y1="${y0 + 18}" x2="${MITTE}" y2="${wsY - 9}" stroke="${farbe.linie}" stroke-width="3"/>` +
    `<circle cx="${MITTE}" cy="${wsY - 7}" r="7" fill="${farbe.akzent}" stroke="${farbe.linie}" stroke-width="2"/>`,
  // Schrauber: Schaft mit flacher Klinge
  Montage: (y0, wsY) =>
    `<rect x="${MITTE - 4}" y="${y0}" width="8" height="${wsY - y0 - 8}" fill="${farbe.linie}"/>` +
    `<polygon points="${MITTE - 7},${wsY - 8} ${MITTE + 7},${wsY - 8} ${MITTE + 3},${wsY} ${MITTE - 3},${wsY}" fill="${farbe.linie}"/>`,
  Demontage: (y0, wsY) =>
    `<rect x="${MITTE - 4}" y="${y0}" width="8" height="${wsY - y0 - 8}" fill="${farbe.linie}"/>` +
    `<polygon points="${MITTE - 7},${wsY - 8} ${MITTE + 7},${wsY - 8} ${MITTE + 3},${wsY} ${MITTE - 3},${wsY}" fill="${farbe.linie}"/>`,
};

// Piktogramme der Prozesse, um (cx, cy) ---------------------------------------
const teil = (x, y, b, h) =>
  `<rect x="${x}" y="${y}" width="${b}" height="${h}" fill="${farbe.teil}" stroke="${farbe.teilRand}" stroke-width="2.5"/>`;
const pfeil = (x1, x2, y) => {
  const r = Math.sign(x2 - x1);
  return (
    `<line x1="${x1}" y1="${y}" x2="${x2 - r * 6}" y2="${y}" stroke="${farbe.linie}" stroke-width="3" stroke-linecap="round"/>` +
    `<polygon points="${x2},${y} ${x2 - r * 12},${y - 7} ${x2 - r * 12},${y + 7}" fill="${farbe.linie}"/>`
  );
};
// Zwei Teile mit Zapfen und Nut; abstand 0 heißt zusammengesteckt.
const zweiTeile = (cx, cy, abstand) => {
  const h = 64;
  const b = 62;
  const y = cy - h / 2;
  const links = cx - abstand / 2 - b;
  const rechts = cx + abstand / 2;
  const zapfen = 16;
  const zy = cy - 10;
  return (
    // linkes Teil mit Zapfen
    `<path d="M ${links} ${y} H ${links + b} V ${zy} H ${links + b + zapfen} V ${zy + 20} H ${links + b} V ${y + h} H ${links} Z" fill="${farbe.teil}" stroke="${farbe.teilRand}" stroke-width="2.5" stroke-linejoin="round"/>` +
    // rechtes Teil mit Nut
    `<path d="M ${rechts} ${y} H ${rechts + b} V ${y + h} H ${rechts} V ${zy + 20} H ${rechts + zapfen} V ${zy} H ${rechts} Z" fill="${farbe.teil}" stroke="${farbe.teilRand}" stroke-width="2.5" stroke-linejoin="round"/>`
  );
};
const piktogramme = {
  Demontage: (cx, cy) => zweiTeile(cx, cy + 14, 44) + pfeil(cx - 18, cx - 78, cy - 40) + pfeil(cx + 18, cx + 78, cy - 40),
  Montage: (cx, cy) => zweiTeile(cx, cy + 14, 0) + pfeil(cx - 92, cx - 30, cy - 40) + pfeil(cx + 92, cx + 30, cy - 40),
  Prüfung: (cx, cy) =>
    teil(cx - 70, cy - 4, 96, 52) +
    `<circle cx="${cx + 16}" cy="${cy - 6}" r="36" fill="#ffffff" fill-opacity="0.55" stroke="${farbe.linie}" stroke-width="5"/>` +
    `<line x1="${cx + 42}" y1="${cy + 20}" x2="${cx + 72}" y2="${cy + 50}" stroke="${farbe.linie}" stroke-width="10" stroke-linecap="round"/>`,
};

// Auswahl darunter ------------------------------------------------------------
const busY = 540;
const feldOben = 590;
const feldB = 300;
const feldH = 250;
const feldAbstand = 450;

function auswahl(sprache) {
  const t = [];
  const mitten = PROZESSE.map((_, i) => MITTE + (i - (PROZESSE.length - 1) / 2) * feldAbstand);
  const gewaehltX = mitten[GEWAEHLT];
  const strich = `stroke="${farbe.leise}" stroke-width="3" stroke-dasharray="10 8"`;
  // Verteiler und Abzweige, gestrichelt
  t.push(`<line x1="${mitten[0]}" y1="${busY}" x2="${mitten.at(-1)}" y2="${busY}" ${strich}/>`);
  mitten.forEach((x, i) => {
    if (i !== GEWAEHLT) t.push(`<line x1="${x}" y1="${busY}" x2="${x}" y2="${feldOben}" ${strich}/>`);
  });
  // Gewählter Weg: von der Maschine über den Verteiler zum Feld
  const sockel = unten + 32;
  t.push(`<polyline points="${MITTE},${sockel} ${MITTE},${busY} ${gewaehltX},${busY} ${gewaehltX},${feldOben}" fill="none" stroke="${farbe.gruen}" stroke-width="5" stroke-linejoin="round"/>`);
  t.push(`<circle cx="${MITTE}" cy="${busY}" r="8" fill="${farbe.gruen}"/>`);

  PROZESSE.forEach((name, i) => {
    const x = mitten[i];
    const gewaehlt = i === GEWAEHLT;
    t.push(
      `<rect x="${x - feldB / 2}" y="${feldOben}" width="${feldB}" height="${feldH}" rx="14" ` +
        (gewaehlt
          ? `fill="${farbe.gruenFlaeche}" stroke="${farbe.gruen}" stroke-width="5"/>`
          : `fill="${farbe.feld}" stroke="${farbe.leise}" stroke-width="2.5"/>`),
    );
    t.push(piktogramme[name](x, feldOben + 100));
    t.push(`<text x="${x}" y="${feldOben + feldH - 36}" text-anchor="middle" class="name">${NAMEN[sprache][name]}</text>`);
  });
  return t.join('\n');
}

const szene = (sprache) => `<svg xmlns="http://www.w3.org/2000/svg" width="${B}" height="${H}" viewBox="0 0 ${B} ${H}">
<style>
  .name { font-family: Etikett; font-weight: 500; font-size: 38px; letter-spacing: 0.5px; fill: ${farbe.text}; }
</style>
<rect width="${B}" height="${H}" fill="${farbe.grund}"/>
${auswahl(sprache)}
${maschine(werkzeuge[PROZESSE[GEWAEHLT]])}
</svg>`;

// Schrift aus dem letzten Bau
const schriftOrdner = '.astro/fonts';
let schrift;
try {
  schrift = readdirSync(schriftOrdner).find((n) => n.startsWith('font-etikett-500-normal-latin-'));
} catch {
  schrift = undefined;
}
if (!schrift) {
  console.error('Schrift nicht gefunden. Erst "npm run build" ausführen, dann liegt sie in .astro/fonts.');
  process.exit(1);
}
const schriftDaten = readFileSync(join(schriftOrdner, schrift)).toString('base64');

const html = (sprache) => `<!doctype html><html><head><meta charset="utf-8"><style>
@font-face { font-family: Etikett; font-weight: 500; src: url(data:font/woff2;base64,${schriftDaten}) format('woff2'); }
html, body { margin: 0; background: ${farbe.grund}; }
svg { display: block; }
</style></head><body>${szene(sprache)}</body></html>`;

const browser = await chromium.launch();
const seite = await browser.newPage({ viewport: { width: B, height: H }, deviceScaleFactor: 1 });
for (const sprache of Object.keys(NAMEN)) {
  await seite.setContent(html(sprache));
  await seite.evaluate(() => document.fonts.ready);
  await seite.locator('svg').screenshot({ path: zielPng(sprache) });
  console.log(`${zielPng(sprache)}: ${B} × ${H} px, gewählt: ${NAMEN[sprache][PROZESSE[GEWAEHLT]]}`);
}
await browser.close();
