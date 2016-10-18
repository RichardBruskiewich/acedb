/*  File: objcache.c
 *  Author: Jean Thierry-Mieg (mieg@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1991
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
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Nov 26 16:01 2007 (edgrif)
 * * Nov 17 15:19 2000 (edgrif): Add cacheUnlock function, needed in case
 *              where cache entry has been locked for update but is then not
 *              altered and needs to be unlocked prior to destruction of entry.
 * * Sep 15 15:57 1998 (edgrif): Fix missing ';' before chronoReturn in cacheKOadd
 * * -----NO USER RECORDED FOR BELOW CHANGE---------
 *	-	resolved WIN32 cache crash by fix at c.a. line 521 (see comment)
 * * Jul 12 12:25 1996 (srk)
 * * Nov  6 18:26 1991 (mieg): added READING_MODELS protection
 * Created: Wed Nov  6 18:25:34 1991 (mieg)
 * CVS info:   $Id: objcache.c,v 1.34 2007-11-26 17:14:01 edgrif Exp $
 *-------------------------------------------------------------------
 */

/***************************************************************/
/**  File objcache.c :                                         **/
/**  Handles the secondary cache of the ACeDB program.        **/
/***************************************************************/
/***************************************************************/
/*                                                             */
/*  ? routines are public :                                    */
/*          create/destroy, update/save, acedump               */
/*    Get/Release, Lock/Unlock, ForceFree                      */

/*    The secondary cache is a linked list of up to            */
/*  MAXKNOWNOBJ  unpinned objects.                             */
/*                                                             */
/* The cache has two piles controlled by Get and Lock/Unlock   */
/*   1 : the "knownCache", a LIFO stack, on top of which each  */
/* cacheEntry is put when "get" (implying a load the first time*/
/* or a pop, or a new). These cacheEntries are lost, LIFO way, */
/* when the cache is full, automatically.                      */
/*   2 : the "Locked CacheEntries". For counting purpose       */
/* This pile is controlled explicitely by Lock/Unlock          */

/* cacheEntrySave saves one marked cacheEntry to disk.         */
/*                                                             */
/* cacheStatus gives the status.                               */
/*                                                             */
/* The Primary Cache handling is done in the separate file     */
/* blocksub.c invoked only by the static routines load/unload. */
/*                                                             */
/*         R.Durbin & J.Thierry-Mieg.                          */
/*                    last modified  7/3/1991 by JTM.          */
/*                                                             */
/***************************************************************/

#include <wh/acedb.h>
#include <wh/cache_.h>
#include <wh/cache.h>					    /* our public header. */
#include <wh/bstree.h>
#include <wh/bs.h>
#include <wh/lex_bl_.h>					    /* for q->cache */
#include <wh/lex.h>
#include <wh/pick.h>
#include <wh/chrono.h>

/*****************************************************************/

static CACHE cacheNew(KEY key) ;
static CACHE cacheCopy(CACHE cache);
static void cacheDestruct(CACHE v);
static void cachePop(CACHE v);
static void cacheKOadd(CACHE v);
static void cacheKOremove(CACHE v);
static void cacheLOadd(CACHE v);
static void cacheLOremove(CACHE v);
static CACHE cacheAlloc (void) ;			    /* self managed calloc */
static void cacheFree (CACHE p);
#ifndef READONLY
static int cacheStore(CACHE v, BOOL doCopy, BOOL dowait) ;
#endif

/*****************************************************************/

/* Use this macro to check that a cache pointer passed into a routine is     */
/* non-NULL and points to a valid cache block.                               */
/*                                                                           */
#define CHECK_CACHE_PTR(FUNCNAME, CACHE_PTR)               \
  if (!(CACHE_PTR) || (CACHE_PTR)->magic != CACHEMAGIC)    \
    messcrash("%s received a %s pointer",                  \
              (FUNCNAME),                                  \
	      (CACHE_PTR) ? "bad cache" : "NULL")


/********************* globals ***********************************/

/*number of cacheEntries, defined in blocksub*/
extern int MAXKNOWNCACHE;

/* Used in _lots_ of places....                                              */
BOOL   READING_MODELS = FALSE ;


/********************* locals  ***********************************/

static CACHE	knownCacheTop = NULL ,
		knownCacheEnd  = NULL ,
		lockedCacheTop = NULL ;
