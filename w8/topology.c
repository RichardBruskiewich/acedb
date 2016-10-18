/*  File: topology.c
 *  Author: Jean Thierry-Mieg (mieg@mrc-lmba.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
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
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmba.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@frmop11.bitnet
 *
 * Description:
 * Exported functions:
    topoConnectedComponents()    
 * HISTORY:
 * Last edited: Mar  6 17:11 1996 (srk)
 * Created: Fri Oct 30 19:08:43 1992 (mieg)
 *-------------------------------------------------------------------
 */

/*  $Id: topology.c,v 1.6 1999-09-01 10:57:41 fw Exp $ */
 
#include "acedb.h"
#include "keyset.h"
#include "topology.h"

/********************************/

static int topoFirstKeyOrder(void*a, void*b)
{
 return ((LINK*) a) -> a -  ((LINK*) b)-> a ;
}

/**************************************/

static void topoDoubleUnorientedLinks(Array links)
{
  register int i = arrayMax(links) ; 
  register LINK *x , *y ;

  arrayExtend(links, 2*i) ;
  arrayMax(links) = 2*i ;
  x = arrp(links, 0, LINK) ;
  y = arrp(links, i, LINK) ;
  while(i--)
    { x -> group = y -> group = 0 ;
      *y = *x ;  /* anything else, i.e. type */
      y -> a = x -> b ; 
      y -> b = x -> a ;
      x++ , y++ ;
    }
  arraySort(links, topoFirstKeyOrder) ;
}

/**************************************/

static void topoEnumerateVertex(Array links, Array vx)
{
  register int i , n = arrayMax(links) , j = 0;
  register KEY k = 0 ;

  arrayMax(vx) = 0 ;
  for(i = 0 ; i < n ; i++)
    if ( k != arrp(links,i,LINK) -> a)
      {  k = arrp(links,i,LINK) -> a ;
	 array(vx, j, VERTEX).a = k ;
	 array(vx, j, VERTEX).group = 0 ;
	 array(vx, j, VERTEX).pos = i ;
	 j++ ;
       }
}

/**************************************/

static Array VX, LK;
static int GP ;      /* number of the connected component */

/******************/

static int topoFindVertex(KEY k)
{
  register int i = arrayMax(VX) ;
  while(i--)
    if(arrp(VX,i,VERTEX)->a == k)
      return i ;
  messcrash("Topofindvertex cannot find %d", k) ;

  return 0 ; /* for compiler happiness */
}

/******************/

static void topoNCC2(int n)
{ KEY a = array(VX,n,VERTEX).a ;
  int pos = array(VX,n,VERTEX).pos ;

  array(VX,n,VERTEX).group = GP ;
  for(;pos < arrayMax(LK)  && array(LK, pos,LINK).a == a;pos++)
    { n = topoFindVertex(array(LK, pos,LINK).b) ;
      if(!arr(VX,n,VERTEX).group)
	  topoNCC2(n) ;
      else if(arr(VX,n,VERTEX).group != GP )
	messcrash
	  ("topoNCC2 : Double numbering, sorry") ;
    }
}

/******************/

static void topoNumberLinks(Array links)
{ KEY a ;
  register int i = arrayMax(links), n ;
  while(i--)
    {  a = arr(links,i,LINK).a ;
       n = topoFindVertex(a) ;
       arr(links,i,LINK).group = arr(VX,n,VERTEX).group ;
     }
}

/*****************/

static int topoNumberConnectedComponents(Array links, Array vx)
{
  register int n = arrayMax(vx) ;

  VX = vx ; LK = links; GP = 0 ;
  while(n--)
    if(! arr(vx,n,VERTEX).group)
      { GP++ ;
	topoNCC2(n) ;
      }
  return GP ;
}

/**************************************************************/
/************* Public Routines ********************************/

int topoConnectedComponents(Array links, Array vx)
{ Array cpLinks = arrayCopy(links) ;
  int i ;

  if(!arrayExists(links) || !arrayExists(vx))
    messcrash("topoConnectedComponents called with bad array") ;

  topoDoubleUnorientedLinks(cpLinks) ;
  topoEnumerateVertex(cpLinks, vx) ;
  i = topoNumberConnectedComponents(cpLinks, vx) ;
  arrayDestroy(cpLinks) ;
  topoNumberLinks(links) ;
  return i ;
}

/**************************************************************/
/**************************************************************/
