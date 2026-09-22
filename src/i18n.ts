import type { Sprache } from './lib/einstellungen-schema';

/**
 * Alle Oberflächentexte in beiden Sprachen. Die URLs bleiben gleich,
 * nur die Beschriftungen wechseln. Englisch legt die Form fest, Deutsch
 * muss dieselben Schlüssel haben.
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
  ueberMich: 'About me',
  projekte: 'Projects',
  weitereInhalte: 'More on this site',
  ausgewaehlteArbeiten: 'Selected work',
  alleProjekteKurz: 'Every project, newest first',
  ueberMichKurz: 'A short text about me',
  lebenslaufKurz: 'Education, experience and skills, also as a PDF',
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
  abbildung: 'Fig.',
  zeichen: 'Mark',
  nummer: 'No.',
  titel: 'Title',
  seite: 'Page',
  pfad: 'Path',
  name: 'Name',
  rechtliches: 'Legal',
  mailSchreiben: 'Send an email',
  video: 'Video',
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
  ueberMich: 'Über mich',
  projekte: 'Projekte',
  weitereInhalte: 'Weitere Inhalte',
  ausgewaehlteArbeiten: 'Ausgewählte Arbeiten',
  alleProjekteKurz: 'Alle Arbeiten, neueste zuerst',
  ueberMichKurz: 'Kurzer Text über mich',
  lebenslaufKurz: 'Stationen, Kenntnisse und PDF',
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
  abbildung: 'Abb.',
  zeichen: 'Zeichen',
  nummer: 'Nr.',
  titel: 'Titel',
  seite: 'Seite',
  pfad: 'Pfad',
  name: 'Name',
  rechtliches: 'Rechtliches',
  mailSchreiben: 'E-Mail schreiben',
  video: 'Video',
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
