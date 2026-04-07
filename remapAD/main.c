//
//  main.c
//  rowmapAD
//
//  Created by Larry on 2/14/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>    // basename ()
#include <sys/xattr.h> // for get/setxattr()

#include "drone.h"
#include "cl_log2.0.h"
#include "remap.h"
#include "gopt.h"
#include "updateTable.h"
#include "common.h"

#define VERSION "4.3"

// Field numbers ('col' member in the struct) are 0-based wrt input.
KEEPCOL sColmap[] = {
    /*  0 */    {SHOW,0,{0,"flt time"},{SAVEMS}},
    /*  1 */    {SHOW,1,{0,"tod"},{TIME}},
    /*  2 */    {SHOW,50,{0},{AUXB_NONE}},
    /*  3 */    {SHOW,2,{0},{FLOAT,6}},
    /*  4 */    {SHOW,3,{0},{FLOAT,6}},
    /*  5 */    {SHOW,22,{0,"compass hdg"},{FLOAT,0}},
    /*  6 */    {SHOW,35,{0,"gimbal hdg","ghdg-chdg","trvl hdg/51"},{HDGDIF,0}},
    /*  7 */    {SHOW,19,{0,"Xmph"},{FLOAT,2}},
    /*  8 */    {SHOW,20,{0,"Ymph"},{FLOAT,2}},
    /*  9 */    {SHOW,21,{0,"Zmph"},{FLOAT,2}},
    /* 10 */    {SHOW,9,{0,"speed"},{FLOAT,1}},
    /* 11 */    {SHOW,7,{0,"above_MSL"},{FLOAT,2}},
    /* 12 */    {SHOW,6,{0,"locMSL"},{FLOAT,2}},
    /* 13 */    {SHOW,4,{0,"AGL","AGL min","AGL max"},{MINMAXAGL}},
    /* 14 */    {SHOW,5,{0,"locAGL"},{FLOAT,2}},
    /* 15 */    {SHOW,10,{0,"dst","max dist","total dst/52"},{MAXDST}},
    /* 16 */    {SHOW,23,{0,"pitch"},{AUXB_NONE}},
    /* 17 */    {SHOW,24,{0,"roll"},{AUXB_NONE}},
    /* 18 */    {SHOW,12,{0,"sats"},{AUXB_NONE}},
    /* 19 */    {SHOW,26,{0,"isVideo"},{VIDEO}},
    /* 20 */    {SHOW,36,{0,"gmbl pitch"},{FLOAT,0}},
    /* 21 */    {SHOW,38,{0,"bat %"},{AUXB_NONE}},
    /* 22 */    {SHOW,51,{0},{MSG}}
    };

KEEPROW rowmap[] = {0};

static uint8_t hidemap[] = {OMXSP,OMYSP,OMZSP};

static int  o2lmap(COLMAP *, int);
static void stripit(char *);
static int  sanityCheck(char *);
static void killaugcols(RMAPINFO *);
static int  augcolsvalid(RMAPINFO *);
static int  getofname(GOPT *p,char **name);
static void csvsepsub(char *);
static void csvrepsub(char *);

