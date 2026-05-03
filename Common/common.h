//
//  common.h
//  D_CSVEdit
//
//  Created by Larry on 4/5/26.
//

#ifndef common_h
#define common_h

typedef struct {
    int    count;
    char **strings;
} MSG_STRNGS;

void  addMString(char *);
char *procMDST(char *, char *, int);
int   procRow(RMAPINFO, char *, char *, int, int);
char *fmtflt(FMTD *, char *);
char *chkXlate(AUXB, char *);
float procHDGDIF(char *, char *);
TMFMT manageTMFMT(ACT, TMFMT);
void  init(char *);

#endif
