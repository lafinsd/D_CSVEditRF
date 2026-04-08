//
//  common.c
//  D_CSVEdit
//
//  Created by Larry on 4/5/26.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/xattr.h>

#include "drone.h"
#include "remap.h"
#include "common.h"

extern MSG_STRNGS sMsg_strngs;
extern int gShowLine;

char *fmtflt(FMTD *fmtinfo, char *str) {
    static char sFmtbuf[30];
    char *cp1  = strchr(str,DECPNT);
    char *cp2  = strchr(str,CSVFLDSEP);
    
    memset(sFmtbuf,0,sizeof(sFmtbuf));
    
    if ((cp1 == NULL) || (cp1 > cp2)) {
        // A float field without a decimal point
        
        /* The (cp1 > cp2) test won't work right if the item is in the last column
         * because the search for a comma will return NULL. Luckily the last
         * column is a general text message field that will never be proocessed
         * as containing a floating point value so the search for a comma will never
         * fail. 'cp1' *could* be NULL if the current float field has no '.' and
         * there are no more '.' characters in subsequent colomns.
         */
        int i, len = (int)(cp2-str+1);
        strncpy(sFmtbuf,str,len);
        
        sFmtbuf[len-1] = DECPNT;
        for (i=0; i<fmtinfo->rod; ++i) {
            sFmtbuf[len+i] = '0';
        }
        sFmtbuf[len+i] = CSVFLDSEP;

    }
    else {
        if (fmtinfo->rod == 0) {
            // there's a decimal point but we don't want to print the numerals to the right
            strncpy(sFmtbuf,str,(cp1-str));
            sFmtbuf[cp1-str] = CSVFLDSEP;
        }
        else {

            // Handle left of decimal.
            int  i=0, cnt = (int)(cp1 - str)+1;
            
            // Insert leading '0' if field begins with a decimal point
            if (cnt == 1) {
                i = 1;
                sFmtbuf[0] = '0';
            }
            
            strncpy(&sFmtbuf[i],str,cnt);
            str+=(cnt+i);
            
            // take care of right of decimal. if there are fewer numerals
            // than the field width fill with 0's.
            char *dp = sFmtbuf + strlen(sFmtbuf);

            if ((cnt=(int)(cp2-cp1-1)) >= fmtinfo->rod) {
                strncpy(dp,str,fmtinfo->rod);
            }
            else {
                strncpy(dp,str,cnt);
                
                for (i=0; i<fmtinfo->rod-cnt; ++i) {
                    strcat(sFmtbuf, "0");
                }
            }
            sFmtbuf[strlen(sFmtbuf)] = CSVFLDSEP;
        }
    }

    return sFmtbuf;
}

