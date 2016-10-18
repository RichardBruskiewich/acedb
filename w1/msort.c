/*  File: msort.c
 *  Author: Jean Thierry-Mieg (mieg@ncbi.nlm.nih.gov)
 *  Copyright (C) J Thierry-Mieg, 2002
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
 * 	Richard Durbin (MRC LMB, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@crbm.cnrs-mop.fr
 *
 * Description:
 *      This file implements the classic sort/merge algorithm.
 *      
 *      This file is adapted from the GNU msort 
 *      written by Mike Haertel, September 1988.
 *      I got from him the idea of copying only
 *      (n - n2) bytes back in b, and added the
 *      already sorted case short cut.
 *      The rest of the code is standard.
 *	
 * Exported functions:
 *       mSort () with the same prototype as unix qsort()
 * HISTORY:
 * Last edited:
 * Created: Aug 6 2002 (mieg)
 *-------------------------------------------------------------------
 */

/* $Id: msort.c,v 1.1 2002-12-03 17:58:17 mieg Exp $ */

#include "regular.h"

static void mDoSortInt (int *b, int n, int (*cmp)(), int *buf)
{
  int *b0 = buf, *b1, *b2 ;
  int  n1, n2 ;

  n1 = n / 2 ; n2 = n - n1 ;
  b1 = b ; b2 = b + n1 ;

  if (n1 > 1) mDoSortInt (b1, n1, cmp, buf);
  if (n2 > 1) mDoSortInt (b2, n2, cmp, buf);

  if ((*cmp) (b2 - 1, b2) <= 0) /* already sorted case */
    return ;

  while (n1 > 0 && n2 > 0)
    if ((*cmp) (b1, b2) <= 0)
      { n1-- ; *b0++  = *b1++ ; }
    else
      { n2-- ; *b0++  = *b2++ ; }

  if (n1 > 0)
    memcpy (b0, b1, n1 * sizeof (int)) ;
  memcpy (b, buf, (n - n2) * sizeof (int)) ;
}

/***********************************************************/

static void mDoSort (char *b, int n, int s, int (*cmp)(), char *buf)
{
  char *b0 = buf, *b1, *b2 ;
  int  n1, n2 ;

  n1 = n / 2 ; n2 = n - n1 ;
  b1 = b ; b2 = b + (n1 * s);

  if (n1 > 1) mDoSort (b1, n1, s, cmp, buf);
  if (n2 > 1) mDoSort (b2, n2, s, cmp, buf);

  if ((*cmp) (b2 - s, b2) <= 0) /* already sorted case */
    return ;

  while (n1 > 0 && n2 > 0)
    {
      if ((*cmp) (b1, b2) <= 0)
	{
	  memcpy (b0, b1, s) ;
	  n1-- ; b1 += s ; b0 += s ;
	}
      else
	{
	  memcpy (b0, b2, s) ;
	  n2-- ; b2 += s ; b0 += s ;
	}
    }

  if (n1 > 0)
    memcpy (b0, b1, n1 * s) ;
  memcpy (b, buf, (n - n2) * s) ;
}

/***********************************************************/

void mSort (void *b, int n, int s, int (*cmp)())
{ 
  int size = n * s, s_of_i = sizeof (int) ;
  char  sBuf [4*1024] ;
  char *mBuf = 0, *buf ;

  if (n < 2)
    return ;

  /* If size is small, use the stack, else malloc */
  if (size < 4*1024)
    buf = sBuf ;
  else
    buf = mBuf = messalloc (size) ;

  if (s == s_of_i &&   /* aligned integers.  Use direct int pointers.  */
      (((char *)b - (char *) 0) % s_of_i) == 0)
    mDoSortInt ((int*)b, n, cmp, (int*)buf) ;
  else
    mDoSort (b, n, s, cmp, buf) ;

  messfree (mBuf) ;
}

/***********************************************************/
/***********************************************************/

