#ifndef __PARSER_H__
#define __PARSER_H__

#include "DArray.h"

#define PARSER_MAX_STRING_LENGTH        100
#define PARSER_EMPTY_DELIMETER          ""
#define PARSER_COMPONENTS_DELIMETER     " " 
#define PARSER_COORDINATES_DELIMETER    "/"

typedef struct 
{
    PDynamicArray v;
    PDynamicArray vt;
    PDynamicArray vn;
    PDynamicArray vp;
    PDynamicArray fv;
    PDynamicArray fvt;
    PDynamicArray fvn;
    PDynamicArray l;
} ObjFile;

#endif