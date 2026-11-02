import { z } from 'astro/zod';
import { parse as parseYaml } from 'yaml';
import type { Sprache } from './einstellungen-schema';
import { monatSchema } from './monat';

/** Eine Station: Studium, Stelle, Projekt. */
const stationSchema = z.object({
  titel: z.string().min(1, { error: 'Eine Station braucht ein Feld "titel".' }),
  organisation: z.string().optional(),
  ort: z.string().optional(),
  von: monatSchema('von'),
  bis: monatSchema('bis').nullable().optional(),
  punkte: z.array(z.string()).default([]),
});

/** Eine Liste: Kenntnisse, Sprachen, Werkzeuge. */
const listeSchema = z.object({
  bereich: z.string().min(1, { error: 'Eine Liste braucht ein Feld "bereich".' }),
  liste: z.array(z.string()).default([]),
});

const eintragSchema = z.union([stationSchema, listeSchema], {
  error: 'Ein Eintrag braucht entweder "titel" (Station) oder "bereich" und "liste".',
});

const abschnittSchema = z.object({
  titel: z.string().min(1, { error: 'Ein Abschnitt braucht ein Feld "titel".' }),
  startseite: z.boolean().default(false),
  eintraege: z.array(eintragSchema).default([]),
});

const lebenslaufSchema = z.object({
  /** Ein paar Sätze in der Ich-Form. Leerzeilen trennen Absätze. */
  profil: z.string().nullable().optional(),
  abschnitte: z.array(abschnittSchema).default([]),
});

export type Station = z.infer<typeof stationSchema>;
export type Liste = z.infer<typeof listeSchema>;
export type Eintrag = z.infer<typeof eintragSchema>;
export type Abschnitt = z.infer<typeof abschnittSchema>;

export function istListe(eintrag: Eintrag): eintrag is Liste {
  return 'bereich' in eintrag;
}

export function parseLebenslauf(text: string, datei = 'inhalt/lebenslauf.yaml') {
  const ergebnis = lebenslaufSchema.safeParse(parseYaml(text));
  if (!ergebnis.success) {
    const zeilen = ergebnis.error.issues.map(
      (problem) => `  - ${problem.path.join('.') || 'abschnitte'}: ${problem.message}`,
    );
    throw new Error([`Fehler in ${datei}:`, ...zeilen].join('\n'));
  }
  return ergebnis.data;
}

// Deutsch aus lebenslauf.yaml, Englisch aus lebenslauf.en.yaml.
const dateien = import.meta.glob<string>('/inhalt/lebenslauf*.yaml', {
  query: '?raw',
  import: 'default',
  eager: true,
});

function laden(sprache: Sprache) {
  const datei = sprache === 'de' ? 'inhalt/lebenslauf.yaml' : 'inhalt/lebenslauf.en.yaml';
  const text = dateien[`/${datei}`];
  if (text === undefined) return undefined;
  const lebenslauf = parseLebenslauf(text, datei);
  return {
    abschnitte: lebenslauf.abschnitte,
    /** Die Abschnitte für die Startseite. */
    kurzfassung: lebenslauf.abschnitte.filter((abschnitt) => abschnitt.startseite),
    /** Das Profil als Absätze. Leer, wenn keins eingetragen ist. */
    profil: (lebenslauf.profil ?? '')
      .split(/\n\s*\n/)
      .map((absatz) => absatz.replace(/\s+/g, ' ').trim())
      .filter((absatz) => absatz !== ''),
  };
}

const deutsch = laden('de');
if (!deutsch) throw new Error('inhalt/lebenslauf.yaml fehlt.');
const englisch = laden('en');

/**
 * Der Lebenslauf in einer Sprache. Fehlt lebenslauf.en.yaml, steht auch auf
 * der englischen Seite der deutsche.
 */
export function lebenslaufIn(sprache: Sprache) {
  return (sprache === 'en' && englisch) || deutsch!;
}

/** Sprache, in der der Lebenslauf auf einer Seite tatsächlich steht. */
export function lebenslaufSprache(sprache: Sprache): Sprache {
  return sprache === 'en' && englisch ? 'en' : 'de';
}
