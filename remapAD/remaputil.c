//
//  remaputil.c
//  remapAD
//
//  Created by Larry on 2/20/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drone.h"
#include "remap.h"
#include "common.h"

static char  *procDTG (char *, char *);
static char  *procMMAGL(char *, char *, int);
static char  *procSAVEMS(char *);
static char  *numnames(KEEPCOL, char *);
static char   procVTOGGLE(char *, KEEPCOL);
static char  *preAug(COLNAMES, char *, char *);
static char  *postAug(COLNAMES, char *, char *);

TMFMT manageTMFMT(ACT, TMFMT);

static int gCurMsec  = 0;

int gShowLine = 0;

MSG_STRNGS sMsg_strngs;

// Process column heading(s). This is where extra columns are added
// if names are supplied. The target column is specified by the
// 'cnidx' element which may itself use the default name.
// This function guarantees the returned string is populated.
static char *numnames(KEEPCOL col, char *ibuf) {
    static char sTbuf[50];
    
    COLNAMES nm = col.cnamelist;
    int count   = 0;
    int i;

    memset(sTbuf,0,sizeof(sTbuf));
    
    // Go through the array of names and count how many are there.
    for (i=0; i<NUMCNAMES; ++i) {
        if (nm.cnames[i] != NULL) {
            // A name is supplied.
            count++;
        }
        else {
            if (i == nm.cnidx) {
                // No name supplied for the column. Use default. Force the loop to run.
                count++;
            }
        }
    }
    
    // Now construct the returned column names in an augmented csv format.
    for (i=0; (i<NUMCNAMES) && count; ++i) {
        if (nm.cnames[i] != NULL) {
            char *cp = strchr(nm.cnames[i],AUGNCHAR);   // Augmented column indicator?
            
            if (cp == NULL) {
                strcat(sTbuf,nm.cnames[i]);
            }
            else {
                char tbuf[50];
                long idx = cp - nm.cnames[i];
                int  cnt;
                
                // Processing an augmented column. Heading taken from augmented input file 
                // heading if there isn't a name in the table.
                if (!idx) {
                    int   acol;
                    
                    sscanf((cp+1),"%d",&acol);
                    cnt = cpyfld(tbuf,fldloc(ibuf,acol),CSVFLDSEP);
                    tbuf[cnt] = '\0';
                    strcat(sTbuf,tbuf);
                }
                else {
                    strcpy(tbuf,nm.cnames[i]);
                    tbuf[idx] = '\0';
                    strcat(sTbuf,tbuf);
                }
            }
            if (--count) {
                strcat(sTbuf,",");
            }
        }
        else {
            // We're using the default column name in the constructed string.
            // Copy it into the string.
            if (i == nm.cnidx) {
                int k = (int)strlen(sTbuf);
                char *fld = fldloc(ibuf,col.col);
                
                while ((*fld != CSVFLDSEP) && (*fld != EOL)) {
                    sTbuf[k++] = *fld++;
                }
                if (--count) {
                    sTbuf[k] = CSVFLDSEP;
                }
            }
        }
    }

    return sTbuf;
}

