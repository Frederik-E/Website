/**
 * Erzeugt die Formen des Zeichens: src/styles/zeichen.css und src/lib/zeichen.ts
 *
 *   npm run zeichen
 *
 * Alle Formen bestehen aus derselben Zahl kubischer Segmente und derselben
 * Befehlsfolge. Nur dann kann der Browser zwischen ihnen ueberblenden.
 * Deshalb werden auch die Piktogramme aus einer weichen Kontur gebaut und
 * nicht als fertige Icons eingesetzt.
 *
 * Umschalten zwischen den beiden Fassungen: MIT_PIKTOGRAMMEN unten.
 */
import { writeFile } from 'node:fs/promises';

/**
 * true: der Blob wird zwischendurch zu Roboter, Chip und Zahnrad.
 * Ausprobiert und wieder verworfen, der Code bleibt zum Wiederanschalten.
 */
const MIT_PIKTOGRAMMEN = false;

const SEGMENTE = 28;
const KASTEN = 60;
const MITTE = KASTEN / 2;
const TAU = Math.PI * 2;
// Eine Nachkommastelle entspricht bei 280px Darstellung etwa einem halben
// Pixel. Zwei Stellen waeren nicht sichtbar, kosten aber rund ein Zehntel
// der Dateigroesse.
const rund = (n) => Math.round(n * 10) / 10;

// --- Blob -------------------------------------------------------------------

/*
  Wellen: k = Wellen rund um die Form, m = Umlaeufe pro Runde.
  Kleine k halten die Form rund, gegenlaeufige m verhindern, dass die
  Verformung bloss rotiert. Ganze Zahlen halten die Schleife nahtlos.
*/
const WELLEN = [
  { k: 1, m: -1, a: 0.07, p: 0.6 },
  { k: 2, m: 1, a: 0.11, p: 0.0 },
  { k: 3, m: -2, a: 0.06, p: 1.7 },
];
const BLOB_RADIUS = 18;

/**
 * Blob aus der exakten Ableitung des Radius nach dem Winkel. Geschaetzte
 * Tangenten, etwa ueber Catmull-Rom, erzeugen hier sichtbare Knicke.
 */
function blob(t) {
  const mx = MITTE + 2.2 * Math.cos(TAU * t + 0.4) + 1.1 * Math.cos(2 * TAU * t + 2.2);
  const my = MITTE + 2.2 * Math.sin(TAU * t + 1.1) + 1.1 * Math.sin(2 * TAU * t + 0.3);

  const radius = (w) => {
    let f = 1;
    for (const q of WELLEN) f += q.a * Math.sin(q.k * w + q.m * TAU * t + q.p);
    return BLOB_RADIUS * f;
  };
  const ableitung = (w) => {
    let f = 0;
    for (const q of WELLEN) f += q.a * q.k * Math.cos(q.k * w + q.m * TAU * t + q.p);
    return BLOB_RADIUS * f;
  };

  const punkt = (w) => [mx + radius(w) * Math.cos(w), my + radius(w) * Math.sin(w)];
  const tangente = (w) => {
    const r = radius(w), dr = ableitung(w);
    return [dr * Math.cos(w) - r * Math.sin(w), dr * Math.sin(w) + r * Math.cos(w)];
  };

  const dw = TAU / SEGMENTE;
  const griff = dw / 3;
  let d = '';
  for (let i = 0; i < SEGMENTE; i += 1) {
    const w1 = dw * i, w2 = dw * (i + 1);
    const p1 = punkt(w1), p2 = punkt(w2), t1 = tangente(w1), t2 = tangente(w2);
    if (i === 0) d = `M ${rund(p1[0])} ${rund(p1[1])}`;
    d += ` C ${rund(p1[0] + griff * t1[0])} ${rund(p1[1] + griff * t1[1])}` +
         ` ${rund(p2[0] - griff * t2[0])} ${rund(p2[1] - griff * t2[1])}` +
         ` ${rund(p2[0])} ${rund(p2[1])}`;
  }
  return d + ' Z';
}

// --- Piktogramme ------------------------------------------------------------

