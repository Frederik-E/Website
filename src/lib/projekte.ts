import { getCollection, type CollectionEntry } from 'astro:content';

export type Projekt = CollectionEntry<'projekte'>;

/**
 * Videos bleiben im Projektordner. Vite loest sie beim Build zu einer Adresse
 * mit Hash auf, damit sie ausgeliefert und lange zwischengespeichert werden.
 */
const videos = import.meta.glob<string>('/inhalt/projekte/*/*.{mp4,webm}', {
  query: '?url',
  import: 'default',
  eager: true,
});

export function videoAdresse(projektId: string, dateiname: string): string {
  const schluessel = `/inhalt/projekte/${projektId}/${dateiname}`;
  const adresse = videos[schluessel];
  if (!adresse) {
    const vorhanden = Object.keys(videos).join(', ') || 'keine';
    throw new Error(
      `Das Video "${dateiname}" fehlt in inhalt/projekte/${projektId}/. Gefundene Videos: ${vorhanden}`,
    );
  }
  return adresse;
}

/** Entwuerfe erscheinen nur in der lokalen Vorschau. */
export async function alleProjekte(): Promise<Projekt[]> {
  const eintraege = await getCollection('projekte', ({ data }) => import.meta.env.DEV || !data.entwurf);
  return eintraege.sort((a, b) => b.data.datum.localeCompare(a.data.datum));
}

/** Ohne Text unter den Kopfdaten entsteht keine Detailseite. */
export function hatDetailseite(projekt: Projekt): boolean {
  return (projekt.body ?? '').trim().length > 0;
}

/** Projekte fuer die Startseite: markierte zuerst, sonst die drei neuesten. */
export function startseitenProjekte(projekte: Projekt[]): Projekt[] {
  const markiert = projekte.filter((projekt) => projekt.data.startseite);
  const auswahl = markiert.length > 0 ? markiert : projekte.slice(0, 3);
  return [...auswahl].sort((a, b) => {
    const links = a.data.reihenfolge ?? Number.MAX_SAFE_INTEGER;
    const rechts = b.data.reihenfolge ?? Number.MAX_SAFE_INTEGER;
    if (links !== rechts) return links - rechts;
    return b.data.datum.localeCompare(a.data.datum);
  });
}

/** Das naechste Projekt mit Detailseite, fuer den Link am Seitenende. */
export function naechstesProjekt(projekte: Projekt[], aktuell: Projekt): Projekt | undefined {
  const mitSeite = projekte.filter(hatDetailseite);
  const stelle = mitSeite.findIndex((projekt) => projekt.id === aktuell.id);
  if (stelle === -1 || mitSeite.length < 2) return undefined;
  return mitSeite[(stelle + 1) % mitSeite.length];
}

/**
 * Feste Nummer eines Projekts, z. B. "P-03". Gezählt wird ab dem ältesten
 * Projekt, damit ein neues Projekt die Nummern der alten nicht verschiebt.
 */
export function projektNummer(projekte: Projekt[], projekt: Projekt): string {
  const reihe = [...projekte].sort((a, b) => a.data.datum.localeCompare(b.data.datum) || a.id.localeCompare(b.id));
  const stelle = reihe.findIndex((eintrag) => eintrag.id === projekt.id);
  return `P-${String(stelle + 1).padStart(2, '0')}`;
}
