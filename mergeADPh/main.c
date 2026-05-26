//
//  main.c
//  mergedat
//
//  Created by Larry on 8/4/25.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>    // basename ()
#include <sys/xattr.h> // for get/setxattr()

#include "cl_log2.0.h"
#include "drone.h"
#include "mergedat.h"
#include "gopt.h"
#include "tableget.h"
#include "libcommon.h"

#define VERSION "3.0"

// This table must be in increasing order of Airdata column location.
static MAPTBL sMap[] = {
  // adcol  phcol            colinfo
    {   50,    24,  {SUBSTITUTE,NULL}},
    {   -1,    23,  {ADD,"travel hdg,"}},
    {   -1,    10,  {ADD,"travel dist,"}}
};

static int  sanitycheck(char *,char *);
static void tabsort(MAPINFO *);

int main(int argc, char **argv) {
    GOPT opts = {{argc,argv}};
    MAPINFO minfo;
    IOINFO io;
    int numin = 0;
    char *pbuf = buildcontext(argv[0]);
    
    printf("%s %s-Llib:%s-Dlib:%s-Clib:%s (%s %s Build:%s)\n\n", basename(argv[0]), VERSION, getLogLibVer(),getDroneLibVer(), getCommonLibVer(),__DATE__, __TIME__,pbuf);
    
    
    if (procopt(&opts)) {
        printf("Usage: %s [-w<%s:<m>>] [-o<outfilename>] <adfilename> <phfilename>\n\n",basename(argv[0]),mkhintstring());
        return 1;
    }
    
    if ((io.aird.fp=fopen(opts.adcsv, "r")) == NULL)  {
        printf("ERROR***  Cannot open input file %s\n", opts.adcsv);
        return 1;
    }
    
    if ((io.phlp.fp=fopen(opts.phcsv, "r")) == NULL)  {
        printf("ERROR***  Cannot open input file %s\n", opts.phcsv);
        return 1;
    }
    
    if ((io.outf.fp=fopen(opts.ofile,"w")) == NULL) {
        printf("ERROR***  Cannot open input file output file %s\n", opts.ofile);
        return 1;
    }
    
    io.aird.buf = calloc(BUFSIZE+1,1);
    io.phlp.buf = calloc(BUFSIZE+1,1);
    io.outf.buf = calloc(BUFSIZE,1);
    
    char *usr = NULL;
    if (opts.relpaths) {
        logCLI(&opts.cl, VERSION,__DATE__,__TIME__,&usr);
    }
    else {
        logCLI(&opts.cl, VERSION,__DATE__,__TIME__,NULL);
    }
    
    fgets(io.aird.buf,BUFSIZE,io.aird.fp);
    fgets(io.phlp.buf,BUFSIZE,io.phlp.fp);

    {
        int res;
        
        if ((res=getminfo(argv,&minfo))) {
            if (res > 0) {
                return 1;
            }
            minfo.mtbl = sMap;
            minfo.size = sizeof(sMap)/sizeof(sMap[0]);
        }
    }
    tabsort(&minfo);
    
    if (sanitycheck(io.aird.buf,io.phlp.buf)) {
        printf("ERROR*** Failed sanity check\n\n");
        return 1;
    }
    
    do {
        int res;
        
        numin++;
        memset(io.outf.buf,0,BUFSIZE);
        
        if ((res=mapit(&minfo,&io,numin))) {
            break;
        }
        
        fprintf(io.outf.fp, "%s", io.outf.buf);
        
        memset(io.aird.buf,0,BUFSIZE);
        memset(io.phlp.buf,0,BUFSIZE);
    } while ((fgets(io.aird.buf,BUFSIZE,io.aird.fp) != NULL) && (fgets(io.phlp.buf,BUFSIZE,io.phlp.fp) != NULL));
    
    fclose(io.aird.fp);
    fclose(io.phlp.fp);
    if(usr != NULL) {
        fsetxattr(fileno(io.outf.fp),"user.cli",usr,strlen(usr),0,0);
    }
    fclose(io.outf.fp);
    
    return 0;
}

static int sanitycheck(char *adstr, char *phstr) {
    char *tstrph = "CUSTOM.date [local]";
    char *tstrad = "time(millisecond)";
    
    if (strncmp(tstrad,adstr,strlen(tstrad))) {
        return 1;
    }
    
    if (strncmp(tstrph,phstr,strlen(tstrph))) {
        return 1;
    }
    
    return 0;
}

#define MAXACOL 1000
static void tabsort(MAPINFO *m) {
    int    sz = m->size;
    MAPTBL mtbl[sz];
    int    i,keepgoing,residx=0;
    
    for (i=0;i<sz;++i) {
        mtbl[i] = m->mtbl[i];
    }
    
    //find the smallest substitute command until all are done.
    keepgoing = 1;
    while (keepgoing && (keepgoing <= sz)) {  // Include a run-away guard
        int smlst=-1;
        int cursmlst=MAXACOL;
        
        for (i=0;i<sz;++i) {
            if ((mtbl[i].colinfo.type == SUBSTITUTE) && (mtbl[i].adcol < cursmlst)) {
                cursmlst = mtbl[i].adcol;
                smlst    = i;
            }
        }
        if (cursmlst == MAXACOL) {
            keepgoing = 0;
        }
        else {
            m->mtbl[residx++] = mtbl[smlst];
            mtbl[smlst].adcol = MAXACOL;
            keepgoing++;
        }
    }
    
    if (keepgoing > (sz+1)) {
        printf("\nFATAL*** Runaway processing MAPTBL\n\n");
        exit(0);
    }
    
    // Done with substitutions. Copy the adds.
    for (i=0;i<sz;++i) {
        if (mtbl[i].colinfo.type == ADD) {
            m->mtbl[residx++] = mtbl[i];
        }
    }
    return;
}
