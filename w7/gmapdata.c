/*  File: gmadata.c
 *  Author: Richard Durbin (rd@mrc-lmb.cam.ac.uk)
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
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: map data routines for gmap
 *              
 * Exported functions:
 *     logLikeMulti
 *     multiBoundCheck
 *     logLike2pt
 *     best2p
 *     logLikeLocus
 *     boundFind
 *     gjmMenu incomplete
 *              
 * HISTORY:
 * Last edited: Jan  6 18:04 2011 (edgrif)
 * Created: Sun Nov 22 23:49:55 1992 (rd)
 * CVS info:   $Id: gmapdata.c,v 1.5 2011-01-06 18:06:01 edgrif Exp $
 *-------------------------------------------------------------------
 */


#include <wh/keysetdisp.h>
#include <wh/heap.h>
#include <wh/bindex.h>
#include <wh/maputils.h>
#include <w7/gmap_.h>


extern void physGenesSummary (void) ;


/* The original....... */
/* #define DBN_X(z) ((((z)+1)*dbnMax + (NBIN-(z))*dbnMin)/(NBIN+1)) */

/* My version */
#define MY_DBN_X(X, MIN, MAX, BINS) \
  (  ( (((X)+1) * (MAX)) + (((BINS) - (X)) * (MIN)) )  /  ((BINS) + 1)   )


static void clearMapData (void) ;
static void gjmOptAll (void) ;
static void highlightDataless (void) ;
static void check2point (void) ;
static void positionIntervals (void) ;
void calcAll2pt (void) ;
static void geneSummary (void) ;
void rearrSummary (void) ;
static void shortSummary (void) ;



MENUOPT gjmMenu[] = {
  { clearMapData, "Clear Map Data" },
  { gjmOptAll, "Global max likelihood" },
  { highlightDataless, "Highlight unbound" },
  { check2point, "Check 2 point" },
  { positionIntervals, "Position Rearrangements" },
  { calcAll2pt, "Calc all 2point" },
  { geneSummary, "Gene Summary" },
  { rearrSummary, "Rearr Summary" },
  { shortSummary, "Short Summary" },
  { physGenesSummary, "Physical Genes Summary" },
  { 0, 0 }
} ;


static BOOL GMAP_DEBUG_G = FALSE ;
static KEYSET badData = 0 ;
static KEYSET newBadData = 0 ;



/****************************/
/*  multi-point calculation */
/****************************/
BOOL logLikeMulti (MAPCONTROL map, MULTIPTDATA data, KEY dataKey, float *ll)
{
  /* assumes single recombinant in locA, locB interval */
  int i = 0, n = arrayMax(data->loci) ;
  float y, z, denom, sum, least, most ;


  if (!getPos(map, arr(data->loci, 0, KEY), &y)
      || !getPos(map, arr(data->loci, n-1, KEY), &z)
      || y == z)
    return FALSE ;


  if (GMAP_DEBUG_G)
    printf("logLikeMulti()\ty: %f\tz: %f", y, z) ;
  
  *ll = 0 ;
  sum = 0 ;			/* number of recombinants */

  if (z > y)
    {
      denom = z - y ;

      if (GMAP_DEBUG_G)
	printf("\tdenom: %f\n", denom) ;

      while (i < n)
	{
	  least = y ;
	  most = y ;

	  if (GMAP_DEBUG_G)
	    printf("least: %f\tmost: %f\n", least, most) ;

	  while (++i < n && arr(data->counts,i-1,int) == 0)
	    {
	      if (getPos(map, arr(data->loci,i,KEY), &y))
		{
		  if (y < least)
		    least = y ;
		  else if (y > most)
		    most = y ;

		  if (GMAP_DEBUG_G)
		    {
		      KEY gene_key, public_key ;

		      gene_key = arr(data->loci, i, KEY) ;
		      public_key = geneGetPubKey(gene_key) ;

		      printf("key: %s", name(gene_key)) ;
		      printf(" (%s)", name(public_key)) ;
		      printf("\tleast: %f\tmost: %f\n", least, most) ;
		    }
		}
	    }

	  --i ;

	  if (sum)
	    {
	      if (GMAP_DEBUG_G)
		printf("\tsum: %f", sum) ;

	      if (least <= z)	/* order violates recombinants */
		{
		  if (newBadData)
		    keySetInsert (newBadData, dataKey) ; 

		  return FALSE ;
		}
	      else
		{
		  float tmp ;

		  tmp = sum * log((least - z) / denom) ;

		  if (GMAP_DEBUG_G)
		    printf("\tsum * log((least - z) / denom): %f\n", tmp) ;

		  *ll += sum * log((least - z) / denom) ;
		  sum = 0 ;
		}
	    }
	  z = most ;

	  while (++i < n)
	    {
	      sum += arr(data->counts,i-1,int) ;
	      if (getPos(map, arr(data->loci,i,KEY), &y))
		break ;
	    }
	}
    }
  else
    {
      denom = y - z ;

      while (i < n)
	{
	  least = y ;
	  most = y ;

	  while (++i < n && arr(data->counts,i-1,int) == 0)
	    if (getPos(map, arr(data->loci,i,KEY), &y))
	      {
		if (y < least)
		  least = y ;
		else if (y > most)
		  most = y ;
	      }
	  --i ;

	  if (sum)
	    {
	      if (most >= z)				    /* order violates recombinants */
		{
		  if (newBadData)
		    keySetInsert (newBadData, dataKey) ; 

		  return FALSE ;
		}
	      else
		{
		  *ll += sum * log((z - most) / denom) ;
		  sum = 0 ;
		}
	    }
	  z = least ;

	  while (++i < n)
	    {
	      sum += arr(data->counts,i-1,int) ;

	      if (getPos(map, arr(data->loci,i,KEY), &y))
		break ;
	    }
	}
    }

  return TRUE ;
}

