import type { APIRoute } from 'astro';

export const GET: APIRoute = ({ site }) => {
  const zeilen = [
    'User-agent: *',
    'Allow: /',
    'Disallow: /impressum/',
    'Disallow: /datenschutz/',
    'Disallow: /en/impressum/',
    'Disallow: /en/datenschutz/',
  ];
  if (site) zeilen.push('', `Sitemap: ${new URL('sitemap-index.xml', site).href}`);
  return new Response(zeilen.join('\n') + '\n', {
    headers: { 'Content-Type': 'text/plain; charset=utf-8' },
  });
};
