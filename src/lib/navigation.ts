import { navigationsSeiten } from './seiten';
import { adresse } from './sprache';
import type { Sprache } from './einstellungen-schema';
import type { Texte } from '../i18n';

export interface NavigationsEintrag {
  adresse: string;
  text: string;
}

/** Feste Seiten plus alles aus inhalt/seiten/ mit navigation: true. */
export async function navigation(texte: Texte, sprache: Sprache): Promise<NavigationsEintrag[]> {
  // Das Profil steht vorn: erst der Lebenslauf, dann die Projekte.
  const feste = [
    { adresse: adresse('/cv/', sprache), text: texte.navLebenslauf },
    { adresse: adresse('/projects/', sprache), text: texte.navProjekte },
  ];
  const seiten = (await navigationsSeiten(sprache)).map((seite) => ({
    adresse: adresse(`/${seite.id}/`, sprache),
    text: seite.data.titel,
  }));
  return [...feste, ...seiten];
}
