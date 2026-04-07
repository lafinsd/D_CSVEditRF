//
//  process.c
//  mergedat
//
//  Created by Larry on 8/4/25.
//

#include <stdio.h>
#include <string.h>

#include "drone.h"
#include "mergedat.h"

#define SYNC_CHK_CRIT  10

static int   sync_chk(IOINFO *,int);
static void  colsubstitute(MAPTBL *,IOINFO *,int);
static void  coladd(MAPTBL *mtbl,IOINFO *io, int numin);

int mapit(MAPINFO *minfo, IOINFO *io, int numin) {
    int   i, retval = 0;
    
    if ((numin > 1) && ((numin-1) <= SYNC_CHK_CRIT)) {
        if (sync_chk(io,numin)) {
            printf("ERROR*** Line %d: sync check failed\n\n",numin);
            return 1;
        }
    }
    
    for (i=0; i<minfo->size; ++i) {
        MAPTBL *m = &minfo->mtbl[i];
        switch (minfo->mtbl[i].colinfo.type) {
            case SUBSTITUTE:
                colsubstitute(m, io, numin);
            case ADD:
                break;
        }
        memset(io->aird.buf,0,BUFSIZE);
        strcpy(io->aird.buf,io->outf.buf);
    }
    
    for (i=0; i<minfo->size; ++i) {
        switch (minfo->mtbl[i].colinfo.type) {
            case ADD:
                coladd(&minfo->mtbl[i], io, numin);
            case SUBSTITUTE:
                break;
        }
        
        if (i < (minfo->size-1)) {
            memset(io->aird.buf,0,BUFSIZE);
            strcpy(io->aird.buf,io->outf.buf);
        }
    }

    return retval;
}

static int sync_chk(IOINFO *io, int numin) {
    static struct {int adcol;int phcol;int len;} sCol2chk[] = {{2,4,10},{3,5,11}};
    char *adcp,*phcp;
    int   i,sz=sizeof(sCol2chk)/sizeof(sCol2chk[0]);
    
    for (i=0;i<sz;++i) {
        adcp = fldloc(io->aird.buf,sCol2chk[i].adcol);
        phcp = fldloc(io->phlp.buf,sCol2chk[i].phcol);
        
        if ((strchr(adcp,CSVFLDSEP)-adcp) < sCol2chk[i].len) {
            return 1;
        }
        if ((strchr(phcp,CSVFLDSEP)-phcp) < sCol2chk[i].len) {
            return 1;
        }
        
        if (strncmp(adcp,phcp,sCol2chk[i].len)) {
            return 1;
        }
    }
    
    return 0;
}

static void colsubstitute(MAPTBL *mtbl,IOINFO *io,int numin)
{
    int   curafldidx,curpfldidx,res;
    long  tlen;
    char *adfldptr, *phfldptr;
    char *optr     = io->outf.buf;
    char *curadptr = io->aird.buf;
    
    curafldidx = mtbl->adcol;
    curpfldidx = mtbl->phcol;
    
    adfldptr = fldloc(io->aird.buf, curafldidx);
    phfldptr = fldloc(io->phlp.buf, curpfldidx);
    
    strncpy(optr,curadptr,(tlen=(adfldptr-curadptr)));
    optr     += tlen;
    curadptr += tlen;
    if (numin == 1) {
        res = cpyfld(optr,curadptr,CSVFLDSEP);
        optr     += res;
        curadptr += res+1;
        *optr++   = CSVFLDSEP;
    }
    else {
        res = cpyfld(optr,phfldptr,CSVFLDSEP);
        optr     += res;
        curadptr = fldloc(io->aird.buf, curafldidx+1);
        *optr++   = CSVFLDSEP;
    }

    // Copy the rest when we're done
    strcpy(optr,curadptr);
}

static void coladd(MAPTBL *mtbl,IOINFO *io, int numin) {
    int  cnt = countcols(io->aird.buf) - 1;
    char *ncolptr = fldloc(io->aird.buf, cnt);
    char *optr    = io->outf.buf;
    char *srcptr  = fldloc(io->phlp.buf,mtbl->phcol);
    long len      = ncolptr - io->aird.buf;
    
    memset(optr,0,BUFSIZE);
    
    strncpy(optr,io->aird.buf,len);
    optr += len;
    if (numin == 1) {
        strcat((optr),mtbl->colinfo.header);
        strcat(optr,ncolptr);
    }
    else {
        int len = cpyfld(optr,srcptr,CSVFLDSEP);
        cpyfld(optr,srcptr,CSVFLDSEP);
        *(optr+len) = CSVFLDSEP;
        strcat(optr,ncolptr);
    }
    return;
}