/*******************/

void multiBoundCheck (MAPCONTROL map, KEY locus, KEY multikey, 
		      float *min, float *max, KEY *minLoc, KEY *maxLoc)
{
	/* assumes single recombinant in locA, locB interval */

  int i, n;
  float x, y ;
  STORE_HANDLE dataHandle = handleCreate();
  MULTIPTDATA multi;

  if (!getPos (map, locus, &x) ||
      !gMapGetMultiPtData(map, multikey, dataHandle, &multi))
    return;
  
  n = arrayMax(multi->loci);

  for (i = 0 ; i < n ; ++i)
    if (arr(multi->loci, i, KEY) == locus)
      break ;
  while (i < n)
    if (arr(multi->counts, i++, int))
      break ;
  while (i < n)
    if (getPos (map, arr(multi->loci, i++, KEY), &y))
      {
	if (y < x && y > *min)
	  { *min = y ; *minLoc = arr(multi->loci, i-1, KEY) ; }
	else if (y > x && y < *max)
	  { *max = y ; *maxLoc = arr(multi->loci, i-1, KEY) ; }
      }

  for (i = n-1 ; i >= 0 ; --i)
    if (arr(multi->loci, i, KEY) == locus)
      break ;
  while (--i >= 0)
    if (arr(multi->counts, i, int))
      break ;
  while (i >= 0)
    if (getPos (map, arr(multi->loci, i--, KEY), &y))
      {
	if (y < x && y > *min)
	  { *min = y ; *minLoc = arr(multi->loci, i+1, KEY) ; }
	else if (y > x && y < *max)
	  { *max = y ; *maxLoc = arr(multi->loci, i+1, KEY) ; }
      }

  handleDestroy(dataHandle);
}

/*******************/
/* 2pt calculation */
/*******************/
int logLike2ptDirectLimit = 0 ;
float logLike2pt (TWOPTDATA data, float dist)
{
  /* log probability of observed counts at given distance */

  float p = dist / 100.0 ; /* p(recombination) */
	/* if no interference 0.5 * (1 - exp(-0.02 * dist)) */
  float XY = (1-p) * (1-p) ;
  float W = 2 + XY ;		/* 3 - 2*p + p*p */
  float X = 1 - XY ;
  int n1 = data->n1;
  int n2 = data->n2;
  int n3 = data->n3;
  int n4 = data->n4;
  KEY type = data->type;

  if (p < 0.0001)  /* prevent log (x < 0) */
    {
      p = 0.0001 ;
      XY = (1-p) * (1-p) ;
      W = 2 + XY ;
      X = 1 - XY ;
    }

  if (p > 0.9999)  /* prevent log (x < 0) */
    {
      p = .9999 ;
      XY = (1-p) * (1-p) ;
      W = 2 + XY ;
      X = 1 - XY ;
    }
   
  /* segregation of recessives from xy/++ hermaphrodites */
  if (type == _Full)			/* WT X Y XY */
    return n1*log(W) + (n2+n3)*log(X) + n4*log(XY) ;
  else if (type == _One_recombinant)	/* WT X */
    return n1*log(W) + n2*log(X) ;
  else if (type == _Selected)		/* X XY */
    return n1*log(X) + n2*log(XY) ;
  else if (type == _One_all)		/* X ALL */
    return n1*log(X) + (n2-n1)*log(W+X+XY) ;
  else if (type == _Recs_all)		/* X Y ALL */
    return (n1+n2)*log(X) + (n3-n1-n2)*log(W+XY) ;
  /* segregration from xy+/++z  - z is linked recessive lethal */
  else if (type == _One_let)		/* X ALL */
    return n1*log(X) + (n2-n1)*log(2-X) ;
  /* recessive segregation from x+/+y hermaphrodites */
  else if (type == _Tested)		/* H X */
    return n1*log(2*p*(1-p)) + (n2-n1)*2*log(1-p) ;
  else if (type == _Selected_trans)	/* X XY */
    return n1*log(1-p*p) + n2*log(p*p) ;
  else if (type == _Backcross)		/* WT X Y XY */
    return (n1+n4)*log(1-p) + (n2+n3)*log(p) ;
  else if (type == _Back_one)		/* WT X */
    return n1*log(1-p) + n2*log(p) ;
  else if (type == _Sex_full)		/* WT X Y XY */
    return (n1+n4)*log(p) + (n2+n3)*log(1-p) ;
  else if (type == _Sex_one)		/* WT X */
    return n1*log(p) + n2*log(1-p) ;
  else if (type == _Sex_cis)		/* X ALL */
    return n1*log(p) + (n2-n1)*log(2-p) ;
  else if (type == _Dom_one)		/* WT nonWT */
    return n1*log(X) + n2*log(W+X+XY) ;
  else if (type == _Dom_selected)		/* WT X */
    return n1*log(X) + n2*log(XY) ;
  else if (type == _Dom_semi)		/* XD ALL */
    return n1*log(2*p*(1-p)) + (n2-n1)*log(4-2*p*(1-p)) ;
  else if (type == _Dom_let)		/* WT ALL */
    return n1*log(X) + (n2-n1)*log(W) ;
  else if (type == _Direct)		/* R T */
    return  (n2 > logLike2ptDirectLimit) ?  n1*log(p) + (n2-n1)*log(1-p) : 0 ; 
  else if (type == _Complex_mixed)	/* X ALL */
    return n1*log(X) + (n2-n1)*log(5-X) ;
  else if (type == _Lethal_tested)	/* N WT */
    return n1*log(X) + (n2-n1)*log(2-2*p) ;
  else if (type == _Tetrad)		/* PD NPD TT */
    return (n1 + 0.5*n3)*log(1-p) + (n2 + 0.5*n3)*log(p) ;
  else if (type == _Centromere_segregation) /* 1st 2nd */
    return 2*n1*log(1-p) + n2*log(p) ;
  else if (type == 0)		/* Gaussian N1 = mu, N2 = err (2sd) */
      {
	if (data->error)
	  {
	    p = (data->distance - dist) / (0.5 * data->error) ;
	    return -p*p/2 ;
	  }
	else
	  return 0 ;
      }
    else
      { messerror ("Unknown calculation type %s for 2point",
		   name (type)) ;
	return 0 ;
      }


}

