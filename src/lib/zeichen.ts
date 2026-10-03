/**
 * Das Zeichen der Seite: eine Punktmatrix. Von einem Punkt laufen Ringe nach
 * außen, abwechselnd in Cool Sky und Dusty Rose, wie Wellen um einen Tropfen.
 * Jeder Ring startet an einem anderen, zufällig gewählten Punkt der Matrix.
 * Alle 40 Sekunden zeichnen die Ringe die Initialen aus den Einstellungen:
 * hinter der Front eines Rings bleibt ein Buchstabe stehen, der nächste
 * verwandelt ihn in den folgenden, der letzte wischt ihn weg.
 *
 * Beim Bauen wird eine zufällige Folge von Startpunkten gezogen, bei jedem Bau
 * eine neue. Im Browser rechnet CSS: die Folge steht in Keyframes, jeder Punkt
 * berechnet mit hypot() seinen Abstand zum aktuellen Startpunkt und daraus, ob
 * der Ring ihn gerade trifft. Kein JavaScript.
 */

/** Sekunden je Ring. */
export const RING_TAKT = 2.5;

/** Mit diesem Ring (0-basiert) beginnen die Initialen: nach 7,5 Sekunden. */
export const INITIALEN_AB_RING = 3;

/** Danach alle so viele Ringe wieder (16 × 2,5 s = 40 s). */
export const INITIALEN_ALLE = 16;

/** Während die Initialen stehen, leuchten die übrigen Ringe gedämpft. */
export const DAEMPFUNG = 0.55;

export type Fassung = 'gross' | 'klein';

export interface Ring {
  spalte: number;
  zeile: number;
  /** 1: der Ring ist blau, 0: rosa. Die Farben wechseln sich ab. */
  blau: 0 | 1;
  /**
   * Welcher Buchstabe vor und hinter der Ringfront steht (1 = erster
   * Buchstabe, 0 = keiner). Wo die Front vorbeikommt, wechselt der Punkt vom
   * einen zum anderen.
   */
  vorher: number;
  nachher: number;
  /** 1: der Ring leuchtet gedämpft, weil Buchstaben stehen. */
  gedaempft: 0 | 1;
}

export interface Punkt {
  spalte: number;
  zeile: number;
  x: number;
  y: number;
  /** Nummern der Buchstaben, in denen dieser Punkt blau bzw. rosa leuchtet. */
  buchstabenBlau: number[];
  buchstabenRose: number[];
}

export interface Matrix {
  n: number;
  /** Kantenlänge der viewBox. */
  kante: number;
  schritt: number;
  rand: number;
  radiusPunkt: number;
  radiusWelle: number;
  /** Radius, bis zu dem ein Ring wächst, in Punktabständen. */
  weite: number;
  /** Länge der Spur hinter der Ringfront, in Punktabständen. */
  spur: number;
  punkte: Punkt[];
  folge: Ring[];
  /** Standbild: Startpunkt und Fortschritt des Rings (0 bis 1). */
  ruhe: { spalte: number; zeile: number; t: number };
}

/*
  gross: 9 × 9 Punkte für das Startbild, 32 Ringe (80 s), zweimal die Initialen.
  klein: 4 × 4 Punkte für die Kopfzeile, mit größeren Punkten und weniger Rand,
  damit sie bei 28 Pixeln noch lesbar sind. 12 Ringe, keine Initialen.

  weite: bis dahin wächst ein Ring. Etwas mehr als die längste Diagonale
  (11,3 Punktabstände), damit am Ende eines Rings jeder Punkt erreicht ist.
*/
const FASSUNGEN = {
  gross: {
    n: 9,
    rand: 1,
    punkt: 0.14,
    welle: 0.3,
    ringe: 32,
    weite: 12,
    spur: 1.6,
    ruhe: { spalte: 4, zeile: 4, radius: 2.2 },
  },
  klein: {
    n: 4,
    rand: 0.6,
    punkt: 0.2,
    welle: 0.36,
    ringe: 12,
    weite: 4.4,
    spur: 1.1,
    ruhe: { spalte: 1, zeile: 1, radius: 1.2 },
  },
} as const;