/** Verteilt N Punkte auf ein Vieleck: alle Ecken bleiben, der Rest fuellt die laengsten Kanten. */
function verdichten(ecken, anzahl) {
  if (ecken.length > anzahl) {
    throw new Error(
      `Ein Piktogramm hat ${ecken.length} Ecken, erlaubt sind hoechstens ${anzahl} (SEGMENTE). ` +
        'Entweder das Vieleck vereinfachen oder SEGMENTE erhoehen.',
    );
  }
  const punkte = ecken.map((p) => [...p]);
  while (punkte.length < anzahl) {
    let stelle = 0, laengste = -1;
    for (let i = 0; i < punkte.length; i += 1) {
      const a = punkte[i], b = punkte[(i + 1) % punkte.length];
      const laenge = Math.hypot(b[0] - a[0], b[1] - a[1]);
      if (laenge > laengste) { laengste = laenge; stelle = i; }
    }
    const a = punkte[stelle], b = punkte[(stelle + 1) % punkte.length];
    punkte.splice(stelle + 1, 0, [(a[0] + b[0]) / 2, (a[1] + b[1]) / 2]);
  }
  return punkte;
}

/** Weiche Kontur durch die Punkte. Die Spannung rundet die Ecken ab. */
function glaetten(punkte, spannung = 0.85) {
  const n = punkte.length;
  const hole = (i) => punkte[(i + n * 2) % n];
  let d = `M ${rund(punkte[0][0])} ${rund(punkte[0][1])}`;
  for (let i = 0; i < n; i += 1) {
    const p0 = hole(i - 1), p1 = hole(i), p2 = hole(i + 1), p3 = hole(i + 2);
    const f = spannung / 6;
    d += ` C ${rund(p1[0] + f * (p2[0] - p0[0]))} ${rund(p1[1] + f * (p2[1] - p0[1]))}` +
         ` ${rund(p2[0] - f * (p3[0] - p1[0]))} ${rund(p2[1] - f * (p3[1] - p1[1]))}` +
         ` ${rund(p2[0])} ${rund(p2[1])}`;
  }
  return d + ' Z';
}

/** Flaeche nach Gauss. Negativ heisst gegen den Uhrzeigersinn (y zeigt nach unten). */
function flaeche(punkte) {
  let summe = 0;
  for (let i = 0; i < punkte.length; i += 1) {
    const a = punkte[i], b = punkte[(i + 1) % punkte.length];
    summe += a[0] * b[1] - b[0] * a[1];
  }
  return summe / 2;
}

/**
 * Alle Formen müssen gleich herum laufen und moeglichst an derselben Stelle
 * beginnen, sonst dreht sich die Form beim Ueberblenden in sich und der
 * Uebergang wirkt unruhig. Deshalb: im Uhrzeigersinn, Start ganz rechts.
 */
function ausrichten(punkte) {
  const geordnet = flaeche(punkte) < 0 ? [...punkte].reverse() : punkte;
  let beste = 0, kleinster = Infinity;
  geordnet.forEach((p, i) => {
    const winkel = Math.abs(Math.atan2(p[1] - MITTE, p[0] - MITTE));
    if (winkel < kleinster) { kleinster = winkel; beste = i; }
  });
  return [...geordnet.slice(beste), ...geordnet.slice(0, beste)];
}

const piktogramm = (ecken, spannung) => glaetten(ausrichten(verdichten(ecken, SEGMENTE)), spannung);

/**
 * Zahnrad: acht Zaehne, im Uhrzeigersinn ab rechts.
 * Geringe Spannung, damit die Zahnkoepfe flach bleiben. Bei hoher Spannung
 * werden sie spitz und die Form liest sich als Stern.
 */
function zahnrad() {
  const aussen = 26, wurzel = 20, ecken = [];
  for (let i = 0; i < SEGMENTE; i += 1) {
    const r = i % 4 < 2 ? aussen : wurzel;
    const w = (i - 0.5) * (TAU / SEGMENTE);
    ecken.push([MITTE + r * Math.cos(w), MITTE + r * Math.sin(w)]);
  }
  return glaetten(ausrichten(ecken), 0.42);
}

/** Chip: quadratischer Koerper mit je drei Beinchen links und rechts. */
function chip() {
  const k = 11, g = 49, a = 55, i = 5; // Koerper, Beinchen aussen
  const beine = [[17, 21], [27, 31], [37, 41]];
  const ecken = [[g, k]];
  for (const [o, u] of beine) ecken.push([g, o], [a, o], [a, u], [g, u]);
  ecken.push([g, g], [k, g]);
  for (const [o, u] of [...beine].reverse()) ecken.push([k, u], [i, u], [i, o], [k, o]);
  ecken.push([k, k]);
  return piktogramm(ecken, 0.55);
}