/****************/

BOOL best2p (TWOPTDATA data, 
	     float *best, 
	     float *lo, 
	     float *hi)
{
  int n ;
  float p = 0, p22 = 0, p12 = 0 ;
  float testScore, x, x1, x2 ;
  int n1 = data->n1;
  int n2 = data->n2;
  int n3 = data->n3;
  int n4 = data->n4;
  KEY type = data->type;

  /* segregation of recessives from xy/++ hermaphrodites */
  if (type == _Full)			/* WT X Y XY */
    p22 = 2.0 * (n2+n3) / (float)(n1+n2+n3+n4) ;	
  else if (type == _One_recombinant)	/* WT X */
    p22 = 3.0 * n2 / (float)(n1+n2) ; 		
  else if (type == _Selected)		/* X XY */
    p22 = n1 / (float)(n1+n2) ; 			
  else if (type == _One_all)		/* X ALL */
    p22 = 4 * n1 / (float)n2 ; 			
  else if (type == _Recs_all)		/* X Y ALL */
    p22 = 2 * (n1+n2) / (float)n3 ; 			
  /* segregration from xy+/++z  - z is linked recessive lethal */
  else if (type == _One_let)		/* X ALL */
    p22 = n1 / (float)n2 ; 				
  /* recessive segregation from x+/+y hermaphrodites */
  else if (type == _Tested)		/* H X */
    p = n1 / (float)(2*n2 - n1) ; 			
  else if (type == _Selected_trans)	/* X XY */
    p = sqrt (n2 / (float)(n1+n2)) ; 			
  else if (type == _Backcross)		/* WT X Y XY */
    p = (n2+n3) / (float)(n1+n2+n3+n4) ; 		
  else if (type == _Back_one)		/* WT X */
    p = n2 / (float)(n1+n2) ; 			
  else if (type == _Sex_full)		/* WT X Y XY */
    p = (n1+n4) / (float)(n1+n2+n3+n4) ; 		
  else if (type == _Sex_one)		/* WT X */
    p = n1 / (float)(n1+n2) ; 			
  else if (type == _Sex_cis)		/* X ALL */
    p = 2 * n1 / (float)n2 ; 				
  else if (type == _Dom_one)		/* WT nonWT */
    p22 = 4 * n1 / (float)(n1 + n2) ;			
  else if (type == _Dom_selected)		/* WT X */
    p22 = n1 / (float)(n1 + n2) ;			
  else if (type == _Dom_semi)		/* XD ALL */
    p12 = 2 * n1 / (float)n2 ;			
  else if (type == _Dom_let)		/* WT ALL */
    p22 = 3 * n1 / (float)n2 ;			 
  else if (type == _Direct)		/* R T */
    p = n1 / (float)n2 ;				
  else if (type == _Complex_mixed)	/* X ALL */
    p22 = 5 * n1 / (float)n2 ;			
  else if (type == _Lethal_tested)	/* N WT */
    {
      x = n1 / (float)n2 ;
      p = 1 - sqrt (1 - 2*x*(1+x)) ;
    }
  else if (type == _Tetrad)		/* PD NPD TT */
    p = (n2 + 0.5*n3) / (float)(n1 + n2 + n3) ;	
  else if (type == _Centromere_segregation) /* 1st 2nd */
    p = n2 / (float)(2*n1 + n2) ;			
  else if (type == 0)			/* Gaussian N1 = mu, N2 = err */
    {
      *best = data->distance;
      *hi = (data->distance + data->error);
      *lo = (data->distance - data->error);
      if (*lo < 0) *lo = 0 ;
      return (data->error > 0) ;
    }
  else
    {
      messerror ("Unknown calculation type %s for 2point",
		 name (type)) ;
      return FALSE ;
    }
  
  /* solve quadratic eqn if necessary */
  if (p22)			/* p22 = 2p-p^2 */
    p = 1 - sqrt(1 - p22) ;
  else if (p12)			/* p12 = p-p^2 */
    p = (1 - sqrt(1 - 4*p12)) / 2 ;
  
  *best = 100 * p ;
				/* find *lo, *hi by splitting */
  testScore = logLike2pt (data, *best) - LOG_10 ;

  if (logLike2pt (data, 0) > testScore)
    *lo = 0 ;
  else
    { x1 = 0 ; x2 = *best ;
      for (n = 6 ; n-- ; )
	{ x = (x1 + x2) / 2 ;
	  if (logLike2pt (data, x) < testScore)
	    x1 = x ;
	  else
	    x2 = x ;
	}
      *lo = (x1 + x2) / 2 ;
    }
  
  if (!*best)
    { x1 = 0.01 ;
      if (logLike2pt (data, x1) > testScore)      
	{ *hi = x1 ;
	  return TRUE ;
	}
    }
  else
    x1 = *best ;
  x2 = 2*x1 ;
  while (x2 < 100.0 && logLike2pt (data, x2) > testScore) x2 *= 2 ;
  for (n = 6 ; n-- ; )
    { x = (x1 + x2) / 2 ;
      if (logLike2pt (data, x) < testScore)
	x2 = x ;
      else
	x1 = x ;
    }
  *hi = x ;

  return TRUE ;
}