int procRow(RMAPINFO minfo, char *ibuf, char *obuf, int numlines, int numout) {
    int   i, rmaplen = minfo.c.size;
    char *lobuf = obuf;

    if (isrowSHOW(minfo, ibuf)) {
        KEEPCOL *remap = minfo.c.map;
        
        for (i=0; i<rmaplen; ++i) {
            RTCV val = remap[i].status;
            
            if (val == HIDE) {
                *lobuf++ = HIDECHAR;
            }
            
            lobuf = procAUX(&minfo.c, i, numlines, ibuf, lobuf, obuf, numout);
            if (lobuf == NULL) {
                return -1;
            }
            
            if (i == (rmaplen-1)) {
                if (val == HIDE) {
                    *lobuf++ = HIDECHAR;
                }
                *lobuf++ = '\xd';
                *lobuf++ = '\xa';
            }
            else {
                *lobuf++ = CSVFLDSEP;
                if (val == HIDE) {
                    *lobuf++ = HIDECHAR;
                }
            }
        }
        return 1;
    }
    return 0;
 }
 
 float procHDGDIF(char *ghdg, char *ibuf) {
     float gmbl,cmpss;
     char  *chdg = fldloc(ibuf,OMCHDG);
    
     sscanf(ghdg,"%f", &gmbl);
     sscanf(chdg,"%f", &cmpss);
    
     return gmbl - cmpss;
 }
 
 char *procMDST(char *str, char *obuf, int num) {
     static int sMaxdst  = 0;
     static int sNumline = 2;
    
     int tmpdst;
     float ftmpdst;
     char dstbuf[75];
     char *cp = "";
    
     sscanf(str,"%f", &ftmpdst);
     tmpdst = ftmpdst;
     memset(dstbuf,0,sizeof(dstbuf));
    
     if (tmpdst > sMaxdst) {
         sMaxdst = tmpdst;
         cp       = "new:";
         sNumline = num + 1;
     }

     if (!gShowLine) {
         sprintf(dstbuf,"%d,%s%d",tmpdst,cp,sMaxdst);
     }
     else {
         sprintf(dstbuf,"%d,%d (%d)",tmpdst,sMaxdst,sNumline);
     }
    
     strcpy(obuf,dstbuf);
     return obuf+strlen(dstbuf);
 }
 
 int DSTos(void) {
     static int sDSTos = -1;
    
     if (sDSTos > -1) {
         return sDSTos;
     }
    
     time_t current_time   = time(NULL);
     struct tm *local_time = localtime(&current_time);
    
     sDSTos = 0;
     if (local_time->tm_isdst > 0) {
         sDSTos = 1;
     }
     return sDSTos;
 }
 
 TMFMT manageTMFMT(ACT req, TMFMT val) {
      static TMFMT sTmfmt = FMTUNDEF;
     
      switch (req) {
          case SET:
              sTmfmt = val;
              break;
             
          case GET:
              break;
      }
     
      return sTmfmt;
  }
  
static STATFS manageSTATFS(ACT req, STATFS val) {
  static STATFS sFstat = {NOSTATE};
 
   switch (req) {
       case SET:
           if (strcmp(val.str,sFstat.str)) {
               strcpy(sFstat.str,val.str);
               sFstat.state = DIFFERENT;
               val.state    = DIFFERENT;
           }
           else {
               val.state    = SAME;
               sFstat.state = SAME;
           }
         
           if (val.state != SAME) {
               addMString(val.str);
           }
           break;
         
       case GET:
           val = sFstat;
           break;
   }
 
   return val;
}
   
void addMString(char *str) {
   int cnt = sMsg_strngs.count;
       
    if (!cnt) {
        sMsg_strngs.strings = malloc(sizeof(char *));
    }
    else {
        sMsg_strngs.strings = realloc(sMsg_strngs.strings,(cnt+1)*sizeof(char *));
    }
    sMsg_strngs.strings[cnt] = calloc(strlen(str)+1,1);
    strcpy(sMsg_strngs.strings[cnt], str);
    sMsg_strngs.count++;
}
   
char *chkXlate(AUXB which, char *fld) {
   static XLTAB xlstrings[] = {
       {"Detour", "P_GPS(CA),"},
       {"Motors_Started", "Motors_On,"},
       {"Starting Motors", "Motors_On,"},
   };
   static int sztab = sizeof(xlstrings)/sizeof(xlstrings[0]);

   if (which == FSXLATE) {
       int i;
    
       for (i=0; i<sztab; ++i) {
           long len1 = (long)strlen(xlstrings[i].fstr);
           long len2 = strchr(fld,',') - fld;
        
           if ((len1 == len2) && (strncmp(fld,xlstrings[i].fstr,len1) == 0)){
               fld = xlstrings[i].tstr;
               break;
           }
       }
   }

   {
       int len = (int)(strchr(fld,',') - fld);
       STATFS s;
    
       strncpy(s.str,fld,len);
       s.str[len] = '\0';
    
       manageSTATFS(SET, s);
   }

   return fld;
}

void init(char *prog) {
   char    buf[20];
   ssize_t res;

   res = getxattr(prog, "user.showline", buf, sizeof(buf), 0, 0);

   if ((res == 1) && (*buf == 'y')) {
       gShowLine = 1;
   }

   return;
}
   
