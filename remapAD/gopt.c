//
//  gopt.c
//  RID
//
//  Created by Larry on 2/14/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "cl_log2.0.h"
#include "drone.h"
#include "remap.h"
#include "gopt.h"

static int adjustfilenames(GOPT *);

int procopt(GOPT *p) {
    
    int lastoptind=1,gopt;
    
    mkoptinfo(OPT_STRING);
    
    p->options.msecdiff  = 0;
    p->options.kill      = 0;
    p->options.xtnd      = 1;
    p->options.relpaths  = 0;
    p->options.nofile    = 0;
    p->options.overwrite = 0;
    p->infile            = NULL;
    p->outfile           = NULL;
    p->options.fsxlate   = FSSAVE;
    p->options.reformat  = KEEPFMT;
    p->options.tcolSHOW  = SHOW;
    p->options.scolSHOW  = SHOW;
    p->options.dcolSHOW  = HIDE;
    p->options.collapse  = FLOAT;
    
    opterr = 0;   // suppress error message from getopt()
    while ((gopt=getopt(p->cl.argc, p->cl.argv, OPT_STRING)) != -1) {
        errno = 0;
        switch (gopt) {
            case 'o':
                if (*optarg == OVERWRITE) {
                    p->options.overwrite = 1;
                    if (strlen(optarg) > 1) {
                        p->outfile = optarg+1;
                    }
                }
                else {
                    p->outfile = optarg;
                }
                break;
                
            case 'w':
                p->options.relpaths = 1;
                p->options.mission  = optarg;
                break;
                
            case 'k':
                p->options.xtnd = 0;
                break;
                
            case 'r':
                p->options.kill = 1;
                break;
                
            case 'R':
                p->options.nofile = 1;
                break;
                
            case 'd':
                p->options.dcolSHOW = SHOW;
                break;
                
            case 't':
                // Translate fly state text
                p->options.fsxlate = FSXLATE;
                break;
                
            case 'h':
                // Hide the XYZ columns
                p->options.scolSHOW = HIDE;
                break;
                
            case 'c':
                // Collapse the XYZ speeds
                p->options.collapse = FLOATx;
                break;
                
            case 'S':
                // select and reformat time spacing between rows but keep the elapsed time column.
                if (optarg) {
                    p->options.msecdiff = (int)strtol(optarg,NULL,10);
                }
                else {
                    p->options.msecdiff = 0;
                }
                p->options.reformat = REFMT;
                break;
                
            case 's':
                // select msec time spacing between rows and kill the msec elapsed time column
                p->options.tcolSHOW = HIDE;
                if (optarg) {
                    p->options.msecdiff = (int)strtol(optarg,NULL,10);
                }
                else {
                    p->options.msecdiff = 0;
                }
                p->options.reformat = KEEPFMT;
                break;
                
            case '?':
            default:
                printf("ERROR***  Illegal option \"%s\"\n", p->cl.argv[lastoptind]);
                return(1);
        }
        lastoptind = optind;
    }
    
    if (p->options.relpaths) {
        if (!missionisvalid(&(p->options.mission))) {
            return 1;
        }
    }
    
    if (optind == p->cl.argc) {
        if (!p->options.relpaths) {
            // missing input file
            printf("ERROR***  No Mission specified and no input file name\n\n");
            return 1;
        }
    }
    else if (optind != (p->cl.argc-1)) {
        printf("ERROR***  Extraneous argument(s)\n\n");
        return 1;
    }
    else {
        p->infile = p->cl.argv[optind];
    }
    
    if (adjustfilenames(p)) {
        return 1;
    }
    
    return 0;
}

static int adjustfilenames(GOPT *p) {
    char *dpath=NULL,*fname,*fullfn;
    long  len;
    
    // Input file
    if (p->infile != NULL) {
        if (pathtype(p->infile) == ABSOLUTE) {
            // We're done.
            return 0;
        }
        if (pathtype(p->infile) == RELATIVE) {
            printf("ERROR*** Relative file paths not valid\n\n");
            return 1;
        }
        if (isaglob(p->infile)) {
            WHICHP w[] = {ADATA,PRCS};
            int sz = sizeof(w)/sizeof(w[0]);
            DGLOB_CONTEXT gctxt = {p->options.mission,p->infile,".csv"};
            
            if (dloop_glob(sz,w,&gctxt)) {
                return 1;
            }
            
            fname = gctxt.globres;
            dpath = gctxt.pathptr;
            dmylstfree(&gctxt.dlst);
        }
        else {
            PFXTYPE pfx = isprefixstd(p->infile);
            if (pfx == NONE || pfx == CSVRAW) {
                if (buildadatapath (p->options.mission,&dpath)) {
                    printf("ERROR*** buildadatapath()  %s:%d",__func__,__LINE__);
                    return 1;
                }
            }
            if (pfx == MERGEADPH) {
                if (buildprcspath(p->options.mission,&dpath)) {
                    printf("ERROR*** buildprcspath()  %s:%d",__func__,__LINE__);
                    return 1;
                }
            }
            fname = p->infile;
        }
    }
    else {
        // Mission name provided and no input file specified. Assume we want the
        // (single) csv file in the Airdata Data directory.
        DLST_INFO dlst;
        
        if (buildadatapath(p->options.mission, &dpath)) {
            printf("ERROR*** buildadatapath()  %s:%d",__func__,__LINE__);
            return 1;
        }
        
        if (dgetdirfilelist(dpath, &dlst,T_CSVFILE) != 1) {
            printf("ERROR***  No single csv file found in Airdata directory\n\n");
            return 1;
        }
        fname = strdup(dlst.flist[0]);
        dmylstfree(&dlst);
    }
    
    len    = strlen(dpath) + strlen(fname) + 1;
    fullfn = calloc(len,1);
    sprintf(fullfn,"%s%s",dpath,fname);
    p->infile = fullfn;
    free(dpath);
    
    // Output file
    
    return 0;
}
