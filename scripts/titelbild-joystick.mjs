// Zeichnet das Titelbild für inhalt/projekte/haptic-shared-control/:
//
//   node scripts/titelbild-joystick.mjs [ordner]
//
//   joystick.png  ein Proband von hinten, die rechte Hand am Joystick mit
//                 Kraftrückkopplung. Vor ihm der Bildschirm mit der simulierten
//                 Drohne zwischen Hindernissen. Ein Pfeil in Cool Sky zeigt die
//                 Kraft, mit der der Joystick die Hand vom Hindernis wegdrückt;
//                 derselbe Pfeil steht auf dem Bildschirm an der Drohne.
//
// Das Bild entsteht zweimal: auf Deutsch und auf Englisch (joystick.en.png).
// Nur die Beschriftung ist übersetzt.
//
// Schrift: die Etikettenschrift der Seite (Barlow Semi Condensed) aus
// .astro/fonts; sie liegt dort nach einem Bau.

import { chromium } from 'playwright';
import { readdirSync, readFileSync } from 'node:fs';
import { join } from 'node:path';

const ordnerZiel = process.argv[2] ?? 'inhalt/projekte/haptic-shared-control';
const zielPng = (sprache) => join(ordnerZiel, sprache === 'de' ? 'joystick.png' : 'joystick.en.png');

// Beschriftung je Sprache
const TEXTE = {
  de: {
    proband: 'Proband',
    simulation: 'Simulation der Drohne',
    kraft: 'Kraft auf die Hand',
    joystick: 'Joystick mit Kraftrückkopplung',
  },
  en: {
    proband: 'Participant',
    simulation: 'Drone simulation',
    kraft: 'Force on the hand',
    joystick: 'Joystick with force feedback',
  },
};

const B = 1520;
const H = 880;

const farbe = {
  grund: '#fcf7ff',
  linie: '#394648', // Iron Grey
  text: '#394648',
  koerper: '#ece9f0', // 8 % Iron Grey in Mist
  blende: '#d5d4da', // 20 % Iron Grey in Mist
  metall: '#8a9296',
  dunkel: '#4b5759',
  akzent: '#65afff', // Cool Sky
  karte: '#f1f6ff',
  raster: '#dde8f8',
  rasterStark: '#c9daf2',
  hindernis: '#d5d4da',
  rose: '#b98389', // Dusty Rose
  hemd: '#d9bcc0', // Dusty Rose, aufgehellt
  hemdSchatten: '#c7a3a8',
  haut: '#f1d7c6',
  haar: '#394648',
};

const strich = (b = 3) => `stroke="${farbe.linie}" stroke-width="${b}" stroke-linejoin="round" stroke-linecap="round"`;

// Pfeil mit Spitze bei (x2, y2)
function pfeil(x1, y1, x2, y2, breite, spitze) {
  const w = Math.atan2(y2 - y1, x2 - x1);
  const bx = x2 - Math.cos(w) * spitze;
  const by = y2 - Math.sin(w) * spitze;
  const nx = -Math.sin(w) * spitze * 0.6;
  const ny = Math.cos(w) * spitze * 0.6;
  return (
    `<line x1="${x1}" y1="${y1}" x2="${bx}" y2="${by}" stroke="${farbe.akzent}" stroke-width="${breite}" stroke-linecap="round"/>` +
    `<polygon points="${x2},${y2} ${bx + nx},${by + ny} ${bx - nx},${by - ny}" fill="${farbe.akzent}" stroke="${farbe.akzent}" stroke-width="2" stroke-linejoin="round"/>`
  );
}

// Tisch ----------------------------------------------------------------------
const tischHinten = 580;
const tischVorne = 652;
const tischKante = 26;

function tisch() {
  const t = [];
  for (const x of [150, 1340]) {
    t.push(`<rect x="${x}" y="${tischVorne + tischKante - 4}" width="30" height="${H}" fill="${farbe.blende}" ${strich()}/>`);
  }
  t.push(
    `<polygon points="150,${tischHinten} 1370,${tischHinten} 1400,${tischVorne} 120,${tischVorne}" fill="${farbe.koerper}" ${strich()}/>`,
  );
  t.push(`<rect x="120" y="${tischVorne}" width="1280" height="${tischKante}" fill="${farbe.blende}" ${strich()}/>`);
  return t.join('\n');
}

// Bildschirm -----------------------------------------------------------------
const mon = { x: 420, y: 14, b: 680, h: 400, rand: 16 };
const schirm = { x: mon.x + mon.rand, y: mon.y + mon.rand, b: mon.b - 2 * mon.rand, h: mon.h - 2 * mon.rand };

