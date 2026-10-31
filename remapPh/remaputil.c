//
//  remaputil.c
//  remapPh
//
//  Created by Larry on 7/17/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drone.h"
#include "remap.h"
#include "common.h"

#define ISVIDEO "Video"
#define ISPHOTO "Photo"
#define VPTHERE "True"

static char  *procDTG (char *, char *, char *);
static char  *procSAVEMS(char *);
static char  *numnames(COLNAMES, char *);

static int gCurMsec  = 0;

int gShowLine = 0;

MSG_STRNGS sMsg_strngs;

// Process column heading(s). This is where extra columns are added
// if names are supplied. The target column is specified by the
// 'cnidx' element which may itself use the default name.
// This function guarantees the returned string is populated.
static char *numnames(COLNAMES nm, char *fld) {
    static char sTbuf[50];
    int count = 0;
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
            strcat(sTbuf,nm.cnames[i]);
            if (--count) {
                strcat(sTbuf,",");
            }
        }
        else {
            // We're using the default column name in the constructed string.
            // Copy it into the string.
            if (i == nm.cnidx) {
                int k = (int)strlen(sTbuf);
                while ((*fld != CSVFLDSEP) && ((*fld != NEWLINE) && (*fld != CR))) {
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
            char *cp = numnames(col.cnamelist, fld);
            int   len;
            
            if ((len=(int)strlen(cp))) {
                strcpy(obuf, cp);
                return obuf + len;
            }
        }
    }
    
    switch (col.aux.which) {
        case HDGDIF:
            {
                char lfmt[10];
                strcpy(lfmt,"%.3f");
                char *cp = strchr(lfmt,'.') + 1;
                
                float diff = procHDGDIF(fld,ibuf);
                
                fld = fmtflt(&col.aux.helper.fmt,fld);
                *cp = '0' + col.aux.helper.fmt.rod;
                while (*fld != CSVFLDSEP) {
                    *obuf++ = *fld++;
                }
                *obuf++ = ',';
                {
                    char tbuf[20];
                    
                    sprintf(tbuf,lfmt,diff);
                    strcat(obuf,tbuf);
                    obuf += strlen(tbuf);
                }
            }
            
            break;
            
        case WARN:
            {
                char *buf = fldloc(ibuf,col.col);
                int   len;
                
                if ((len=(int)strlen(buf)) > 2) {
                    char *tbuf = calloc(len,1);
                    
                    strncpy(tbuf,buf,(len-2));
                    addMString(tbuf);
                    free(tbuf);
                }
            }
            break;
            
        case PHOTO:
            {
                char *buf = fldloc(ibuf,col.col);
                
                if (!strncmp(buf,VPTHERE,strlen(VPTHERE))) {
                    strcpy(obuf,ISPHOTO);
                    obuf+=strlen(ISPHOTO);
                }
            }
            break;
            
        case VIDEO:
            {
                char *buf = fldloc(ibuf,col.col);
                
                if (!strncmp(buf,VPTHERE,strlen(VPTHERE))) {
                    strcpy(obuf,ISVIDEO);
                    obuf+=strlen(ISVIDEO);
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
                        while ((*fld != CSVFLDSEP) && ((*fld != NEWLINE) && (*fld != CR))) {
                            *obuf++ = *fld++;
                        }
                        *obuf++ = '"';
                    }
                    sMsg_strngs.count = 0;
                }
                else {
                    *obuf++ = '"';
                    while ((*fld != CSVFLDSEP) && ((*fld != NEWLINE) && (*fld != CR))) {
                        *obuf++ = *fld++;
                    }
                    *obuf++ = '"';
                }
            }
            
            break;
            
        case MAXDST:
            obuf = procMDST(fld,obuf, numout);
            break;
            
        case FLOATx:
            // Collapse the 3 axes speed components into the single speed column
            fld = fmtflt(&col.aux.helper.fmt,fld);
            while (*fld != CSVFLDSEP) {
                *obuf++ = *fld++;
            }
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
            while (*fld != CSVFLDSEP) {
                *obuf++ = *fld++;
            }
            break;
            
        case SAVEMS:
            // procSAVEMS() might return a pointer to a static character array it owns
            // so be careful about what is done with the return value.
            {
                char *cp = procSAVEMS(fld);
                long len = strlen(cp);
                strcpy(obuf,cp);
                obuf+= len;
            }
            break;
                
        case FSXLATE:
        case FSSAVE:
            fld = chkXlate(col.aux.which, fld);
        // NO BREAK
        case AUXB_NONE:
            while ((*fld != CSVFLDSEP) && ((*fld != NEWLINE) && (*fld != CR))) {
                *obuf++ = *fld++;
            }
            break;
            
        case TIME:
            if (numline > 1) {
                obuf = procDTG(ibuf, fld, obuf);
            }
            if (*(obuf-1) == BLANKSP) {
                printf ("Missing or corrupt DTG line %d\n", numline);
            }
            break;
    }
    return obuf;
}

static char *procSAVEMS(char *str)  {
    static char rfmt[50];
    int mins,secs,tenths;
    
    {
        float fmsec;
        
        sscanf(str,"%f.1", &fmsec);
        gCurMsec = fmsec*1000;
    }
    
    if (manageTMFMT(GET, FMTDUMMY) == REFMT) {
        mins   = gCurMsec/60000;
        secs   = gCurMsec/1000 - mins*60;
        tenths = (gCurMsec - mins*60000 - secs*1000)/100;
        sprintf(rfmt,"%2d:%02d.%d",mins,secs,tenths);
    }
    else {
        sprintf(rfmt,"%d",gCurMsec);
    }
    
    return rfmt;
}

static char *procDTG(char *date, char *str, char *obuf) {
    int hr,oset;
    
    // find the space after the date to get the AM/PM part
    {
        char *ampm = strchr(str, BLANKSP) + 1;
        
        oset = !strncmp(ampm,"AM",2) ? 0 : 12;
    }
    
    sscanf(str,"%d",&hr);
    hr-=((GMTC-DSTos(date)) + oset);
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
    int cmsec;
    
    ROWMAP r = minfo.r;

    // ensure the first two rows are printed (lables and initial '0')
    if (sMsec < 0) {
        sMsec++;
        return 1;
    }
    
    if (sMsecdiff < 0) {
        sMsecdiff = r.map[0].msecdiff;
    }
    
    {
        float fmsec;
        
        sscanf(fldloc(ibuf,OMMSEC),"%f.1", &fmsec);
        cmsec = fmsec*1000;
    }
    
    if ((cmsec - sMsec) >= sMsecdiff) {
        sMsec = cmsec;
        return 1;
    }
    
    // Not time to output line. But if there's a message or warning then process the line.
    {
        char *fldm = fldloc(ibuf,OMMSG);
        char *fldw = fldloc(ibuf,OMWRN);
        
        // Either messages column or warnings column populated?
        if ((*fldm != CSVFLDSEP) || (strlen(fldw) > 2)) {
            return 1;
        }
    }
    
    return 0;
}

