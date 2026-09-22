/**
 * Kleiner Server fuer den Ordner dist/. Er verhaelt sich wie Cloudflare Workers
 * mit statischen Dateien: /seite wird auf /seite/ umgeleitet, /seite/ liefert
 * /seite/index.html, und unbekannte Adressen bekommen 404.html mit Status 404.
 *
 * Absicht: keine Abhaengigkeit von "astro preview", das ab Astro 7 als
 * gemeinsamer Hintergrund-Dienst laeuft und sich schlecht steuern laesst.
 */
import { createServer } from 'node:http';
import { readFile, stat } from 'node:fs/promises';
import { extname, join, resolve, sep } from 'node:path';

const TYPEN = {
  '.html': 'text/html; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.js': 'text/javascript; charset=utf-8',
  '.mjs': 'text/javascript; charset=utf-8',
  '.json': 'application/json; charset=utf-8',
  '.xml': 'application/xml; charset=utf-8',
  '.txt': 'text/plain; charset=utf-8',
  '.svg': 'image/svg+xml',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.jpeg': 'image/jpeg',
  '.webp': 'image/webp',
  '.avif': 'image/avif',
  '.ico': 'image/x-icon',
  '.mp4': 'video/mp4',
  '.webm': 'video/webm',
  '.woff2': 'font/woff2',
  '.pdf': 'application/pdf',
};

async function datei(pfad) {
  try {
    const angaben = await stat(pfad);
    return angaben.isFile() ? pfad : null;
  } catch {
    return null;
  }
}

export async function statikServerStarten(wurzelOrdner = 'dist', port = 0) {
  const wurzel = resolve(wurzelOrdner);

  const server = createServer(async (anfrage, antwort) => {
    const adresse = new URL(anfrage.url ?? '/', 'http://localhost');
    let pfad = decodeURIComponent(adresse.pathname);

    const ziel = resolve(join(wurzel, pfad));
    if (ziel !== wurzel && !ziel.startsWith(wurzel + sep)) {
      antwort.writeHead(403).end('Verboten');
      return;
    }

    let gefunden = pfad.endsWith('/') ? await datei(join(ziel, 'index.html')) : await datei(ziel);

    // Ohne Schraegstrich am Ende auf die Fassung mit Schraegstrich umleiten.
    if (!gefunden && !pfad.endsWith('/') && extname(pfad) === '') {
      if (await datei(join(ziel, 'index.html'))) {
        antwort.writeHead(308, { Location: pfad + '/' + adresse.search }).end();
        return;
      }
    }

    if (!gefunden) {
      const seite404 = await datei(join(wurzel, '404.html'));
      if (seite404) {
        antwort.writeHead(404, { 'Content-Type': TYPEN['.html'] }).end(await readFile(seite404));
      } else {
        antwort.writeHead(404, { 'Content-Type': TYPEN['.txt'] }).end('Nicht gefunden');
      }
      return;
    }

    const typ = TYPEN[extname(gefunden).toLowerCase()] ?? 'application/octet-stream';
    antwort.writeHead(200, { 'Content-Type': typ }).end(await readFile(gefunden));
  });

  await new Promise((fertig, fehler) => {
    server.on('error', fehler);
    server.listen(port, '127.0.0.1', fertig);
  });

  const { port: echterPort } = server.address();

  return {
    basis: `http://localhost:${echterPort}`,
    beenden: () =>
      new Promise((fertig) => {
        server.closeAllConnections?.();
        server.close(() => fertig());
      }),
  };
}