function drohne(cx, cy) {
  const t = [];
  const a = 15;
  for (const [dx, dy] of [[-1, -1], [1, -1], [1, 1], [-1, 1]]) {
    t.push(`<line x1="${cx}" y1="${cy}" x2="${cx + dx * a}" y2="${cy + dy * a}" stroke="${farbe.linie}" stroke-width="4"/>`);
  }
  for (const [dx, dy] of [[-1, -1], [1, -1], [1, 1], [-1, 1]]) {
    t.push(`<circle cx="${cx + dx * a}" cy="${cy + dy * a}" r="8.5" fill="#ffffff" ${strich(2.5)}/>`);
  }
  t.push(`<rect x="${cx - 7}" y="${cy - 7}" width="14" height="14" rx="3" fill="${farbe.linie}"/>`);
  return t.join('');
}

const hindernisse = [
  // Wand oben links
  [585, 178, 13], [611, 178, 13], [637, 178, 13], [663, 178, 13],
  // Wand rechts
  [972, 268, 13], [972, 294, 13], [972, 320, 13], [972, 346, 13],
  // einzelne
  [492, 150, 15], [528, 262, 14], [478, 330, 12], [566, 360, 13], [720, 116, 13],
  [706, 300, 16], [770, 182, 14], [757, 392, 13], [858, 262, 17], [846, 360, 13],
  [890, 128, 14], [930, 200, 12], [1020, 168, 15], [1040, 300, 13], [905, 420, 13],
  [1010, 410, 14], [668, 250, 12], [1064, 230, 12],
];

function bildschirm() {
  const t = [];
  // Ständer hinter dem Bildschirm
  const mx = mon.x + mon.b / 2;
  t.push(`<rect x="${mx - 22}" y="${mon.y + mon.h - 10}" width="44" height="${tischHinten + 26 - (mon.y + mon.h - 10)}" fill="${farbe.dunkel}" ${strich()}/>`);
  t.push(`<rect x="${mx - 90}" y="${tischHinten + 18}" width="180" height="16" rx="8" fill="${farbe.dunkel}" ${strich()}/>`);
  // Gehäuse und Fläche
  t.push(`<rect x="${mon.x}" y="${mon.y}" width="${mon.b}" height="${mon.h}" rx="14" fill="${farbe.linie}"/>`);
  t.push(`<clipPath id="schirm"><rect x="${schirm.x}" y="${schirm.y}" width="${schirm.b}" height="${schirm.h}" rx="3"/></clipPath>`);
  t.push(`<g clip-path="url(#schirm)">`);
  t.push(`<rect x="${schirm.x}" y="${schirm.y}" width="${schirm.b}" height="${schirm.h}" fill="${farbe.karte}"/>`);
  // Millimeterraster
  for (let i = 0, x = schirm.x + 12; x < schirm.x + schirm.b; i++, x += 16) {
    const stark = i % 5 === 0;
    t.push(`<line x1="${x}" y1="${schirm.y}" x2="${x}" y2="${schirm.y + schirm.h}" stroke="${stark ? farbe.rasterStark : farbe.raster}" stroke-width="${stark ? 1.5 : 1}"/>`);
  }
  for (let i = 0, y = schirm.y + 10; y < schirm.y + schirm.h; i++, y += 16) {
    const stark = i % 5 === 0;
    t.push(`<line x1="${schirm.x}" y1="${y}" x2="${schirm.x + schirm.b}" y2="${y}" stroke="${stark ? farbe.rasterStark : farbe.raster}" stroke-width="${stark ? 1.5 : 1}"/>`);
  }
  // Die Karte ist für einen Bildschirm ab y = 60 angelegt
  t.push(`<g transform="translate(0 ${mon.y - 60})">`);
  // Wirkbereich des Hindernisses neben der Drohne
  const [ox, oy] = [858, 262];
  t.push(`<circle cx="${ox}" cy="${oy}" r="62" fill="${farbe.akzent}" fill-opacity="0.08" stroke="${farbe.akzent}" stroke-width="2" stroke-dasharray="7 6"/>`);
  for (const [x, y, r] of hindernisse) {
    t.push(`<circle cx="${x}" cy="${y}" r="${r}" fill="${farbe.hindernis}" ${strich(2.5)}/>`);
  }
  // Start und Ziel
  t.push(`<rect x="458" y="412" width="24" height="24" fill="none" ${strich(3)}/>`);
  t.push(`<circle cx="1046" cy="112" r="20" fill="none" stroke="${farbe.rose}" stroke-width="4"/>`);
  t.push(`<circle cx="1046" cy="112" r="8" fill="${farbe.rose}"/>`);
  // Geflogener Weg bis zur Drohne
  t.push(
    `<path d="M 470 424 C 540 410, 600 398, 660 370 S 760 300, 800 264" fill="none" stroke="${farbe.akzent}" stroke-width="4" stroke-linecap="round"/>`,
  );
  t.push(drohne(800, 264));
  // Kraft auf die Drohne: weg vom Hindernis
  t.push(pfeil(776, 264, 728, 264, 6, 18));
  t.push(`</g></g>`);
  return t.join('\n');
}

