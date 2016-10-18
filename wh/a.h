/*  File: a.h
 *  Author: Jean Thierry-Mieg
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1998
 * -------------------------------------------------------------------
 * Acedb is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * or see the on-line version at http://www.gnu.org/copyleft/gpl.txt
 * -------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@crbm.cnrs-mop.fr
 *
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Dec  4 14:52 1998 (fw)
 * Created: Mon Nov 23 15:20:38 1998 (fw)
 *-------------------------------------------------------------------
 */

/* $Id: a.h,v 1.6 1999-09-01 11:01:10 fw Exp $ */

#ifndef DEFINE_A_h
#define DEFINE_A_h

/***************************************************************/
/*   classType 'A' : permanent Arrays, public functions.       */
/***************************************************************/
#include "acedb.h"

#define arrayGet(key,type,format)	uArrayGet(key,sizeof(type),format)
#define arrayHandleGet(key,type,f,h) uArrayHandleGet(key,sizeof(type), f,h)
Array  uArrayGet(KEY key, int size, char *format) ;  /* returns 0 if bad */
Array  uArrayHanleGet(KEY key, int size, char *format, STORE_HANDLE handle) ; 
void   arrayStore(KEY key, Array a, char *format) ;
void arrayKill(KEY key) ;      /* kills the lex entry */
Stack  stackGet(KEY key) ;     /* returns 0 if !isold(key) */
Stack  stackHandleGet(KEY key, STORE_HANDLE handle) ; 
void   stackStore(KEY key, Stack a) ;
BOOL  arrayStackHandleGet (KEY key, Array* aap, char *format, 
			   Stack *sp, STORE_HANDLE handle) ;
void arrayStackStore(KEY key, Array a, char *format, Stack s) ; 

/*
  Notes :
  Stacks and arrays must be destroyed explicitly.
  If you want to update, you should first lexlock(key), checking
  that it returns TRUE, and, when done, lexunlock, whether you save 
  or not.
*/

/***** other functions in asubs.c */

void aStatus(int *nrp, int *nwp);

#endif
/****************************************************************/



