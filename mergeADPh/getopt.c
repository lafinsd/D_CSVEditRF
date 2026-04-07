//
//  getopt.c
//  mergedat
//
//  Created by Larry on 8/4/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>    // basename ()

#include "cl_log2.0.h"
#include "drone.h"
#include "mergedat.h"
#include "gopt.h"
#include "adjustfilepaths.h"

int procopt(GOPT *p) {
    int lastoptind=1,gopt;
    
    p->adcsv    = NULL;
    p->phcsv    = NULL;
    p->ofile    = NULL;
    p->relpaths = 0;
    
    mkoptinfo(OPT_STRING);
    
    opterr = 0;   // suppress error message from getopt()
    while ((gopt=getopt(p->cl.argc, p->cl.argv, OPT_STRING)) != -1) {
        switch (gopt) {
            case 'w':
                p->relpaths  = 1;
                p->mission = optarg;
                break;
                
            case 'o' :
                p->ofile = optarg;
                break;
                
            case '?':
            default:
                printf("ERROR***  Illegal option \"%s\"\n\n", p->cl.argv[lastoptind]);
                return(1);
        }
        lastoptind = optind;
    }
    
    if (((p->cl.argc-2) != optind) && !p->relpaths) {
        printf("ERROR*** Bad argument count\n");
        return 1;
    }
    
    if (p->relpaths) {
        if (!missionisvalid(&p->mission)) {
            return 1;
        }
    }
    
    // Permit supplying no files, Airdata .csv only, or both Airdata and Phantom Help .csv files.
    int cnt = p->cl.argc - optind;
    
    if (cnt > 2) {
        printf("ERROR*** Too many arguments\n\n");
        return 1;
    }
    if (cnt > 0) {
        p->adcsv = p->cl.argv[optind];
    }
    
    if (cnt > 1) {
        p->phcsv = p->cl.argv[optind+1];
    }
    
    if (adjustfilepaths(p)) {
        return 1;
    }
    
    return 0;
}
