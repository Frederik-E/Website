/**
 * Kennung eines LinkedIn-Beitrags aus seiner Adresse. Erkannt werden die
 * Formen, die LinkedIn beim Teilen und in der Adresszeile zeigt:
 *
 *   https://www.linkedin.com/feed/update/urn:li:activity:7477294675029307392/
 *   https://www.linkedin.com/feed/update/urn:li:ugcPost:7477290419605864448/
 *   https://www.linkedin.com/posts/name_titel-activity-7477294675029307392-MI6x
 *   https://www.linkedin.com/embed/feed/update/urn:li:share:7477290419605864448
 *
 * Kurzlinks (lnkd.in) enthalten keine Kennung und geben undefined.
 */
export function linkedinKennung(adresse: string): string | undefined {
  let text = adresse;
  try {
    text = decodeURIComponent(adresse);
  } catch {
    // Kaputte Prozentzeichen: dann eben die Adresse, wie sie ist.
  }
  if (!/^https:\/\/([a-z]{2,3}\.|www\.)?linkedin\.com\//.test(text)) return undefined;
  const urn = text.match(/urn:li:(activity|ugcPost|share):(\d+)/);
  if (urn) return `urn:li:${urn[1]}:${urn[2]}`;
  const beitrag = text.match(/activity-(\d+)/);
  if (beitrag) return `urn:li:activity:${beitrag[1]}`;
  return undefined;
}

/** Adresse für den eingebetteten Rahmen. */
export const linkedinEinbettung = (kennung: string) => `https://www.linkedin.com/embed/feed/update/${kennung}`;

/** Adresse des Beitrags auf LinkedIn selbst. */
export const linkedinBeitrag = (kennung: string) => `https://www.linkedin.com/feed/update/${kennung}/`;
