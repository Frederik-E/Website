// Zeichnet das Titelbild für inhalt/projekte/hackxplore-2026/:
//
//   node scripts/titelbild-baumsensor.mjs [ordner]
//
//   baumsensor.png  ein Stadtbaum mit Feuchtigkeitssensor im Boden. Der
//                   Sensor funkt nur zum Sensor am Nachbarbaum, die Kette
//                   endet an der Station an der Ampel. Von dort gehen die
//                   Daten zur IoT-Plattform: eine Karte mit dem Zustand der
//                   Bäume und der Route für die Bewässerung.
//
// Alles stammt aus dem Projekttext: Sensor an jedem Baum, Verbindung nur zum
// direkten Nachbarn, Stationen an Ampeln, Plattform mit Zustand und Routen.
// Die Farben der Punkte auf der Karte folgen dem Dashboard (grün gesund,
// orange bald prüfen, rot braucht Wasser).
//
// Das Bild entsteht zweimal: auf Deutsch und auf Englisch
// (baumsensor.en.png). Nur die Beschriftung ist übersetzt.
//
// Schrift: die Etikettenschrift der Seite (Barlow Semi Condensed) aus
// .astro/fonts; sie liegt dort nach einem Bau.

import { chromium } from 'playwright';
import { readdirSync, readFileSync } from 'node:fs';
import { join } from 'node:path';

const ordnerZiel = process.argv[2] ?? 'inhalt/projekte/hackxplore-2026';
const zielPng = (sprache) => join(ordnerZiel, sprache === 'de' ? 'baumsensor.png' : 'baumsensor.en.png');

