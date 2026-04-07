//
//  gopt.h
//  mergedat
//
//  Created by Larry on 8/4/25.
//

#ifndef gopt_h
#define gopt_h

#define OPT_STRING "o:w:"

typedef struct {
    CMD_LINE  cl;
    char     *adcsv;
    char     *phcsv;
    char     *ofile;
    int       relpaths;
    char     *mission;
} GOPT;

int procopt(GOPT *);

#endif