/************************************************/
/************ likelihood stuff ******************/

float logLikeLocus (MAPCONTROL map, KEY locus)
{
  KEYSET data = 0;
  float ll = 0.0, bit = 0.0, pos, y1, y2 ;
  MULTIPTDATA mdata;
  TWOPTDATA tdata;
  STORE_HANDLE handle = handleCreate();
  int i, ip ;
  

  if (!getPos (map, locus, &pos))
    return ll ;
  
  if (gMapMultiPt(map, &data, locus))
    {
      for (i = keySetMax(data) ; i-- ;)
	{
	  if (!(badData && keySetFind (badData, keySet(data, i), &ip)) &&
	      gMapGetMultiPtData(map, keySet(data, i), handle, &mdata) &&
	      logLikeMulti(map, mdata, keySet(data, i), &bit))
	    {
	      ll += bit ;
	    }
	}
    }



  if (gMap2Pt(map, &data, locus))
    for (i = keySetMax(data) ; i-- ;)
      if (!(badData && keySetFind (badData, keySet(data, i), &ip)) &&
	  gMapGet2PtData(map, keySet(data, i), handle, &tdata) &&
	  getPos (map, tdata->loc1, &y1) && getPos (map, tdata->loc2, &y2))
	{
	  if (y2 > y1)
	    ll += logLike2pt (tdata, y2-y1) ;
	  else
	    ll += logLike2pt (tdata, y1-y2) ;
	}

  handleDestroy(handle);
  keySetDestroy(data);

  return ll ;
}

BOOL boundFind (MAPCONTROL map, KEY locus, float *min, float *max,
		KEY *minLoc, KEY *maxLoc)
{
  KEYSET in = 0, out = 0;
  float x, y, lx ;
  int i, j, ip = 0 ;
  GeneticMap look = (GeneticMap)(map->look);
  *min = -1000000 ; *max = 1000000 ;
  *minLoc = 0 ; *maxLoc = 0 ;
  if (!getPos (map, locus, &x))
    return FALSE ;

  if (gMapPositive(map, &in, locus))
    for (i = keySetMax(in) ; i-- ;)
      if (!(badData && keySetFind (badData, keySet(in, i), &ip)) &&
	  gMapNegative(map, &out, keySet(in,i)))
	for (j = keySetMax(out) ; j-- ;)
	  if (getPos (map, keySet(out,j), &y))
	    {
	      if (y < x && y > *min)
		{ *min = y ; *minLoc = keySet(out, j) ; }
	      else if (y > x && y < *max)
		{ *max = y ; *maxLoc = keySet(out, j) ; }
	    }

  if (gMapNegative(map, &out, locus))
    for (i = keySetMax(out) ; i-- ;)
      if (!(badData && keySetFind (badData, keySet(out, i), &ip)) &&
	  gMapPositive(map, &in, keySet(out,i)))
	for (j = keySetMax(in) ; j-- ;)
	  if (getPos (map, keySet(in,j), &y))
	    {
	      if (y < x && y > *min)
		{ *min = y ; *minLoc = keySet(in, j) ; }
	      else if (y > x && y < *max)
		{ *max = y ; *maxLoc = keySet(in, j) ; }
	    }

  if (gMapMultiPt(map, &in, locus))
    for (i = keySetMax(in) ; i-- ;)
      if (!(badData && keySetFind (badData, keySet(in, i), &ip)))
	multiBoundCheck (map, locus, keySet(in, i), min, max, minLoc, maxLoc) ;

  if (keySetFind (look->orderedLoci, locus, &ip))
    for (i = keySetMax(look->orderedLoci) ; i-- ;)
      if (keySet(look->orderedLoci,i) != locus &&
	  getPos(map, keySet(look->orderedLoci,i), &y))
	{
	  if (y < x && y > *min)
	    { *min = y ; *minLoc = keySet(look->orderedLoci,i) ; }
	  else if (y > x && y < *max)
	    { *max = y ; *maxLoc = keySet(look->orderedLoci,i) ; }
	}
  
  if (*min < -999999)
    { lx = logLikeLocus (map, locus) - 3 ;
      for (y = 0.1 ; y < 10 ; y *=2)
	{ setTestPos (map, locus, x-y) ;
	  if (logLikeLocus (map, locus) < lx)
	    break ;
	}
      if (logLikeLocus (map, locus))
	*min = x-y ;
      setTestPos (map, locus, x) ;
    }

  if (*max > 999999)
    { lx = logLikeLocus (map, locus) - 3 ;
      for (y = 0.1 ; y < 10 ; y *=2)
	{ setTestPos (map, locus, x+y) ;
	  if (logLikeLocus (map, locus) < lx)
	    break ;
	}
      if (logLikeLocus (map, locus))
	*max = x+y ;
      setTestPos (map, locus, x) ;
    }

  keySetDestroy(in);
  keySetDestroy(out);

  return (*min > -999999 && *max < 999999) ;
}

