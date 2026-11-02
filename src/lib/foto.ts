/**
 * Das Foto aus inhalt/foto/. Liegen dort mehrere Bilder, gilt das erste in
 * alphabetischer Reihenfolge. Ohne Bild ist `foto` undefined und die Seite
 * zeigt einen Platzhalter.
 */
const bilder = import.meta.glob<{ default: ImageMetadata }>(
  '/inhalt/foto/*.{jpg,jpeg,png,webp,avif,JPG,JPEG,PNG,WEBP,AVIF}',
  { eager: true },
);

const erstes = Object.keys(bilder).sort()[0];

export const foto: ImageMetadata | undefined = erstes ? bilder[erstes]?.default : undefined;
