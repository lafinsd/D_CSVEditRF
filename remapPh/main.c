//
//  main.c
//  remapPh
//
//  Created by Larry on 7/17/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>   // basename ()
#include <ctype.h>    // tolower()
#include <time.h>
#include <sys/xattr.h> // for get/setxattr()

#include "libcommon.h"
#include "drone.h"
#include "remap.h"
#include "gopt.h"
#include "common.h"
#include "libcommon.h"

#define VERSION "1.6"

// Field numbers ('col' member in the struct) are 0-based wrt input.
KEEPCOL colmap[] = {
    {SHOW,3,{0,"flt time"},{SAVEMS}},
    {SHOW,1,{0,"tod"},{TIME}},
    {SHOW,24,{0,"flycState"},{AUXB_NONE}},
    {SHOW,4,{0,"latitude"},{FLOAT,6}},
    {SHOW,5,{0,"longitude"},{FLOAT,6}},
    {SHOW,6,{0,"AGL"},{FLOAT,1}},
    {SHOW,9,{0,"alt"},{FLOAT,1}},
    {SHOW,22,{0,"compass hdg"},{FLOAT,1}},
    {SHOW,59,{0,"gimbal hdg","ghdg-chdg"},{HDGDIF,0}},
    {SHOW,10,{0,"Total travel"},{AUXB_NONE}},
    {SHOW,11,{0,"XY speed"},{FLOAT,1}},
    {SHOW,133,{0,"home dst","max dist"},{MAXDST}},
    {SHOW,13,{0,"Xmph"},{FLOAT,2}},
    {SHOW,15,{0,"Ymph"},{FLOAT,2}},
    {SHOW,17,{0,"Zmph"},{FLOAT,2}},
    {SHOW,11,{0,"speed"},{FLOAT,1}},
    {SHOW,64,{0,"Photo"},{PHOTO}},
    {SHOW,65,{0,"Video"},{VIDEO}},
    {SHOW,56,{0,"gimbal"},{FLOAT,1}},
    {SHOW,86,{0,"bat %"},{AUXB_NONE}},
    {HIDE,192,{0},{WARN}},
    {SHOW,191,{0,"Message"},{MSG}}
    };

KEEPROW rowmap[] = {0};

static uint8_t hidemap[] = {OMXSP,OMYSP,OMZSP};
static int     o2lmap(COLMAP *, int);
static void    stripit(char *);
static int     sanityCheck(char *);
static void    csvsepsub(char *);
static void    csvrepsub(char *);

