//
//  mergedat.h
//  mergedat
//
//  Created by Larry on 8/4/25.
//

#ifndef mergedat_h
#define mergedat_h

#define BUFSIZE 5000
#define CMDADD  '+'
#define CMDSUB  '>'
#define CMDNIL  ' '

typedef enum {
    ADD,
    SUBSTITUTE,
} STYPE;

typedef struct {
    STYPE type;
    char *header;
} COLINFO;

typedef struct {
    int     adcol;
    int     phcol;
    COLINFO colinfo;
} MAPTBL;

typedef struct {
    int     size;
    MAPTBL *mtbl;
} MAPINFO;

typedef struct {
    FILE *fp;
    char *buf;
} FINFO;

typedef struct {
    FINFO aird;
    FINFO phlp;
    FINFO outf;
} IOINFO;

int mapit(MAPINFO *, IOINFO *, int);

#endif
