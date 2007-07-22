/***************************************************************************
 *   Copyright (C) 2006 by Matteo Franchin                                 *
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

#ifndef _PATHS_H
#  define _PATHS_H

#  include <stdio.h>

#  include "list.h"

extern List *libraries;
extern List *lib_dirs;
extern List *inc_dirs;
extern List *inc_exts;

void Path_Init(void);

void Path_Destroy(void);

void Path_Set_All_From_Env(void);

void Path_Add_Lib(char *lib);

void Path_Add_Lib_Dir(char *path);

void Path_Add_Inc_Dir(char *path);

FILE *Path_Open_Inc_File(const char *file, const char *mode);

#endif