// Joystick ---------------------------------------------------------------------
// Steht vorn rechts auf dem Tisch. Griff von gO (oben) bis gU (unten).
const js = { x: 932, boden: 644 };
const gO = js.boden - 204;
const gU = js.boden - 108;
// Der Proband lenkt nach rechts zum Hindernis, der Stick ist leicht geneigt.
const neigung = 7;
const dreh = { x: js.x, y: js.boden - 90 };
const gedreht = ([px, py]) => {
  const w = (neigung * Math.PI) / 180;
  const dx = px - dreh.x;
  const dy = py - dreh.y;
  return [dreh.x + dx * Math.cos(w) - dy * Math.sin(w), dreh.y + dx * Math.sin(w) + dy * Math.cos(w)];
};
const neigen = (inhalt) => `<g transform="rotate(${neigung} ${dreh.x} ${dreh.y})">${inhalt}</g>`;

function joystick() {
  const t = [];
  const { x, boden } = js;
  // Gehäuse: Vorderseite und Deckel
  t.push(`<rect x="${x - 72}" y="${boden - 44}" width="144" height="44" rx="4" fill="${farbe.linie}"/>`);
  t.push(`<polygon points="${x - 72},${boden - 44} ${x + 72},${boden - 44} ${x + 60},${boden - 64} ${x - 60},${boden - 64}" fill="${farbe.dunkel}" ${strich(3)}/>`);
  // Faltenbalg
  t.push(`<path d="M ${x - 34} ${boden - 54} L ${x - 16} ${boden - 90} H ${x + 16} L ${x + 34} ${boden - 54} Z" fill="${farbe.linie}"/>`);
  for (let i = 1; i < 4; i++) {
    const y = boden - 54 - i * 9;
    const w = 34 - i * 4.5;
    t.push(`<line x1="${x - w}" y1="${y}" x2="${x + w}" y2="${y}" stroke="${farbe.metall}" stroke-width="2"/>`);
  }
  const g = [];
  // Stange
  g.push(`<rect x="${x - 6}" y="${gU - 4}" width="12" height="${boden - 86 - gU + 4}" fill="${farbe.metall}" ${strich(2)}/>`);
  // Kreuzschalter steht oben aus dem Kopf
  g.push(`<rect x="${x - 9}" y="${gO - 11}" width="18" height="17" rx="4" fill="${farbe.metall}" ${strich(2.5)}/>`);
  // Griff: unten schmal, oben ein breiterer Kopf
  g.push(
    `<path d="M ${x - 19} ${gU} C ${x - 22} ${gU - 30}, ${x - 20} ${gO + 52}, ${x - 24} ${gO + 34} ` +
      `C ${x - 28} ${gO + 14}, ${x - 16} ${gO}, ${x} ${gO} C ${x + 16} ${gO}, ${x + 30} ${gO + 12}, ${x + 26} ${gO + 34} ` +
      `C ${x + 22} ${gO + 52}, ${x + 22} ${gU - 30}, ${x + 19} ${gU} Z" fill="${farbe.dunkel}" ${strich(3)}/>`,
  );
  // Tasten am Kopf
  g.push(`<circle cx="${x - 10}" cy="${gO + 18}" r="6" fill="${farbe.metall}" ${strich(2)}/>`);
  g.push(`<rect x="${x + 6}" y="${gO + 12}" width="11" height="13" rx="3" fill="${farbe.metall}" ${strich(2)}/>`);
  t.push(neigen(g.join('')));
  return t.join('\n');
}

// Proband ----------------------------------------------------------------------
const kopf = { x: 640, y: 442, rx: 62, ry: 74 };
const schulterY = 572;

