/** Ersetzt deutsche Sonderzeichen und macht aus einem Titel einen Ordnernamen. */
export function slug(text: string): string {
  return text
    .toLowerCase()
    .replaceAll('ä', 'ae')
    .replaceAll('ö', 'oe')
    .replaceAll('ü', 'ue')
    .replaceAll('ß', 'ss')
    .normalize('NFKD')
    .replace(/[̀-ͯ]/g, '')
    .replace(/[^a-z0-9]+/g, '-')
    .replace(/^-+|-+$/g, '');
}

/** Dateiname für den heruntergeladenen Lebenslauf, z. B. Anna-Meier-CV.pdf */
export function cvDateiname(name: string): string {
  const sauber = name
    .replaceAll('ä', 'ae')
    .replaceAll('ö', 'oe')
    .replaceAll('ü', 'ue')
    .replaceAll('Ä', 'Ae')
    .replaceAll('Ö', 'Oe')
    .replaceAll('Ü', 'Ue')
    .replaceAll('ß', 'ss')
    .normalize('NFKD')
    .replace(/[̀-ͯ]/g, '')
    .replace(/[^A-Za-z0-9]+/g, '-')
    .replace(/^-+|-+$/g, '');
  return `${sauber || 'Lebenslauf'}-CV.pdf`;
}

/** Stand der Seite als "JJJJ-MM": der Monat, in dem sie gebaut wurde. */
export const STAND = new Date().toISOString().slice(0, 7);
