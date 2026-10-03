import { defineCollection } from 'astro:content';
import { glob } from 'astro/loaders';
import { z } from 'astro/zod';
import { monatSchema } from './lib/monat';

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
          .string({ error: 'Feld "kurz" fehlt.' })
          .min(1, { error: 'Feld "kurz" darf nicht leer sein.' })
          .max(200, { error: 'Feld "kurz" darf hoechstens 200 Zeichen haben.' }),
        datum: monatSchema('datum'),
        bis: monatSchema('bis').optional(),
        rolle: z.string().optional(),
        tags: z.array(z.string()).default([]),
        titelbild: image().optional(),
        titelbild_alt: z.string().optional(),
        video: z.string().optional(),
        code: z.url({ error: 'Feld "code" ist keine gueltige Adresse.' }).optional(),
        demo: z.url({ error: 'Feld "demo" ist keine gueltige Adresse.' }).optional(),
        startseite: z.boolean().default(false),
        reihenfolge: z.number().optional(),
        entwurf: z.boolean().default(false),
      })
      .refine((daten) => !daten.bis || daten.bis >= daten.datum, {
        error: 'Feld "bis" liegt vor "datum".',
        path: ['bis'],
      })
      .refine((daten) => !daten.video || daten.titelbild, {
        error: 'Wenn "video" gesetzt ist, braucht das Projekt auch "titelbild" als Posterbild.',
        path: ['titelbild'],
      })
      .refine((daten) => !daten.titelbild || (daten.titelbild_alt ?? '').trim().length > 0, {
        error: 'Zu "titelbild" gehoert "titelbild_alt" mit einer Bildbeschreibung.',
        path: ['titelbild_alt'],
      }),
});

/** Frei angelegte Seiten aus inhalt/seiten/<name>.md, erreichbar unter /<name>/. */
const seiten = defineCollection({
  loader: glob({ base: './inhalt/seiten', pattern: '*.md' }),
  schema: z.object({
    titel: z.string({ error: 'Feld "titel" fehlt.' }).min(1, { error: 'Feld "titel" darf nicht leer sein.' }),
    beschreibung: z.string().optional(),
    navigation: z.boolean().default(false),
    reihenfolge: z.number().optional(),
    noindex: z.boolean().default(false),
    entwurf: z.boolean().default(false),
  }),
});

export const collections = { projekte, seiten };