function proband() {
  const t = [];
  const k = kopf;
  const sy = schulterY;

  // Rumpf
  t.push(
    `<path d="M ${k.x - 30} ${sy - 28} C ${k.x - 60} ${sy - 18}, ${k.x - 110} ${sy - 14}, ${k.x - 136} ${sy + 4} ` +
      `C ${k.x - 164} ${sy + 24}, ${k.x - 170} ${sy + 62}, ${k.x - 172} ${sy + 112} L ${k.x - 178} ${H + 10} H ${k.x + 178} ` +
      `L ${k.x + 172} ${sy + 112} C ${k.x + 170} ${sy + 62}, ${k.x + 164} ${sy + 24}, ${k.x + 136} ${sy + 4} ` +
      `C ${k.x + 110} ${sy - 14}, ${k.x + 60} ${sy - 18}, ${k.x + 30} ${sy - 28} Z" fill="${farbe.hemd}" ${strich()}/>`,
  );
  // Falte am Rücken
  t.push(`<path d="M ${k.x - 40} ${sy + 50} C ${k.x - 30} ${sy + 100}, ${k.x - 34} ${sy + 140}, ${k.x - 44} ${sy + 170}" fill="none" stroke="${farbe.hemdSchatten}" stroke-width="3" stroke-linecap="round"/>`);

  // Linker Arm hängt neben dem Rumpf
  t.push(
    `<path d="M ${k.x - 128} ${sy + 2} C ${k.x - 168} ${sy + 14}, ${k.x - 188} ${sy + 56}, ${k.x - 192} ${sy + 110} ` +
      `L ${k.x - 198} ${H + 10} H ${k.x - 140} L ${k.x - 138} ${sy + 80} Z" fill="${farbe.hemd}" ${strich()}/>`,
  );

  // Hals und Kragen
  t.push(`<rect x="${k.x - 30}" y="${k.y + 40}" width="60" height="${sy - k.y - 38}" rx="14" fill="${farbe.haut}" ${strich(2.5)}/>`);
  t.push(`<path d="M ${k.x - 40} ${sy - 24} C ${k.x - 20} ${sy - 4}, ${k.x + 20} ${sy - 4}, ${k.x + 40} ${sy - 24}" fill="none" ${strich(3)}/>`);
  // Ohren
  for (const s of [-1, 1]) {
    t.push(`<ellipse cx="${k.x + s * (k.rx - 2)}" cy="${k.y + 12}" rx="12" ry="20" fill="${farbe.haut}" ${strich(2.5)}/>`);
  }
  // Hinterkopf mit Haar
  t.push(
    `<path d="M ${k.x - k.rx} ${k.y} C ${k.x - k.rx} ${k.y - 54}, ${k.x - 34} ${k.y - k.ry}, ${k.x} ${k.y - k.ry} ` +
      `C ${k.x + 34} ${k.y - k.ry}, ${k.x + k.rx} ${k.y - 54}, ${k.x + k.rx} ${k.y} ` +
      `C ${k.x + k.rx} ${k.y + 34}, ${k.x + 46} ${k.y + 62}, ${k.x + 26} ${k.y + 68} ` +
      `C ${k.x + 12} ${k.y + 62}, ${k.x - 12} ${k.y + 62}, ${k.x - 26} ${k.y + 68} ` +
      `C ${k.x - 46} ${k.y + 62}, ${k.x - k.rx} ${k.y + 34}, ${k.x - k.rx} ${k.y} Z" fill="${farbe.haar}"/>`,
  );

  // Rechter Arm: Oberarm zum Ellbogen, Unterarm nach vorn zum Joystick
  const x = js.x;
  t.push(
    `<path d="M ${k.x + 128} ${sy + 2} C ${k.x + 172} ${sy + 12}, ${k.x + 206} ${sy + 46}, ${k.x + 224} ${sy + 88} ` +
      `L ${k.x + 238} ${sy + 122} C ${k.x + 246} ${sy + 148}, ${k.x + 220} ${sy + 166}, ${k.x + 198} ${sy + 156} ` +
      `C ${k.x + 176} ${sy + 146}, ${k.x + 164} ${sy + 124}, ${k.x + 158} ${sy + 96} L ${k.x + 140} ${sy + 50} Z" fill="${farbe.hemd}" ${strich()}/>`,
  );
  // Unterarm: vom Ellbogen schräg nach oben zum Handgelenk
  const hgL0 = [x - 30, gU + 16];
  const hgR0 = [x + 20, gU + 30];
  const hgL = gedreht(hgL0);
  const hgR = gedreht(hgR0);
  t.push(
    `<path d="M ${k.x + 196} ${sy + 110} L ${hgL[0]} ${hgL[1]} L ${hgR[0]} ${hgR[1]} L ${k.x + 244} ${sy + 132} ` +
      `C ${k.x + 232} ${sy + 160}, ${k.x + 196} ${sy + 160}, ${k.x + 190} ${sy + 132} Z" fill="${farbe.hemd}" ${strich()}/>`,
  );
  // Ärmelbund
  t.push(`<line x1="${hgL[0] + 4}" y1="${hgL[1] + 12}" x2="${hgR[0] - 4}" y2="${hgR[1] + 12}" ${strich(3)}/>`);

  // Rechte Hand um den Griff, mit dem Stick geneigt
  const hand = [];
  hand.push(
    `<path d="M ${hgL0[0] + 2} ${hgL0[1] + 2} C ${x - 36} ${gU - 20}, ${x - 34} ${gO + 66}, ${x - 26} ${gO + 52} ` +
      `C ${x - 18} ${gO + 40}, ${x + 6} ${gO + 40}, ${x + 22} ${gO + 46} C ${x + 36} ${gO + 52}, ${x + 40} ${gO + 72}, ${x + 38} ${gO + 88} ` +
      `C ${x + 36} ${gU - 4}, ${x + 30} ${gU + 14}, ${hgR0[0]} ${hgR0[1] + 2} Z" fill="${farbe.haut}" ${strich(2.5)}/>`,
  );
  // Fingerglieder auf der rechten Seite
  for (const y of [gO + 60, gO + 76, gO + 92]) {
    hand.push(`<path d="M ${x + 28} ${y} C ${x + 36} ${y + 3}, ${x + 36} ${y + 10}, ${x + 30} ${y + 13}" fill="none" stroke="${farbe.linie}" stroke-width="2" stroke-linecap="round"/>`);
  }
  // Daumen liegt links am Kopf des Griffs
  hand.push(
    `<path d="M ${x - 26} ${gO + 58} C ${x - 30} ${gO + 44}, ${x - 26} ${gO + 32}, ${x - 16} ${gO + 30} ` +
      `C ${x - 8} ${gO + 30}, ${x - 6} ${gO + 38}, ${x - 10} ${gO + 46} L ${x - 14} ${gO + 58} Z" fill="${farbe.haut}" ${strich(2.5)}/>`,
  );
  t.push(neigen(hand.join('')));

  // Stuhllehne vor dem Rücken
  t.push(`<rect x="${k.x - 128}" y="${sy + 160}" width="256" height="${H}" rx="44" fill="${farbe.linie}"/>`);
  t.push(`<rect x="${k.x - 106}" y="${sy + 180}" width="212" height="${H}" rx="30" fill="none" stroke="${farbe.dunkel}" stroke-width="3"/>`);
  return t.join('\n');
}

