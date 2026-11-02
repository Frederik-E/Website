// Zeichnet die Bilder der Szene "Korridor" für inhalt/projekte/cone-corridor/:
//
//   node scripts/titelbild-korridor.mjs [ordner]
//
//   korridor.png         Titelbild, statisch (Playwright-Screenshot)
//   abb/17_animation.svg Abb. 04, dieselbe Szene als Animation (nur CSS)
//
// Beide Bilder entstehen zweimal: auf Deutsch und auf Englisch
// (korridor.en.png, abb/17_animation.en.svg). Nur die Legende ist übersetzt.
//
// Die Szene ist in Metern beschrieben: die Kartenlinie läuft waagerecht,
// der Pylonenkorridor zweigt im Winkel WINKEL nach oben rechts ab. Der Weg
// folgt der Linie, biegt auf einem Kreisbogen ab (Dubins) und fährt in der
// Mitte des Korridors bis hinter die letzte Pylone.
//
// Ablauf der Animation: Das Auto kommt von links auf der Kartenlinie. Die
// Trigger-Pylone erscheint, 0,5 s später zeichnet sich der Weg, das Auto
// folgt ihm, hält am Ende 1,5 s und alles beginnt von vorn. Bei
// prefers-reduced-motion steht das Endbild still.
//
// Schrift: die Etikettenschrift der Seite (Barlow Semi Condensed) aus
// .astro/fonts; sie liegt dort nach einem Bau. Im SVG ist sie eingebettet,
// weil ein Bild in <img> keine Schriften nachladen darf.

import { chromium } from 'playwright';
import { mkdirSync, readdirSync, readFileSync, writeFileSync } from 'node:fs';
import { join } from 'node:path';
import { autoPiktogramm } from './lib/auto-piktogramm.mjs';

const ordnerZiel = process.argv[2] ?? 'inhalt/projekte/cone-corridor';
// Dateinamen je Sprache: Deutsch ohne Zusatz, Englisch mit ".en".
const zielPng = (sprache) => join(ordnerZiel, sprache === 'de' ? 'korridor.png' : 'korridor.en.png');
const zielSvg = (sprache) =>
  join(ordnerZiel, 'abb', sprache === 'de' ? '17_animation.svg' : '17_animation.en.svg');

// Beschriftung der Legende
const TEXTE = {
  de: {
    linie: 'Kartenlinie (gegeben)',
    pylonen: 'Pylonen (gemessen)',
    weg: 'Weg durch den Korridor (berechnet)',
    fahrzeug: 'Fahrzeug',
    umriss: 'Fahrzeugumriss (berechnet)',
  },
  en: {
    linie: 'Map line (given)',
    pylonen: 'Cones (measured)',
    weg: 'Path through the corridor (computed)',
    fahrzeug: 'Vehicle',
    umriss: 'Vehicle outline (computed)',
  },
};

// Szene (Meter)
const WINKEL = 55; // Grad zwischen Kartenlinie und Korridor
const BREITE_KORRIDOR = 0.9; // Abstand der Pylonenreihen, Mitte zu Mitte
const PYLONEN = 6; // je Reihe
const ABSTAND_PYLONEN = 0.4;
const RADIUS_PYLONE = 0.1;
const TRIGGER_X = 3.55; // erste Pylone der rechten Reihe, nahe an der Linie
const TRIGGER_Y = 0.03;
const RADIUS_KURVE = 0.9; // Mindestradius des Übergangs
const NACHLAUF = 0.25; // Weg hinter der letzten Pylone
const AUTO_L = 0.4;
const AUTO_B = 0.3;

// Bild (Pixel)
const B = 1520;
const K = 250; // Pixel je Meter
const RAND_OBEN = 45;
const RAND_UNTEN = 45;
const LEGENDE = 0.55; // Maßstab der Legende

// Animation (Sekunden, Meter)
const TEMPO = 1.0; // Fahrgeschwindigkeit in der Szene, m/s
const BREMSWEG = 0.5; // gleichmäßig verzögert bis zum Halt
const AUTO_START = -0.1; // Abstand auf dem Weg; negativ heißt links außerhalb des Bilds
const TRIGGER_BEI = 1.6; // Trigger-Pylone erscheint, wenn das Auto hier ist (Abstand auf dem Weg)
const VERZOEGERUNG_WEG = 0.5; // von der Trigger-Pylone bis zum Weg
const DAUER_WEG = 0.8; // so lange zeichnet sich der Weg
const HALT = 1.5; // Stand am Ende
const AUSBLENDEN = 0.4; // danach blendet alles aus, dann beginnt es neu

