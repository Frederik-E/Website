import { defineCollection } from 'astro:content';
import { glob } from 'astro/loaders';
import { z } from 'astro/zod';
import { monatSchema } from './lib/monat';
import { linkedinKennung } from './lib/linkedin';

const projekte = defineCollection({
  loader: glob({
    base: './inhalt/projekte',
    pattern: '*/index.md',
    // Die Adresse eines Projekts ist der Ordnername, nicht "ordner/index".
    generateId: ({ entry }) => entry.replace(/\/index\.md$/, ''),
  }),
  schema: ({ image }) =>
    z
      .object({
        titel: z.string({ error: 'Feld "titel" fehlt.' }).min(1, { error: 'Feld "titel" darf nicht leer sein.' }),
        kurz: z
          .string()
          .min(1, { error: 'Feld "kurz" darf nicht leer sein.' })
          .max(200, { error: 'Feld "kurz" darf hoechstens 200 Zeichen haben.' })
          .optional(),
        // Stichpunkte stehen im Eintrag rechts, dort wo sonst das Titelbild steht.
        stichpunkte: z.array(z.string().min(1, { error: 'Ein Stichpunkt darf nicht leer sein.' })).default([]),
        datum: monatSchema('datum'),
        bis: monatSchema('bis').optional(),
        rolle: z.string().optional(),
        tags: z.array(z.string()).default([]),
        titelbild: image().optional(),
        titelbild_alt: z.string().optional(),
        video: z.string().optional(),
        code: z.url({ error: 'Feld "code" ist keine gueltige Adresse.' }).optional(),
        demo: z.url({ error: 'Feld "demo" ist keine gueltige Adresse.' }).optional(),
        // Ein LinkedIn-Beitrag, der am Ende der Detailseite eingebettet wird.
        // Er lädt erst, wenn jemand auf "Beitrag laden" klickt.
        linkedin: z
          .url({ error: 'Feld "linkedin" ist keine gueltige Adresse.' })
          .refine((adresse) => linkedinKennung(adresse) !== undefined, {
            error:
              'Feld "linkedin" ist keine Adresse eines LinkedIn-Beitrags. Einen Kurzlink (lnkd.in) erst im Browser oeffnen ' +
              'und die lange Adresse aus der Adresszeile eintragen.',
          })
          .optional(),
        // Unterschrift unter dem Beitrag: von wem er ist und was er zeigt.
        linkedin_text: z.string().min(1).optional(),
        startseite: z.boolean().default(false),
        reihenfolge: z.number().optional(),
        entwurf: z.boolean().default(false),
      })
      .refine((daten) => daten.kurz || daten.stichpunkte.length > 0, {
        error: 'Feld "kurz" fehlt. Ohne "kurz" braucht das Projekt "stichpunkte".',
        path: ['kurz'],
      })
      .refine((daten) => !daten.bis || daten.bis >= daten.datum, {
        error: 'Feld "bis" liegt vor "datum".',
        path: ['bis'],
      })
      .refine((daten) => !daten.video || daten.titelbild, {
        error: 'Wenn "video" gesetzt ist, braucht das Projekt auch "titelbild" als Posterbild.',
        path: ['titelbild'],
      })
      .refine((daten) => !daten.linkedin_text || daten.linkedin, {
        error: 'Feld "linkedin_text" ist gesetzt, aber "linkedin" fehlt.',
        path: ['linkedin'],
      })
      .refine((daten) => !daten.titelbild || (daten.titelbild_alt ?? '').trim().length > 0, {
        error: 'Zu "titelbild" gehoert "titelbild_alt" mit einer Bildbeschreibung.',
        path: ['titelbild_alt'],
      }),
});

/**
 * Englische Fassung eines Projekts: inhalt/projekte/<ordner>/index.en.md.
 * Sie enthält nur die übersetzten Texte. Datum, Links, Reihenfolge, Video und
 * Entwurf stehen allein in index.md und gelten für beide Sprachen. Ein eigenes
 * Titelbild ist nur nötig, wenn im Bild Text steht.
 */
const projekteEn = defineCollection({
  loader: glob({
    base: './inhalt/projekte',
    pattern: '*/index.en.md',
    generateId: ({ entry }) => entry.replace(/\/index\.en\.md$/, ''),
  }),
  schema: ({ image }) =>
    z.strictObject(
      {
        titel: z.string({ error: 'Feld "titel" fehlt.' }).min(1, { error: 'Feld "titel" darf nicht leer sein.' }),
        kurz: z
          .string()
          .min(1, { error: 'Feld "kurz" darf nicht leer sein.' })
          .max(200, { error: 'Feld "kurz" darf hoechstens 200 Zeichen haben.' })
          .optional(),
        stichpunkte: z.array(z.string().min(1, { error: 'Ein Stichpunkt darf nicht leer sein.' })).optional(),
        rolle: z.string().optional(),
        tags: z.array(z.string()).optional(),
        titelbild: image().optional(),
        titelbild_alt: z.string().optional(),
        linkedin_text: z.string().min(1).optional(),
      },
      {
        error: (problem) =>
          problem.code === 'unrecognized_keys'
            ? `In index.en.md stehen nur titel, kurz, stichpunkte, rolle, tags, titelbild, titelbild_alt und linkedin_text. ` +
              `${problem.keys.join(', ')} gehört in index.md und gilt dann für beide Sprachen.`
            : undefined,
      },
    ),
});

/** Frei angelegte Seiten aus inhalt/seiten/<name>.md, erreichbar unter /<name>/. */
const seiten = defineCollection({
  // Englische Fassungen (<name>.en.md) gehören in die Sammlung darunter.
  loader: glob({ base: './inhalt/seiten', pattern: ['*.md', '!*.en.md'] }),
  schema: z.object({
    titel: z.string({ error: 'Feld "titel" fehlt.' }).min(1, { error: 'Feld "titel" darf nicht leer sein.' }),
    beschreibung: z.string().optional(),
    navigation: z.boolean().default(false),
    reihenfolge: z.number().optional(),
    noindex: z.boolean().default(false),
    entwurf: z.boolean().default(false),
  }),
});

/** Englische Fassung einer freien Seite: inhalt/seiten/<name>.en.md, unter /en/<name>/. */
const seitenEn = defineCollection({
  loader: glob({
    base: './inhalt/seiten',
    pattern: '*.en.md',
    generateId: ({ entry }) => entry.replace(/\.en\.md$/, ''),
  }),
  schema: z.object({
    titel: z.string({ error: 'Feld "titel" fehlt.' }).min(1, { error: 'Feld "titel" darf nicht leer sein.' }),
    beschreibung: z.string().optional(),
  }),
});

export const collections = { projekte, projekteEn, seiten, seitenEn };
