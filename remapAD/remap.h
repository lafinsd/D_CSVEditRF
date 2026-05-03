//
//  remap.h
//  remapAD
//
//  Created by Larry on 2/20/25.
//

#ifndef remap_h
#define remap_h

#define BUFSIZE   1000

// Local array indices
#define LMSECREC   0
#define LFLSTREC   2
#define LSPDREC   10
#define LDISDIF    6

// Field positions in original CSV
#define OMXSP    19
#define OMYSP    20
#define OMZSP    21
#define OMMSG    51
#define OMMSEC    0
#define OMGHDG   35
#define OMCHDG   22

#define NUMCNAMES  4
#define HIDECHAR   '!'
#define EOL        0xA
#define SUBFSEP   '|'
#define BLANKSP    ' '
#define LMARK      '<'
#define RMARK      '>'
#define LPAREN     '('
#define RPAREN     ')'
#define AUGNCHAR   '/'
#define FLDSEPREP  '\x01'

typedef struct {
    int msecdiff;
    int coladj;
    int maxcol;
} KEEPROW;

typedef enum {
    AUXB_NONE,
    TIME,
    FLOAT,
    FLOATx,
    MINMAXAGL,
    MAXDST,
    SAVEMS,
    FSXLATE,
    FSSAVE,
    MSG,
    HDGDIF,
    VIDEO
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
    int   augcol;
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

int    procRow(RMAPINFO, char *, char *, int, int);
int    isrowSHOW(RMAPINFO, char *);
char  *procAUX(COLMAP *, int, int, char *, char *, char *, int);

#endif  /* remap.h */