static int nKnownCache = 0;
static int nAllocatedCache = 0 ;        /* useful for debug */
static int ncache1read = 0, ncache1written = 0 , ncache2served = 0, ncache2saved = 0 ;

static KEYSET knownKeySet = 0 ;

static Stack freeStack = 0 ;


/* #define TESTCACHE_to_test   */


/**********************  PUBLIC  *********************************/

#ifndef ACEDB5
void bsTreeArrayFlush(void) { return ; }
#endif

/* General utility routines */

void cacheMark(CACHE cache)
{
  CHECK_CACHE_PTR("cacheMark", cache) ;

  if (cache->lock == 0)
    messcrash("cacheMark called on cache that is not locked.") ;

  if (cache->lock > 0)
    cache->lock=2 ;

  return ;
}

/***********************/

BOOL isCacheModified(CACHE cache)
{
  CHECK_CACHE_PTR("isCacheModified", cache) ;

  return cache->lock == 2 ;
}

/**********************/

BOOL isCacheLocked(CACHE cache)
{
  CHECK_CACHE_PTR("isCacheLocked", cache) ;

  return cache->lock > 0 ? TRUE : FALSE ;
}


#ifndef READONLY

/**********************/
/* The purpose of this function is to allow cache to be unlocked in a        */
/* special set of circumstances:                                             */
/*     sequence is:       bsUpdate()                                         */
/*                        object is not changed for some reason              */
/*                        bsDestroy()                                        */
/*                 at this point, the cache entry for the object is still    */
/*                 locked from the bsUpdate and so if the caller trys        */
/*                 to cacheKill() the cache entry the program crashes.       */
/*                                                                           */
/* This function allows the caller to unlock the cache entry for the object  */
/* after the bsDestroy() so that the cacheKill() will work.                  */
/*                                                                           */
/* Because this function is potentially dangerous we check lock and refCount.*/
/*                                                                           */
void cacheUnlock(CACHE cache)
{
  CHECK_CACHE_PTR("cacheUnlock", cache) ;

  if (cache->lock == 0)
    messcrash("Attempt to unlock cache that is already unlocked") ;
      
  if (cache->refCount != 0)
    messcrash("Attempt to unlock cache that is still in use, cache->refCount = %d",
	      cache->refCount) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* I'm not very happy about not including this test, but it appears that   */
  /* an object can be locked with no one referring to it, but it has been    */
  /* modified...maybe this is ok...not sure...                               */
  if (cache->hasBeenModified)
    messcrash("Attempt to unlock cache that although not in use or locked has been modified") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  /* In fear and loathing I unlock the cache entry and will never again      */
  /* sleep peacefully in my bed.                                             */
  cache->lock = 0 ;

  return ;
}


int cacheSaveAll(void)
{
  CACHE v = knownCacheEnd ;
  int n = 0 ;

  chrono("cacheSaveAll") ;
  
  while(v)
    {
      if(v->hasBeenModified)
      {
	cacheStore(v,TRUE,TRUE) ; n++ ;
      }
      v = v->up ;
      if (n >= 128) { n = 0 ; bsTreeArrayFlush () ; }
    }
  bsTreeArrayFlush () ;
  
  chronoReturn(); return(0);
}
#endif

/************************************************************************/
                          /* To be called only by session manager */
/*
void cacheClear(void)
{
  CACHE v, w ;;
 
  v = knownCacheTop;
  while(v) 
    { if (v->x)
	switch(v->type)
	  {
	  case 'B' :
	    bsTreePrune(v->x) ;
	    break ;
	    
	  default:
	    messcrash("cacheDestruct received unknown type") ;
	  }
      w = v ;
      v = v->next ;
      cacheFree(w);
    }

  v = lockedCacheTop;
  while(v) 
    { if (v->x)
	switch(v->type)
	  {
	  case 'B' :
	    bsTreePrune(v->x) ;
	    break ;
	    
	  default:
	    messcrash("cacheDestruct received unknown type") ;
	  }
      w = v ;
      v = v->next ;
      cacheFree(w);
    }

  knownCacheTop = lockedCacheTop = knownCacheEnd = 0 ;
}
*/
/************************************************************************/
                          /* Gives the cache status*/
                          /*    Public;  Calls nothing*/
