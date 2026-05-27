//
//  adjustfilepaths.c
//  mergedat
//
//  Created by Larry on 1/6/26.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>    // basename ()

#include "libcommon.h"
#include "drone.h"
#include "gopt.h"

int adjustfilepaths(GOPT *p) {
    DLST_INFO  dlst;
    char      *cp;
    long       len;
    
    // Do input files
    
    if (p->relpaths) {
        char *cpath;
        
        // Take care of Airdata csv
        if (buildadatapath(p->mission, &cp)) {
            printf("ERROR ***  buildadatapath() %s:%d\n\n",__func__,__LINE__);
            return 1;
        }
        
        len = strlen(cp);
        
        if (p->adcsv == NULL) {
            if (dgetdirfilelist(cp,&dlst,T_CSVFILE) == 1) {
                len += strlen(dlst.flist[0]) + 2;
                cpath = calloc(len,1);
                strcat(cpath,cp);
                strcat(cpath,dlst.flist[0]);
                p->adcsv = cpath;
                dmylstfree(&dlst);
            }
            else {
                printf("ERROR***  Invalid 'Airdata' directory content when using relative paths\n\n");
                return 1;
            }
        }
        else {
            // Cat file name to path unless the path is an absolute path
            if (pathtype(p->adcsv) != ABSOLUTE) {
                if (pathtype(p->adcsv) == BASE) {
                    char *tptr;
                    
                    if (!isaglob(p->adcsv)) {
                        tptr = p->adcsv;
                    }
                    else {
                        int idx;
                        
                        dgetdirfilelist(cp,&dlst,T_CSVFILE);
                        if ((idx=dglobme(&dlst,p->adcsv,".csv")) < 0) {
                            printf("ERROR*** Failed dglobme()  %s:%d\n\n",__func__,__LINE__);
                            return 1;
                        }
                        tptr = strdup(dlst.flist[idx]);
                        dmylstfree(&dlst);
                    }
                    len   += strlen(tptr);
                    cpath  = calloc(len,1);
                    strcat(cpath,cp);
                    strcat(cpath,tptr);
                    p->adcsv = cpath;
                }
                else {
                    printf("ERROR*** Relative paths not valid\n\n");
                    return 1;
                }
            }
            else {
                // All good. Nothing to do.
            }
        }
        
        // Take care of PhantomHelp csv
        if (buildphdatapath(p->mission, &cp)) {
            printf("ERROR ***  buildadatapath() %s:%d\n\n",__func__,__LINE__);
            return 1;
        }
        
        len = strlen(cp);
        
        if (p->phcsv == NULL) {
            if (dgetdirfilelist(cp,&dlst,T_CSVFILE) == 1) {
                len += strlen(dlst.flist[0]) + 2;
                cpath = calloc(len,1);
                strcat(cpath,cp);
                strcat(cpath,dlst.flist[0]);
                p->phcsv = cpath;
                dmylstfree(&dlst);
            }
            else {
                printf("ERROR***  Invalid 'PhantomHelp' directory content when using relative paths\n\n");
                return 1;
            }
        }
        else {
            // Cat file name to path unless the path is an absolute path
            if (pathtype(p->phcsv) != ABSOLUTE) {
                if (pathtype(p->phcsv) == BASE) {
                    char *tptr;
                    
                    if (!isaglob(p->phcsv)) {
                        tptr = p->phcsv;
                    }
                    else {
                        int idx;
                        
                        dgetdirfilelist(cp,&dlst,T_CSVFILE);
                        if ((idx=dglobme(&dlst,p->phcsv,".csv")) < 0) {
                            printf("ERROR*** Failed dglobme()  %s:%d\n\n",__func__,__LINE__);
                            return 1;
                        }
                        tptr = strdup(dlst.flist[idx]);
                        dmylstfree(&dlst);
                    }
                    len   += strlen(tptr);
                    cpath  = calloc(len,1);
                    strcat(cpath,cp);
                    strcat(cpath,tptr);
                    p->phcsv = cpath;
                }
                else {
                    printf("ERROR*** Relative paths not valid\n\n");
                    return 1;
                }
            }
            else {
                // All good. Nothing to do.
            }
        }
    }
    else {
        // No mission specified so no implied paths.
        if ((pathtype(p->adcsv) != ABSOLUTE) || (pathtype(p->phcsv) != ABSOLUTE)) {
            printf("ERROR***  Absolute path required for Airdata and PhantomHelp csv files when no mission specified\n\n");
            return 1;
        }
        else {
            // We're good. Nothing to do.
        }
    }
    

    // Output file
    if (p->relpaths) {
        // Mission specified. Output wil go to the Processed directory unless there's
        // an ouptut file name specified with an absolute path prefix.
        char *path,*name;
        
        buildprcspath(p->mission,&path);
        len = strlen(path);
        if (p->ofile != NULL) {
            if (pathtype(p->ofile) != ABSOLUTE) {
                if (pathtype(p->ofile) != BASE) {
                    printf("ERROR***  Output file name must not contain a relative path when Mission is specifed\n\n");
                    return 1;
                }
                len += strlen(p->ofile) + 1;
                name = calloc(len,1);
                sprintf(name,"%s%s",path,p->ofile);
                p->ofile = name;
            }
            else {
                // Nothing to do.
            }
        }
        else {
            // No output file name. Make one up. Use Airdata base name with prefix.
            char *lpath;
            
            len += strlen(basename(p->adcsv)) + strlen(ADPHMRGPREFX) + 1;
            lpath = calloc(len,1);
            sprintf(lpath,"%s%s%s",path,ADPHMRGPREFX,basename(p->adcsv));
            p->ofile = lpath;
        }
    }
    else {
        // No Mission specified. Use the input file name path prefix if either the
        // output file isn't specified or it is a relative path.
        char *path,*d = dirname(p->adcsv);
        long  len = strlen(d);
        if (p->ofile) {
            if (pathtype(p->ofile) != ABSOLUTE) {
                if (pathtype(p->ofile) != BASE) {
                    printf("ERROR***  Output file name must not contain a relative path when Mission is specifed\n\n");
                    return 1;
                }
                len += strlen(p->ofile) + 1;
                path = calloc(len,1);
                sprintf(path,"%s/%s",d,p->ofile);
                p->ofile = path;
            }
            else {
                // Nothing to do. Output file spec is an absolute path.
            }
        }
        else {
            len += strlen(basename(p->adcsv)) + strlen(ADPHMRGPREFX) + 1;
            path = calloc(len,1);
            sprintf(path,"%s/%s%s",d,ADPHMRGPREFX,basename(p->adcsv));
            p->ofile = path;
        }
    }
    return 0;
}
