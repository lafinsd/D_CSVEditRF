//
//  getopt.c
//  remapPh
//
//  Created by Larry on 7/17/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>   // basename ()

#include "libcommon.h"
#include "drone.h"
#include "remap.h"
#include "gopt.h"

static int adjustpaths(GOPT *);

int procopt(GOPT *p) {
    
    int lastoptind=1,gopt;
    
    mkoptinfo(OPT_STRING);
    
    p->options.msecdiff = 0;
    p->options.relpaths = 0;
    p->options.xtnd     = 1;
    p->options.fsxlate  = FSSAVE;
    p->options.reformat = KEEPFMT;
    p->options.tcolSHOW = SHOW;
    p->options.scolSHOW = SHOW;
    p->options.collapse = FLOAT;
    p->options.dcolSHOW = HIDE;
    p->outfile          = NULL;
    
    opterr = 0;   // suppress error message from getopt()
    while ((gopt=getopt(p->cl.argc, p->cl.argv, OPT_STRING)) != -1) {
        errno = 0;
        switch (gopt) {
            case 'o':
                p->outfile = optarg;
                break;
                
            case 'w':
                p->options.relpaths = 1;
                p->options.mission  = optarg;
                
                break;
                
            case 'k':
                p->options.xtnd = 0;
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
                printf("ERROR***  Illegal option \"%s\"\n\n", p->cl.argv[lastoptind]);
                return(1);
        }
        lastoptind = optind;
    }
    
    if (optind == p->cl.argc) {
        // missing input file
        printf("ERROR***  Missing input file name\n\n");
        return 1;
    }
    else if (optind != (p->cl.argc-1)) {
        printf("ERROR***  Extraneous argument(s)\n\n");
        return 1;
    }
    
    if (p->options.relpaths) {
        if (!missionisvalid(&(p->options.mission))) {
            return 1;
        }
    }
    
    p->infile = p->cl.argv[optind];
    
    if (adjustpaths(p)) {
        return 1;
    }
    
    return 0;
}

static int adjustpaths(GOPT *p) {
    char *cp,*path;
    long  len;
    
    // input file name
    if (pathtype(p->infile) != ABSOLUTE) {
        if (!p->options.relpaths) {
            printf("ERROR***  Absolute paths required without Mission option ('-w')\n\n");
            return 1;
        }
        if (pathtype(p->infile) == RELATIVE) {
            // We're done
            printf("ERROR***  Relative paths not allowed\n\n");
            return 1;
        }
        
        if (isaglob(p->infile)) {
            WHICHP w[] = {PDATA};
            int sz = sizeof(w)/sizeof(w[0]);
            DGLOB_CONTEXT gctxt = {p->options.mission,p->infile,".csv"};
            
            if (dloop_glob(sz,w,&gctxt)) {
                return 1;
            }
            
            p->infile = gctxt.globres;
            cp = gctxt.pathptr;
        }
        else {
            if (buildphdatapath(p->options.mission, &cp)) {
                printf("ERROR***  buildphdatapath() %s:%d\n\n",__func__,__LINE__);
                return 1;
            }
        }
        len = strlen(cp) + strlen(p->infile) + 1;
        path = calloc(len,1);
        sprintf(path,"%s%s",cp,p->infile);
        p->infile = path;
        free(cp);
    }

    
    
    // output file name
    
    if (p->outfile != NULL) {
        // Name supplied
        if (pathtype(p->outfile) == ABSOLUTE) {
            // We're done
            return 0;
        }
        
        if (!p->options.relpaths) {
            printf("ERROR***  Absolute paths required without Mission option ('-w')\n\n");
            return 1;
        }
        
        if (pathtype(p->outfile) == RELATIVE) {
            // We're done
            printf("ERROR***  Relative paths not allowed\n\n");
            return 1;
        }
        
        if (buildprcspath(p->options.mission, &cp)) {
            printf("ERROR***  buildprcspath() %s:%d\n\n",__func__,__LINE__);
            return 1;
        }
        
        len = strlen(cp) + strlen(p->outfile) + 1;
        path = calloc(len,1);
        sprintf(path,"%s%s",cp,p->outfile);
        p->outfile = path;
        free(cp);
    }
    else {
        // No name supplied
        if (buildprcspath(p->options.mission, &cp)) {
            printf("ERROR***  buildprcspath() %s:%d\n\n",__func__,__LINE__);
            return 1;
        }
        
        char *pfx = CMPRSDCSVPH;
        len = strlen(cp) + strlen(pfx) + strlen(basename(p->infile)) + 1;
        path = calloc(len,1);
        sprintf(path,"%s%s%s",cp,CMPRSDCSVPH,basename(p->infile));
        p->outfile = path;
        free(cp);
    }
    
    return 0;
}
