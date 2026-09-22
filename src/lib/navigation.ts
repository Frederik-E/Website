import { navigationsSeiten } from './seiten';
import type { Texte } from '../i18n';

export interface NavigationsEintrag {
  adresse: string;
  text: string;
  /** Zweistellig, z. B. "01". Steht in der Kopfzeile und im Seitenkopf. */
  nummer: string;
}

/** Feste Seiten plus alles aus inhalt/seiten/ mit navigation: true, durchnummeriert. */
export async function navigation(texte: Texte): Promise<NavigationsEintrag[]> {
  const feste = [
    { adresse: '/projects/', text: texte.navProjekte },
    { adresse: '/about/', text: texte.ueberMich },
    { adresse: '/cv/', text: texte.navLebenslauf },
  ];
  const seiten = (await navigationsSeiten()).map((seite) => ({
    adresse: `/${seite.id}/`,
    text: seite.data.titel,
  }));
  return [...feste, ...seiten].map((eintrag, stelle) => ({
    ...eintrag,
    nummer: String(stelle + 1).padStart(2, '0'),
  }));
}

/** Der Navigationspunkt, zu dem eine Adresse gehört. Projektseiten gehören zu "Projekte". */
export async function eintragFuer(pfad: string, texte: Texte): Promise<NavigationsEintrag | undefined> {
  return (await navigation(texte)).find((eintrag) => pfad.startsWith(eintrag.adresse));
}