int main(int argc, char **argv) {
    GOPT  opts;
    FILE *fin, *fout;
    char ibuf[BUFSIZE], obuf[BUFSIZE];
    int  retval = 0, numlines=0, numout = 0;
    RMAPINFO minfo;
    char *pbuf = buildcontext();
    
    init();
    
    printf("%s %s-Dlib:%s-Clib:%s (%s %s Build:%s)\n\n", basename(argv[0]), VERSION, getDroneLibVer(), getCommonLibVer(),__DATE__, __TIME__,pbuf);
    
    opts.cl.argc = argc;
    opts.cl.argv = argv;
    
    if (procopt(&opts)) {
        printf("Usage: %s [-w<%s:<m>>] [-k] [-d] [-t] [-c] [-h] [-o<fn>] [-S[milliseconds]] [-s[milliseconds]]  <file name>\n\n",basename(argv[0]),mkhintstring());
        
        printf (\
            "-w  use file paths relative to the named mission 'm' of specified platform\n"\
            "-k  kill saving command line options as extended attribute\n"\
            "-d  display (gimbal hdg - compass heading)\n"\
            "-t  translate flycState text\n"\
            "-h  hide XYZ speed axis columns\n"\
            "-c  collapse XYZ speed columns\n"\
            "-o  output file name\n"\
            "-S[msec] set and reformat time spacing between rows but keep the elapsed time column.\n"\
            "-s[msec] set msec time spacing between rows and kill the msec elapsed time column \n\n"\
        );
        
        return 1;
    }
    
    char *usr = NULL;
    if (opts.options.xtnd) {
        logCLI(&opts.cl, VERSION,__DATE__,__TIME__,&usr);
    }
    else {
        logCLI(&opts.cl, VERSION,__DATE__,__TIME__,NULL);
    }
    
    // Sanity check: is the selected file a .csv file?
    {
        char *cp = strrchr(opts.infile,'.');
        char *tstr = ".csv";
        //uint16_t i,len = strlen(tstr);
        
        if (cp == NULL) {
            printf("Input file must be a .csv file\n\n");
            return 1;
        }
        
        if (strcasecmp(tstr,cp)) {
            printf("Input file must be a .csv file\n\n");
            return 1;
        }
    }
    
    minfo.c.size   = sizeof(colmap)/sizeof(colmap[0]);
    minfo.c.map    = colmap;
    minfo.r.size   = sizeof(rowmap)/sizeof(rowmap[0]);
    minfo.r.map    = rowmap;

    manageTMFMT(SET, opts.options.reformat);
    
    minfo.c.map[LMSECREC].status    = opts.options.tcolSHOW;
    minfo.c.map[LSPDREC].aux.which  = opts.options.collapse;
    minfo.c.map[LFLSTREC].aux.which = opts.options.fsxlate;
    
    if (opts.options.dcolSHOW == HIDE) {
        minfo.c.map[LDISDIF].cnamelist.cnames[1] = NULL;
        minfo.c.map[LDISDIF].aux.which = AUXB_NONE;
    }
    
    // Trim size if the last 'n' columns are to be hidden.
    {
        uint16_t last = minfo.c.size - 1;
        
        while (minfo.c.map[last].status == HIDE) {
            last--;
            minfo.c.size--;
        }
    }
    
    minfo.r.map[0].msecdiff = opts.options.msecdiff;
    
    if ((fin=fopen(opts.infile, "r")) == NULL)  {
        printf("ERROR***  Cannot open %s\n", opts.infile);
        return 1;
    }
    
    if ((fout=fopen(opts.outfile, "w")) == NULL)  {
        printf("ERROR***  Cannot open %s\n", opts.outfile);
        return 1;
    }
    
    // Initialize the reverse index table.
    {
        uint16_t i;
        
        // Read in the first line so the number of fields can be calculated.
        // The line will then be processed in the main process loop.
        memset(ibuf,0,sizeof(ibuf));
        fgets(ibuf, sizeof(ibuf)-1, fin);
        
        // Sanity check: is the file an Airdata file?
        if (sanityCheck(ibuf)) {
            printf("ERROR***  csv file is not an Airdata file\n\n");
            return 1;
        }
        
        minfo.c.revmap.count = countcols(ibuf);
        minfo.c.revmap.idx   = malloc(minfo.c.revmap.count*sizeof(int16_t));
        for (i=0; i<minfo.c.revmap.count; ++i) {
            minfo.c.revmap.idx[i] = o2lmap(&minfo.c, i);
        }
        printf("%d columns in source\n\n", minfo.c.revmap.count);
        
        // Hide the x, y, and z speed columns if neccesssary
        if (opts.options.scolSHOW == HIDE) {
            uint16_t idx,size = sizeof(hidemap)/sizeof(hidemap[0]);
            
            for (i=0; i<size; ++i) {
                if ((idx=minfo.c.revmap.idx[hidemap[i]]) < 0) {
                    printf("ERROR***  Mapping original column (%d) to local column (%d) failed\n", hidemap[i],idx);
                    return 1;
                }
                minfo.c.map[idx].status = HIDE;
            }
        }
    }
    
    // Main process loop. The first line is already in the input buffer.
    do  {
        int pres;
        
        csvsepsub(ibuf);
        numlines++;
        if (minfo.c.revmap.count != countcols(ibuf)) {
            printf("ERROR***  Number of columns do not match at line %d (ref=%d, cur=%d)\n", numlines,minfo.c.revmap.count,countcols(ibuf));
            retval = 1;
            break;
        }
        
        memset(obuf, 0, sizeof(obuf));
        if ((pres=procRow(minfo, ibuf, obuf, numlines, numout)) > 0) {
            stripit(obuf);
            csvrepsub(obuf);
            fprintf(fout, "%s", obuf);
            numout++;
        }
        else {
            if (pres < 0) {
                printf("ERROR***  Failed to process line %d\nExiting\n\n", numlines);
                retval = 1;
                break;
            }
        }
        memset(ibuf,0,sizeof(ibuf));
    } while (NULL != fgets(ibuf, sizeof(ibuf)-1, fin));
    
    if(usr != NULL) {
        fsetxattr(fileno(fout),XATR_U_CLI,usr,strlen(usr),0,0);
    }
    fclose(fout);
    fclose(fin);

    printf("\n%5d lines processed\n%5d lines out\n\n", numlines, numout);
    
    return retval;
}
static void stripit(char *buf) {
    char *cp = buf;
    
    while (*cp != '\0') {
        while (*cp != HIDECHAR) {
            if (*cp == '\0') {
                *buf = '\0';
                return;
            }
            *buf++ = *cp++;
        }
        cp++;
        while (*cp != HIDECHAR) {
            cp++;
        }
        cp++;
    }
    *buf = '\0';
    return;
}

static int o2lmap(COLMAP *c, int oidx) {
    int i;
    
    for (i=0; i<c->size; ++i) {
        if (oidx == c->map[i].col) {
            return i;
        }
    }

    return -1;
}

static int sanityCheck(char *str) {
    char *tstr = "CUSTOM.date [local]";
    
    if (strncmp(tstr,str,strlen(tstr))) {
        return 1;
    }
    
    return 0;
}

static void csvsepsub(char *line) {
    long cnt=0,len = strlen(line);
    char *cp = line;
    while (cnt<len) {
        if (*cp != '"') {
            cp++;
            cnt++;
            continue;
        }
        cnt++;
        cp++;
        while (*cp != '"' && cnt < len) {
            if (*cp == CSVFLDSEP) {
                *cp = FLDSEPREP;
            }
            cp++;
            cnt++;
        }
        cnt++;
        cp++;
    }
    return;
}

static void csvrepsub(char *line) {
    long cnt=0,len = strlen(line);
    char *cp = line;
    while (cnt<len) {
        if (*cp != '"') {
            cp++;
            cnt++;
            continue;
        }
        cnt++;
        cp++;
        while (*cp != '"' && cnt < len) {
            if (*cp == FLDSEPREP) {
                *cp = CSVFLDSEP;
            }
            cp++;
            cnt++;
        }
        cnt++;
        cp++;
    }
    return;
}
