#include "timer.h"
#include <stdio.h>

void ROMP_show_stats(FILE*);

// Conditionalize a parallel for with
//
typedef enum ROMP_level { 
    ROMP_serial,                // always run this code serially
    ROMP_experimental,          // hasn't even been tested for correctness yet
    ROMP_fast,                  // is known to get differing results parallel and serial
    ROMP_assume_reproducible,   // is suspected of getting same results
    ROMP_shown_reproducible,    // is tested and shown to get same results 
    ROMP__size
    } ROMP_level;
extern ROMP_level romp_level;

#define if_ROMP(LEVEL) if (ROMP_##LEVEL <= romp_level)

// Surround a parallel for
typedef struct ROMP_pf_static_struct { 
    void * volatile ptr; 
    const char*     file; 
    unsigned int    line; 
} ROMP_pf_static_struct;
 
typedef struct ROMP_pf_stack_struct  { 
    struct ROMP_pf_static_struct * staticInfo; 
    NanosecsTimer beginTime;
    int tid;
} ROMP_pf_stack_struct;

#define ROMP_PF_begin \
    { \
    static ROMP_pf_static_struct ROMP_pf_static = { nullptr, __FILE__, __LINE__ }; \
    ROMP_pf_stack_struct  ROMP_pf_stack;  \
    ROMP_pf_begin(&ROMP_pf_static, &ROMP_pf_stack);

#define ROMP_PF_end \
    ROMP_pf_end(&ROMP_pf_stack); \
    }

void ROMP_pf_begin(
    ROMP_pf_static_struct * pf_static,
    ROMP_pf_stack_struct  * pf_stack);

void ROMP_pf_end(
    ROMP_pf_stack_struct  * pf_stack);

// Begin and end of the loop body
// Note: continue does need to be annotated
//       break and return and similar exits are illegal
//
typedef struct ROMP_pflb_stack_struct {
    ROMP_pf_stack_struct * pf_stack;
    NanosecsTimer beginTime;
    int tid;
} ROMP_pflb_stack_struct;

#define ROMP_PFLB_begin \
    ROMP_pflb_stack_struct  ROMP_pflb_stack;  \
    ROMP_pflb_begin(&ROMP_pf_stack, &ROMP_pflb_stack);

#define ROMP_PFLB_end \
    ROMP_pflb_end(&ROMP_pflb_stack);

#define ROMP_PFLB_continue \
    { ROMP_PFLB_end; continue; }

void ROMP_pflb_begin(
    ROMP_pf_stack_struct   * pf_stack,
    ROMP_pflb_stack_struct * pflb_stack);

void ROMP_pflb_end(
    ROMP_pflb_stack_struct  * pflb_stack);


#if 0

// example
//
#include <malloc.h>
int _tmain(int argc, _TCHAR* argv[])
{
    static const int v_size = 1000;
    int* v = (int*)malloc(sizeof(int)*v_size);
    int i;
    double sum = 0;

    omp_set_num_threads(1);
    fprintf(stdout, "#threads:%d\n", omp_get_max_threads());

    ROMP_PF_begin
    #pragma omp parallel for reduction(+:sum)
    for (i = 0; i < v_size; i++) {
    	ROMP_PFLB_begin
    	sum += 1.0 / i;
    	ROMP_PFLB_end;
    }
    ROMP_PF_end

    ROMP_show_stats(stdout);
    return 0;
}

#endif
