import type { APIRoute } from 'astro';
import { BLOB, ZEICHEN_BREITE, ZEICHEN_HOEHE } from '../lib/zeichen';

// Dasselbe Zeichen wie auf der Seite, unbewegt.
export const GET: APIRoute = () => {
  const versatz = `translate(5 4) rotate(140 ${ZEICHEN_BREITE / 2} ${ZEICHEN_HOEHE / 2}) scale(0.94)`;
  const svg = `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" role="img" aria-label="Zeichen der Website">
  <rect width="64" height="64" fill="#fcf7ff"/>
  <g transform="translate(2 2)">
    <path d="${BLOB}" fill="#65afff"/>
    <g transform="${versatz}">
      <path d="${BLOB}" fill="#b98389"/>
    </g>
  </g>
</svg>
`;
  return new Response(svg, {
    headers: {
      'Content-Type': 'image/svg+xml',
      'Cache-Control': 'public, max-age=3600',
    },
  });
};
