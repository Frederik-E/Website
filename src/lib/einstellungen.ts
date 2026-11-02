import rohtext from '../../inhalt/einstellungen.yaml?raw';
import { parseEinstellungen, type Sprache } from './einstellungen-schema';

/** Die Einstellungen aus `inhalt/einstellungen.yaml`. */
export const einstellungen = parseEinstellungen(rohtext);

/**
 * Untertitel und Einleitung in einer Sprache. Auf Englisch gelten die Felder
 * mit "_en"; fehlt eins, bleibt der deutsche Text stehen.
 */
export function einstellungenIn(sprache: Sprache) {
  if (sprache === 'de') return einstellungen;
  return {
    ...einstellungen,
    untertitel: einstellungen.untertitel_en ?? einstellungen.untertitel,
    einleitung: einstellungen.einleitung_en ?? einstellungen.einleitung,
  };
}