/*
  Pixelschrift für die Initialen: 5 × 7 Punkte, mittig auf dem 9 × 9-Raster
  (Spalten 2 bis 6, Zeilen 1 bis 7). Umlaute stehen ohne Punkte.
*/
const SCHRIFT: Record<string, string> = {
  A: '.###.#...##...#######...##...##...#',
  B: '####.#...##...#####.#...##...#####.',
  C: '.###.#...##....#....#....#...#.###.',
  D: '####.#...##...##...##...##...#####.',
  E: '######....#....####.#....#....#####',
  F: '######....#....####.#....#....#....',
  G: '.###.#...##....#.####...##...#.###.',
  H: '#...##...##...#######...##...##...#',
  I: '.###...#....#....#....#....#...###.',
  J: '..###...#....#....#.#..#.#..#..##..',
  K: '#...##..#.#.#..##...#.#..#..#.#...#',
  L: '#....#....#....#....#....#....#####',
  M: '#...###.###.#.##.#.##...##...##...#',
  N: '#...###..##.#.##..###...##...##...#',
  O: '.###.#...##...##...##...##...#.###.',
  P: '####.#...##...#####.#....#....#....',
  Q: '.###.#...##...##...##.#.##..#..##.#',
  R: '####.#...##...#####.#.#..#..#.#...#',
  S: '.#####....#.....###.....#....#####.',
  T: '#####..#....#....#....#....#....#..',
  U: '#...##...##...##...##...##...#.###.',
  V: '#...##...##...##...##...#.#.#...#..',
  W: '#...##...##...##.#.##.#.##.#.#.#.#.',
  X: '#...##...#.#.#...#...#.#.#...##...#',
  Y: '#...##...#.#.#...#....#....#....#..',
  Z: '#####....#...#...#...#...#....#####',
};
const OHNE_PUNKTE: Record<string, string> = { Ä: 'A', Ö: 'O', Ü: 'U' };

/** Punkte eines Buchstabens als Spalte und Zeile auf dem 9 × 9-Raster. */
function buchstabe(zeichen: string): Set<string> {
  const muster = SCHRIFT[OHNE_PUNKTE[zeichen] ?? zeichen] ?? '';
  const punkte = new Set<string>();
  [...muster].forEach((stelle, i) => {
    if (stelle === '#') punkte.add(`${2 + (i % 5)},${1 + Math.floor(i / 5)}`);
  });
  return punkte;
}

const runden = (wert: number) => Math.round(wert * 100) / 100;

export interface Optionen {
  /** Bis zu drei Buchstaben, nur in der großen Fassung. */
  initialen?: string;
  zufall?: () => number;
}

export function punktmatrix(fassung: Fassung, { initialen = '', zufall = Math.random }: Optionen = {}): Matrix {
  const f = FASSUNGEN[fassung];
  const n = f.n;
  const schritt = 20;
  const rand = schritt * f.rand;
  const kante = schritt * (n - 1) + 2 * rand;

  // Buchstaben abwechselnd blau und rosa, der erste blau.
  const buchstaben = fassung === 'gross' ? [...initialen.toUpperCase()].slice(0, 3).map(buchstabe) : [];

  const punkte: Punkt[] = [];
  for (let zeile = 0; zeile < n; zeile += 1) {
    for (let spalte = 0; spalte < n; spalte += 1) {
      const buchstabenBlau: number[] = [];
      const buchstabenRose: number[] = [];
      buchstaben.forEach((punkteDesBuchstabens, index) => {
        if (!punkteDesBuchstabens.has(`${spalte},${zeile}`)) return;
        (index % 2 === 0 ? buchstabenBlau : buchstabenRose).push(index + 1);
      });
      punkte.push({
        spalte,
        zeile,
        x: runden(rand + spalte * schritt),
        y: runden(rand + zeile * schritt),
        buchstabenBlau,
        buchstabenRose,
      });
    }
  }

  // Zufällige Startpunkte, nie zweimal derselbe hintereinander.
  const folge: Ring[] = [];
  let vorher = -1;
  for (let i = 0; i < f.ringe; i += 1) {
    let index = Math.floor(zufall() * n * n);
    while (index === vorher) index = Math.floor(zufall() * n * n);
    vorher = index;

    // Initialen: jeder Buchstabe entsteht hinter der Front eines Rings und
    // steht während des nächsten; dann verwandelt ihn der folgende Ring in
    // den nächsten Buchstaben, der letzte Ring wischt ihn weg.
    // Beispiel mit zwei Buchstaben: 0→1, 1→1, 1→2, 2→2, 2→0.
    const k = buchstaben.length;
    const j = (((i - INITIALEN_AB_RING) % INITIALEN_ALLE) + INITIALEN_ALLE) % INITIALEN_ALLE;
    const imBlock = k > 0 && j <= 2 * k;
    folge.push({
      spalte: index % n,
      zeile: Math.floor(index / n),
      blau: i % 2 === 0 ? 1 : 0,
      vorher: imBlock ? Math.ceil(j / 2) : 0,
      nachher: imBlock && j < 2 * k ? Math.floor(j / 2) + 1 : 0,
      gedaempft: imBlock ? 1 : 0,
    });
  }

  return {
    n,
    kante: runden(kante),
    schritt,
    rand: runden(rand),
    radiusPunkt: runden(schritt * f.punkt),
    radiusWelle: runden(schritt * f.welle),
    weite: f.weite,
    spur: f.spur,
    punkte,
    folge,
    ruhe: { spalte: f.ruhe.spalte, zeile: f.ruhe.zeile, t: runden(f.ruhe.radius / f.weite) },
  };
}