int cacheStatus (int *used, int *locked, int*known, int *modified, 
		 int *nc2rp, int *nc2wp, int *nccrp, int *nccwp)
{
  register CACHE v;
  register int i, l, m;

  i = l = m = 0; v = knownCacheTop;
  while(v) 
    { if (v->refCount) l++ ; else i++;
      if(v->hasBeenModified) m++ ;
      v = v->next; 
    }
  
  v = lockedCacheTop;
  while(v) 
    { l++ ;
      if(v->hasBeenModified) m++ ;
      v=v->next;
    }

  *used = nAllocatedCache ;
  *known = i ;
  *locked = l ;
  *modified = m ;
  *nc2rp = ncache2served ; *nc2wp = ncache2saved ;
  *nccrp = ncache1read ; *nccwp = ncache1written ;
  
  return TRUE;
}

/************************************************************************/

        /*    Public;  but the user must never destroy it */
KEYSET cacheKeySet (void)
{
  register CACHE v ; register int i ;
  /* 
     it may be slower to keysetinsert remove all the time
     if (!knownKeySet) knownKeySet = keySetCreate() ;
  return knownKeySet ;
  */
  knownKeySet = arrayReCreate (knownKeySet, nAllocatedCache, KEY) ;
  i = 0 ; v = knownCacheTop;
  while(v) 
    { 
      keySet(knownKeySet, i++) = v->key ;
      v = v->next; 
    }
  keySetSort(knownKeySet) ;
  keySetCompress (knownKeySet) ;
  return knownKeySet ;
}

/************************************************************************/
                      /*  Public ;  Called by cacheCreate and cacheUpdate     */
                      /*            Calls buCopy                  */
static CACHE cacheCopy(CACHE cache)
{
  CACHE copy ;

  chrono("cacheCopy") ;

  CHECK_CACHE_PTR("cacheCopy", cache) ;

  copy = cacheAlloc();

  *copy = *cache; /* everything but those relevant to this package */
  switch(cache->type)
    {
    case 'A' :
      copy->x = (CACHE_HANDLE) arrayCopy((Array)cache->x) ;
      break ;
    case 'B' :
      copy->x = bsTreeCopy(cache->x);
      break ;
    default :
      messcrash("cacheCopy received unknown type") ;
    }

  chronoReturn();

  return copy;
}

/**************************************************************/
  /* to get a cache address in main memory, with read only status*/
  /* getting it  if necessary  from the primary cache disk */

  /* the system is allowed to unload when refCount == 0 */
  /*   Called by bsCreate and cacheUpdate                 */
  /*   Calls either cachePop (if k is already in memory)*/
  /*             or cacheNew                            */
  /* or cacheCopy if the cacheEntry is locked but not yet modified */


CACHE cacheCreate(KEY k, CACHE_HANDLE *bsp, BOOL newCopy)
{
  LEXP q=KEY2LEX(k);
  CACHE cache ;
  
  chrono("cacheCreate") ;
  ncache2served++ ;

#ifdef TESTCACHE
  messout("Hello from cacheCreate(%s)",name(k));
#endif
  if(!q)
    messcrash("Error : cacheCreate called with unknown key");
    
  if ((cache = q->cache))
    switch(cache->lock)
      {    
      case 0 :                  /* already in memory*/    
	if (!newCopy)
	  { (cache->refCount)++;
	    cachePop(cache);
	    *bsp = cache->x ;
	    chronoReturn() ; return cache ;
	  }
	/* else fall thru on case 1 */
	
      case 1 :    /* accessed read write else where, make a copy */
	cache = cacheCopy(cache);
	break;
	
      default:   /* already modified, grab it again from disk */
	cache = 0 ;
	break;
      }
  
      /* cache->hasBeenModified is set to 0 by cacheNew */
      /* otherwise is stays as is */
  if (!cache)
    cache = cacheNew(k) ;

  if (cache->key != k)
    messcrash("Cache mismatch %s:%s != %s:%s",  
	      className(k), name(k),
	      className(cache->key), name(cache->key) ) ;

  if (cache)
    {
      cache->refCount = 1;
      if (!newCopy)
	{ cacheKOadd(cache);
	  cache->lock = 0;
	  q=KEY2LEX(k);   /* possible relocation */
	  q->cache = cache ;   /* for future access */
	}
      else
	{ cacheLOadd(cache);
	  cache->lock = -1 ;
	}
      *bsp = cache->x ;
    }
  else
    *bsp = 0 ; 

  chronoReturn(); return cache ;
}

