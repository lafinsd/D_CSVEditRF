//
//  remap.h
//  remapPh
//
//  Created by Larry on 7/17/25.
//

#ifndef remap_h
#define remap_h

#define BUFSIZE   5000

// Local array indices
#define LMSECREC   0
#define LFLSTREC   2
#define LSPDREC   15
#define LMSGREC   21
#define LDISDIF    8

// Field positions in original CSV
#define OMXSP    13
#define OMYSP    15
#define OMZSP    17
#define OMMSG   191
#define OMWRN   192
#define OMMSEC    3
#define OMCHDG   22

#define NUMCNAMES 4
#define HIDECHAR  '!'
#define NEWLINE   0xA
#define CR        0xD
#define SUBFSEP   '|'
#define BLANKSP   ' '
#define DECPNT    '.'
#define LMARK     '<'
#define RMARK     '>'
#define LPAREN    '('
#define RPAREN    ')'
#define FLDSEPREP  '\x01'

typedef struct {
    int msecdiff;
} KEEPROW;

typedef enum {
    AUXB_NONE,
    TIME,
    FLOAT,
    FLOATx,
    MAXDST,
    SAVEMS,
    FSXLATE,
    FSSAVE,
    MSG,
    VIDEO,
    PHOTO,
    HDGDIF,
    WARN
} AUXB;

typedef enum {
    FMTDUMMY,
    FMTUNDEF,
    KEEPFMT,
    REFMT
} TMFMT;

typedef enum {
    NOSTATE,
    SAME,
    DIFFERENT
} CHGSTAT;

typedef enum {
    SET,
    GET
} ACT;

typedef struct {
    CHGSTAT state;
    char    str[50];
} STATFS;

typedef struct {
    char *fstr;
    char *tstr;
} XLTAB;

typedef struct {
    int rod;
} FMTD;

typedef union {
    FMTD  fmt;
} AUXH;

typedef struct {
    AUXB which;
    AUXH helper;
} AUXI;

typedef struct {
    int  cnidx;
    char *cnames[NUMCNAMES];
} COLNAMES;

typedef enum {
    SHOW,
    HIDE
} RTCV;

typedef struct {
    uint16_t  count;
    int16_t  *idx;
} REVMAP;

typedef struct {
    RTCV     status;
    int      col;
    COLNAMES cnamelist;
    AUXI     aux;
} KEEPCOL;

typedef struct {
    int      size;
    KEEPCOL *map;
    REVMAP   revmap;
} COLMAP;

typedef struct {
    int      size;
    KEEPROW *map;
} ROWMAP;

typedef struct {
    COLMAP c;
    ROWMAP r;
} RMAPINFO;

int   isrowSHOW(RMAPINFO, char *);
char *procAUX(COLMAP *, int, int, char *, char *, char *, int);

#endif  /* remap.h */
