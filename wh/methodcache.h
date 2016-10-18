/*  File: methodcache.h
 *  Author: Fred Wobus (fw@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 1999
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
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: public header for of MethodCache class
 *              maintains a list of objects that represent
 *              object of class Method in memory.
 * HISTORY:
 * Last edited: Sep 23 16:45 1999 (fw)
 * Created: Thu Sep 23 10:40:39 1999 (fw)
 * CVS info:   $Id: methodcache.h,v 1.1 1999-09-23 15:45:21 fw Exp $
 *-------------------------------------------------------------------
 */


#ifndef ACEDB_METHODCACHE_H
#define ACEDB_METHODCACHE_H

#include "acedb.h"
#include "method.h"

/* only expose opaque handle to the outside */
typedef struct MethodCacheStruct *MethodCache;

MethodCache methodCacheCreate (STORE_HANDLE handle);
void methodCacheDestroy (MethodCache mcache);

/* get method from cache or init from DB-object
 * method stays within the cache and must not 
 * be destroyed by user-code - it is finalised by methodCacheDestroy */
METHOD *methodCacheGet (MethodCache mcache, 
			KEY methodKey, 
			char *aceDiff);
METHOD *methodCacheGetByName (MethodCache mcache,
			      char *methodName, 
			      char *aceText, char *aceDiff);

#endif /* !ACEDB_METHODCACHE_H */

/*************************** end of file *******************************/