/**************************************************************/
                    /* new : to get a new cache address in cache, */
                    /*  Static; called by cacheCreate; Calls cacheAlloc*/
static CACHE cacheNew(KEY key)
{ 
 CACHE cache = cacheAlloc();
 chrono("cacheNew") ;

 if (cache->key) 
  messcrash("cacheNew: double use of %s", name(cache->key)) ;
 
 cache->key = key; 
 cache->magic = CACHEMAGIC ;
 cache->type = pickType(key) ;
 cache->hasBeenModified = 0;
 
 
 switch (cache->type)
   {
   case 'A' :
     messcrash("cacheNew received Array type, read a.h") ;

   case 'B' :
     ncache1read++ ;
#ifdef ACEDB5
       cache->x = bsTreeArrayGet(key) ;	/* cannot fail */
#else
       cache->x = bsTreeGet(key) ; 	/* cannot fail */
#endif
     break ;

   default :
     messcrash("cacheNew received unknown type") ;
   }

 chronoReturn() ; return cache ;
}

/************************************************************************/

                    /* Release : complementary to Get */
void cacheDestroy (CACHE v)
{
  LEXP q ;
  CACHE presentCache ;

  chrono("cacheDestroy") ;

  CHECK_CACHE_PTR("cacheDestroy", v) ;

  q = KEY2LEX(v->key) ;

  presentCache = q->cache ;

  if(!(v->refCount) --)
    messerror("Unbalanced Release of cacheEntry %s",
	      name(v->key));

  switch (v->lock)
    {
    case 0 :  /* This cacheEntry was opened read only */
      break ;
    case 1 :  /* This cacheEntry has not been modified,
		 thus we can still use it next time */
      lexunlock(v->key);  
      cacheLOremove(v);
      cacheKOadd(v);
      break ;
    case 2 :   /* We do want to lose the modifications */
      lexunlock(v->key);  
       /* fall thru on case -1 */
    case -1:
      cacheLOremove(v);
      cacheDestruct(v) ;

      chronoReturn(); return ;
    }
  
  if (presentCache && presentCache != v &&    /* There is a newer version around */
      !v->refCount)                           /* v is not beeing used */
    /* mieg: april 96
       I use to have:
      && !v->hasBeenModified)    If it has, it is likely to become lame duck when 
			      * presentCache will be saved,
			      * so it is better not to save it now 
      but it seems that in the alias case oldCacche was lost prior
      to the cacheSave call, hence one could have still a modified
      flag around the present obj. hence
      this block remained around and was saved to disk after the key had been aliased
			      */
    { cacheKOremove(v);
      cacheDestruct (v) ;
    }

  chronoReturn(); return;
}

void cacheClone(CACHE source, CACHE dest, CACHE_HANDLE *bsp)
{ bsTreePrune(dest->x); /* remove old data */
  *bsp = dest->x = bsTreeCopy(source->x);
}


/***********************************************************************/

/* Locks a cache, it must then be unlocked or ForceDestroy explicitely*/
/* before the memory can be recovered*/

                      /*    Public;   Calls cacheCreate */
CACHE cacheUpdate(KEY k, CACHE_HANDLE *bsp, BOOL inXref)
{ unsigned char cc = lexGetStatus (k) ;
  LEXP q = KEY2LEX(k);
  CACHE  cache ;

  chrono("cacheUpdate") ;

  if (cc & LOCKSTATUS)
      { 
	messout ("Sorry, cacheEntry %s is already locked", name (k)) ;
	
        chronoReturn(); return NULL;
      }

  lexlock (k) ;

  if ((cache = q->cache ))                /* already in memory*/    
    {    
      if (!cache->refCount && (!cache->hasBeenModified || inXref))
	/* unused and untouched */
	/* Except during doXref where I am garanteed that I will not edit then bsDestroy.
	 * The problem is that the sequence bsUpdate/cacheMark/bsDestroy would destroy
	 * the earlier modif of an ealier update/save sequence.
	 * Obviously, if cache is in use, you need to work on a copy.
	 */
	cacheKOremove(cache);
      else
	cache = cacheCopy(cache);
    }
  else
    {
      cache = cacheNew(k) ;
      /*	Important fix by rbrusk: 28/12/96: Need to reset q here for times
		when cacheNew(k) indirectly results in a
		dangling q pointer reference from Lexi2 arrayExtend() */
      q = KEY2LEX(k) ; 
      q->cache = cache ;
    }
    
/*   lex2Check() ; */
  if (cache->key != k)
    messcrash("Cache mismatch %s:%s != %s:%s",  
	      className(k), name(k),
	      className(cache->key), name(cache->key) ) ;

  if (cache)
    {
      cache->refCount = 1;
      cacheLOadd(cache);
      cache->lock = 1;        /* locking */
      *bsp = cache->x ;
    }

  ncache2served++ ;
  chronoReturn();
  return cache ;
}

