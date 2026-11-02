import { z } from 'astro/zod';

/**
 * Monatsangabe im Format JJJJ-MM. YAML liefert je nach Schreibweise einen Text,
 * eine Zahl oder ein Datum; hier wird daraus immer "JJJJ-MM". Leerzeichen um
 * den Bindestrich sind erlaubt.
 */
export const monatSchema = (feld: string) =>
  z
    .union([z.string(), z.number(), z.date()])
    .transform((wert) => {
      if (wert instanceof Date) {
        return `${wert.getUTCFullYear()}-${String(wert.getUTCMonth() + 1).padStart(2, '0')}`;
      }
      // "2020 - 10" ist eindeutig gemeint, die Leerzeichen fallen weg.
      return String(wert).trim().replace(/\s*-\s*/, '-');
    })
    .refine((wert) => /^\d{4}-(0[1-9]|1[0-2])$/.test(wert), {
      error: `Feld "${feld}" braucht das Format JJJJ-MM, zum Beispiel 2025-10.`,
    });