// Beschriftung je Sprache
const TEXTE = {
  de: {
    sensor: 'Feuchtigkeitssensor',
    nachbar: 'Funk zum Nachbarn',
    station: 'Station an der Ampel',
    plattform: 'IoT-Plattform',
  },
  en: {
    sensor: 'Moisture sensor',
    nachbar: 'Link to the neighbour',
    station: 'Station at the traffic light',
    plattform: 'IoT platform',
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
  leise: '#b2b4ba',
  akzent: '#65afff', // Cool Sky
  schirm: '#b8d7ff',
  karte: '#f1f6ff',
  raster: '#dde8f8',
  rasterStark: '#c9daf2',
  rose: '#b98389', // Dusty Rose
  gruen: '#3f9f68', // wie im Titelbild der Kreislauffabrik
  krone: '#d3ead9',
  kroneSchatten: '#acd5b8',
  stamm: '#9c8b7c',
  erde: '#f6ebe0',
  erdeStrich: '#e6d3c1',
  wurzel: '#a08f80',
  orange: '#c9703a', // wie der Rand der Pylonen im Korridor-Bild
  rot: '#b2483a',
};

const strich = (b = 3, f = farbe.linie) => `stroke="${f}" stroke-width="${b}" stroke-linejoin="round" stroke-linecap="round"`;

// Pfeil mit Spitze bei (x2, y2)
function pfeil(x1, y1, x2, y2, breite, spitze, f = farbe.akzent) {
  const w = Math.atan2(y2 - y1, x2 - x1);
  const bx = x2 - Math.cos(w) * spitze;
  const by = y2 - Math.sin(w) * spitze;
  const nx = -Math.sin(w) * spitze * 0.6;
  const ny = Math.cos(w) * spitze * 0.6;
  return (
    `<line x1="${x1}" y1="${y1}" x2="${bx}" y2="${by}" stroke="${f}" stroke-width="${breite}" stroke-linecap="round"/>` +
    `<polygon points="${x2},${y2} ${bx + nx},${by + ny} ${bx - nx},${by - ny}" fill="${f}" stroke="${f}" stroke-width="2" stroke-linejoin="round"/>`
  );
}

// Boden ------------------------------------------------------------------------
// Gehweg als Platten, darunter die Erde im Schnitt mit Schraffur.
const boden = 610;
const platte = 14;

function erde() {
  const t = [];
  t.push(`<clipPath id="erde"><rect x="0" y="${boden}" width="${B}" height="${H - boden}"/></clipPath>`);
  t.push(`<rect x="0" y="${boden}" width="${B}" height="${H - boden}" fill="${farbe.erde}"/>`);
  t.push(`<g clip-path="url(#erde)">`);
  for (let x = -H; x < B; x += 26) {
    t.push(`<line x1="${x}" y1="${H}" x2="${x + (H - boden)}" y2="${boden}" stroke="${farbe.erdeStrich}" stroke-width="2"/>`);
  }
  t.push(`</g>`);
  return t.join('\n');
}

// Nach unten blendet der Schnitt in den Grund aus, statt als Block zu enden.
function ausblenden() {
  return (
    `<linearGradient id="unten" x1="0" y1="0" x2="0" y2="1">` +
    `<stop offset="0" stop-color="${farbe.grund}" stop-opacity="0"/><stop offset="1" stop-color="${farbe.grund}" stop-opacity="1"/>` +
    `</linearGradient>` +
    `<rect x="0" y="${H - 170}" width="${B}" height="170" fill="url(#unten)"/>`
  );
}

// Gehwegplatten mit Lücken für die Baumscheiben
function gehweg(baumscheiben) {
  const t = [];
  let x = 0;
  const kanten = [...baumscheiben].sort((a, b) => a[0] - b[0]);
  const stuecke = [];
  for (const [von, bis] of kanten) {
    stuecke.push([x, von]);
    x = bis;
  }
  stuecke.push([x, B]);
  for (const [von, bis] of stuecke) {
    if (bis - von < 4) continue;
    t.push(`<rect x="${von}" y="${boden}" width="${bis - von}" height="${platte}" fill="${farbe.blende}"/>`);
    for (let fx = von + 80; fx < bis - 10; fx += 80) {
      t.push(`<line x1="${fx}" y1="${boden}" x2="${fx}" y2="${boden + platte}" stroke="${farbe.leise}" stroke-width="2"/>`);
    }
  }
  t.push(`<line x1="0" y1="${boden}" x2="${B}" y2="${boden}" ${strich(3)}/>`);
  t.push(`<line x1="0" y1="${boden + platte}" x2="${B}" y2="${boden + platte}" stroke="${farbe.leise}" stroke-width="2"/>`);
  return t.join('\n');
}

// Baum -----------------------------------------------------------------------
// Krone aus überlappenden Kreisen: erst alle mit dickem Rand, dann alle ohne
// Rand darüber. So bleibt nur der äußere Umriss stehen. Innen eine dunklere
// Unterkante je Kreis, damit die Krone Volumen bekommt.
const GROESSE = 0.9;
const KRONE = [
  [-100, -250, 78],
  [100, -250, 80],
  [-58, -340, 84],
  [62, -344, 86],
  [0, -410, 78],
  [0, -262, 96],
].map(([dx, dy, r]) => [dx * GROESSE, dy * GROESSE, r * GROESSE]);
const STAMM = 260 * GROESSE;

let baumNummer = 0;
function baum(x) {
  const t = [];
  const id = `krone${baumNummer++}`;
  t.push(
    `<path d="M ${x - 21} ${boden} L ${x - 13} ${boden - STAMM} L ${x + 13} ${boden - STAMM} L ${x + 21} ${boden} Z" ` +
      `fill="${farbe.stamm}" ${strich(3)}/>`,
  );
  const kreise = KRONE.map(([dx, dy, r]) => [x + dx, boden + dy, r]);
  t.push(`<clipPath id="${id}">${kreise.map(([cx, cy, r]) => `<circle cx="${cx}" cy="${cy}" r="${r}"/>`).join('')}</clipPath>`);
  for (const [cx, cy, r] of kreise) {
    t.push(`<circle cx="${cx}" cy="${cy}" r="${r}" fill="none" stroke="${farbe.linie}" stroke-width="6"/>`);
  }
  t.push(`<g clip-path="url(#${id})">`);
  t.push(`<rect x="${x - 300}" y="${boden - 520}" width="600" height="520" fill="${farbe.kroneSchatten}"/>`);
  for (const [cx, cy, r] of kreise) {
    t.push(`<circle cx="${cx - 5}" cy="${cy - 12}" r="${r - 4}" fill="${farbe.krone}"/>`);
  }
  t.push(`</g>`);
  return t.join('\n');
}

function wurzeln(x) {
  const t = [];
  const w = (d, b) => t.push(`<path d="${d}" fill="none" stroke="${farbe.wurzel}" stroke-width="${b}" stroke-linecap="round"/>`);
  w(`M ${x - 12} ${boden + platte} C ${x - 26} ${boden + 50}, ${x - 60} ${boden + 66}, ${x - 104} ${boden + 104}`, 7);
  w(`M ${x - 66} ${boden + 70} C ${x - 80} ${boden + 92}, ${x - 80} ${boden + 120}, ${x - 92} ${boden + 146}`, 4);
  w(`M ${x + 12} ${boden + platte} C ${x + 26} ${boden + 56}, ${x + 56} ${boden + 74}, ${x + 92} ${boden + 120}`, 7);
  w(`M ${x - 2} ${boden + platte} C ${x - 6} ${boden + 80}, ${x + 8} ${boden + 130}, ${x - 8} ${boden + 186}`, 6);
  w(`M ${x - 4} ${boden + 110} C ${x - 24} ${boden + 132}, ${x - 40} ${boden + 150}, ${x - 46} ${boden + 180}`, 4);
  w(`M ${x + 44} ${boden + 70} C ${x + 62} ${boden + 66}, ${x + 80} ${boden + 74}, ${x + 104} ${boden + 66}`, 4);
  return t.join('\n');
}

// Sensor -----------------------------------------------------------------------
// Gehäuse mit Antenne über dem Boden, die Sonde mit zwei Zinken in der Erde.
function sensor(x) {
  const t = [];
  const tief = 150;
  // Sonde
  t.push(`<rect x="${x - 10}" y="${boden - 4}" width="20" height="${tief - 60}" rx="3" fill="${farbe.dunkel}" ${strich(3)}/>`);
  for (const dx of [-7, 7]) {
    t.push(`<line x1="${x + dx}" y1="${boden + tief - 66}" x2="${x + dx}" y2="${boden + tief}" ${strich(7)}/>`);
    t.push(`<line x1="${x + dx}" y1="${boden + tief - 66}" x2="${x + dx}" y2="${boden + tief}" ${strich(2.5, farbe.metall)}/>`);
  }
  // Gehäuse
  t.push(`<rect x="${x - 26}" y="${boden - 62}" width="52" height="62" rx="7" fill="${farbe.koerper}" ${strich(3)}/>`);
  t.push(`<line x1="${x - 26}" y1="${boden - 18}" x2="${x + 26}" y2="${boden - 18}" ${strich(2)}/>`);
  t.push(`<circle cx="${x - 9}" cy="${boden - 40}" r="6" fill="${farbe.akzent}" ${strich(2)}/>`);
  // Antenne
  t.push(`<line x1="${x + 14}" y1="${boden - 62}" x2="${x + 14}" y2="${boden - 96}" ${strich(3)}/>`);
  t.push(`<circle cx="${x + 14}" cy="${boden - 99}" r="5" fill="${farbe.linie}"/>`);
  return { svg: t.join('\n'), antenne: [x + 14, boden - 99], spitze: [x, boden + tief] };
}

// Feuchte rund um die Sonde: Messbereich und Tropfen
function feuchte([x, y]) {
  const t = [];
  t.push(
    `<circle cx="${x}" cy="${y - 30}" r="70" fill="${farbe.akzent}" fill-opacity="0.12" stroke="${farbe.akzent}" stroke-width="2.5" stroke-dasharray="8 7"/>`,
  );
  const tropfen = (cx, cy, g) =>
    `<path d="M ${cx} ${cy - 13 * g} C ${cx + 4 * g} ${cy - 6 * g}, ${cx + 8 * g} ${cy - 2 * g}, ${cx + 8 * g} ${cy + 3 * g} ` +
    `A ${8 * g} ${8 * g} 0 0 1 ${cx - 8 * g} ${cy + 3 * g} C ${cx - 8 * g} ${cy - 2 * g}, ${cx - 4 * g} ${cy - 6 * g}, ${cx} ${cy - 13 * g} Z" ` +
    `fill="${farbe.akzent}"/>`;
  t.push(tropfen(x - 42, y - 50, 1.1));
  t.push(tropfen(x + 40, y - 14, 1));
  t.push(tropfen(x - 30, y + 8, 0.85));
  t.push(tropfen(x + 30, y - 66, 0.8));
  return t.join('\n');
}

// Funkstrecke zwischen zwei Antennen: gestrichelter Bogen in Dusty Rose
function funk([x1, y1], [x2, y2], hoehe) {
  const mx = (x1 + x2) / 2;
  const my = Math.min(y1, y2) - hoehe;
  return `<path d="M ${x1} ${y1} Q ${mx} ${my} ${x2} ${y2}" fill="none" stroke="${farbe.rose}" stroke-width="5" stroke-dasharray="12 10" stroke-linecap="round"/>`;
}

// Funkwellen an einer Antenne, geöffnet in Richtung winkel (Grad)
function wellen([x, y], winkel, f = farbe.akzent) {
  const t = [];
  for (const r of [20, 36, 52]) {
    const a1 = ((winkel - 38) * Math.PI) / 180;
    const a2 = ((winkel + 38) * Math.PI) / 180;
    t.push(
      `<path d="M ${x + r * Math.cos(a1)} ${y + r * Math.sin(a1)} A ${r} ${r} 0 0 1 ${x + r * Math.cos(a2)} ${y + r * Math.sin(a2)}" ` +
        `fill="none" stroke="${f}" stroke-width="5" stroke-linecap="round"/>`,
    );
  }
  return t.join('');
}

// Ampel mit Station ------------------------------------------------------------
const ampel = { x: 1020, kopf: 170 };
const station = { x: ampel.x + 10, y: 440, b: 74, h: 78 };
const stationAntenne = [station.x + station.b - 18, station.y - 48];

function ampelMitStation() {
  const t = [];
  const { x, kopf } = ampel;
  t.push(`<rect x="${x - 10}" y="${kopf + 120}" width="20" height="${boden - kopf - 120}" fill="${farbe.dunkel}" ${strich(3)}/>`);
  // Signalkopf
  t.push(`<rect x="${x - 34}" y="${kopf}" width="68" height="150" rx="12" fill="${farbe.linie}"/>`);
  const lampen = [farbe.metall, farbe.metall, farbe.gruen];
  lampen.forEach((f, i) => {
    t.push(`<circle cx="${x}" cy="${kopf + 30 + i * 45}" r="16" fill="${f}" stroke="${farbe.dunkel}" stroke-width="3"/>`);
  });
  // Station am Mast, mit Schelle
  t.push(`<rect x="${x - 14}" y="${station.y + 14}" width="28" height="10" rx="3" fill="${farbe.metall}" ${strich(2)}/>`);
  t.push(`<rect x="${x - 14}" y="${station.y + station.h - 24}" width="28" height="10" rx="3" fill="${farbe.metall}" ${strich(2)}/>`);
  t.push(`<rect x="${station.x}" y="${station.y}" width="${station.b}" height="${station.h}" rx="8" fill="${farbe.schirm}" ${strich(3)}/>`);
  t.push(`<line x1="${station.x + 14}" y1="${station.y + 26}" x2="${station.x + station.b - 14}" y2="${station.y + 26}" ${strich(2.5)}/>`);
  t.push(`<line x1="${station.x + 14}" y1="${station.y + 40}" x2="${station.x + station.b - 30}" y2="${station.y + 40}" ${strich(2.5)}/>`);
  t.push(`<circle cx="${station.x + station.b - 18}" cy="${station.y + station.h - 18}" r="6" fill="${farbe.akzent}" ${strich(2)}/>`);
  // Antenne
  t.push(`<line x1="${stationAntenne[0]}" y1="${station.y}" x2="${stationAntenne[0]}" y2="${stationAntenne[1]}" ${strich(3)}/>`);
  t.push(`<circle cx="${stationAntenne[0]}" cy="${stationAntenne[1] - 3}" r="6" fill="${farbe.linie}"/>`);
  // Die Station hängt an der Leitung der Ampel: Kabel im Mast nach unten
  return t.join('\n');
}

// IoT-Plattform ------------------------------------------------------------------
// Ein Bildschirm mit Kopfzeile, drei Zählern und der Karte. Auf der Karte die
// Bäume als Punkte, die Route der Bewässerung führt zu den roten.
const tafel = { x: 1156, y: 40, b: 340, h: 272, rand: 14 };

function plattform() {
  const t = [];
  const { x, y, b, h, rand } = tafel;
  const i = { x: x + rand, y: y + rand, b: b - 2 * rand, h: h - 2 * rand };
  t.push(`<rect x="${x}" y="${y}" width="${b}" height="${h}" rx="14" fill="${farbe.linie}"/>`);
  t.push(`<clipPath id="tafel"><rect x="${i.x}" y="${i.y}" width="${i.b}" height="${i.h}" rx="3"/></clipPath>`);
  t.push(`<g clip-path="url(#tafel)">`);
  t.push(`<rect x="${i.x}" y="${i.y}" width="${i.b}" height="${i.h}" fill="#ffffff"/>`);
  // Zähler: farbiger Punkt und zwei graue Zeilen
  const zaehler = [farbe.rot, farbe.orange, farbe.gruen];
  const zb = (i.b - 4 * 10) / 3;
  zaehler.forEach((f, k) => {
    const zx = i.x + 10 + k * (zb + 10);
    t.push(`<rect x="${zx}" y="${i.y + 10}" width="${zb}" height="50" rx="5" fill="${farbe.koerper}"/>`);
    t.push(`<circle cx="${zx + 18}" cy="${i.y + 35}" r="8" fill="${f}"/>`);
    t.push(`<line x1="${zx + 36}" y1="${i.y + 28}" x2="${zx + zb - 14}" y2="${i.y + 28}" stroke="${farbe.leise}" stroke-width="5" stroke-linecap="round"/>`);
    t.push(`<line x1="${zx + 36}" y1="${i.y + 43}" x2="${zx + zb - 34}" y2="${i.y + 43}" stroke="${farbe.blende}" stroke-width="5" stroke-linecap="round"/>`);
  });
  // Karte
  const k = { x: i.x + 10, y: i.y + 70, b: i.b - 20, h: i.h - 80 };
  t.push(`<rect x="${k.x}" y="${k.y}" width="${k.b}" height="${k.h}" rx="5" fill="${farbe.karte}"/>`);
  t.push(`<clipPath id="karte"><rect x="${k.x}" y="${k.y}" width="${k.b}" height="${k.h}" rx="5"/></clipPath>`);
  t.push(`<g clip-path="url(#karte)">`);
  for (let rx = k.x + 8; rx < k.x + k.b; rx += 16) {
    t.push(`<line x1="${rx}" y1="${k.y}" x2="${rx}" y2="${k.y + k.h}" stroke="${farbe.raster}" stroke-width="1"/>`);
  }
  for (let ry = k.y + 8; ry < k.y + k.h; ry += 16) {
    t.push(`<line x1="${k.x}" y1="${ry}" x2="${k.x + k.b}" y2="${ry}" stroke="${farbe.raster}" stroke-width="1"/>`);
  }
  // Karte in Anteilen von Breite und Höhe
  const p = ([u, v]) => [k.x + u * k.b, k.y + v * k.h];
  const linie = (punkte) => 'M ' + punkte.map((q) => p(q).join(' ')).join(' L ');
  const kreuzung = { b: [0.3, 0.24], c: [0.77, 0.19], f: [0.28, 0.77], g: [0.79, 0.82] };
  const strassen = [
    [[-0.05, 0.27], kreuzung.b, kreuzung.c, [1.05, 0.17]],
    [[-0.05, 0.74], kreuzung.f, kreuzung.g, [1.05, 0.85]],
    [[0.31, -0.05], kreuzung.b, kreuzung.f, [0.27, 1.05]],
    [[0.76, -0.05], kreuzung.c, kreuzung.g, [0.8, 1.05]],
  ];
  for (const z of strassen) t.push(`<path d="${linie(z)}" fill="none" stroke="${farbe.blende}" stroke-width="14" stroke-linecap="round" stroke-linejoin="round"/>`);
  for (const z of strassen) t.push(`<path d="${linie(z)}" fill="none" stroke="#ffffff" stroke-width="8" stroke-linecap="round" stroke-linejoin="round"/>`);
  // Bäume auf der Karte
  const rot1 = [0.47, 0.11];
  const rot2 = [0.9, 0.5];
  const punkte = [
    [[0.1, 0.1], farbe.gruen],
    [rot1, farbe.rot],
    [[0.57, 0.42], farbe.gruen],
    [[0.93, 0.06], farbe.orange],
    [[0.13, 0.5], farbe.orange],
    [[0.47, 0.6], farbe.gruen],
    [rot2, farbe.rot],
    [[0.43, 0.93], farbe.gruen],
    [[0.93, 0.95], farbe.gruen],
  ];
  // Route der Bewässerung: vom Depot über die Straßen zu den beiden roten Bäumen
  const depot = [0.08, 0.755];
  t.push(
    `<path d="${linie([depot, kreuzung.f, kreuzung.b, [0.47, 0.222], rot1])} M ${p([0.47, 0.222]).join(' ')} ${linie([[0.47, 0.222], kreuzung.c, [0.783, 0.5], rot2]).slice(1)}" ` +
      `fill="none" stroke="${farbe.akzent}" stroke-width="5" stroke-linecap="round" stroke-linejoin="round"/>`,
  );
  const [dx, dy] = p(depot);
  t.push(`<rect x="${dx - 10}" y="${dy - 10}" width="20" height="20" rx="3" fill="#ffffff" ${strich(3)}/>`);
  for (const [q, f] of punkte) {
    const [px, py] = p(q);
    t.push(`<circle cx="${px}" cy="${py}" r="9" fill="${f}" stroke="#ffffff" stroke-width="3"/>`);
  }
  t.push(`</g></g>`);
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

// Baum 1 trägt den Sensor rechts, Baum 2 links. So liegt die Funkstrecke
// zwischen beiden frei unter den Kronen, und ihre Beschriftung passt darüber.
const BAUM1 = 230;
const BAUM2 = 770;
const SENSOR1 = BAUM1 + 112;
const SENSOR2 = BAUM2 - 112;

function szene(sprache) {
  const texte = TEXTE[sprache];
  const s1 = sensor(SENSOR1);
  const s2 = sensor(SENSOR2);
  const t = [];
  t.push(erde());
  t.push(wurzeln(BAUM1));
  t.push(wurzeln(BAUM2));
  t.push(feuchte(s1.spitze));
  t.push(ausblenden());
  t.push(gehweg([[BAUM1 - 64, BAUM1 + 64], [BAUM2 - 64, BAUM2 + 64]]));
  t.push(baum(BAUM1));
  t.push(baum(BAUM2));
  t.push(ampelMitStation());
  t.push(s1.svg);
  t.push(s2.svg);
  // Funk: vom Nachbarn links herein, zum Nachbarn rechts, weiter zur Station
  t.push(funk([-160, s1.antenne[1] + 10], s1.antenne, 50));
  t.push(funk(s1.antenne, s2.antenne, 34));
  // flach, damit der Bogen unter der Krone von Baum 2 bleibt
  t.push(funk(s2.antenne, stationAntenne, -80));
  // Station zur Plattform
  t.push(wellen(stationAntenne, -35));
  t.push(pfeil(stationAntenne[0] + 64, stationAntenne[1] - 44, tafel.x + 40, tafel.y + tafel.h + 14, 7, 22));
  t.push(plattform());
  // Beschriftung
  t.push(hinweis(SENSOR1 + 150, 836, 'start', s1.spitze[0] + 12, s1.spitze[1] - 30, texte.sensor));
  t.push(`<text x="${(SENSOR1 + SENSOR2) / 2}" y="${s1.antenne[1] - 30}" text-anchor="middle" class="name">${texte.nachbar}</text>`);
  t.push(hinweis(station.x + station.b + 44, station.y + 52, 'start', station.x + station.b, station.y + 52, texte.station));
  t.push(`<text x="${tafel.x + tafel.b}" y="${tafel.y + tafel.h + 48}" text-anchor="end" class="name">${texte.plattform}</text>`);
  return `<svg xmlns="http://www.w3.org/2000/svg" width="${B}" height="${H}" viewBox="0 0 ${B} ${H}">
<style>
  .name { font-family: Etikett; font-weight: 500; font-size: 32px; letter-spacing: 0.5px; fill: ${farbe.text}; }
</style>
<rect width="${B}" height="${H}" fill="${farbe.grund}"/>
${t.join('\n')}
</svg>`;
}

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
