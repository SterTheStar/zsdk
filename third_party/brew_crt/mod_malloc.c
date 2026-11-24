/* =========================================================================

FILE: mod_malloc.c


DESCRIPTION: Override standard memory routines like malloc etc. to call
			 BREW equivalent.
  
PUBLIC CLASSES:  
   N/A

INITIALIZATION AND SEQUENCING REQUIREMENTS:  N/A


           Copyright (c)  2010 QUALCOMM Incorporated.
                     All Rights Reserved.
              QUALCOMM Proprietary and Confidential 
========================================================================= */


#include "AEEStdLib.h"

void * malloc ( size_t size )
{
	return MALLOC(size|ALLOC_NO_ZMEM);
}

void free(void *p)
{
	FREE(p);
}

void * realloc (void *p, size_t size )
{
	return REALLOC(p,size|ALLOC_NO_ZMEM);
}

void *calloc(size_t nelements, size_t elementSize)
{
	return MALLOC(nelements * elementSize);
}