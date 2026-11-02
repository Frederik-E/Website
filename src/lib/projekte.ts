import { getCollection, type CollectionEntry } from 'astro:content';
import type { Sprache } from './einstellungen-schema';

type Eintrag = CollectionEntry<'projekte'>;
type EintragEn = CollectionEntry<'projekteEn'>;

/**
 * Ein Projekt in der Sprache der Seite. Die Kopfdaten kommen aus index.md;
 * auf Englisch ersetzen die Felder aus index.en.md die Texte darin. Fehlt ein
 * Feld oder die ganze Übersetzung, bleibt der deutsche Text stehen.
 */
export interface Projekt {
  id: string;
  data: Eintrag['data'];
  /** Der Eintrag, dessen Text die Detailseite zeigt. */
  text: Eintrag | EintragEn;
  /** Sprache dieses Texts. Ohne Übersetzung Deutsch, auch auf /en/. */
  textSprache: Sprache;
  /** Sprache von Titel, Kurztext und Stichpunkten. Ohne index.en.md Deutsch. */
  kopfSprache: Sprache;
  /** Ob es eine Detailseite gibt. Das entscheidet der Text in index.md. */
  detailseite: boolean;
}

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

// Nur die Dateinamen. Ohne englische Fassungen wird die Sammlung gar nicht
// abgefragt, sonst warnt Astro, sie sei leer.
const englischeDateien = Object.keys(import.meta.glob('/inhalt/projekte/*/index.en.md'));

const hatText = (eintrag: { body?: string } | undefined) => (eintrag?.body ?? '').trim().length > 0;

function inSprache(de: Eintrag, en: EintragEn | undefined): Projekt {
  const detailseite = hatText(de);
  if (!en) return { id: de.id, data: de.data, text: de, textSprache: 'de', kopfSprache: 'de', detailseite };
  const e = en.data;
  return {
    id: de.id,
    data: {
      ...de.data,
      titel: e.titel,
      kurz: e.kurz ?? de.data.kurz,
      stichpunkte: e.stichpunkte ?? de.data.stichpunkte,
      rolle: e.rolle ?? de.data.rolle,
      tags: e.tags ?? de.data.tags,
      titelbild: e.titelbild ?? de.data.titelbild,
      titelbild_alt: e.titelbild_alt ?? de.data.titelbild_alt,
    },
    text: hatText(en) ? en : de,
    textSprache: hatText(en) ? 'en' : 'de',
    kopfSprache: 'en',
    detailseite,
  };
}

/** Alle Projekte in einer Sprache, neueste zuerst. Entwuerfe erscheinen nur in der lokalen Vorschau. */
export async function alleProjekte(sprache: Sprache): Promise<Projekt[]> {
  const eintraege = await getCollection('projekte', ({ data }) => import.meta.env.DEV || !data.entwurf);
  const uebersetzungen = new Map<string, EintragEn>();
  if (sprache === 'en' && englischeDateien.length > 0) {
    for (const eintrag of await getCollection('projekteEn')) uebersetzungen.set(eintrag.id, eintrag);
  }
  return eintraege
    .map((eintrag) => inSprache(eintrag, uebersetzungen.get(eintrag.id)))
    .sort((a, b) => b.data.datum.localeCompare(a.data.datum));
}

/** Pfade der Detailseiten, für getStaticPaths in src/pages/projects/ und src/pages/en/projects/. */
export async function projektPfade(sprache: Sprache) {
  const projekte = await alleProjekte(sprache);
  return projekte.filter(hatDetailseite).map((projekt) => ({
    params: { id: projekt.id },
    props: { projekt, projekte },
  }));
}

/**
 * Lauter kurze Stichpunkte, etwa eine Liste von Skills. Sie stehen im Eintrag
 * in Spalten und in der Übersicht in einer Zeile statt untereinander.
 */
export function kurzeStichpunkte(projekt: Projekt): boolean {
  const punkte = projekt.data.stichpunkte;
  return punkte.length > 3 && punkte.every((punkt) => punkt.length <= 24);
}

/** Ohne Text unter den Kopfdaten von index.md entsteht keine Detailseite, in keiner Sprache. */
export function hatDetailseite(projekt: Projekt): boolean {
  return projekt.detailseite;
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

