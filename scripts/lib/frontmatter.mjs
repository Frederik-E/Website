/** Kleine Helfer fuer die Kopfdaten (Frontmatter) einer Markdown-Datei. */

export function teileAuf(text) {
  const treffer = /^---\r?\n([\s\S]*?)\r?\n---\r?\n?/.exec(text);
  if (!treffer) {
    throw new Error('Die Datei hat keine Kopfdaten zwischen --- und ---.');
  }
  return { kopf: treffer[1], rest: text.slice(treffer[0].length) };
}

/** Setzt ein Feld in den Kopfdaten, auch wenn es bisher auskommentiert war. */
export function setzeFeld(text, feld, wert) {
  const { kopf, rest } = teileAuf(text);
  const zeilen = kopf.split('\n');
  const muster = new RegExp(`^#?\\s*${feld}\\s*:`);
  const stelle = zeilen.findIndex((zeile) => muster.test(zeile));
  const neu = `${feld}: ${wert}`;
  if (stelle >= 0) zeilen[stelle] = neu;
  else zeilen.push(neu);
  return `---\n${zeilen.join('\n')}\n---\n${rest}`;
}

export function hatFeld(text, feld) {
  const { kopf } = teileAuf(text);
  return new RegExp(`^${feld}\\s*:\\s*\\S`, 'm').test(kopf);
}
