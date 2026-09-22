import rohtext from '../../inhalt/einstellungen.yaml?raw';
import { parseEinstellungen } from './einstellungen-schema';

/** Die Einstellungen aus `inhalt/einstellungen.yaml`. */
export const einstellungen = parseEinstellungen(rohtext);
