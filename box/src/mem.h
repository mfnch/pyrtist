/***************************************************************************
 *   Copyright (C) 2007 by Matteo Franchin                                 *
 *   fnch@libero.it                                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/* $Id$ */

/** @file mem.h
 * @brief Wrappings for the system calls to allocate memory (malloc & co.).
 */

#ifndef _BOX_MEM_H
#  define _BOX_MEM_H

#  include <stdlib.h> /* defines size_t */
#  include "types.h"

/** Return the lowest multiple of sizeof(uint32_t) which is greater than n */
size_t BoxMem_Size_Align(size_t n);

void *BoxMem_Alloc(UInt size);

void *BoxMem_Realloc(void *ptr, UInt size);

void BoxMem_Free(void *ptr);

char *BoxMem_Strdup(const char *s);

void BoxMem_Exit(const char *msg);

#endif
