import { getCollection, type CollectionEntry } from 'astro:content';
import type { Sprache } from './einstellungen-schema';

type Eintrag = CollectionEntry<'seiten'>;
type EintragEn = CollectionEntry<'seitenEn'>;

/**
 * Eine freie Seite in der Sprache der Seite. Einstellungen wie navigation und
 * entwurf kommen aus <name>.md; auf Englisch ersetzen Titel, Beschreibung und
 * Text aus <name>.en.md die deutschen. Ohne Übersetzung bleibt es Deutsch.
 */
export interface Seite {
  id: string;
  data: Eintrag['data'];
  text: Eintrag | EintragEn;
  textSprache: Sprache;
}

/** Adressen, die der Code schon belegt. Eine Datei mit diesem Namen wäre unsichtbar. */
const VERGEBEN = new Set([
  'cv',
  'projects',
  'impressum',
  'datenschutz',
  '404',
  'robots',
  'favicon',
  'index',
  'en',
]);

// Nur die Dateinamen, ohne sie zu laden. Ohne eigene Seiten wird die Sammlung
// gar nicht abgefragt, sonst warnt Astro bei jeder Seite, sie sei leer.
const alleDateien = Object.keys(import.meta.glob('/inhalt/seiten/*.md'));
const seitenDateien = alleDateien.filter((datei) => !datei.endsWith('.en.md'));
const englischeDateien = alleDateien.filter((datei) => datei.endsWith('.en.md'));

/** Alle veröffentlichten Seiten in einer Sprache. Entwürfe erscheinen nur in der lokalen Vorschau. */
export async function alleSeiten(sprache: Sprache): Promise<Seite[]> {
  if (seitenDateien.length === 0) return [];

  const eintraege = await getCollection('seiten', ({ data }) => import.meta.env.DEV || !data.entwurf);
  const uebersetzungen = new Map<string, EintragEn>();
  if (sprache === 'en' && englischeDateien.length > 0) {
    for (const eintrag of await getCollection('seitenEn')) uebersetzungen.set(eintrag.id, eintrag);
  }

  for (const eintrag of eintraege) {
    if (VERGEBEN.has(eintrag.id)) {
      throw new Error(
        `inhalt/seiten/${eintrag.id}.md kann nicht verwendet werden: die Adresse /${eintrag.id}/ ist schon vergeben. ` +
          `Benenne die Datei um, zum Beispiel in ${eintrag.id}-2.md.`,
      );
    }
  }

  const seiten: Seite[] = eintraege.map((eintrag) => {
    const en = uebersetzungen.get(eintrag.id);
    if (!en) return { id: eintrag.id, data: eintrag.data, text: eintrag, textSprache: 'de' };
    return {
      id: eintrag.id,
      data: { ...eintrag.data, titel: en.data.titel, beschreibung: en.data.beschreibung ?? eintrag.data.beschreibung },
      text: en,
      textSprache: 'en',
    };
  });

  return seiten.sort((a, b) => {
    const links = a.data.reihenfolge ?? Number.MAX_SAFE_INTEGER;
    const rechts = b.data.reihenfolge ?? Number.MAX_SAFE_INTEGER;
    if (links !== rechts) return links - rechts;
    return a.data.titel.localeCompare(b.data.titel);
  });
}

/** Die Seiten, die zusätzlich in der Kopfzeile stehen sollen. */
export async function navigationsSeiten(sprache: Sprache): Promise<Seite[]> {
  return (await alleSeiten(sprache)).filter((seite) => seite.data.navigation);
}

/** Pfade der freien Seiten, für getStaticPaths. */
export async function seitenPfade(sprache: Sprache) {
  const seiten = await alleSeiten(sprache);
  return seiten.map((seite) => ({ params: { seite: seite.id }, props: { seite } }));
}
