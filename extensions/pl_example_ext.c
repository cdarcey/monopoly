/*
   pl_example_ext.c
*/

/*
Index of this file:
// [SECTION] includes
// [SECTION] public api implementation
// [SECTION] extension loading
*/

//-----------------------------------------------------------------------------
// [SECTION] includes
//-----------------------------------------------------------------------------

#include <stdio.h>
#include "pl.h"
#include "pl_example_ext.h"

//-----------------------------------------------------------------------------
// [SECTION] public api implementation
//-----------------------------------------------------------------------------

static void
pl__example_print_to_console(const char* pcText)
{
    printf("%s\n", pcText);
}

//-----------------------------------------------------------------------------
// [SECTION] extension loading
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_load_ext(plApiRegistryI* ptApiRegistry, bool bReload)
{
    const plExampleI tApi = {
        .print_to_console = pl__example_print_to_console
    };
    pl_set_api(ptApiRegistry, plExampleI, &tApi);
}

PL_EXPORT void
pl_unload_ext(plApiRegistryI* ptApiRegistry, bool bReload)
{

    if(bReload)
        return;

    const plExampleI* ptApi = pl_get_api_latest(ptApiRegistry, plExampleI);
    ptApiRegistry->remove_api(ptApi);
}
