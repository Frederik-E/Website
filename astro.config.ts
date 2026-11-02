import { readFileSync } from 'node:fs';
import { defineConfig, fontProviders } from 'astro/config';
import sitemap from '@astrojs/sitemap';
import { domainOhneSchraegstrich, parseEinstellungen } from './src/lib/einstellungen-schema';

// Die Domain steht in inhalt/einstellungen.yaml, damit alles an einem Ort liegt.
const einstellungen = parseEinstellungen(
  readFileSync(new URL('./inhalt/einstellungen.yaml', import.meta.url), 'utf8'),
);

export default defineConfig({
  site: domainOhneSchraegstrich(einstellungen.domain),
  trailingSlash: 'always',
  build: { format: 'directory' },

  // Zwei Sprachen: Deutsch unter /, Englisch unter /en/. Die Seiten unter
  // src/pages/en/ binden dieselben Ansichten ein wie die deutschen.
  i18n: {
    locales: ['de', 'en'],
    defaultLocale: 'de',
    routing: { prefixDefaultLocale: false },
  },

  markdown: {
    // Keine Syntaxfaerbung: sie brächte fremde Farben und eine dunkle Flaeche
    // in die Seite. Code-Schnipsel stehen schlicht in IBM Plex Mono.
    syntaxHighlight: false,
  },

  vite: {
    plugins: [
      {
        // Im Entwicklungsserver kennt Vite die Endungen der Quelltexte in
        // public/code/ nicht und schickt sie ohne Typ. Damit die Vorschau
        // sich verhaelt wie die Live-Seite (siehe public/_headers), wird der
        // Typ hier gesetzt.
        name: 'quelltexte-als-text',
        configureServer(server) {
          server.middlewares.use((anfrage, antwort, weiter) => {
            if (anfrage.url && /^\/code\/.*\.(hpp|cpp|h|c|py|ya?ml|md)(\?|$)/.test(anfrage.url)) {
              antwort.setHeader('Content-Type', 'text/plain; charset=utf-8');
            }
            weiter();
          });
        },
      },
    ],
  },

  integrations: [
    sitemap({
      // Rechtliche Seiten gehören nicht in die Sitemap, in keiner Sprache.
      filter: (seite) => !/\/(impressum|datenschutz)\/$/.test(seite),
      // Verknüpft jede Seite mit ihrer Fassung in der anderen Sprache.
      i18n: {
        defaultLocale: 'de',
        locales: { de: 'de-DE', en: 'en-GB' },
      },
    }),
  ],

  // Schriften werden beim Build geladen und von der eigenen Domain ausgeliefert.
  // Barlow: an DIN angelehnt, der Normschrift technischer Zeichnungen, mit
  // ruhigen, eindeutigen Ziffern. Etiketten in Barlow Semi Condensed.
  // Monospace nur noch fuer Codeschnipsel.
  fonts: [
    // Aufrechte Schnitte: werden vorgeladen, weil sie auf jeder Seite stehen.
    {
      provider: fontProviders.fontsource(),
      name: 'Barlow',
      cssVariable: '--font-sans',
      weights: [400, 500],
      styles: ['normal'],
      subsets: ['latin'],
      fallbacks: ['system-ui', '-apple-system', 'Segoe UI', 'sans-serif'],
    },
    // Kursiv getrennt, damit es nur geladen wird, wenn eine Seite es braucht.
    {
      provider: fontProviders.fontsource(),
      name: 'Barlow',
      cssVariable: '--font-sans-kursiv',
      weights: [400],
      styles: ['italic'],
      subsets: ['latin'],
      fallbacks: ['system-ui', '-apple-system', 'Segoe UI', 'sans-serif'],
    },
    // Etiketten, Nummern und Daten in Listen.
    {
      provider: fontProviders.fontsource(),
      name: 'Barlow Semi Condensed',
      cssVariable: '--font-etikett',
      weights: [500],
      styles: ['normal'],
      subsets: ['latin'],
      fallbacks: ['Barlow', 'Arial Narrow', 'system-ui', 'sans-serif'],
    },
    {
      provider: fontProviders.fontsource(),
      name: 'IBM Plex Mono',
      cssVariable: '--font-mono',
      weights: [400],
      styles: ['normal'],
      subsets: ['latin'],
      fallbacks: ['ui-monospace', 'SFMono-Regular', 'Menlo', 'monospace'],
    },
  ],
});
