/****************************************************************************
 * Copyright (C) 2008 by Matteo Franchin                                    *
 *                                                                          *
 * This file is part of Box.                                                *
 *                                                                          *
 *   Box is free software: you can redistribute it and/or modify it         *
 *   under the terms of the GNU Lesser General Public License as published  *
 *   by the Free Software Foundation, either version 3 of the License, or   *
 *   (at your option) any later version.                                    *
 *                                                                          *
 *   Box is distributed in the hope that it will be useful,                 *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Lesser General Public License for more details.                    *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with Box.  If not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/

/* Strutture degli header presenti  in un file di tipo bitmap
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
