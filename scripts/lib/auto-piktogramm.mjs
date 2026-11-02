// Auto in Draufsicht als SVG-Gruppe, für die Abbildungen von cone-corridor.
//
// Mitte im Ursprung, Front zeigt nach +x. Außenmaß laenge × breite (Meter),
// von Rad zu Rad gemessen, also dieselbe Fläche wie vorher das Rechteck.
// massstab: Pixel je Meter. Die Karosserie bleibt ohne fuellung innen offen,
// damit Linien darunter sichtbar bleiben; die Räder sitzen außen an ihr.

const z = (n) => Number(n.toFixed(2));

export function autoPiktogramm({ laenge, breite, massstab, farbe, strich, fuellung = 'none' }) {
  const L = laenge * massstab;
  const B = breite * massstab;

  const koerperHalb = 0.36 * B; // halbe Breite der Karosserie
  const radL = 0.22 * L;
  const radB = B / 2 - koerperHalb + strich / 2;
  const radX = 0.3 * L; // Abstand der Achsen von der Mitte

  const raeder = [-1, 1]
    .flatMap((ax) => [-1, 1].map((seite) => [ax, seite]))
    .map(([ax, seite]) => {
      const x = ax * radX - radL / 2;
      const y = seite > 0 ? koerperHalb - strich / 2 : -B / 2;
      return `<rect x="${z(x)}" y="${z(y)}" width="${z(radL)}" height="${z(radB)}" rx="${z(radB / 3)}" fill="${farbe}"/>`;
    })
    .join('');

  const koerper = `<rect x="${z(-L / 2)}" y="${z(-koerperHalb)}" width="${z(L)}" height="${z(2 * koerperHalb)}" rx="${z(0.13 * L)}" fill="${fuellung}" stroke="${farbe}" stroke-width="${z(strich)}"/>`;

  // Scheiben als Trapeze: vorn die Windschutzscheibe (lange Motorhaube davor),
  // hinten die Heckscheibe. Die Kanten zum Dach sind schmaler.
  const trapez = (xAussen, hAussen, xInnen, hInnen) =>
    `<path d="M${z(xAussen)} ${z(-hAussen)} L${z(xAussen)} ${z(hAussen)} L${z(xInnen)} ${z(hInnen)} L${z(xInnen)} ${z(-hInnen)} Z" fill="${farbe}" stroke="${farbe}" stroke-width="${z(strich / 2)}" stroke-linejoin="round"/>`;
  const scheiben =
    trapez(0.24 * L, 0.3 * B, 0.07 * L, 0.24 * B) + trapez(-0.33 * L, 0.28 * B, -0.22 * L, 0.24 * B);

  return `<g>${raeder}${koerper}${scheiben}</g>`;
}