/**
 * Antenne. Derzeit nicht in Gebrauch: von den vier Formen fügt sie sich am
 * schlechtesten in den Blob, weil sie aus zwei dünnen Stäben besteht und
 * damit weit von einer geschlossenen runden Form entfernt ist.
 * Zum Wiedereinsetzen in PIKTOGRAMME aufnehmen.
 */
function antenne() {
  const ecken = [
    [28, 41], [22, 32], [14, 20], [8, 12], [6, 9], [9, 7],
    [16, 17], [24, 29], [30, 36],
    [36, 29], [44, 17], [51, 7], [54, 9], [52, 12],
    [46, 20], [38, 32], [32, 41],
    [32, 47], [39, 47], [39, 55], [21, 55], [21, 47], [28, 47],
  ];
  return piktogramm(ecken, 0.5);
}

/** Roboter: Kopf mit zwei Fuehlern. */
function roboter() {
  const ecken = [
    [48, 26], [48, 42], [45, 48], [38, 51], [30, 52], [22, 51], [15, 48], [12, 42],
    [12, 26], [15, 20], [19, 18],
    [19, 12], [17, 9], [21, 6], [23, 9], [22, 17],
    [30, 16], [38, 17],
    [38, 12], [37, 9], [41, 6], [43, 9], [41, 18],
    [45, 20],
  ];
  return piktogramm(ecken, 0.6);
}

// --- Uebergaenge ------------------------------------------------------------

/**
 * Mischt zwei Formen. Beide haben dieselbe Befehlsfolge, deshalb lassen sich
 * die Zahlen paarweise verrechnen.
 *
 * "skala" bläht die Zwischenform leicht auf. Ohne das laufen alle Punkte auf
 * geraden Linien zum Ziel und der Uebergang wirkt mechanisch. Mit dem kleinen
 * Bauch quillt die Form nach aussen, wie Tinte, die sich neu sammelt.
 */
function mische(a, b, t, skala = 1) {
  const zahlenA = a.match(/-?\d+(\.\d+)?/g).map(Number);
  const zahlenB = b.match(/-?\d+(\.\d+)?/g).map(Number);
  const gemischt = zahlenA.map((wert, i) => {
    const roh = wert + (zahlenB[i] - wert) * t;
    return MITTE + (roh - MITTE) * skala;
  });

  let stelle = 0;
  const naechste = () => rund(gemischt[stelle++]);
  let d = `M ${naechste()} ${naechste()}`;
  for (let i = 0; i < SEGMENTE; i += 1) {
    d += ` C ${naechste()} ${naechste()} ${naechste()} ${naechste()} ${naechste()} ${naechste()}`;
  }
  return d + ' Z';
}

// --- Zeitablauf -------------------------------------------------------------

const BLOB_SCHRITTE = 14;
const blobs = [];
for (let i = 0; i < BLOB_SCHRITTE; i += 1) blobs.push(blob(i / BLOB_SCHRITTE));

/** Dauer einer Runde. Mit Piktogrammen deutlich länger, damit der Blob lange für sich zu sehen ist. */
const DAUER = MIT_PIKTOGRAMMEN ? '60s' : '20s';

const PIKTOGRAMME = [
  ['roboter', roboter()],
  ['chip', chip()],
  ['zahnrad', zahnrad()],
];

function ablaufOhnePiktogramme() {
  return blobs
    .map((d, i) => ({ anteil: i / BLOB_SCHRITTE, d, verlauf: 'linear' }))
    .concat([{ anteil: 1, d: blobs[0], verlauf: 'linear' }]);
}

/**
 * Je Piktogramm ein Viertel der Runde: aus dem Blob heraus formen, kurz
 * halten, wieder zerfliessen. Die Blobs dazwischen kommen aus derselben
 * Reihe, damit die Bewegung durchgehend organisch bleibt.
 */
/*
  Ablauf je Viertel der Runde. "p" ist die Stelle im Viertel, "zug" wie stark
  das Piktogramm die Form an sich zieht, "verlauf" die Beschleunigung.

  Über die Hälfte des Viertels ist der Blob für sich zu sehen. Erst danach
  zieht ihn das Piktogramm an, hält ihn kurz und lässt wieder los.
*/
const PHASEN = [
  { p: 0.0, zug: 0, verlauf: 'linear' },
  { p: 0.28, zug: 0, verlauf: 'linear' },
  { p: 0.56, zug: 0, verlauf: 'ease-in' },
  { p: 0.66, zug: 0.4, verlauf: 'linear' },
  { p: 0.74, zug: 0.85, verlauf: 'ease-out' },
  { p: 0.8, zug: 0.97, verlauf: 'linear' },
  { p: 0.89, zug: 0.97, verlauf: 'ease-in' },
  { p: 0.95, zug: 0.5, verlauf: 'linear' },
  { p: 0.99, zug: 0.15, verlauf: 'ease-out' },
];

