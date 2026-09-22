import { z } from 'astro/zod';
import { parse as parseYaml } from 'yaml';

/** Sprachen, in denen die Oberflächentexte vorliegen. */
export const sprachen = ['en', 'de'] as const;
export type Sprache = (typeof sprachen)[number];

const text = (feld: string) =>
  z.string({ error: `Feld "${feld}" fehlt oder ist kein Text.` }).trim();

/**
 * Schema für den Alltag: Platzhalter der Form `[TODO: …]` sind erlaubt,
 * damit die Seite auch mit unfertigen Angaben läuft.
 */
export const einstellungenSchema = z.object({
  sprache: z.enum(sprachen, { error: 'Feld "sprache" muss "en" oder "de" sein.' }),
  name: text('name').min(1, { error: 'Feld "name" darf nicht leer sein.' }),
  untertitel: text('untertitel').min(1, { error: 'Feld "untertitel" darf nicht leer sein.' }),
  einleitung: text('einleitung').default(''),
  email: text('email').min(1, { error: 'Feld "email" darf nicht leer sein.' }),
  github: text('github').default(''),
  linkedin: text('linkedin').default(''),
  domain: text('domain').min(1, { error: 'Feld "domain" darf nicht leer sein.' }),
});

export type Einstellungen = z.infer<typeof einstellungenSchema>;

const istPlatzhalter = (wert: string) => wert.includes('[TODO');

const ohnePlatzhalter = (feld: string) =>
  z.string().refine((wert) => !istPlatzhalter(wert), {
    error: `Feld "${feld}" enthält noch einen Platzhalter.`,
  });

const optionaleUrl = (feld: string) =>
  z
    .string()
    .refine((wert) => wert === '' || !istPlatzhalter(wert), {
      error: `Feld "${feld}" enthält noch einen Platzhalter.`,
    })
    .refine((wert) => wert === '' || z.url().safeParse(wert).success, {
      error: `Feld "${feld}" ist keine gültige Adresse (erwartet wird z. B. https://github.com/name).`,
    });

/**
 * Strengeres Schema für `npm run check`: keine Platzhalter, gültige
 * E-Mail-Adresse und Web-Adressen, und nicht mehr die Beispiel-Domain.
 */
export const einstellungenStreng = einstellungenSchema.extend({
  name: ohnePlatzhalter('name'),
  untertitel: ohnePlatzhalter('untertitel'),
  einleitung: ohnePlatzhalter('einleitung'),
  email: z.email({ error: 'Feld "email" ist keine gültige E-Mail-Adresse.' }),
  github: optionaleUrl('github'),
  linkedin: optionaleUrl('linkedin'),
  domain: z
    .url({ error: 'Feld "domain" ist keine gültige Adresse (erwartet wird z. B. https://meine-domain.de).' })
    .refine((wert) => !wert.includes('example.com'), {
      error: 'Feld "domain" steht noch auf example.com. Trage deine eigene Domain ein.',
    }),
});

/** Formt einen Zod-Fehler in eine lesbare, mehrzeilige Meldung um. */
export function fehlerTexte(fehler: z.ZodError): string[] {
  return fehler.issues.map((problem) => {
    const feld = problem.path.join('.');
    return feld ? `${feld}: ${problem.message}` : problem.message;
  });
}

/** Liest den Inhalt von `inhalt/einstellungen.yaml` und prüft ihn. */
export function parseEinstellungen(rohtext: string, streng = false): Einstellungen {
  let daten: unknown;
  try {
    daten = parseYaml(rohtext);
  } catch (ursache) {
    const meldung = ursache instanceof Error ? ursache.message : String(ursache);
    throw new Error(`inhalt/einstellungen.yaml ist kein gültiges YAML.\n${meldung}`);
  }

  const ergebnis = (streng ? einstellungenStreng : einstellungenSchema).safeParse(daten);
  if (!ergebnis.success) {
    throw new Error(
      ['Fehler in inhalt/einstellungen.yaml:', ...fehlerTexte(ergebnis.error).map((z) => `  - ${z}`)].join('\n'),
    );
  }
  return ergebnis.data;
}

/** Domain ohne Schrägstrich am Ende. */
export function domainOhneSchraegstrich(domain: string): string {
  return domain.replace(/\/+$/, '');
}