int main(int argc, char **argv) {
    GOPT  opts;
    FILE *fin, *fout;
    char ibuf[BUFSIZE], obuf[BUFSIZE];
    int  retval = 0, numlines=0, numout = 0;
    RMAPINFO minfo;
    
    printf("%s %s-Llib:%s-Dlib:%s (%s %s)\n\n", basename(argv[0]), VERSION, getLogLibVer(),getDroneLibVer(), __DATE__, __TIME__);
    
    opts.cl.argc = argc;
    opts.cl.argv = argv;
    
    if (procopt(&opts)) {
        printf("Usage: %s [-o<n>] [-w<%s:<m>>] [-k] [-r] [-R] [-d] [-t] [-h] [-c] [-S[milliseconds]] [-s[milliseconds]]  <file name>\n\n",basename(argv[0]),mkhintstring());
        printf (\
            "-o  output file name 'n'\n"\
            "-w  use file paths relative to the named mission 'm' of specified platform\n"\
            "-k  kill the saving of the CL options as an extended attribute\n"\
            "-r  remove any augmented column specs in default table\n"\
            "-R  suppress reading in the external augmentation table\n"\
            "-d  display (gimbal hdg - compass heading)\n"\
            "-t  translate flycState text\n"\
            "-h  hide XYZ speed axis columns\n"\
            "-c  collapse XYZ speed columns\n"\
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
        char *cp   = strrchr(opts.infile,'.');
        char *tstr = "csv";
        
        if ((cp == NULL) || (strcasecmp(++cp,tstr))) {
            printf("Input file must be a .csv file\n\n");
            return 1;
        }
    }
    
    if ((fin=fopen(opts.infile, "r")) == NULL)  {
        printf("ERROR***  Cannot open input file %s\n", opts.infile);
        return 1;
    }
    
    init(argv[0]);
    
    minfo.c.size   = sizeof(sColmap)/sizeof(sColmap[0]);
    minfo.c.map    = sColmap;
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
    
    minfo.r.map[0].msecdiff = opts.options.msecdiff;
    
    // Create output filename and open the file.
    char *ofname;
    
    getofname(&opts,&ofname);
    if (!opts.options.overwrite && file_exists(ofname)) {
        printf("ERROR***  File %s already exists and overwrite not enabled\n\n", ofname);
        return 1;
    }
    
    if ((fout=fopen(ofname, "w")) == NULL) {
        printf("ERROR***  Cannot open output file \"%s\"\n\n",ofname);
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
        
        {
            int tmp = sColmap[minfo.c.size - 1].col;
            
            minfo.r.map[0].maxcol =
            sColmap[minfo.c.size - 1].col = (countcols(ibuf) - 1);
            
            minfo.r.map[0].coladj = sColmap[minfo.c.size - 1].col - tmp;

            if (!minfo.r.map[0].coladj) {
                // No augmented info in .csv file. Kill any augemnted column entries in table.
                printf("INFO*** No augemnted columns in input. Kill any embedded augmented columns.\n\n");
                killaugcols(&minfo);
            }
            else {
                // Augmented columns in the source csv file.
                
                // If kill option specified do it before processing run time table
                if (opts.options.kill) {
                    killaugcols(&minfo);
                }
                
                // Process run time augment columns file unless supressed.
                if (!opts.options.nofile) {
                    if (updateTable(&opts.cl,&minfo)) {
                        return 1;
                    }
                }
                
                // Now make sure augmented table entries (if any) are represented in .csv file.
                if (!augcolsvalid(&minfo)) {
                    return 1;
                }
            }
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
    
    if (usr != NULL) {
        fsetxattr(fileno(fout),"user.cli",usr,strlen(usr),0,0);
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
    char *tstr = "time(millisecond)";
    
    if (strncmp(tstr,str,strlen(tstr))) {
        return 1;
    }
    
    return 0;
}

void killaugcols(RMAPINFO *m) {
    int i,j;
    
    for (i=0; i< m->c.size; ++i) {
        for (j=0;j<NUMCNAMES;++j) {
            char *cp = m->c.map[i].cnamelist.cnames[j];
            if ((cp != NULL) && (strchr(cp,AUGNCHAR) != NULL)) {
                m->c.map[i].cnamelist.cnames[j] = NULL;
            }
        }
    }
    return;
}

static int augcolsvalid(RMAPINFO *minfo) {
    int maxc  = minfo->r.map[0].maxcol;
    int i,j;
    
    for (i=0; i<minfo->c.size; ++i) {
        for (j=0; j<NUMCNAMES; ++j) {
            char *cp,*nptr = minfo->c.map[i].cnamelist.cnames[j];
            
            if (nptr == NULL) continue;
            
            cp = strchr(nptr,AUGNCHAR);
            if (cp != NULL) {
                int acol;
                
                sscanf((cp+1),"%d",&acol);
                
                if (acol >= maxc) {
                    printf("ERROR*** Table row %d specifies an aux column (%d) that does not exist in .csv file (max=%d)\n\n",i,acol,(maxc-1));
                    return 0;
                }
            }
        }
    }
    return 1;
}

static int getofname(GOPT *p,char **name) {
    char *ofname,*odir,*bname,*prefx;
    long  len;
    
    // Build on spec
    if (buildprcspath(p->options.mission, &odir)) {
        printf("ERROR ***  buildprcspath() %s:%d\n\n",__func__,__LINE__);
        return 1;
    }
    
    // The following code is to define 'odir' and 'bname'
    if (p->outfile == NULL) {
        // Use input filename as a base
        bname = basename(p->infile);
        prefx = CMPRSDCSV;
    }
    else {
        // Output file name provided. No imposed prefix in these cases.
        
        prefx = "";
        bname = p->outfile;
        
        if (pathtype(bname) == ABSOLUTE) {
            // An absolute path provided. Use it with no embellishment.
            odir  = "";
        }
        else {
            if (pathtype(bname) == RELATIVE) {
                printf("ERROR ***  Output filename can't be a relative path\n\n");
                return 1;
            }
            
            if (!p->options.relpaths) {
                printf("ERROR***  No Mission specified with base file path\n\n");
                return 1;
            }
        }
    }
    
    len    = strlen(odir) + strlen(prefx) + strlen(bname);
    ofname = calloc(len,1);
    
    sprintf(ofname,"%s%s%s",odir,prefx,bname);
    *name = ofname;
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