/************************************************************************/

/* unlock : to register the recent modifications */
void cacheCheck(KEY key, CACHE v)
{
  if (v->key != key)
    messcrash("Cache mismatch %s:%s != %s:%s",
	      className(key), name(key),
	      className(v->key), name(v->key) ) ;
  return ;
}

/************************************************************************/

void cacheSave(CACHE v)
{
  LEXP q ;
  CACHE oldCache ;

  chrono("cacheSave") ;

  CHECK_CACHE_PTR("cacheSave", v) ;

  q = KEY2LEX(v->key) ;

  oldCache = q->cache ;

  if (v->lock == 2) 
    ncache2saved++ ;

#ifdef TESTCACHE
  messout("Hello from cacheSave(%s)",name(v->key));
#endif

  if(v->lock <= 0)
      messcrash("Attempt to save an unlocked cacheEntry  %s:%s",
		className(v->key),name(v->key)) ;
  
  lexunlock(v->key);  
  cacheLOremove(v);

  /* mieg: apr-96: flag as lame before KOadd */
  if (oldCache && oldCache != v)   /* Lame duck, not to be saved */
    {
      oldCache->hasBeenModified = 0 ;
      if (!oldCache->refCount)
	{
	  cacheKOremove (oldCache) ;
	  cacheDestruct (oldCache) ;
	}
    }

  q = KEY2LEX(v->key) ;		/* possible relocation */
  q->cache = v ;		/* for future access, including inside KOadd */
  cacheKOadd(v);

  if (v->lock == 2)
    (v->hasBeenModified)++ ;

  v->lock = 0;
  if(!(v->refCount) --)
    messerror("Unbalanced Release of cacheEntry %s",
	      name(v->key));

  chronoReturn();
  return;
}

/**************************************************************/
           /* to copy a cacheEntry into the block cache */
             /*        Static, called by cachefree */
             /*                calls ..Store */
/* Note i abandon the forced write system available inside block sub */

#ifndef READONLY

static int cacheStore(CACHE v, BOOL doCopy, BOOL dowait)
{
  chrono("cacheStore") ;

  CHECK_CACHE_PTR("cacheStore", v) ;

#ifdef TESTCACHE
  messout("Hello from cacheStore");
#endif
  
  if(v->hasBeenModified &&
    (v == KEY2LEX(v->key)->cache))
    switch(v->type)
      {
      case 'A' :
	messcrash("cacheStore received A class obj") ;
	break ;

#ifndef READONLY
      case 'B' :
	ncache1written++ ;
#ifdef ACEDB5
	bsTreeArrayStore(v->x, dowait) ; /* non destructive operation */
	if (!doCopy)
	  bsTreePrune(v->x);
#else
	if (doCopy)
	  bsTreeStore(bsTreeCopy(v->x)) ;
	else
	  bsTreeStore(v->x);   /* Destructive operation */
#endif
	break ;
#endif	
	default :
	  messcrash("cacheStore received unknown type") ;
      }

  v->hasBeenModified = FALSE ;
  
  chronoReturn() ; return(0);
}
#endif

/**************************************************************/

static void cacheDestruct(CACHE v)
{
  LEXP q ;
/*  KEY dummy = v->key ; for debugging */

  chrono("cacheDestruct") ;

  CHECK_CACHE_PTR("cacheDestruct", v) ;

  q = KEY2LEX(v->key);

/*   lex2Check() ; */

  if (q->cache == v)
    { q->cache = 0 ; lex2clear(v->key) ; }
  if (v)
    {
      if (v->x)
	switch(v->type)
	  {
	    /*
	       case 'A' :
	       arrayDestroy(v->x) ;
	       break ;
	       */	   
	  case 'B' :
	    bsTreePrune(v->x) ;
	    break ;
	    
	  default:
	    messcrash("cacheDestruct received unknown type") ;
	  }
      
      cacheFree(v);
    }
  chronoReturn(); return;
}