void gMapGetBadData (void)
{
  int i, t ;
  KEY key ;
  KEYSET active ;

  if (keySetExists (badData))
    keySetDestroy (badData) ;
  
  if (lexword2key("bad_map_data", &key, _VKeySet))
    badData = arrayGet(key, KEY, "k") ;

  if (keySetActive(&active, 0))
    {
      if (!badData)
	badData = keySetCreate () ;

      for (i = 0 ; i < keySetMax(active) ; ++i)
	{
	  t = class(arr(active,i,KEY)) ;
	  if (t == _V2_point_data ||
	      t == _VMulti_pt_data ||
	      t == _VInterval)
	    keySetInsert (badData, arr(active,i,KEY)) ;
	}

      if (!keySetMax (badData))
	keySetDestroy (badData) ;
    }

  newBadData = keySetReCreate(newBadData) ;

  return ;
}

void calcAll2pt (void)
{
  KEYSET kset = 0, done ;
  int i, j, n = 0 ;
  KEY key ;
  float best, lo, hi ;
  FILE *fil ;
  MAPCONTROL map = currentMapControl();
  GeneticMap look = currentGeneticMap("calcAll2pt");
  GMAPSEG *seg;
  TWOPTDATA data;
  STORE_HANDLE handle = handleCreate();

  if (!(fil = filqueryopen (0, 0, "ace", "a", "Output ace file:")))
    return ;
  done = keySetCreate () ;

  for (j = 0 ; j < arrayMax(look->segs); j++)
    { 
      seg = arrp(look->segs, j, GMAPSEG);

      if (seg->flag & FLAG_ANY_LOCUS)
	{
	  if (gMap2Pt(map, &kset, seg->key)) 
	    {
	      for (i = 0 ; i < keySetMax(kset) ; ++i)
		{
		  key = keySet(kset, i);

		  if (!keySetInsert (done, key) &&
		      gMapGet2PtData (map, key, handle,  &data) &&
		      best2p (data, &best, &lo, &hi))
		    { 
		      fprintf (fil, "2_point_data \"%s\"\n", name (key)) ;
		      fprintf (fil, "Calc_distance %.2f\n", best) ;
		      fprintf (fil, "Calc_lower_conf %.2f\n", lo) ;
		      fprintf (fil, "Calc_upper_conf %.2f\n", hi) ;
		      fprintf (fil, "\n") ;
		      ++n ;
		    }
		
		}
	    }
	}
    }

  filclose (fil) ;
  handleDestroy(handle);
  keySetDestroy(done);
  keySetDestroy(kset);

  return ;
}


/*****************************************************/


