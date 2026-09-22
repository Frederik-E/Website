/**
 * Veroeffentlicht die Seite bei Cloudflare, aber nur wenn npm run check
 * fehlerfrei durchlaeuft.
 *   npm run deploy
 *   npm run deploy -- --dry-run    (alles ausser dem Hochladen)
 */
import { spawnSync } from 'node:child_process';

const weitereArgumente = process.argv.slice(2);

console.log('Zuerst die Pruefung ...');
console.log('');

const pruefung = spawnSync('node', ['scripts/check.mjs'], { stdio: 'inherit' });

if (pruefung.status !== 0) {
  console.log('');
  console.log('Es wurde nichts hochgeladen. Behebe zuerst die oben genannten Punkte.');
  process.exit(1);
}

console.log('');
console.log('Die Seite wird zu Cloudflare hochgeladen ...');

const hochladen = spawnSync('npx', ['wrangler', 'deploy', ...weitereArgumente], { stdio: 'inherit' });

if (hochladen.status !== 0) {
  console.log('');
  console.log('Das Hochladen hat nicht geklappt.');
  console.log('Bist du angemeldet? Dann fuehre einmal aus:  npx wrangler login');
  process.exit(1);
}