/**************************************************************/
/* Called to explicitly get rid of the cache2 ref for an object that has     */
/* been destroyed. This shouldn't be called until the all BS objects that    */
/* reference the cache entry have been properly cleaned up.                  */
/*                                                                           */
void cacheKill (KEY key)
{
  LEXP q = KEY2LEX(key);
  CACHE v = q ? q->cache : 0 ;

  chrono("cacheKill") ;

  if (v)
    {
      if (v->refCount || v->lock)
        messcrash ("Illicit call to cacheKill") ;
      v->hasBeenModified = 0 ;
      cacheKOremove(v) ;
      cacheDestruct (v) ;
    }

  chronoReturn();
  return;
}

/**************************************************************/
/**************************************************************/
    /* Linked Lists manipulations */

/************************************************************************/
                      /*pops a cache to the top of the knownCache line*/
                       /*  Static; called by cacheCreate; Calls nothing*/
static void cachePop(CACHE v)
{
  chrono("cachePop") ;

  CHECK_CACHE_PTR("cachePop", v) ;

  cacheKOremove(v) ;
  cacheKOadd(v) ;

  chronoReturn();
  return;
}

 /*********************************************************************/
                /* to add an cacheEntry to the Known CacheEntry List */
                /*  Static; called by cacheCreate and cacheSave      */

static void cacheKOadd(CACHE v)
{
  CACHE w ;
  int nBSused, nAa, bsmem, pass = 0, nn ;
  extern int BS_Cache_limit ;

  if (!BS_Cache_limit)
    messcrash("BS_cache_limit not set") ;
  
#ifdef TESTCACHE
  messout("Hello from cacheKOadd(%s)",name(v->key));
#endif

  chrono("cacheKOadd") ;

  CHECK_CACHE_PTR("cacheKOadd", v) ;
  
  nKnownCache++;	  
  switch(v->type)
    {
    case 'A' :
      messcrash("cacheKOadd received A type") ;
      break ;
    case 'B' :
     break ;
    default :
      messcrash("cacheKOadd received unknown type") ;
    }
  
  w = knownCacheTop;
  knownCacheTop=v;

  if (!knownCacheEnd)
    knownCacheEnd = v;

  if (w) 
    w->up = v ;
  v->up =  NULL;
  v->next = w ;
  w = knownCacheEnd ;


 lao:
  nn = 1 ;
  w = knownCacheEnd ;
  while (w)
    {
      if (pass != 1)
	{
	  BSstatus(&nBSused, &nAa, &bsmem) ;

	  if (nBSused <= BS_Cache_limit)
	    break ;
	}

      v = w;
      w = w->up;

      if (!w)
	switch (pass)
	  {
	  case 0:
	    {
	      pass++ ;
	      goto lao ;

	      break ;
	    }
	  case 1: 
	    {
	      bsTreeArrayFlush() ;
	      pass++ ;
	      goto lao ;

	      break ;
	    }
	  case 2:
	    {
	      if (nKnownCache < 10)  /* this means there are some very big obj around, rather than a memory leak */
		{
		  BS_Cache_limit *= 2 ;
		  goto lao ;
		}
	      else
		{
		  messcrash ("Cache 2 is full, %d objetcs are locked\n balance bsCreate/destroy\n%s\n%s %s:%s %s",
			     nKnownCache, " or increase the limits in wspec/cachesize.wrm",
			     " for example ", className(knownCacheEnd->key), name(knownCacheEnd->key), "is locked ") ;
		}
	      break ;
	    }
	  }

      if (nn > 128 || 4*nn > nKnownCache)
	switch (pass)
	  {
	  case 0:
	    pass++ ; goto lao ; 
	    break ;
	  case 1: /* during pass 1, systematically store flagged obj of last quarter */
	    bsTreeArrayFlush() ; pass++ ; goto lao ;
	    break ;
	  case 2: 
	    break ;
	  }

      if (!v->refCount && pass == 0 && v->hasBeenModified)
	{ nn++ ; continue ; }

      if (!v->refCount)
	{
	  cacheKOremove(v);
	  if(v->hasBeenModified) 
	    switch (pass)
	      {
	      case 0:
		break ;
#ifndef READONLY
	      case 1: /* during pass 1, systematically store flagged obj of last quarter */ 
		nn++ ; 
		/* fall thru */
	      case 2: 
		cacheStore(v, FALSE, TRUE) ; 
		v->x = 0;
		break ;
#endif	       
	      }
	  cacheDestruct(v);
	}
    } 

  bsTreeArrayFlush() ;

  chronoReturn() ; 

  return ;
}



