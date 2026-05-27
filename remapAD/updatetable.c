//
//  updatetable.c
//  remapAD
//
//  Created by Larry on 10/21/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/xattr.h> // for get/setxattr()

#include "drone.h"
#include "libcommon.h"
#include "remap.h"

static int  add2end(int,KEEPCOL *,char *);
static int  add2beg(int,KEEPCOL *,char *);
static void delete(KEEPCOL *);
static int  cntaugs(KEEPCOL *);

// This has to be called AFTER checking the input csv file for added columns. If there are
// none then we've already removed any hard coded augmented columns from the table. So
// don't now process anything specified in the amendment file.
int updateTable(CMD_LINE *p,RMAPINFO *minfo) {
    FILE *in;
    char *cp;
    char name[100],line[100],str[100];
    int  retval=0;
    int  tsize = minfo->c.size;
    FILE *logf;
    
    {
        char *fname;
        char *user,*lfn;
        ssize_t res;
        
        sprintf(name,"%s/Applications/Application Support/",getenv("HOME"));
        
        fname = whereami(p);
        
        if ((res=getxattr(fname, "user.intable", NULL, 0, 0, 0)) > 0) {
            user = calloc(++res,1);
            getxattr(fname, "user.intable", user, res, 0, 0);
            strcat(name,user);
            free(user);
        }
        else {
            strcat(name,"drone/keepcol");
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
        printf("INFO*** No run time table augmentation file\n\n");
        if (logf != NULL) {
            fprintf(logf,"   INFO*** No run time table augmentation file\n");
        }
        return retval;
    }
    
    while (((fgets(line,sizeof(line),in)) != NULL) && !retval) {
        int  rnum;
        char cmd;
        
        if ((*line == '#') || (lineisblank(line))) continue;
        
        if (!sscanf(line,"%d",&rnum)) {
            printf("ERROR*** Bad or missing row number in table update line:\n%s\n\n",line);
            retval = 1;
            continue;
        }
        if ((cp=strchr(line,CSVFLDSEP)) == NULL) {
            printf("ERROR*** Bad table update line:\n%s\n\n",line);
            retval = 1;
            continue;
        }
        if (!sscanf(cp+1,"%c",&cmd)) {
            printf("ERROR*** Bad or missing command in table update line:\n%s\n\n",line);
            retval = 1;
            continue;
        }
        if ((cp=strchr(cp+1,CSVFLDSEP)) == NULL) {
            if (cmd != 'd') {
                printf("ERROR*** Bad or missing string in table update line:\n%s\n\n",line);
                retval = 1;
                continue;
            }
        }
        else {
            if (!sscanf(cp+1,"%[^\n]",str)) {  // read a string that may have spaces up to a newline
                printf("ERROR*** Bad or missing string in table update line:\n%s\n\n",line);
                retval = 1;
                continue;
            }
            if (strchr(str,AUGNCHAR) == NULL) {
                printf("ERROR*** Replacement string \"%s\" does not contain separation character '%c'\n\n",str,AUGNCHAR);
                retval = 1;
                continue;
            }
            
            // Strip out comment from 'str'
            {
                char *cp = strchr(str,'#');
                
                if (cp != NULL) {
                    *cp = '\0';
                }
            }
        }
        
        if ((rnum < 0) || (rnum >= tsize)) {
            printf("ERROR*** Table update row (%d) out of range (0-%d)\n\n",rnum,tsize-1);
            retval = 1;
            continue;
        }
        
        switch (cmd) {
            case 'b':
                if (add2beg(rnum,&minfo->c.map[rnum],str))  {
                    retval = 1;
                    continue;
                }
                break;
                
            case 'e':
                if (add2end(rnum,&minfo->c.map[rnum],str)) {
                    retval = 1;
                    continue;
                }
                break;
                
            case 'd':
                delete(&minfo->c.map[rnum]);
                break;
                
            default:
                printf("ERROR*** Bad table update command %c\n\n",cmd);
                retval = 1;
                continue;
        }
        
        printf("Table row %2d updated: \n%s\n",rnum,line);
        if (logf != NULL) {
            fprintf(logf,"   Table row %2d updated: \n   %s\n",rnum,line);
        }
    
    }
    printf("\n");
    if (logf != NULL) {
        fclose (logf);
    }
    
    return retval;
}

static int add2end(int row,KEEPCOL *c,char *str) {
    int  frstnull;
    
    if (cntaugs(c)) {
        printf("ERROR*** Table row %d already contains an augmented column\n\n",row);
        return 1;
    }
    
    for (frstnull=0;frstnull<(NUMCNAMES);++frstnull) {
        if (c->cnamelist.cnames[frstnull] != NULL) {
            continue;
        }
        break;
    }
    
    if (frstnull == NUMCNAMES) {
        printf("ERROR*** Row %d: no room at in column name array for new name '%s'\n\n",row,str);
        return 1;
    }
    
    c->cnamelist.cnames[frstnull] = calloc(strlen(str)+1,1);
    strcpy(c->cnamelist.cnames[frstnull],str);
    
    return 0;
}

static void delete(KEEPCOL *c) {
    int i;
    
    for (i=0;i<NUMCNAMES;++i) {
        if (c->cnamelist.cnames[i] != NULL)  {
            char *cp = strchr(c->cnamelist.cnames[i],AUGNCHAR);
            if (cp != NULL) {
                c->cnamelist.cnames[i] = NULL;
            }
        }
    }
    return;
}

static int add2beg(int row,KEEPCOL *c,char *str) {
    int   i,frstnull;
    
    if (cntaugs(c)) {
        printf("ERROR*** Table row %d already contains an augmented column\n\n",row);
        return 1;
    }
    
    for (frstnull=0;frstnull<(NUMCNAMES-1);++frstnull) {
        if (c->cnamelist.cnames[frstnull] != NULL) {
            continue;
        }
        break;
    }
    
    if (frstnull == NUMCNAMES) {
        printf("ERROR*** Row %d: no room at in column name array for new name '%s'\n\n",row,str);
        return 1;
    }
    
    for (i=frstnull;i>0;--i) {
        c->cnamelist.cnames[i] = c->cnamelist.cnames[i-1];
    }
    
    c->cnamelist.cnames[0] = calloc(strlen(str)+1,1);
    strcpy(c->cnamelist.cnames[0],str);
    
    c->cnamelist.cnidx++;
    
    return 0;
}

static int cntaugs(KEEPCOL *c) {
    int i,cnt = 0;
    char *cp;
    
    for (i=0;i<NUMCNAMES;++i) {
        cp = c->cnamelist.cnames[i];
        if ((cp != NULL) &&  (strchr(cp,AUGNCHAR) != NULL)) {
            cnt++;
        }
    }
    return cnt;
}