const farbe = {
  grund: '#fcf7ff',
  weg: '#65afff',
  auto: '#a9d0fb',
  fahrzeug: '#394648',
  pylone: '#f7ddc6',
  pyloneRand: '#c9703a',
  linie: '#b2b4ba',
  text: '#60696d',
};

const rad = (WINKEL * Math.PI) / 180;
const d = [Math.cos(rad), Math.sin(rad)]; // Richtung des Korridors
const links = [-d[1], d[0]]; // links davon

const plus = (a, b, f = 1) => [a[0] + f * b[0], a[1] + f * b[1]];

// Pylonen: rechte Reihe beginnt an der Trigger-Pylone, linke Reihe gegenüber.
const trigger = [TRIGGER_X, TRIGGER_Y];
const reiheRechts = [];
const reiheLinks = [];
for (let i = 0; i < PYLONEN; i += 1) {
  const p = plus(trigger, d, i * ABSTAND_PYLONEN);
  reiheRechts.push(p);
  reiheLinks.push(plus(p, links, BREITE_KORRIDOR));
}

// Mittellinie des Korridors und ihr Schnitt mit der Kartenlinie (y = 0).
const mitte0 = plus(trigger, links, BREITE_KORRIDOR / 2);
const knick = plus(mitte0, d, -mitte0[1] / d[1]);

// Weg wie ein Dubins-Pfad: Gerade auf der Linie, Kreisbogen mit festem
// Radius um den Knick, Gerade auf der Mittellinie des Korridors.
const start = [-0.3, 0];
const tangente = RADIUS_KURVE * Math.tan(rad / 2);
const p0 = [knick[0] - tangente, 0];
const p3 = plus(knick, d, tangente);
const zentrum = [p0[0], RADIUS_KURVE];
const laengeKorridor = (PYLONEN - 1) * ABSTAND_PYLONEN;
const tAbstand = (p) => (p[0] - mitte0[0]) * d[0] + (p[1] - mitte0[1]) * d[1];
const ende = plus(mitte0, d, laengeKorridor + NACHLAUF);

const punkte = [];
for (let i = 0; i <= 40; i += 1) {
  const x = start[0] + ((p0[0] - start[0]) * i) / 40;
  punkte.push([x, 0]);
}
for (let i = 1; i <= 80; i += 1) {
  const phi = (rad * i) / 80;
  punkte.push([zentrum[0] + RADIUS_KURVE * Math.sin(phi), zentrum[1] - RADIUS_KURVE * Math.cos(phi)]);
}
const restKorridor = tAbstand(ende) - tAbstand(p3);
for (let i = 1; i <= 40; i += 1) {
  punkte.push(plus(p3, d, (restKorridor * i) / 40));
}

// Bogenlänge, damit die Fahrzeugumrisse in festen Abständen stehen.
const laengen = [0];
for (let i = 1; i < punkte.length; i += 1) {
  const [ax, ay] = punkte[i - 1];
  const [bx, by] = punkte[i];
  laengen.push(laengen[i - 1] + Math.hypot(bx - ax, by - ay));
}
const gesamt = laengen.at(-1);

// Lage an der Stelle s des Wegs. Vor dem Anfang (s < 0) geht es gerade
// auf der Kartenlinie weiter, so kann das Auto von links hereinfahren.
function anStelle(s) {
  if (s <= 0) return { p: [start[0] + s, 0], w: 0 };
  let i = laengen.findIndex((l) => l >= s);
  if (i < 0) i = laengen.length - 1;
  if (i === 0) i = 1;
  const f = (s - laengen[i - 1]) / (laengen[i] - laengen[i - 1]);
  const [ax, ay] = punkte[i - 1];
  const [bx, by] = punkte[i];
  return { p: [ax + f * (bx - ax), ay + f * (by - ay)], w: Math.atan2(by - ay, bx - ax) };
}

