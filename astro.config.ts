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

  markdown: {
    // Keine Syntaxfaerbung: sie brächte fremde Farben und eine dunkle Flaeche
    // in die Seite. Code-Schnipsel stehen schlicht in Geist Mono.
    syntaxHighlight: false,
  },

  integrations: [
    sitemap({
      // Rechtliche Seiten gehören nicht in die Sitemap.
      filter: (seite) => !/\/(impressum|datenschutz)\/$/.test(seite),
    }),
  ],

  // Schriften werden beim Build geladen und von der eigenen Domain ausgeliefert.
  // Geist: klare Groteskschrift, die bei grossen Groessen mit enger Laufweite
  // ruhig bleibt und bei kleinen gut lesbar ist.
  fonts: [
    // Aufrechte Schnitte: werden vorgeladen, weil sie auf jeder Seite stehen.
    {
      provider: fontProviders.fontsource(),
      name: 'Geist',
      cssVariable: '--font-sans',
      weights: [400, 500],
      styles: ['normal'],
      subsets: ['latin'],
      fallbacks: ['system-ui', '-apple-system', 'Segoe UI', 'sans-serif'],
    },
    // Kursiv getrennt, damit es nur geladen wird, wenn eine Seite es braucht.
    {
      provider: fontProviders.fontsource(),
      name: 'Geist',
      cssVariable: '--font-sans-kursiv',
      weights: [400],
      styles: ['italic'],
      subsets: ['latin'],
      fallbacks: ['system-ui', '-apple-system', 'Segoe UI', 'sans-serif'],
    },
    {
      provider: fontProviders.fontsource(),
      name: 'Geist Mono',
      cssVariable: '--font-mono',
      weights: [400],
      styles: ['normal'],
      subsets: ['latin'],
      fallbacks: ['ui-monospace', 'SFMono-Regular', 'Menlo', 'monospace'],
    },
  ],
});
