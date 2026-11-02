import type { Sprache } from './lib/einstellungen-schema';

/**
 * Alle Oberflächentexte in beiden Sprachen. Deutsch steht unter /, Englisch
 * unter /en/, sonst sind die Adressen gleich. Englisch legt die Form fest,
 * Deutsch muss dieselben Schlüssel haben.
 */
const en = {
  zumInhalt: 'Skip to content',
  navigation: 'Navigation',
  kontakt: 'Contact',
  navProjekte: 'Projects',
  navLebenslauf: 'CV',
  navGithub: 'GitHub',
  navLinkedin: 'LinkedIn',
  navEmail: 'Email',
  profil: 'Profile',
  projekte: 'Projects',
  weitereInhalte: 'More on this site',
  ausgewaehlteArbeiten: 'Selected work',
  alleProjekteKurz: 'Every project, newest first',
  lebenslaufGanz: 'Full CV',
  lebenslaufPdf: 'PDF',
  foto: 'Photo',
  fotoVon: (name: string) => `Photo of ${name}`,
  alleProjekte: (anzahl: number) => `All projects (${anzahl})`,
  details: 'Details',
  codeAufGithub: 'Code on GitHub',
  demoAnsehen: 'View demo',
  naechstesProjekt: 'Next project',
  zurueckZuProjekten: 'All projects',
  entwurf: 'Draft',
  datum: 'Date',
  rolle: 'Role',
  themen: 'Topics',
  verweise: 'Links',
  index: 'Index',
  stand: 'Updated',
  zuletzt: 'Latest',
  zeichen: 'Mark',
  titel: 'Title',
  name: 'Name',
  rechtliches: 'Legal',
  mailSchreiben: 'Send an email',
  bild: 'Image',
  lebenslauf: 'CV',
  cvHerunterladen: 'Download CV (PDF)',
  heute: 'present',
  impressum: 'Impressum',
  datenschutz: 'Privacy',
  nichtGefunden: 'Page not found',
  nichtGefundenText: 'This address does not exist on this site.',
  zurStartseite: 'Go to the home page',
  keineProjekte: 'No projects published yet.',
  // Link in der Kopfzeile zur selben Seite in der anderen Sprache. Er nennt
  // die Zielsprache in ihrer eigenen Sprache.
  sprachwechsel: 'Deutsch',
  lebenslaufVon: (name: string) => `CV of ${name}.`,
  alleProjekteVon: (name: string) => `All projects by ${name}.`,
};

const de: typeof en = {
  zumInhalt: 'Zum Inhalt springen',
  navigation: 'Navigation',
  kontakt: 'Kontakt',
  navProjekte: 'Projekte',
  navLebenslauf: 'Lebenslauf',
  navGithub: 'GitHub',
  navLinkedin: 'LinkedIn',
  navEmail: 'E-Mail',
  profil: 'Profil',
  projekte: 'Projekte',
  weitereInhalte: 'Weitere Inhalte',
  ausgewaehlteArbeiten: 'Ausgewählte Arbeiten',
  alleProjekteKurz: 'Alle Arbeiten, neueste zuerst',
  lebenslaufGanz: 'Ganzer Lebenslauf',
  lebenslaufPdf: 'PDF',
  foto: 'Foto',
  fotoVon: (name: string) => `Foto von ${name}`,
  alleProjekte: (anzahl: number) => `Alle Projekte (${anzahl})`,
  details: 'Details',
  codeAufGithub: 'Code auf GitHub',
  demoAnsehen: 'Demo ansehen',
  naechstesProjekt: 'Nächstes Projekt',
  zurueckZuProjekten: 'Alle Projekte',
  entwurf: 'Entwurf',
  datum: 'Datum',
  rolle: 'Rolle',
  themen: 'Themen',
  verweise: 'Links',
  index: 'Index',
  stand: 'Stand',
  zuletzt: 'Zuletzt',
  zeichen: 'Zeichen',
  titel: 'Titel',
  name: 'Name',
  rechtliches: 'Rechtliches',
  mailSchreiben: 'E-Mail schreiben',
  bild: 'Bild',
  lebenslauf: 'Lebenslauf',
  cvHerunterladen: 'Lebenslauf herunterladen (PDF)',
  heute: 'heute',
  impressum: 'Impressum',
  datenschutz: 'Datenschutz',
  nichtGefunden: 'Seite nicht gefunden',
  nichtGefundenText: 'Diese Adresse gibt es auf dieser Seite nicht.',
  zurStartseite: 'Zur Startseite',
  keineProjekte: 'Noch keine Projekte veröffentlicht.',
  sprachwechsel: 'English',
  lebenslaufVon: (name: string) => `Lebenslauf von ${name}.`,
  alleProjekteVon: (name: string) => `Alle Projekte von ${name}.`,
};

const texte: Record<Sprache, Texte> = { en, de };

export type Texte = typeof en;

export function t(sprache: Sprache): Texte {
  return texte[sprache];
}

/** Monat und Jahr, z. B. "Oktober 2025" oder "October 2025". */
export function monatJahr(monat: string, sprache: Sprache): string {
  const [jahr, monatsZahl] = monat.split('-').map(Number);
  if (!jahr || !monatsZahl) return monat;
  return new Intl.DateTimeFormat(sprache === 'de' ? 'de-DE' : 'en-GB', {
    month: 'long',
    year: 'numeric',
    timeZone: 'UTC',
  }).format(new Date(Date.UTC(jahr, monatsZahl - 1, 1)));
}

/** Zeitraum für den Lebenslauf, z. B. "Oktober 2025 – heute". */
export function zeitraum(von: string, bis: string | undefined, sprache: Sprache): string {
  const ende = bis ? monatJahr(bis, sprache) : t(sprache).heute;
  return `${monatJahr(von, sprache)} – ${ende}`;
}