char *procAUX(COLMAP *cmap, int idx, int numline, char *ibuf, char *obuf, char *basebuf, int numout) {
    KEEPCOL  col = cmap->map[idx];
    char    *fld = fldloc(ibuf, col.col);
    
    {
        if (numline == 1) {
            char *cp = numnames(col, ibuf);
            int   len;
            
            if ((len=(int)strlen(cp))) {
                strcpy(obuf, cp);
                return obuf + len;
            }
        }
    }
    
    obuf = preAug(col.cnamelist,ibuf,obuf);
    
    switch (col.aux.which) {
        case HDGDIF:
            {
                char lfmt[10];
                strcpy(lfmt,"%.3f");
                char *cp = strchr(lfmt,'.') + 1;
                
                float diff = procHDGDIF(fld,ibuf);
                
                fld = fmtflt(&col.aux.helper.fmt,fld);
                *cp = '0' + col.aux.helper.fmt.rod;
                
                obuf += cpyfld(obuf,fld,CSVFLDSEP);
                *obuf++ = ',';
                {
                    char tbuf[20];
                    
                    sprintf(tbuf,lfmt,diff);
                    strcat(obuf,tbuf);
                    obuf += strlen(tbuf);
                }
            }
            break;
            
        case VIDEO:
            {
                char c = procVTOGGLE(ibuf, col);
                
                if (col.status == SHOW) {
                    *obuf++ = c;
                }
            }
            break;
            
        case MSG:
            {
                if (sMsg_strngs.count) {
                    int i,cnt=sMsg_strngs.count;
                    
                    *obuf++ = '"';*obuf++ = LMARK;
                    for (i=0; i<cnt; ++i) {
                        strcpy(obuf,sMsg_strngs.strings[i]);
                        obuf += strlen(sMsg_strngs.strings[i]);
                        *obuf++ = '.';
                        if (i < (cnt-1)) {
                            *obuf++ = ' ';
                        }
                        free(sMsg_strngs.strings[i]);
                    }
                    *obuf++ = RMARK;*obuf++ = ' ';
                    if (strlen(fld) == 1) {
                        *obuf++ = '"';
                    }
                    else {
                        fld++;   // crush leading '"'
                        while ((*fld != CSVFLDSEP) && (*fld != EOL)) {
                            *obuf++ = *fld++;
                        }
                    }
                    sMsg_strngs.count = 0;
                }
                else {
                    while ((*fld != CSVFLDSEP) && (*fld != EOL)) {
                        *obuf++ = *fld++;
                    }
                }
            }
            break;
            
        case MAXDST:
            obuf = procMDST(fld,obuf,numout);
            break;
            
        case FLOATx:
            fld = fmtflt(&col.aux.helper.fmt,fld);
            
            obuf += cpyfld(obuf,fld,CSVFLDSEP);
            *obuf++ = BLANKSP;
            *obuf++ = LPAREN;
            {
                struct {uint8_t which;char sep;} cpy[] = {{OMXSP,SUBFSEP},{OMYSP,SUBFSEP},{OMZSP,RPAREN}};
                FMTD  fmt;
                char *cp;
                int   o2l,i;
                
                for (i=0;i<sizeof(cpy)/sizeof(cpy[0]); ++i) {
                    int omval = cpy[i].which;
                    
                    // Get the left of decimal and right of decimal counts for the reformatted field
                    if ((o2l=cmap->revmap.idx[omval]) < 0) {
                        return NULL;
                    }
                    
                    fmt = cmap->map[o2l].aux.helper.fmt;
                    cp  = fmtflt(&fmt,fldloc(ibuf,omval));
                    
                    // Now copy the reformatted field to the output buffer
                    while (*cp != CSVFLDSEP) {
                        if (*cp == BLANKSP) {
                            cp++;
                        }
                        else {
                            *obuf++ = *cp++;
                        }
                    }
                    *obuf++ = cpy[i].sep;
                }
            }
            break;
            
        case FLOAT:
            if (numline > 1) {
                fld = fmtflt(&col.aux.helper.fmt,fld);
            }
            
            obuf += cpyfld(obuf,fld,CSVFLDSEP);
            break;
            
        case SAVEMS:
            obuf += cpyfld(obuf,procSAVEMS(fld),CSVFLDSEP);
            break;
                
        case FSXLATE:
        case FSSAVE:
            fld = chkXlate(col.aux.which, fld);
        // NO BREAK
        case AUXB_NONE:
            while ((*fld != CSVFLDSEP) && (*fld != EOL)) {
                *obuf++ = *fld++;
            }
            break;
            
        case MINMAXAGL:
            obuf = procMMAGL(fld,obuf,numout);
            break;
            
        case TIME:
            if (numline > 1) {
                obuf = procDTG(fld, obuf);
            }
            if (*(obuf-1) == BLANKSP) {
                printf ("Missing or corrupt DTG line %d\n", numline);
            }
            break;
    }
    
    obuf = postAug(col.cnamelist,ibuf,obuf);
    
    return obuf;
}


static char *procSAVEMS(char *str)  {
    static char rfmt[50];
    int mins,secs,tenths;
    
    sscanf(str,"%d", &gCurMsec);
    if (manageTMFMT(GET, FMTDUMMY) == REFMT) {
        mins   = gCurMsec/60000;
        secs   = gCurMsec/1000 - mins*60;
        tenths = (gCurMsec - mins*60000 - secs*1000)/100;
        sprintf(rfmt,"%2d:%02d.%d,",mins,secs,tenths);
        return rfmt;
    }
    
    return str;
}