static void gjmOptAll (void)
{
  int i, j, ibestmin, n = 0, ibestmax ;
  GMAPSEG *seg ;
  float x, best, gain = 0, rms = 0, diff ;
  KEY minLoc, maxLoc ;
  /* This gets called from a header menu, hence currentMap will
     get us what we need */
  COLCONTROL control ;
  MAPCONTROL map ;
  GeneticMap look ;
  Heap heap ;
  Array heapIndex ;
  float dbnMin, dbnMax, dMax ;
  static Array dbn = NULL ;
  static float sig_fig = 2.0, accuracy = 0.0 ;		    /* Currently genetic map is to 2 s.f. */


  control = currentColControl("gjmOptAll");
  map = control->currentMap;
  look = currentGeneticMap("gjmOptAll");
  
  heap = heapCreate(10) ;
  heapIndex = arrayCreate(11, int) ;

  if (accuracy == 0.0)
    accuracy = powf(10.0, sig_fig) ;

  gMapGetBadData() ;

  for (j = 0 ; j < arrayMax(look->segs) ; ++j)
    {
      int bins ;
      float seed, range, incr ;

      seg = arrp(look->segs, j, GMAPSEG) ;

      if (!(seg->flag & FLAG_ANY_LOCUS)
	  || keySetFind(look->highlight, seg->key, 0)
	  || !boundFind(map, seg->key, &dbnMin, &dbnMax, &minLoc, &maxLoc))
	continue ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* debugging */
      {
	KEY public_key ;

	public_key = geneGetPubKey(seg->key) ;

	if (g_ascii_strcasecmp("ram-1", name(public_key)) == 0)
	  printf("found it\n") ;
      }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



      /* Calculate how many bins we need to give the required accuracy. */
      range = dbnMax - dbnMin ;
      if ((bins = (int)(range * accuracy)) < 10)
	bins = 10 ;
      incr = 1.0 ;

      
      dbn = arrayReCreate(dbn, bins, float) ;

      dMax = -1E20 ;

      for (seed = 0.0, i = 0 ; i < bins ; seed += incr, ++i)
	{
	  if (GMAP_DEBUG_G)
	    printf("Calculation %d, %f\ttestpos: %f\n", i, seed, MY_DBN_X(seed, dbnMin, dbnMax, bins)) ;

	  setTestPos(map, seg->key, MY_DBN_X(seed, dbnMin, dbnMax, bins)) ;

	  array(dbn, i, float) = logLikeLocus (map, seg->key) ;

	  if (array(dbn, i, float) > dMax)
	    dMax = array(dbn, i, float) ;
	}

      for (seed = 0.0, i = 0 ; i < bins ; seed += incr, ++i)
	{
	  array(dbn, i, float) = exp(array(dbn, i, float) - dMax) ;

	  if (GMAP_DEBUG_G)
	    printf("dbn: %d   %f   %f\n", i, array(dbn, i, float), MY_DBN_X(seed, dbnMin, dbnMax, bins)) ;
	}
      

      best = array(dbn, 0, float) ;
      ibestmin = ibestmax = 0 ;
      for (i = 1 ; i < bins ; ++i)
	{
	  if (array(dbn, i, float) >= best)
	    {
	      if (array(dbn, i, float) > best)
		{
		  best = array(dbn, i, float) ;
		  ibestmin = i ;
		}

	      ibestmax = i ;
	    }
	}


      best = 0.5 * (ibestmin + ibestmax) ;		    /* for if flat dbn */

      x = MY_DBN_X(best, dbnMin, dbnMax, bins) ;

      diff = (x - seg->x) ;
      if (diff < 0)
	diff = -diff ;

      rms += diff * diff ;

      gain += dMax - logLikeLocus(map, seg->key) ;

      if ((i = heapInsert(heap, diff)))
	array(heapIndex, i, int) = j ;

      ++n ;

      if (x != seg->x)
	{
	  seg->x = x ;
	  seg->flag |= FLAG_MOVED ;
	}

      setTestPos(map, seg->key, x) ; /* reset test position */


      for (i = ibestmin ; i > 0 ; --i)
	{
	  if (array(dbn, i, float) < 0.1)
	    break ;
	}
      dbnMin = MY_DBN_X(i, dbnMin, dbnMax, bins) ;


      for (i = ibestmax ; i < bins - 1 ; ++i)
	{
	  if (array(dbn, i, float) < 0.1)
	    break ;
	}
      dbnMax = MY_DBN_X(i, dbnMin, dbnMax, bins) ;

      diff = 0.5 * (dbnMax - dbnMin) ;
      if (diff != seg->dx * 2.0)
	{
	  seg->dx = diff / 2.0 ;
	  seg->flag |= FLAG_MOVED ;
	}

      if (messIsInterruptCalled())
	break ;
    }



  if (n)
    {
      char buf[2000] ; 

      sprintf(buf, "%d loci\n"
	      "RMS moverment %.3f\n"
	      "Average gain %.2f\n",
	      n, sqrt(rms/n), gain/n) ;

      while ((i = heapExtract(heap, &diff)))
	{
	  KEY public_name ;

	  seg = arrp(look->segs, arr(heapIndex, i, int), GMAPSEG) ;

	  public_name = geneGetPubKey(seg->key) ;

	  strcat(buf, messprintf("%s moved %f to %f\n", name(public_name), diff, seg->x)) ;
	}

      messout(buf) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* For debugging.... */
      messdump(buf) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }

  arraySort(look->segs, gMapOrder); /*  have to keep this sorted */
  controlDrawControl(control) ;

  if (newBadData && keySetMax (newBadData))
    {
      keySetNewDisplay(newBadData, "KeySet"); /* new window */
      newBadData = 0;
    }

  heapDestroy(heap) ;
  arrayDestroy(heapIndex) ;

  return ;
}

/**************/


static void clearMapData (void)
{
  GeneticMap look = currentGeneticMap("clearMapData");
  /* zero keys to these means "clear" */
  if (look->dbnCurrentColumn)
    dbnAddKey(look->dbnCurrentColumn, 0);
  if (look->twoPtCurrentColumn)
    twoPtAddKey(look->twoPtCurrentColumn, 0);
  if (look->multiPtCurrentColumn)
    multiPtAddKey(look->multiPtCurrentColumn, 0);
  controlDraw();
}

static void highlightDataless (void)
{
  int i ;
  GMAPSEG *seg ;
  KEY minLoc, maxLoc ;
  float dbnMin, dbnMax;
  MAPCONTROL map = currentMapControl();
  GeneticMap look = currentGeneticMap("highlightDataless");

  for (i = 0 ; i < arrayMax(look->segs) ; ++i)
    { seg = arrp(look->segs,i,GMAPSEG) ;
      if ((seg->flag & FLAG_ANY_LOCUS) &&
	  !boundFind (map, seg->key, &dbnMin, &dbnMax, &minLoc, &maxLoc))
	keySetInsert(look->highlight, seg->key);
    }
  controlDraw();

  return;
} /* highlightDataless */

static void check2point (void)
{
  KEY key = 0 ;
  TWOPTDATA data;
  MAPCONTROL map = currentMapControl();
  STORE_HANDLE handle = handleCreate();

  while (lexNext (_V2_point_data, &key))
    if (gMapGet2PtData(map, key, handle, &data) &&
	!logLike2pt (data, 1))
      fprintf (stderr,"Bad 2_point %s\n", name (key)) ;

  handleDestroy(handle);
}


