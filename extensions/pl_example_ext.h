/*
   pl_example_ext.h
     - example extension
*/

/*
Index of this file:
// [SECTION] header mess
// [SECTION] apis
// [SECTION] public api
*/

//-----------------------------------------------------------------------------
// [SECTION] header mess
//-----------------------------------------------------------------------------

#ifndef PL_EXAMPLE_EXT_H
#define PL_EXAMPLE_EXT_H

//-----------------------------------------------------------------------------
// [SECTION] apis
//-----------------------------------------------------------------------------

#define plExampleI_version {1, 0, 0}

//-----------------------------------------------------------------------------
// [SECTION] public api
//-----------------------------------------------------------------------------

typedef struct _plExampleI
{
    void (*print_to_console)(const char* text);
} plExampleI;

#endif // PL_EXAMPLE_EXT_H