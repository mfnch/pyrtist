/* bitmap.h - Autore: Franchin Matteo - 28-31 luglio 2002
 *
 * Strutture degli header presenti  in un file di tipo bitmap
 *
 * NOTA:  gli specificatori

  __attribute__ ((packed))

 *  indicano che la struttura non deve essere allineata
 *  dal compilatore mentre cerca di ottimizzare il codice.
 */

/* Little endian representation for a two bytes integer */
typedef struct {
  unsigned char byte[2];
} LEUInt16;

/* Little endian representation for a four bytes integer */
typedef struct {
  unsigned char byte[4];
} LEUInt32;

struct bmp_file_header {
  LEUInt16 id;
  LEUInt32 size;
  LEUInt16 reserved1;
  LEUInt16 reserved2;
  LEUInt32 imgoffs;
} __attribute__ ((packed));

struct bmp_image_header {
  LEUInt32 headsize;
  LEUInt32 width;
  LEUInt32 height;
  LEUInt16 numplanes;
  LEUInt16 bitperpixel;
  LEUInt32 compression;
  LEUInt32 compsize;
  LEUInt32 horres;
  LEUInt32 vertres;
  LEUInt32 numcolors;
  LEUInt32 impcolors;
} __attribute__ ((packed));

/* Struttura degli elementi della color map (palette) */
struct bmp_color {
  unsigned char b;
  unsigned char g;
  unsigned char r;
  unsigned char reserved;
} __attribute__ ((packed));
