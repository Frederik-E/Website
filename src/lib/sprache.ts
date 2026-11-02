import type { Sprache } from './einstellungen-schema';

/**
 * Die Seite gibt es zweimal: Deutsch unter /, Englisch unter /en/. Die Pfade
 * sind sonst gleich, aus /cv/ wird /en/cv/.
 */

/** Die Sprache einer Seite, aus `Astro.currentLocale`. */
export function spracheVon(locale: string | undefined): Sprache {
  return locale === 'en' ? 'en' : 'de';
}

/** Adresse eines Pfads in einer Sprache: "/cv/" wird auf Englisch "/en/cv/". */
export function adresse(pfad: string, sprache: Sprache): string {
  return sprache === 'en' ? `/en${pfad}` : pfad;
}

/** Der Pfad ohne Sprachpräfix: "/en/cv/" wird "/cv/". */
export function ohneSprache(pfad: string): string {
  return pfad.replace(/^\/en(?=\/|$)/, '') || '/';
}

/** Die jeweils andere Sprache. */
export function andereSprache(sprache: Sprache): Sprache {
  return sprache === 'de' ? 'en' : 'de';
}
