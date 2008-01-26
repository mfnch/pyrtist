/* bitmap.h - Autore: Franchin Matteo - 28-31 luglio 2002
 *
 * Strutture degli header presenti  in un file di tipo bitmap
 *
 * NOTA:  gli specificatori

  __attribute__ ((packed))

 *  indicano che la struttura non deve essere allineata
 *  dal compilatore mentre cerca di ottimizzare il codice.
 */

struct bmp_file_header {
  unsigned short id;
  unsigned long size;
  short reserved1;
  short reserved2;
  unsigned long imgoffs;
} __attribute__ ((packed));

struct bmp_image_header {
  unsigned long headsize;
  unsigned long width;
  unsigned long height;
  unsigned short numplanes;
  unsigned short bitperpixel;
  unsigned long compression;
  unsigned long compsize;
  unsigned long horres;
  unsigned long vertres;
  unsigned long numcolors;
  unsigned long impcolors;
} __attribute__ ((packed));

/* Struttura degli elementi della color map (palette) */
struct bmp_color {
  unsigned char b;
  unsigned char g;
  unsigned char r;
  unsigned char reserved;
} __attribute__ ((packed));
