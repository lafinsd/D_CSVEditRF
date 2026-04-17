//
//  tableget.c
//  mergedat
//
//  Created by Larry on 10/23/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/xattr.h> // for get/setxattr()

#include "drone.h"
#include "cl_log2.0.h"
#include "mergedat.h"

static int subit(char *,MAPTBL *);
static int addit(char *,MAPTBL *);

int getminfo(char **prog,MAPINFO *m) {
    FILE *in,*logf=NULL;
    char *linecp,name[150],line[100];
    int cnt=0,keepgoing = 1;
    MAPTBL *mp=NULL,*curmp;
    
    m->size = 0;
    
    {
        char *fname;
        char *user,*lfn;
        ssize_t res;
        CMD_LINE p;
        
        p.argv = prog;
        
        sprintf(name,"%s/Applications/Application Support/",getenv("HOME"));
        
        fname = whereami(&p);
        
        if ((res=getxattr(fname, "user.intable", NULL, 0, 0, 0)) > 0) {
            user = calloc(++res,1);
            getxattr(fname, "user.intable", user, res, 0, 0);
            strcat(name,user);
            free(user);
        }
        else {
            strcat(name,"drone/merge");
        }
        
        if ((res=getxattr(fname, "user.log", NULL, 0, 0, 0)) > 0) {
            lfn = calloc(++res,1);
            getxattr(fname, "user.log", lfn, res, 0, 0);
            logf = fopen(lfn,"a+");
            free(lfn);
        }
        else {
            logf = NULL;
        }
        free(fname);
    }
    
    if ((in=fopen(name, "r")) == NULL) {
        printf("INFO*** No merge table file. Using default.\n\n");
        if (logf != NULL) {
            fprintf(logf,"   INFO*** No merge table file. Using default.\n");
        }
        return -1;
    }
    
    while (((fgets(line,sizeof(line),in)) != NULL) && keepgoing) {
        if ((*line == '#') || (lineisblank(line))) continue;

        linecp = line;
        while (*linecp == ' ') ++linecp;
        
        if (!cnt) {
            mp    = (MAPTBL *)malloc(sizeof(MAPTBL));
            curmp = mp;
        }
        else {
            mp    = (MAPTBL *)realloc(mp,(cnt+1)*sizeof(MAPTBL));
            curmp = &mp[cnt];
        }
        
        switch (*linecp) {
            case CMDSUB:
                subit(line,curmp);
                break;
                
            case CMDADD:
                addit(line,curmp);
                break;
                
            default:
                printf("ERROR*** Illegal command '%c'\n%s\n\n",*linecp,line);
                return 1;
        }
        cnt++;
        
        printf("%s",line);
    }
    m->size = cnt;
    m->mtbl = mp;
    return 0;
}

static int subit(char *line,MAPTBL *m) {
    char *cp = fldloc(line,1);
    
    if (!sscanf(cp,"%d",&m->adcol)) {
        printf("ERROR*** subit() missing or bad field in line:\n%s\n\n",line);
        return 1;
    }
    cp = fldloc(line,2);
    if (!sscanf(cp,"%d",&m->phcol)) {
        printf("ERROR*** subit() missing or bad field in line:\n%s\n\n",line);
        return 1;
    }
    m->colinfo.type   = SUBSTITUTE;
    m->colinfo.header = NULL;
    return 0;
}

static int addit(char *line,MAPTBL *m) {
    char *cp = fldloc(line,1);
    char str[50];
    
    if (!sscanf(cp,"%d",&m->adcol)) {
        printf("ERROR*** subit() missing or bad field in line:\n%s\n\n",line);
        return 1;
    }
    m->adcol = -1;
    
    if (!sscanf(cp,"%d",&m->phcol)) {
        printf("ERROR*** subit() missing or bad field in line:\n%s\n\n",line);
        return 1;
    }
    cp = fldloc(line,2);
    if (sscanf(cp,"%[^\n]",str)) {
        long len = strlen(str);
        char *cp = calloc(len+2,1);
        
        strcpy(cp,str);
        if (*(cp+len-1) != ',') {
            *(cp+len) = ',';
        }
        m->colinfo.type = ADD;
        m->colinfo.header = cp;
    }
    
    return 0;
}
