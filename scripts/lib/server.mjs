/** Gemeinsame Helfer fuer die Skripte: Server starten, Seiten finden, Anfragen pruefen. */
import { spawn } from 'node:child_process';
import { statikServerStarten } from './statik-server.mjs';

/** Feste Seiten der Website. Projektseiten kommen aus /projects/ dazu. */
const GRUNDSEITEN = ['/', '/projects/', '/about/', '/cv/', '/impressum/', '/datenschutz/', '/404/', '/404'];

async function erreichbar(adresse, zeitlimit = 1500) {
  try {
    const antwort = await fetch(adresse, { signal: AbortSignal.timeout(zeitlimit) });
    return antwort.ok;
  } catch {
    return false;
  }
}

function laufen(befehl, argumente) {
  return new Promise((erfuellen, ablehnen) => {
    const prozess = spawn(befehl, argumente, { stdio: 'inherit' });
    prozess.on('error', ablehnen);
    prozess.on('exit', (code) =>
      code === 0 ? erfuellen() : ablehnen(new Error(`${befehl} ${argumente.join(' ')} endete mit Code ${code}`)),
    );
  });
}

/** Baut die Seite nach dist/. */
export async function bauen() {
  await laufen('npx', ['astro', 'build']);
}

/**
 * Baut die Seite und liefert einen eigenen Server dafuer.
 *
 * Absichtlich wird kein laufender Dev-Server mitbenutzt: dessen HMR laesst
 * veraltetes CSS und veraltete Skripte in der Seite stehen, und die Screenshots
 * zeigen dann etwas anderes als das, was veroeffentlicht wird.
 */
export async function serverBereitstellen({ ohneBauen = false } = {}) {
  if (!ohneBauen) await bauen();
  return statikServerStarten('dist', 0);
}

/** Alle erreichbaren Seiten, inklusive der Projekt-Detailseiten. */
export async function seitenFinden(basis) {
  const gefunden = new Set();

  for (const pfad of GRUNDSEITEN) {
    if (pfad === '/404' && gefunden.has('/404/')) continue;
    if (await erreichbar(basis + pfad, 5000)) gefunden.add(pfad);
  }

  // Frei angelegte Seiten stehen in der Kopfzeile der Startseite.
  const startseite = await (await fetch(basis + '/')).text();
  for (const treffer of startseite.matchAll(/href="(\/[a-z0-9-]+\/)"/g)) {
    if (await erreichbar(basis + treffer[1], 5000)) gefunden.add(treffer[1]);
  }

  if (gefunden.has('/projects/')) {
    const html = await (await fetch(basis + '/projects/')).text();
    for (const treffer of html.matchAll(/href="(\/projects\/[^"/]+\/)"/g)) {
      gefunden.add(treffer[1]);
    }
  }

  return [...gefunden];
}

/**
 * Gibt den Hostnamen zurueck, wenn eine Anfrage an eine fremde Domain geht,
 * sonst null. Eigene Adressen, data: und blob: zaehlen nicht.
 */
export function fremdeAnfrage(adresse, basis) {
  if (!/^https?:/i.test(adresse)) return null;
  try {
    const ziel = new URL(adresse);
    const eigen = new URL(basis);
    if (ziel.host === eigen.host) return null;
    return ziel.host;
  } catch {
    return null;
  }
}
