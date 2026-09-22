import { getCollection, type CollectionEntry } from 'astro:content';

export type Seite = CollectionEntry<'seiten'>;

/** Adressen, die der Code schon belegt. Eine Datei mit diesem Namen wäre unsichtbar. */
const VERGEBEN = new Set([
  'about',
  'cv',
  'projects',
  'impressum',
  'datenschutz',
  '404',
  'robots',
  'favicon',
  'index',
]);

/** Alle veröffentlichten Seiten. Entwürfe erscheinen nur in der lokalen Vorschau. */
export async function alleSeiten(): Promise<Seite[]> {
  const eintraege = await getCollection('seiten', ({ data }) => import.meta.env.DEV || !data.entwurf);

  for (const eintrag of eintraege) {
    if (VERGEBEN.has(eintrag.id)) {
      throw new Error(
        `inhalt/seiten/${eintrag.id}.md kann nicht verwendet werden: die Adresse /${eintrag.id}/ ist schon vergeben. ` +
          `Benenne die Datei um, zum Beispiel in ${eintrag.id}-2.md.`,
      );
    }
  }

  return eintraege.sort((a, b) => {
    const links = a.data.reihenfolge ?? Number.MAX_SAFE_INTEGER;
    const rechts = b.data.reihenfolge ?? Number.MAX_SAFE_INTEGER;
    if (links !== rechts) return links - rechts;
    return a.data.titel.localeCompare(b.data.titel);
  });
}

/** Die Seiten, die zusätzlich in der Kopfzeile stehen sollen. */
export async function navigationsSeiten(): Promise<Seite[]> {
  return (await alleSeiten()).filter((seite) => seite.data.navigation);
}