/**
 * Die Blob-Bewegung läuft während der Verwandlung weiter, das Piktogramm zieht
 * die Form nur zunehmend zu sich. Dadurch wirkt der Übergang wie eine Folge
 * der Bewegung und nicht wie ein Schnitt zwischen zwei Bildern.
 *
 * Der Blob dreht genau eine Runde pro Schleife, deshalb bleibt sie nahtlos.
 */
function ablaufMitPiktogrammen() {
  const schritte = [];
  const teil = 1 / PIKTOGRAMME.length;

  PIKTOGRAMME.forEach(([, form], nr) => {
    for (const { p, zug, verlauf } of PHASEN) {
      const anteil = (nr + p) * teil;
      // Kleiner Bauch in der Mitte des Übergangs, damit die Punkte nicht
      // geradlinig zu ihrem Ziel laufen.
      const skala = 1 + 0.06 * Math.sin(Math.PI * zug);
      schritte.push({ anteil, d: mische(blob(anteil), form, zug, skala), verlauf });
    }
  });

  schritte.push({ anteil: 1, d: blob(0), verlauf: 'linear' });
  return schritte;
}

const ablauf = MIT_PIKTOGRAMMEN ? ablaufMitPiktogrammen() : ablaufOhnePiktogramme();

const zeilen = ablauf.map(({ anteil, d, verlauf }) => {
  const p = Math.round(anteil * 1000) / 10;
  return `  ${p}% {\n    animation-timing-function: ${verlauf};\n    d: path('${d}');\n  }`;
});

const beschreibung = MIT_PIKTOGRAMMEN
  ? `Der Blob formt sich nacheinander zu vier Piktogrammen und zerfliesst
  wieder: ${PIKTOGRAMME.map(([name]) => name).join(', ')}. Die Piktogramme sind aus
  derselben weichen Kontur gebaut wie der Blob, damit sie zur Form passen und
  nicht wie eingesetzte Icons wirken.`
  : `Der Blob verformt sich langsam und wandert dabei.`;

await writeFile(
  'src/styles/zeichen.css',
  `/*
  Die Formen des Zeichens. Erzeugt mit "npm run zeichen", nicht von Hand.

  ${beschreibung}

  Alle Zustaende haben dieselbe Befehlsfolge (${SEGMENTE} kubische Segmente,
  geschlossen). Nur dann kann der Browser zwischen ihnen ueberblenden.

  Der Zustand bei 100 % ist identisch mit dem bei 0 %, die Schleife hat
  deshalb keinen sichtbaren Uebergang.
*/

@keyframes blob {
${zeilen.join('\n')}
}
`,
);

await writeFile(
  'src/lib/zeichen.ts',
  `/**
 * Das Zeichen der Seite: eine organische Form, die langsam quillt und wandert.
 * ${MIT_PIKTOGRAMMEN ? 'Zwischendurch formt sie sich zu vier Piktogrammen und zerfliesst wieder.' : ''}
 *
 * Diese Datei und src/styles/zeichen.css werden erzeugt:
 *
 *   npm run zeichen
 *
 * Die Werte dafuer stehen oben in scripts/zeichen-erzeugen.mjs. Dort laesst
 * sich mit MIT_PIKTOGRAMMEN auch zwischen den beiden Fassungen umschalten.
 */

/** Ausgangsform, zugleich das unbewegte Bild fuer das Favicon. */
export const BLOB = '${ablauf[0].d}';

export const ZEICHEN_BREITE = ${KASTEN};
export const ZEICHEN_HOEHE = ${KASTEN};

/** Dauer einer Runde der Animation. */
export const ZEICHEN_DAUER = '${DAUER}';
`,
);

console.log(`Formen erzeugt: ${ablauf.length} Zustaende, ${SEGMENTE} Segmente, Piktogramme: ${MIT_PIKTOGRAMMEN ? 'ja' : 'nein'}`);
