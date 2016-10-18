/*
 * Copyright (c) Medical Research Council 1994. All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * this copyright and notice appears in all copies.
 *
 * This file was written by James Bonfield, Simon Dear, Rodger Staden,
 * as part of the Staden Package at the MRC Laboratory of Molecular
 * Biology, Hills Road, Cambridge, CB2 2QH, United Kingdom.
 *
 * MRC disclaims all warranties with regard to this software.
 */

/* 
 * File: 	Read.c
 * Purpose:	Performs read/write IO on the Read data stucture.
 * Last update: 01/09/94
 */


/*
    The Read data type is designed so that it can hold a varying degree
    of information about sequences, yet have a single set of calls
    to access the data.

    There are plenty of assumptions around that both the number of
    bases and the number of points will fit into an int_2, a short.

*/

/* ---- Includes ---- */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "Read.h"
#ifndef NO_ABI
#include "abi.h"
#endif
#include "scf.h"
#ifndef NO_ALF
#include "alf.h"
#endif
#include "plain.h"
#include "xalloc.h"
#include "filecompress.h"
#include "stadentranslate.h"
#include "expFileIO.h"
#include "dummy.h"
#include "traceType.h"
#include "misc.h"

/*
 * Read a sequence from a file "fnin" of format "format". If "format" is 0
 * (ANY_FORMAT), we automatically determine the correct format.
 * Returns:
 *   Read *   for success
 *   NULLRead for failure
 */
Read *read_reading(char *fn, int format) {
    Read *read;
    FILE *fp;

    if (NULL == (fp = fopen_compressed(fn, NULL))) {
	errout("'%s': couldn't open\n", fn);
	return NULL;
    }

    read = fread_reading(fp, fn, format);
    fclose(fp);

    return read;
}

/*
 * Write a sequence to a file "fn" of format "format". If "format" is 0,
 * we choose our favourite - SCF.
 *
 * Returns:
 *   0 for success
 *  -1 for failure
 */
int write_reading(char *fn, Read *read, int format) {
    Scf *scf;
    int ret;

    switch (format) {
    default:
	/* Defaults to SCF type */

    case TT_SCF:
	scf = read2scf(read);
	ret = write_scf(scf, fn);
	scf_deallocate(scf);
	break;

    case TT_ABI:
	ret = write_abi(fn, read);
	break;

    case TT_ALF:
	ret = write_alf(fn, read);
	break;

    case TT_EXP: {
	Exp_info *e = read2exp(read, fn);
	
	if (NULL == e) {
	    fprintf(stderr, "Failed to create experiment file.\n");
	    return -1;
	} else {
	    FILE *fp;

	    if (NULL == (fp = fopen(fn, "w"))) {
		fprintf(stderr, "Couldn't open file '%s'\n", fn);
		return -1;
	    }

	    exp_print_file(fp, e);
	    fclose(fp);
	    exp_destroy_info(e);

	    ret = 0;
	}

	break;
    }
	
    case TT_PLN:
	ret = write_pln(fn, read);
	break;
    }

    if (ret == 0)
	compress_file(fn);

    return ret;
}

/*
 * Read a sequence from a FILE *fp of format "format". If "format" is 0
 * (ANY_FORMAT), we automatically determine the correct format.
 * We still pass a filename 'fn' although this isn't used other than for
 * filling in the read->trace_name field.
 *
 * Returns:
 *   Read *   for success
 *   NULLRead for failure
 */
Read *fread_reading(FILE *fp, char *fn, int format) {
    Read *read;
    Scf *scf;
    FILE *newfp;

    newfp = freopen_compressed(fp, NULL);
    if (newfp != fp) {
	fp = newfp;
    } else {
	newfp = NULL;
    }
    if (format == TT_ANY)
	format = fdetermine_trace_type(fp);
    rewind(fp);

    switch (format) {
    case TT_UNK:
    case TT_ERR:
	errout("File '%s' has unknown trace type\n", fn);
	read = NULLRead;
	break;

    case TT_SCF:
	scf = fread_scf(fp);

	if (scf) {
	    read = scf2read(scf);
	    scf_deallocate(scf);

	    if ((read->trace_name = (char *)xmalloc(strlen(fn)+1)))
		strcpy(read->trace_name, fn);
	} else
	    read = NULLRead;

	break;

    case TT_CTF:
	read = fread_ctf(fp);
	break;

    case TT_ABI:
	read = fread_abi(fp);
	break;

    case TT_ALF:
	read = fread_alf(fp);
	break;

    case TT_EXP: {
	/* FIXME: we shouldn't redirect like this */
	Exp_info *e = exp_fread_info(fp);
	
	read = e ? exp2read(e) : NULLRead;       
	break;
    }
    case TT_PLN:
	read = fread_pln(fp);
	break;

    default:
	errout("Unknown format %d specified to read_reading()\n", format);
	read = NULLRead;
    }

    if (newfp) fclose(newfp);

    return read;
}

/*
 * Write a sequence to a FILE *fp of format "format". If "format" is 0,
 * we choose our favourite - SCF.
 *
 * Returns:
 *   0 for success
 *  -1 for failure
 */
int fwrite_reading(FILE *fp, Read *read, int format) {
    Scf *scf;
    int r = -1;

    switch (format) {
    default:
	/* Defaults to SCF type */

    case TT_SCF:
	scf = read2scf(read);
	r = fwrite_scf(scf, fp);
	scf_deallocate(scf);
	break;

    case TT_CTF:
	return fwrite_ctf(fp, read); 
	break;

    case TT_ABI:
	/*return fwrite_abi(fp, read); */
	break;

    case TT_ALF:
	/* return fwrite_alf(fp, read); */
	break;

    case TT_EXP: {
	Exp_info *e = read2exp(read, "unknown");
	
	if (NULL == e) {
	    fprintf(stderr, "Failed to create experiment file.\n");
	    r = -1;
	} else {
	    exp_print_file(fp, e);
	    exp_destroy_info(e);
	    r = 0;
	}
	break;
    }

    case TT_PLN:
	r = fwrite_pln(fp, read);
	break;
    }

    if (r == 0) {
	fcompress_file(fp);
    }

    return r;
}
