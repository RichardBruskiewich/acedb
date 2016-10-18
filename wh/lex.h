/*  File: lex.h
 *  Author: Fred Wobus (fw@sanger.ac.uk)
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
 * Description: public function header for lexsubs4.c
 *              for everything dealing with the lexique
 * HISTORY:
 * Last edited: Apr  7 13:07 2003 (edgrif)
 * Created: Mon Nov 23 12:06:50 1998 (fw)
 *-------------------------------------------------------------------
 */

/* $Id: lex.h,v 1.28 2004-11-14 20:34:45 mieg Exp $ */

#ifndef _LEX_H
#define _LEX_H

/************************************************************/

#include <wh/disk.h>		/* for DISK type */


/* Returned by iskey, LEXKEY_IS_UNKNOWN: key is not known to acedb
 *                        LEXKEY_IS_VOC: key is vocabulary entry only
 *                        LEXKEY_IS_OBJ: key is a full object. 
 * don't change this enum, lots of the code relies on the values 0, 1 & 2 being returned. */
typedef enum {LEXKEY_IS_UNKNOWN = 0, LEXKEY_IS_VOC, LEXKEY_IS_OBJ} LexKeyType ;



/************************************************************/
int   lexMax(int t) ;  /* Numbers of entries in the lexique */
int   lexi2Max(int t) ;  /* Numbers of cached entries of the t lexique */
int   vocMax(int t) ;
void  lex2clear (KEY key) ; /* call this function to clear the lex2 array */

LexKeyType   iskey(KEY kk);
BOOL  iskeyold(KEY kk);					    /* TRUE if key is on disk or in first cache */
BOOL  lexReClass(KEY key,KEY *kp, int t);

BOOL  lexIsInClass(KEY key, KEY classe) ;  /* Valid also for sub classes */
BOOL  lexClassKey(char *cp,KEY *kp) ; /* Recognises class:name */
BOOL  lexword2key(char *cp,KEY *kp, KEY classe);
                           /*given a word *cp, sets its key *kp */
                           /* search done in t lexique */
                           /*returns TRUE if found, FALSE if not */

BOOL  nextName(KEY key, char **cpp)  ;
                           /* Runs along tha aliases of key,
			    * on first call *cpp should be null
			    */  

/* lexAlias() cleans up newName and modify the lexique return TRUE if newName is accepted
   FALSE if error or newName corresponds to some other object.
   lexDoAlias() is the same but if err_msg_out is non-NULL an error msg will be
   returned if the function fails, caller must messfree err_msg_out. */
int lexIsAlias (KEY key) ;  /* returns 0:no, 1:may_be, 2:yes, key is alias of another key */
BOOL  lexAlias(KEY key, char *newName, BOOL doAsk, BOOL keepBothNames) ;
BOOL lexDoAlias(KEY key, char* newName, BOOL doAsk, BOOL keepOldName, char **err_msg_out) ;

KEY   lexAliasOf(KEY key) ;  /* Returns the end of the alias list */
BOOL lexDestroyAlias (KEY key); /* removes an aliased key and 
				   relinks the alias list */

BOOL  lexIsKeyVisible(KEY key);
/* returns false if key does not exist, has aliasstatus or emptystatus, 
   or is a KEYKEY 0 model */
/* cannot use lexGetStatus for this as it follows the alias list */

BOOL  lexaddkey(char *cp,KEY *kp, KEY classe);
                  /*add to the t lexique the word *cp */
               /*returns TRUE if added, FALSE if known, crashes if Full */

char  *lexcleanup(char *cp, STORE_HANDLE h) ;   /* Remove leadind and trailing spaces */
BOOL lexIsGoodName(char *name); /* Check that name non-blank */

int   lexstrcmp(char *a,char *b);
   /*returns 1 if *a>*b  or 0 or -1 as strcmp but is case unsensitive*/
   /*This is used to better the presentation, yet avoid doubles*/
BOOL lexNext(KEY classe, KEY *p); /*gives the next KEY in table n*/
KEY lexLastKey(int classe) ;

void lexmark(int table);
void lexkill(KEY key);      /*deallocates a key*/
void lexshow (void) ;   /* RMD 6/19/90, for debugging */
void lexInit(void);            /*Called at beginning of program*/
void lexRead(void);            /*Called at beginning of program*/
void lexSave(void) ;
void lexavail(unsigned long *vocnum,
	      unsigned long *lex1num,
	      unsigned long *lex2num,
	      unsigned long *vocspace,
              unsigned long *hspace,
	      unsigned long *totalspace
	      ) ;

#define LOCKSTATUS	(unsigned char)1  /* bits 1 and 2 are reset to 0 by lexiread */
#define CALCULSTATUS	(unsigned char)2  /* all bits are reset by lexAlias */
#define EMPTYSTATUS	(unsigned char)4
#define ALIASSTATUS	(unsigned char)8
#define TOUCHSTATUS	(unsigned char)16 /* key touched this session */
#define ALPHASTATUS	(unsigned char)32 /* reserved for the alphabetic sorter */
#define SERVERSTATUS    (unsigned char)64 /* reserved for xclient */
#define ISALIASSTATUS   (unsigned char)128  /* this key is the alias of some other key */

#define LEXPRIVATESTATUS (unsigned char) 9  /* bits 1 8 are protected in lexsetstatus */

void lexSetStatus(KEY key, unsigned char c) ;
void lexUnsetStatus(KEY key, unsigned char c) ;
void lexClearClassStatus(KEY classe, unsigned char c) ;
unsigned char lexGetStatus(KEY key) ;
void lexunlock(KEY key) ; /* these 2 check for double locking */
BOOL lexlock(KEY key) ;
#define lexiskeylocked(key) (lexGetStatus(key) & LOCKSTATUS)

void lexSetIsA(KEY key, unsigned char c);

void lexSetCreationUserSession(KEY key, KEY timeStamp);
KEY lexCreationUserSession(KEY key);
KEY lexUpdateUserSession(KEY key);
mytime_t lexCreationStamp(KEY key);
mytime_t lexUpdateStamp(KEY key);
/************ in lexalpha.c ****************/

void lexAlphaMark (int c) ;
void lexAlphaMakeAll (void) ;
void lexAlphaSaveAll (void) ;
void lexAlphaClear (void) ;
int  lexMaxVisible (int c) ;


/* Fairly grim, global used only by the client version of xace.              */
/* DON'T use this anywhere else...PLEASE...                                  */
extern BOOL X_CLIENT_PARSING ;


#endif /*  _LEX_H */

/***************************** eof ****************************/
 

