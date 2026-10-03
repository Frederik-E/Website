import type { APIRoute } from 'astro';
import { punktmatrix } from '../lib/zeichen';

// Die kleine Punktmatrix als Standbild: ein dunkler Startpunkt, darum ein
// blauer Ring und weiter außen ein rosa Ring.
const QUELLE = { spalte: 1, zeile: 1 };
const RINGE = [
  { radius: 1.1, breite: 0.45, farbe: '#65afff' },
  { radius: 2.15, breite: 0.4, farbe: '#b98389' },
];

export const GET: APIRoute = () => {
  const m = punktmatrix('klein');
  const kreise = m.punkte
    .map((p) => {
      const abstand = Math.hypot(p.spalte - QUELLE.spalte, p.zeile - QUELLE.zeile);
      if (abstand === 0) return `<circle cx="${p.x}" cy="${p.y}" r="${m.radiusWelle * 0.75}" fill="#394648"/>`;
      const ring = RINGE.find((r) => Math.abs(abstand - r.radius) <= r.breite);
      return ring
        ? `<circle cx="${p.x}" cy="${p.y}" r="${m.radiusWelle}" fill="${ring.farbe}"/>`
        : `<circle cx="${p.x}" cy="${p.y}" r="${m.radiusPunkt}" fill="#a3a8ad"/>`;
    })
    .join('');
  const svg = `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${m.kante} ${m.kante}" role="img" aria-label="Zeichen der Website">
  <rect width="${m.kante}" height="${m.kante}" fill="#fcf7ff"/>
  ${kreise}
</svg>
`;
  return new Response(svg, {
    headers: {
      'Content-Type': 'image/svg+xml',
      'Cache-Control': 'public, max-age=3600',
    },
  });
};