static char *procMMAGL(char *str, char *obuf, int num) {
    static float sMinagl = 0;
    static float sMaxagl = 0;
    static int   sMinlin = 2;
    static int   sMaxlin = 2;
    
    float tmpagl;
    char  aglbuf[75];
    char *cpmin = "";
    char *cpmax = "";
    
    sscanf(str,"%f", &tmpagl);
    
    if (tmpagl < sMinagl) {
        sMinagl = tmpagl;
        cpmin   = "new:";
        sMinlin = num + 1;
    }
    
    if (tmpagl > sMaxagl) {
        sMaxagl = tmpagl;
        cpmax   = "new:";
        sMaxlin = num + 1;
    }
    memset(aglbuf,0,sizeof(aglbuf));
    if (!gShowLine) {
        sprintf(aglbuf,"%.1f,%s%.1f,%s%.1f",tmpagl,cpmin,sMinagl,cpmax,sMaxagl);
    }
    else {
        sprintf(aglbuf,"%.1f,%.1f (%d),%.1f (%d)",tmpagl,sMinagl,sMinlin,sMaxagl,sMaxlin);
    }
    
    strcpy(obuf,aglbuf);
    return obuf+strlen(aglbuf);
}

static char *procDTG(char *str, char *obuf) {
    int hr;

    if ((*(str) == CSVFLDSEP) || ((strchr(str, BLANKSP)) == NULL) || (strchr(str, BLANKSP) > strchr(str, CSVFLDSEP))) {
        // Missing or corrupt DTG
        *obuf++ = BLANKSP;
        return obuf;
    }
    
    
    // find the space after the date to get the time part
    str = strchr(str, BLANKSP);
    
    sscanf(str,"%d",&hr);
    hr-=(GMTC-DSTos());
    hr = (hr<0) ? hr+24 : hr;
    sprintf(obuf,"%02d",hr);
    obuf+=2;
    str = strchr(str,':');
    strncpy(obuf,str,6);
    return obuf+6;
}



int isrowSHOW(RMAPINFO minfo, char *ibuf) {
    static int sMsec     = -2;
    static int sMsecdiff = -1;
    
    ROWMAP r = minfo.r;

    // ensure the first two rows are printed (lables and initial '0')
    if (sMsec < 0) {
        sMsec++;
        return 1;
    }
    
    if (sMsecdiff < 0) {
        sMsecdiff = r.map[0].msecdiff;
    }
    
    int cmsec = (int)strtol(fldloc(ibuf,OMMSEC),NULL,10);
    
    if ((cmsec - sMsec) >= sMsecdiff) {
        sMsec = cmsec;
        return 1;
    }
    
    // Not time to output line. But if there's a message and messages are on, output line.
    {
        char *fld = fldloc(ibuf, (OMMSG + r.map[0].coladj));
        
        if (strlen(fld) > 1) {
            return 1;
        }
    }
    
    return 0;
}


static char procVTOGGLE(char *ibuf, KEEPCOL col) {
    static char sVidstat = '\0';
    char curv = *(fldloc(ibuf,col.col));
    
    if (sVidstat == '\0') {
        sVidstat = curv;
    }
    else {
        if (sVidstat != curv) {
            if (curv == '0') {
                addMString("Video switched off");
            }
            else {
                addMString("Video switched on");
            }
            sVidstat = curv;
        }
    }
    return curv;
}

static char *preAug(COLNAMES names,char *ibuf,char *obuf) {
    char *cp = "";
    int   i;
    
    for (i=0; i<NUMCNAMES; ++i) {
        if (names.cnames[i] == NULL) {
            continue;
        }
        if ((cp=strchr(names.cnames[i],AUGNCHAR)) == NULL) {
            return obuf;
        }
        break;
    }
    
    if (i == NUMCNAMES) {
        return obuf;
    }
    
    int acol;
    
    if (sscanf(cp+1,"%d",&acol)) {
        cp = fldloc(ibuf,acol);
        obuf += cpyfld(obuf,cp,CSVFLDSEP);
        *obuf++ = CSVFLDSEP;
    }
    
    return obuf;
}

static char *postAug(COLNAMES names,char *ibuf,char *obuf) {
    char *cp = "";
    int   i;
    
    for (i=(NUMCNAMES-1); i>=0; --i) {
        if (names.cnames[i] == NULL) {
            continue;
        }
        if ((cp=strchr(names.cnames[i],AUGNCHAR)) == NULL) {
            return obuf;
        }
        break;
    }
    
    if (i < 0) {
        return obuf;
    }
    
    int acol;
    
    if (sscanf(cp+1,"%d",&acol)) {
        cp = fldloc(ibuf,acol);
        *obuf++ = CSVFLDSEP;
        obuf += cpyfld(obuf,cp,CSVFLDSEP);
    }
    
    return obuf;
}