// Auf der Geraden stehen die Umrisse dicht, ab der Kurve gleichmäßig weiter
// auseinander, der letzte genau am Ende des Wegs.
const sKurve = laengen[40] - 0.05;
const stellen = [];
for (let s = sKurve; s > 0.1; s -= 0.48) stellen.unshift(s);
const sEnde = gesamt - AUTO_L / 2;
const schritte = Math.round((sEnde - sKurve) / 0.62);
for (let i = 1; i <= schritte; i += 1) stellen.push(sKurve + ((sEnde - sKurve) * i) / schritte);

// Umrechnung in Pixel. Die Höhe ergibt sich aus dem höchsten Punkt der
// Szene: Pylone oder Ecke eines Fahrzeugumrisses.
const ecken = stellen.flatMap((s) => {
  const { p, w } = anStelle(s);
  return [[1, 1], [1, -1], [-1, 1], [-1, -1]].map(([a, b]) => [
    p[0] + (a * AUTO_L * Math.cos(w)) / 2 - (b * AUTO_B * Math.sin(w)) / 2,
    p[1] + (a * AUTO_L * Math.sin(w)) / 2 + (b * AUTO_B * Math.cos(w)) / 2,
  ]);
});
const oben = Math.max(
  ...reiheLinks.map((p) => p[1] + RADIUS_PYLONE),
  ...ecken.map((p) => p[1]),
);
const H = Math.round((oben + AUTO_B / 2) * K + RAND_OBEN + RAND_UNTEN);
const yLinie = H - RAND_UNTEN - (AUTO_B / 2) * K;
const px = ([x, y]) => [x * K, yLinie - y * K];
const f1 = (n) => n.toFixed(1);
const lage = (s) => {
  const { p, w } = anStelle(s);
  const [x, y] = px(p);
  return { x, y, grad: (-w * 180) / Math.PI };
};

const wegPfad = punkte
  .map((p, i) => `${i ? 'L' : 'M'}${f1(px(p)[0])} ${f1(px(p)[1])}`)
  .join(' ');
const wegLaengePx = gesamt * K;

const umriss = autoPiktogramm({ laenge: AUTO_L, breite: AUTO_B, massstab: K, farbe: farbe.auto, strich: 4 });
const fahrzeug = autoPiktogramm({
  laenge: AUTO_L,
  breite: AUTO_B,
  massstab: K,
  farbe: farbe.fahrzeug,
  strich: 4,
  fuellung: farbe.grund,
});

const kreis = (p, klasse = '') => {
  const [x, y] = px(p);
  return `<circle${klasse ? ` class="${klasse}"` : ''} cx="${f1(x)}" cy="${f1(y)}" r="${f1(RADIUS_PYLONE * K)}"/>`;
};

// Legende in eigenen Einheiten, mit LEGENDE skaliert. Das Titelbild zeigt die
// berechneten Umrisse, die Animation nur das fahrende Auto.
const legende = (animiert, texte) =>
  [
    [texte.linie, `<line x1="0" x2="54" stroke="${farbe.linie}" stroke-width="7" stroke-dasharray="17 11"/>`],
    [texte.pylonen, `<circle cx="27" r="12" fill="${farbe.pylone}" stroke="${farbe.pyloneRand}" stroke-width="3.5"/>`],
    [texte.weg, `<line x1="0" x2="54" stroke="${farbe.weg}" stroke-width="9"/>`],
    animiert
      ? [texte.fahrzeug, `<g transform="translate(27 0)">${autoPiktogramm({ laenge: AUTO_L, breite: AUTO_B, massstab: 125, farbe: farbe.fahrzeug, strich: 3.5, fuellung: farbe.grund })}</g>`]
      : [texte.umriss, `<g transform="translate(27 0)">${autoPiktogramm({ laenge: AUTO_L, breite: AUTO_B, massstab: 125, farbe: farbe.auto, strich: 3.5 })}</g>`],
  ]
    .map(
      ([text, zeichen], i) =>
        `<g transform="translate(0 ${i * 42.5})">${zeichen}<text x="76" y="9">${text}</text></g>`,
    )
    .join('\n    ');

// ---------------------------------------------------------------------------
// Zeitplan der Animation

const fahrtKonstant = (sEnde - BREMSWEG - AUTO_START) / TEMPO;
const verzoegerung = (TEMPO * TEMPO) / (2 * BREMSWEG);
const fahrtBremsen = TEMPO / verzoegerung;
const fahrtEnde = fahrtKonstant + fahrtBremsen;
const TAKT = fahrtEnde + HALT + AUSBLENDEN;
const ausAb = TAKT - AUSBLENDEN;

