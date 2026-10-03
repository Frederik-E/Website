/**
 * Bereitet die Veroeffentlichung vor: prueft die Seite und zeigt, was noch
 * nicht auf GitHub liegt. Hochgeladen wird mit git push, den Rest erledigt
 * GitHub Actions (.github/workflows/veroeffentlichen.yml).
 *   npm run deploy
 */
import { spawnSync } from 'node:child_process';

console.log('Zuerst die Pruefung ...');
console.log('');

const pruefung = spawnSync('node', ['scripts/check.mjs'], { stdio: 'inherit' });

if (pruefung.status !== 0) {
  console.log('');
  console.log('Behebe zuerst die oben genannten Punkte. GitHub wuerde die Seite sonst ebenfalls nicht veroeffentlichen.');
  process.exit(1);
}

const git = (...argumente) => spawnSync('git', argumente, { encoding: 'utf8' }).stdout.trim();
const offen = git('status', '--short');

console.log('');
if (offen) {
  console.log('Die Pruefung ist bestanden. Diese Aenderungen sind noch nicht gespeichert:');
  console.log(offen);
  console.log('');
  console.log('Speichern und hochladen:');
  console.log('  git add -A');
  console.log('  git commit -m "Beschreibe kurz, was du geaendert hast"');
  console.log('  git push');
} else if (git('rev-list', '--count', '@{u}..HEAD') !== '0') {
  console.log('Die Pruefung ist bestanden. Es gibt Stand, der noch nicht hochgeladen ist:');
  console.log('  git push');
} else {
  console.log('Die Pruefung ist bestanden. Alles liegt schon auf GitHub.');
}
console.log('');
console.log('Nach dem Push baut GitHub die Seite. Den Fortschritt siehst du im Repository unter "Actions".');
