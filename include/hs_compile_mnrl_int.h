#ifndef HS_COMPILE_MNRL_INT_H_
#define HS_COMPILE_MNRL_INT_H_

#include "ue2common.h"
#include "hs.h"

#ifdef __cplusplus
namespace ue2 {
hs_error_t hs_compile_mnrl_int(const char * graphFN,
		hs_database_t **db,
		hs_compile_error_t **comp_error, const Grey &g);

}
#endif

#endif