const sZurZeit = (t) => {
  if (t <= fahrtKonstant) return AUTO_START + TEMPO * t;
  const tau = Math.min(t - fahrtKonstant, fahrtBremsen);
  return sEnde - BREMSWEG + TEMPO * tau - (verzoegerung * tau * tau) / 2;
};

const tTrigger = (TRIGGER_BEI - AUTO_START) / TEMPO;
const tWeg = tTrigger + VERZOEGERUNG_WEG;
// Der Weg wächst ab der Stelle, an der das Auto gerade ist; das Stück
// dahinter blendet nur ein.
const sPlan = Math.max(0, sZurZeit(tWeg));
const LpxPlan = sPlan * K;
const LpxVorne = wegLaengePx - LpxPlan;
const gross = (wegLaengePx * 3).toFixed(0);
const prozent = (t) => `${((t / TAKT) * 100).toFixed(3)}%`;
const posTransform = ({ x, y, grad }) =>
  `translate(${x.toFixed(1)}px, ${y.toFixed(1)}px) rotate(${grad.toFixed(2)}deg)`;

// Auto: Lage alle 0,1 s; dazwischen linear, das reicht auch für den Bogen.
const fahrtFrames = [];
for (let t = 0; t < fahrtEnde; t += 0.1) {
  fahrtFrames.push(`${prozent(t)} { transform: ${posTransform(lage(sZurZeit(t)))}; opacity: 1; }`);
}
const endLage = lage(sEnde);
fahrtFrames.push(`${prozent(fahrtEnde)}, ${prozent(ausAb)} { transform: ${posTransform(endLage)}; opacity: 1; }`);
fahrtFrames.push(`100% { transform: ${posTransform(endLage)}; opacity: 0; }`);

const stil = (schriftDaten) => `
  @font-face { font-family: Etikett; src: url(data:font/woff2;base64,${schriftDaten}) format('woff2'); }
  .fahrzeug { animation: fahrt ${TAKT.toFixed(3)}s linear infinite; }
  @keyframes fahrt {
    ${fahrtFrames.join('\n    ')}
  }
  .trigger { transform-box: fill-box; transform-origin: center; animation: trigger ${TAKT.toFixed(3)}s linear infinite; }
  @keyframes trigger {
    0%, ${prozent(tTrigger)} { opacity: 0; transform: scale(0.3); animation-timing-function: cubic-bezier(0.2, 0.7, 0.2, 1.3); }
    ${prozent(tTrigger + 0.35)}, ${prozent(ausAb)} { opacity: 1; transform: scale(1); }
    100% { opacity: 0; transform: scale(1); }
  }
  .weg-hinten { stroke-dasharray: ${LpxPlan.toFixed(1)} ${gross}; animation: weg-hinten ${TAKT.toFixed(3)}s linear infinite; }
  @keyframes weg-hinten {
    0%, ${prozent(tWeg)} { opacity: 0; }
    ${prozent(tWeg + 0.4)}, ${prozent(ausAb)} { opacity: 1; }
    100% { opacity: 0; }
  }
  .weg-vorne { stroke-dashoffset: ${(-LpxPlan).toFixed(1)}; stroke-dasharray: ${(LpxVorne + 10).toFixed(1)} ${gross}; animation: weg-vorne ${TAKT.toFixed(3)}s linear infinite; }
  @keyframes weg-vorne {
    0%, ${prozent(tWeg)} { stroke-dasharray: 0 ${gross}; opacity: 0; }
    ${prozent(tWeg + 0.01)} { opacity: 1; }
    ${prozent(tWeg + DAUER_WEG)}, ${prozent(ausAb)} { stroke-dasharray: ${(LpxVorne + 10).toFixed(1)} ${gross}; opacity: 1; }
    100% { stroke-dasharray: ${(LpxVorne + 10).toFixed(1)} ${gross}; opacity: 0; }
  }
  @media (prefers-reduced-motion: reduce) {
    * { animation: none !important; }
  }`;

// ---------------------------------------------------------------------------
// Bild zusammensetzen. Ohne Animation gilt überall der Endzustand.
// Die berechneten Umrisse stehen nur im Titelbild; in der Animation ist
// allein das fahrende Auto zu sehen (Wunsch des Betreibers).

