import { z } from 'astro/zod';
import { parse as parseYaml } from 'yaml';
import rohtext from '../../inhalt/lebenslauf.yaml?raw';
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
  abschnitte: z.array(abschnittSchema).default([]),
});

export type Station = z.infer<typeof stationSchema>;
export type Liste = z.infer<typeof listeSchema>;
export type Eintrag = z.infer<typeof eintragSchema>;
export type Abschnitt = z.infer<typeof abschnittSchema>;

export function istListe(eintrag: Eintrag): eintrag is Liste {
  return 'bereich' in eintrag;
}

export function parseLebenslauf(text: string) {
  const ergebnis = lebenslaufSchema.safeParse(parseYaml(text));
  if (!ergebnis.success) {
    const zeilen = ergebnis.error.issues.map(
      (problem) => `  - ${problem.path.join('.') || 'abschnitte'}: ${problem.message}`,
    );
    throw new Error(['Fehler in inhalt/lebenslauf.yaml:', ...zeilen].join('\n'));
  }
  return ergebnis.data;
}

export const lebenslauf = parseLebenslauf(rohtext);
export const kurzfassung = lebenslauf.abschnitte.filter((abschnitt) => abschnitt.startseite);
