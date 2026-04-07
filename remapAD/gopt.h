//
//  gopt.h
//  wigbCLITool
//
//  Created by Larry on 11/19/24.
//

#ifndef gopt_h
#define gopt_h

#define OPT_STRING "o:w:krRdthcS::s::"

typedef struct {
    int   msecdiff;
    int   kill;
    int   xtnd;
    int   relpaths;
    int   nofile;
    int   overwrite;
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
    char    *infile;
    char    *outfile;
    CLOPTS   options;
} GOPT;

int procopt(GOPT *);

#endif /* gopt_h */