// Beschriftung -----------------------------------------------------------------
function hinweis(tx, ty, anker, px, py, text) {
  return (
    `<line x1="${tx}" y1="${ty}" x2="${px}" y2="${py}" stroke="${farbe.linie}" stroke-width="2"/>` +
    `<circle cx="${px}" cy="${py}" r="5" fill="${farbe.linie}"/>` +
    `<text x="${anker === 'end' ? tx - 12 : tx + 12}" y="${ty + 10}" text-anchor="${anker}" class="name">${text}</text>`
  );
}

function beschriftung(texte) {
  const t = [];
  t.push(hinweis(300, 500, 'end', kopf.x - 150, schulterY + 30, texte.proband));
  t.push(hinweis(1150, 120, 'start', mon.x + mon.b, 120, texte.simulation));
  // Kraft des Joysticks auf die Hand: nach links, weg vom Hindernis
  const py = gO + 20;
  t.push(pfeil(1062, py, js.x + 52, py, 9, 26));
  t.push(`<text x="1078" y="${py + 10}" class="name">${texte.kraft}</text>`);
  t.push(hinweis(1062, gO + 92, 'start', js.x + 66, js.boden - 60, texte.joystick));
  return t.join('\n');
}

const szene = (sprache) => `<svg xmlns="http://www.w3.org/2000/svg" width="${B}" height="${H}" viewBox="0 0 ${B} ${H}">
<style>
  .name { font-family: Etikett; font-weight: 500; font-size: 32px; letter-spacing: 0.5px; fill: ${farbe.text}; }
</style>
<rect width="${B}" height="${H}" fill="${farbe.grund}"/>
${tisch()}
${bildschirm()}
${joystick()}
${proband()}
${beschriftung(TEXTE[sprache])}
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
for (const sprache of Object.keys(TEXTE)) {
  await seite.setContent(html(sprache));
  await seite.evaluate(() => document.fonts.ready);
  await seite.locator('svg').screenshot({ path: zielPng(sprache) });
  console.log(`${zielPng(sprache)}: ${B} × ${H} px`);
}
await browser.close();