static void positionIntervals (void)
{
  COLCONTROL control = currentColControl("positionIntervals");
  MAPCONTROL map = control->currentMap;
  GeneticMap look = currentGeneticMap("positionIntervals");
  KEY rearr ;
  Array in = 0, out = 0 ;
  int i, j ;
  float minIn, maxIn, maxBelow, minAbove, x, x1, x2 ;
  GMAPSEG *seg ;

  gMapGetBadData () ;

  for (j = 0; j < arrayMax(look->segs); j++)
    { 
      seg = arrayp(look->segs, j, GMAPSEG) ;
      rearr = seg->key;
      if (!(seg->flag & FLAG_ANY_INTERVAL))
	continue;
      if (!gMapPositive(map, &in, rearr))
	continue;
      
      minIn = 1000000 ;
      maxIn = -1000000 ;
      for (i = keySetMax(in) ; i-- ;)
	if (getPos (map, keySet(in,i), &x))
	  { if (x < minIn) minIn = x ;
	    if (x > maxIn) maxIn = x ;
	  }
      if (minIn > 999999)
	continue ;
      
      maxBelow = -1000000 ;
      minAbove = 1000000 ;
      if (gMapNegative(map, &out, rearr))
	for (i = keySetMax(out) ; i-- ;)
	  if (getPos (map, keySet(out,i), &x))
	    { if (x < minIn)
		{ if (x > maxBelow) maxBelow = x ; }
	    else if (x > maxIn) 
	      { if (x < minAbove) minAbove = x ; }
	    else
	      if (newBadData)
		keySetInsert (newBadData, rearr) ;
	    }
      
      if (maxBelow < -999999) 
	x1 = minIn - 0.01 ;
      else 
	x1 = 0.5 * (minIn + maxBelow) ;
      if (minAbove > 999999) 
	x2 = maxIn + 0.01 ;
      else 
	x2 = 0.5 * (maxIn + minAbove) ;

      if (seg->x != x1 || seg->dx*2.0 != x2-x1)
	{ 
	  seg->dx = (x2 - x1)/2.0 ;
	  seg->x = x1 + seg->dx;
	  seg->flag |= FLAG_MOVED ;
	}
    }
  
  arraySort(look->segs, gMapOrder); /* must keep segs in order */
  controlDrawControl(control);

  if (newBadData && keySetMax (newBadData))
    {
      keySetNewDisplay(newBadData, "KeySet"); /* new window */
      newBadData = 0;
    }

  keySetDestroy(in);
  keySetDestroy(out);
}


static void geneSummary (void)
{
  int i, j ;
  GMAPSEG *seg ;
  float x, best ;
  KEY minLoc, maxLoc, clone ;
  OBJ obj ;
  FILE *fil ;
  MAPCONTROL map = currentMapControl();
  GeneticMap look = currentGeneticMap("geneSummary");
  static float sig_fig = 2.0, accuracy = 0.0 ;
  float dbnMin, dbnMax, dMax ;
  static Array dbn = NULL ;


  if (accuracy == 0.0)
    accuracy = powf(10.0, sig_fig) ;


  if (!(fil = filqueryopen (0, 0, "out", "a", "Output file to add genes to")))
    return ;

  gMapGetBadData () ;

  for (j = 0 ; j < arrayMax(look->segs) ; ++j)
    {
      int bins ;
      float seed, range, incr ;

      seg = arrp(look->segs,j,GMAPSEG) ;

      if (!(seg->flag & FLAG_ANY_LOCUS) ||
	  keySetFind(look->highlight, seg->key, 0) ||
	  !boundFind (map, seg->key, &dbnMin, &dbnMax, &minLoc, &maxLoc) ||
	  !getPos(map, seg->key, &x))
	continue ;

      range = dbnMax - dbnMin ;
      if ((bins = (int)(range * accuracy)) < 10)
	bins = 10 ;
      incr = 1.0 ;

      dbn = arrayReCreate(dbn, bins, float) ;
      
      dMax = -1E20 ; ;

      for (seed = 0.0, i = 0 ; i < bins ; seed += incr, ++i)
	{
	  setTestPos(map, seg->key, MY_DBN_X(seed, dbnMin, dbnMax, bins)) ;

	  array(dbn, i, float) = logLikeLocus (map, seg->key) ;

	  if (array(dbn, i, float) > dMax)
	    dMax = array(dbn, i, float) ;
	}

      for (i = 0 ; i < bins ; ++i)
	{
	  array(dbn, i, float) = exp(array(dbn, i, float) - dMax) ;
	}

      
      setTestPos (map, seg->key, x) ;

      fprintf (fil,"%s	%s	%.2f	", name(seg->key), name(map->key), x) ;

      if (minLoc)
	fprintf (fil,"%s	", name(minLoc)) ;
      else
	fprintf (fil,"	") ;
      if (maxLoc)
	fprintf (fil,"%s	", name(maxLoc)) ;
      else
	fprintf (fil,"	") ;

      best = array(dbn, 0, float) ;
      for (i = 1 ; i < bins ; ++i)
	{
	  if (array(dbn, i, float) >= best)
	    best = array(dbn, i, float) ;
	}

      best -= LOG_10 ;

      if (minLoc && array(dbn, 0, float) > best)
	{
	  getPos (map, minLoc, &x) ;
          fprintf (fil,"%.2f	", x) ;
	}
      else
	{
	  for (i = 1 ; i < bins ; ++i)
	    {
	      if (array(dbn, i, float) > best)
		break ;
	    }

	  fprintf (fil,"%.2f*	", MY_DBN_X(seed, dbnMin, dbnMax, bins)) ;
	}


      if (maxLoc && array(dbn, bins - 1, float) > best)
	{
	  getPos (map, maxLoc, &x) ;
          fprintf (fil,"%.2f	", x) ;
	}
      else
	{
	  for (i = bins - 2 ; i >= 0 ; ++i)
	    if (array(dbn, i, float) > best)
	      break ;
	  fprintf (fil,"%.2f*	", MY_DBN_X(seed, dbnMin, dbnMax, bins)) ;
	}


      if (seg->flag & FLAG_CLONED && (obj = bsCreate (seg->key)))
	{
	  if (bsGetKey (obj, _Positive_clone, &clone))
	    fprintf (fil, "%s", name(clone)) ;
	  bsDestroy (obj) ;
	}

      fprintf (fil,"\n") ;
    }

  filclose (fil) ;

  return ;
}



