//
//  gopt.h
//  remapPh
//
//  Created by Larry on 7/17/25.
//
//
//  gopt.c
//  remapPh
//
//  Created by Larry on 7/17/25.
//

#ifndef gopt_h
#define gopt_h

#define OPT_STRING "o:w:kdthcS::s::"

typedef struct {
    int   msecdiff;
    int   xtnd;
    int   relpaths;
    char *mission;
    AUXB  fsxlate;
    AUXB  collapse;
    TMFMT reformat;
    RTCV  tcolSHOW;
    RTCV  scolSHOW;
    RTCV  dcolSHOW;
} CLOPTS;

typedef struct {
    CMD_LINE cl;
    char   *infile;
    char   *outfile;
    CLOPTS  options;
} GOPT;

int procopt(GOPT *);

#endif /* gopt_h */