function szene({ animiert, schriftDaten, sprache }) {
  const umrisse = animiert
    ? ''
    : stellen
        .map((s) => {
          const { x, y, grad } = lage(s);
          return `<g transform="translate(${f1(x)} ${f1(y)}) rotate(${grad.toFixed(2)})">${umriss}</g>`;
        })
        .join('\n    ');

  const reiheA = animiert ? reiheRechts.slice(1) : reiheRechts;
  const auto = animiert
    ? `<g class="fahrzeug" transform="translate(${f1(endLage.x)} ${f1(endLage.y)}) rotate(${endLage.grad.toFixed(2)})">${fahrzeug}</g>`
    : '';

  return `<svg xmlns="http://www.w3.org/2000/svg" width="${B}" height="${H}" viewBox="0 0 ${B} ${H}">
  ${animiert ? `<style>${stil(schriftDaten)}\n  </style>` : ''}
  <rect width="${B}" height="${H}" fill="${farbe.grund}"/>
  <g>
    ${umrisse}
  </g>
  ${(animiert ? ['weg-hinten', 'weg-vorne'] : [''])
    .map(
      (klasse) =>
        `<path${klasse ? ` class="${klasse}"` : ''} d="${wegPfad}" fill="none" stroke="${farbe.weg}" stroke-width="10" stroke-linecap="round" stroke-linejoin="round"/>`,
    )
    .join('\n  ')}
  <g fill="${farbe.pylone}" stroke="${farbe.pyloneRand}" stroke-width="4">
    ${reiheA.map((p) => kreis(p)).join('\n    ')}
    ${animiert ? kreis(reiheRechts[0], 'trigger') : ''}
  </g>
  <g fill="${farbe.pylone}" stroke="${farbe.pyloneRand}" stroke-width="4" stroke-dasharray="7 6">
    ${reiheLinks.map((p) => kreis(p)).join('\n    ')}
  </g>
  <line x1="0" y1="${f1(yLinie)}" x2="${B}" y2="${f1(yLinie)}" stroke="${farbe.linie}" stroke-width="8" stroke-dasharray="26 18"/>
  <g transform="translate(37 ${RAND_OBEN}) scale(${LEGENDE})" font-family="Etikett" font-size="26.5" fill="${farbe.text}" stroke="${farbe.text}" stroke-width="0.6">
    ${legende(animiert, TEXTE[sprache])}
  </g>
  ${auto}
</svg>
`;
}

const schriftOrdner = '.astro/fonts';
const schrift = readdirSync(schriftOrdner).find((n) => n.startsWith('font-etikett-500-normal-latin-'));
if (!schrift) {
  console.error('Schrift nicht gefunden. Erst "npm run build" ausführen, dann liegt sie in .astro/fonts.');
  process.exit(1);
}
const schriftDaten = readFileSync(join(schriftOrdner, schrift)).toString('base64');

mkdirSync(join(ordnerZiel, 'abb'), { recursive: true });
const browser = await chromium.launch();
const seite = await browser.newPage({ viewport: { width: B, height: H }, deviceScaleFactor: 1 });

for (const sprache of Object.keys(TEXTE)) {
  // Animiertes SVG
  writeFileSync(zielSvg(sprache), szene({ animiert: true, schriftDaten, sprache }));

  // Statisches PNG
  const html = `<!doctype html><html><head><style>
@font-face { font-family: Etikett; src: url(data:font/woff2;base64,${schriftDaten}) format('woff2'); }
html, body { margin: 0; background: ${farbe.grund}; }
svg { display: block; }
</style></head><body>${szene({ animiert: false, sprache })}</body></html>`;

  await seite.setContent(html);
  await seite.evaluate(() => document.fonts.ready);
  await seite.locator('svg').screenshot({ path: zielPng(sprache) });

  console.log(`${zielPng(sprache)}: ${B} × ${H} px, Korridor ${WINKEL}° zur Kartenlinie`);
  console.log(
    `${zielSvg(sprache)}: Takt ${TAKT.toFixed(2)} s (Trigger ${tTrigger.toFixed(2)} s, Weg ab ${tWeg.toFixed(2)} s, Halt ab ${fahrtEnde.toFixed(2)} s)`,
  );
}

await browser.close();
