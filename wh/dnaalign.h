/*  File: dnaalign.h
 *  Author: Ulrich Sauvage (ulrich@kaa.cnrs-mop.fr)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1994
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
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: May 20 15:15 1996 (ulrich)
 * Created: Mon Jan 31 12:52:02 1994 (ulrich)
 *-------------------------------------------------------------------
 */

/*  @(#)dnaalign.h	1.15 7/31/96 */

#ifndef DEFINE_DNAALIGN_h
#define DEFINE_DNAALIGN_h

#include "interval.h"  /* for DEFCPT struct */

typedef 
  enum {
    AMBIGUE, INSERTION_DOUBLE, TROU_DOUBLE,
    INSERTION, TROU, ERREUR
       } ALIGN_ERROR_TYPE ;


typedef struct dnaAlignErrStruct 
    { ALIGN_ERROR_TYPE type ;
      int iLong, iShort ;   /* offset in "long" reference dna and "short" tested dna */
      char baseShort ;  /* A 0 for a missing base, or its value */
      int sens ; /* 1 if short and long are in the same way, otherwise - 1 */
      }  A_ERR ;

typedef struct { int u1, u2 ; } PAIR ;

void dnaAlignInit(void) ;
void dnaAlignDestroy (DEFCPT look) ;
void dnaAlignForget(DEFCPT look, KEY key) ;
char * dnaAlignDecodeOligo(KEY key) ;
/* int dnaAlignGiveNbBad(void) ; */

void dnaAlignGetData(DEFCPT look) ;
void dnaAlignLoadFile(DEFCPT look, char *cp) ;
int dnaAlignLoad (DEFCPT look, char *cp) ;
void dnaAlignInsertBad(DEFCPT look) ;
void dnaAlignAddSeqIn (DEFCPT look, KEY key) ;

void dnaAlignTryAll(DEFCPT look) ;

void dnaAlignNewTryPaires(DEFCPT look) ;
/* KEY dnaAlignPaires(DEFCPT look, KEY key1, KEY key2, int *taux) ; */
void  dnaAlignAsmbPaire (DEFCPT look, KEY key1, KEY key2) ;
KEY  dnaAlignAsmbPaire21 (KEY key1, KEY key2, int id, int tour, int taille,
			  int pre, char *nm) ;
void  dnaAlignAsmbPaire2 (KEY key1, KEY key2) ;
Array dnaAlignMatchTriplet (Array dna1, int x1, int y1, int *n1, 
			    Array dna2, int x2, int y2, int *n2,
			    BOOL direct) ;
BOOL dnaAlignAsmbPaireDna (Array dna1, Array dna2, int taille, int max, int zone, 
			   int nn0, int *v1, int *v2, int *sens, BOOL tryboth) ;

int contigFindMatch (KEY contig1, KEY contig2, Array dnaContig1, Array dnaContig2,
		     Array r1, Array r2, BOOL direct, Array *fitp) ;
void dnaAlignCptSegments(DEFCPT look) ;
void dnaAlignAddKeySetIn (KEY link, KEY contig, KEYSET reads, int taux) ;

void dnaAlignFixSegConsensus(DEFCPT look, KEYSET segment) ;
void dnaAlignFixActKeyset(void) ;
void dnaAlignFixContig (KEY link, KEY key) ;
void dnaAlignSaveAs (DEFCPT look, char **cp) ;
void dnaAlignSave (DEFCPT look, char *cp, BOOL order) ;
BOOL dnaAlignCopyContig (KEY key, KEY *kp, char *cp, BOOL left) ;
char *dnaAlignSaveDefault (DEFCPT look) ;

Array dnaAlignCptErreur(Array longDna, Array shortDna, int *debutp, int *finp, 
			int *shdebp, int *shfinp) ;

Array dnaAlign(Array longDna, Array shortDna, int *debutp, int *finp, 
	       int *shdebutp, int *shfinp) ;
void dnaAlignRecale(Array longDna, int *xl, int *yl, Array shortDna,
		     int xs, int ys) ;

BOOL dnaAlignForceMatch(Array shortDna, int x1, int x2, Array longDna, int y1, int y2,
			int *topp, int *endp, int *sensp) ;
void dnaAlignFindRepeat (Array source,
			 Array dna, Array color, int x1, int x2, int taille) ;
void dnaAlignTestPhil(void) ;

/* void assembleAllTraces(void) ; */
void oldAssembleAllTraces(void) ;
void newAssembleAllTraces(void) ;
void dnaDispGraph (Array ks, int tour) ;

/* Fonction de Aligntools */
KEYSET alignToolsPurifyContig (KEY link, KEY key) ;
BOOL alignToolsAdjustContigSize (KEY link, KEY key) ;
Array alignToolsMakeShortMatch (Array dna1, Array dna2,
				int sens,int taille, int max, int zone) ;
void contigLengthSort (KEYSET ks) ;
void alignToolsAdjustLink (KEY link, KEYSET subSeq, Array bilan) ;
void alignToolsDestroy_Segs(int id) ;
BOOL dnaAlignCutContig (KEY contigKey, int where, KEY *keyp1, KEY *keyp2,
			int *max1, int *max2, char action) ;
KEYSET dnaAlignMakeSubSequence (KEY link, KEYSET reads, char *cp) ;

BOOL dnaAlignCheckSequence (Array dna, int clipTop, int clipEnd, int mini) ;
void dnaAlignAdjustLink (KEY link) ;
BOOL dnaAlignGetClip (KEY link, KEY contig, KEY key, int *ctp, int *cep) ;
BOOL dnaAlignCompare (KEY key1, KEY key2, Array dna1, Array dna2) ;
Array dnaAlignCompareDna (Array dna1, Array dna2, int *x1, int *x2, int *sens, BOOL countN) ;
BOOL dnaAlignAssemblyCompare (KEY key1, KEY key2) ;

BOOL dnaAlignReInsertLoners (KEY seqKey, char tag) ;
BOOL dnaAlignDoReInsertLoners (KEY seqKey, KEYSET read) ;

void statisticsMakeErreur (KEYSET ks) ;
void statisticsDoMakeErreur (KEY target, KEYSET ks) ;
BOOL dnaAlignContigForcePair (KEY seqKey, KEY target, KEYSET petitKs) ;
KEY dnaAlignDoMakeSuperLink (KEYSET ks, char *cp) ;
void dnaAlignCleanLeftArrows (KEY link, KEY oldCont, KEY newCont, BOOL seton, int choix) ;
char *dnaAlignNewName (char *cp) ;
Array spliceTrackErrors (Array  dna1, int pos1, int *pp1, 
			 Array dna2, int pos2, int *pp2, 
			 int *NNp, Array err, int maxJump, int maxError) ;

/* de basecall.c */
KEYSET  baseCallNewScf(void) ;

#endif
