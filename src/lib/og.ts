import { getImage } from 'astro:assets';

/** Vorschaubild fuer geteilte Links, immer 1200x630. */
export async function ogAdresse(
  bild: ImageMetadata | undefined,
  basis: URL | undefined,
): Promise<string | undefined> {
  if (!bild) return undefined;
  const optimiert = await getImage({
    src: bild,
    width: 1200,
    height: 630,
    fit: 'cover',
    format: 'jpeg',
  });
  return basis ? new URL(optimiert.src, basis).href : optimiert.src;
}
