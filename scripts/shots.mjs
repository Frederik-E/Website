/**
 * Screenshots aller Seiten bei 390px und 1440px Breite nach .screenshots/.
 * Nutzt einen laufenden Server auf Port 4321, sonst wird gebaut und ein
 * Vorschau-Server gestartet.
 */
import { mkdir, rm, writeFile } from 'node:fs/promises';
import { chromium } from 'playwright';
import { serverBereitstellen, seitenFinden, fremdeAnfrage } from './lib/server.mjs';

const BREITEN = [
  { name: '390', breite: 390, hoehe: 844 },
  { name: '1440', breite: 1440, hoehe: 900 },
];

const ordner = '.screenshots';

function dateiname(pfad) {
  if (pfad === '/') return 'start';
  return pfad.replace(/^\/|\/$/g, '').replaceAll('/', '-');
}

const { basis, beenden } = await serverBereitstellen();
const browser = await chromium.launch();
const fremde = new Set();

try {
  await rm(ordner, { recursive: true, force: true });
  await mkdir(ordner, { recursive: true });

  const seiten = await seitenFinden(basis);
  console.log(`Server: ${basis}`);
  console.log(`Seiten: ${seiten.join(', ')}`);

  for (const { name, breite, hoehe } of BREITEN) {
    const kontext = await browser.newContext({
      viewport: { width: breite, height: hoehe },
      deviceScaleFactor: 2,
      reducedMotion: 'no-preference',
    });
    kontext.on('request', (anfrage) => {
      const fremd = fremdeAnfrage(anfrage.url(), basis);
      if (fremd) fremde.add(fremd);
    });
    const seite = await kontext.newPage();

    for (const pfad of seiten) {
      await seite.goto(basis + pfad, { waitUntil: 'networkidle' });
      await seite.evaluate(() => document.fonts.ready);
      const datei = `${ordner}/${dateiname(pfad)}@${name}.png`;
      await seite.screenshot({ path: datei, fullPage: true });
      console.log(`  ${datei}`);

      // Zusätzlich der sichtbare Bereich der Startseite (beide Sprachen), um
      // zu prüfen, dass das erste Projekt-Medium ohne Scrollen beginnt.
      if ((pfad === '/' || pfad === '/en/') && breite === 1440) {
        const fenster = `${ordner}/${dateiname(pfad)}@1440-fenster.png`;
        await seite.screenshot({ path: fenster });
        console.log(`  ${fenster}`);
      }
    }
    await kontext.close();
  }

  if (fremde.size > 0) {
    const liste = [...fremde].sort();
    await writeFile(`${ordner}/fremde-anfragen.txt`, liste.join('\n') + '\n');
    console.log('\nFremde Anfragen gefunden:');
    for (const eintrag of liste) console.log(`  ${eintrag}`);
    process.exitCode = 1;
  } else {
    console.log('\nKeine Anfragen an fremde Domains.');
  }
} finally {
  await browser.close();
  await beenden();
}