void rearrSummary (void)
{
  KEY rearr ;
  Array in = 0, out = 0 ;
  KEY minLoc, maxLoc, maxBelowLoc, minAboveLoc ;
  int i, j ;
  float minIn, maxIn, maxBelow, minAbove, x, x1, x2 ;
  GMAPSEG *seg ;
  FILE *fil ;
  BOOL isBad ;
  MAPCONTROL map = currentMapControl();
  GeneticMap look = currentGeneticMap("rearrSummary");

  if (!(fil = filqueryopen (0, 0, "out", "a", "Output file to add rearr to")))
    return ;

  gMapGetBadData () ;

  for (j = 0 ; j < arrayMax(look->segs) ; ++j)
    { seg = arrp(look->segs,j,GMAPSEG) ;
      if (keySetFind(look->highlight, seg->key, 0) ||
	  !(seg->flag & FLAG_ANY_INTERVAL))
	continue ;

      rearr = seg->key ;
      if (!gMapPositive (map, &in, rearr))
	continue ;

      minIn = 1000000 ;
      maxIn = -1000000 ;
      for (i = keySetMax(in) ; i-- ;)
	if (getPos (map, keySet(in,i), &x))
	  { if (x < minIn) { minIn = x ; minLoc = keySet(in,i) ; }
	    if (x > maxIn) { maxIn = x ; maxLoc = keySet(in,i) ; }
	  }
      if (minIn > 999999)
	continue ;

      maxBelow = -1000000 ;
      minAbove = 1000000 ;
      maxBelowLoc = 0 ;
      minAboveLoc = 0 ;
      isBad = FALSE ;
      if (gMapNegative(map, &out, rearr))
	for (i = keySetMax(out) ; i-- ;)
	  if (getPos (map, keySet(out,i), &x))
	    {
	      if (x < minIn)
		{
		  if (x > maxBelow) 
		    { maxBelow = x ; maxBelowLoc = keySet(out,i) ; }
		}
	      else if (x > maxIn) 
		{
		  if (x < minAbove) 
		    { minAbove = x ; minAboveLoc = keySet(out,i) ; }
		}
	      else
		isBad = TRUE ;
	    }

      if (maxBelow < -999999) 
	x1 = minIn ;
      else 
	x1 = 0.5 * (minIn + maxBelow) ;
      if (minAbove > 999999) 
	x2 = maxIn ;
      else 
	x2 = 0.5 * (maxIn + minAbove) ;

      fprintf (fil, "%s	%s	%.2f	%.2f	",
	       name(seg->key), name(map->key), x1, x2) ;
      if (maxBelowLoc) 
	fprintf (fil, "%s", name(maxBelowLoc)) ;
      fprintf (fil, "	%s	%s	", name(minLoc), name(maxLoc)) ;
      if (minAboveLoc) 
	fprintf (fil, "%s", name(minAboveLoc)) ;
      if (isBad)
	fprintf (fil, "	*") ;
/*
	{ fprintf (fil, "	|") ;
	  for (i = keySetMax(out) ; i-- ;)
	    if (getPos (map, keySet(out,i), &x) && x > minIn && x < maxIn)
	      fprintf (fil, " %8s", name(keySet(out,i))) ;
	}
*/
      fprintf (fil, "\n") ;
    }

  filclose (fil) ;
  keySetDestroy(in);
  keySetDestroy(out);
}

static void shortSummary (void)
{
  int i ;
  float x ;
  GMAPSEG *seg ;
  FILE *fil ;
  MAPCONTROL map = currentMapControl();
  GeneticMap look = currentGeneticMap("shortSummary");

  if (!(fil = filqueryopen (0, 0, "out", "a", 
			    "Output file to add short to")))
    return ;

  for (i = 0 ; i < arrayMax(look->segs) ; ++i)
    { seg = arrp(look->segs,i,GMAPSEG) ;
      if (keySetFind(look->highlight, seg->key, 0) ||
	  !(seg->flag & FLAG_ANY_LOCUS))
	continue ;

      getPos (map, seg->key, &x) ;
      if (seg->flag & FLAG_PHYS_GENE)
	fprintf (fil, "%s	%s	%.2f	P\n", 
		 name(seg->key), name(map->key), x) ;
      else
	fprintf (fil, "%s	%s	%.2f\n", 
		 name(seg->key), name(map->key), x) ;
    }

  filclose (fil) ;
}

/*************** end of file **************/





 
 