/*********************************************************************/
/* to remove an cacheEntry to the Known CacheEntry List */
/*  Static; called by cacheUpdate and cacheNew        */
/*          Calls nothing.      */
static void cacheKOremove(CACHE v)
{
  CACHE u, w;

  chrono("cacheKOremove") ;

  CHECK_CACHE_PTR("cacheKOremove", v) ;

#ifdef TESTCACHE
  messout("Hello from cacheKOremove(%s)",name(v->key));
#endif
  /*
    if (knownKeySet) keySetRemove(knownKeySet, v->key) ;
    */
  nKnownCache--;
  u=v->up;
  w=v->next;

 if (knownCacheTop == v) 
   knownCacheTop = w ;

 if(knownCacheEnd == v)
   knownCacheEnd = u ;

 if(u)
   u->next = w;
 if (w) 
   w->up = u;

 v->up = v->next = NULL;
  chronoReturn() ; 
}

 /*********************************************************************/
                /* to add an cacheEntry to the Locked CacheEntry List */
                /*  Static; called by cacheSave and cacheForceFree */
                /*          Calls nothing.      */
static void cacheLOadd(CACHE v)
{
  CACHE w ;

  chrono("cacheLOadd") ;

  CHECK_CACHE_PTR("cacheLOadd", v) ;

#ifdef TESTCACHE
  messout("Hello from cacheLOadd(%s)",name(v->key));
#endif

  w= lockedCacheTop ;
 lockedCacheTop = v ;

 if (w) 
   w->up = v ;

 v->up =  NULL;
 v->next = w ;
  chronoReturn() ; 
}

 /*********************************************************************/
                /* to add an cacheEntry to the Locked CacheEntry List */
                /*  Static; called by cacheSave and cacheDestroy */
                /*          Calls nothing.      */
static void cacheLOremove(CACHE v)
{
 CACHE u, w ;
  chrono("cacheLOremove") ;

  CHECK_CACHE_PTR("cacheLOremove", v) ;

#ifdef TESTCACHE
  messout("Hello from cacheLOremove(%s)",name(v->key));
#endif
  u=v->up;
  w=v->next;

 if (lockedCacheTop == v) 
   lockedCacheTop = w ;

 if(u)
   u->next = w;
 if (w) 
   w->up = u;

 v->up = v->next = NULL;
  chronoReturn() ; 
}

/**************************************************************/
/**************************************************************/
            

static CACHE cacheAlloc (void)       /* self managed calloc */
{
  static int blocSize = 256 ;
  CACHE p ;
  int i = -12 ;

  chrono("cacheAlloc") ;

  if (!freeStack)
    freeStack = stackCreate (4*blocSize) ;
  if (stackEmpty (freeStack))
    { p = (CACHE) messalloc (blocSize * SIZE_OF_CACHE) ;
      for (i = blocSize ; i-- ; ++p)
          push (freeStack,p,CACHE) ;
/* printf ("Adding %d to CACHE free list\n",blocSize) ; */
      nAllocatedCache += blocSize ;
      blocSize *= 2 ;
    }
  p = pop (freeStack,CACHE) ;
  p->magic = CACHEMAGIC ;
  chronoReturn() ; return p ;
}

/***********************/

static void cacheFree (CACHE p)
{
  chrono("cacheFree") ;

  CHECK_CACHE_PTR("cacheFree", p) ;

  memset (p, 0, SIZE_OF_CACHE) ;
  push (freeStack,p,CACHE) ;
  if (p->refCount)
    invokeDebugger() ;
  chronoReturn() ; 
/*   lex2Check () ; */
}

/***********************/

/* functions private to the cache package, 
   that allow objcachedisp.c access to cache system variables
   within this file */

CACHE cacheGetKnownTop (void)
{
  return knownCacheTop;
}

CACHE cacheGetLockedTop (void)
{
  return lockedCacheTop;
}
/**************************************************************/
/**************************************************************/






 
