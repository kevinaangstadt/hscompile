#ifndef HS_PCRE_MNRL_INT_H_
#define HS_PCRE_MNRL_INT_H_

#include "ue2common.h"
#include "hs.h"
#include <mnrl.hpp>

#ifdef __cplusplus

namespace ue2 {
    hs_error_t
    hs_pcre_mnrl_multi_int(const char *const *expressions, MNRL::MNRLNetwork &mnrl, const unsigned *flags,
                            const unsigned *ids, unsigned elements,
                            hs_compile_error_t **comp_error, const Grey &g);
        
}
#endif
#endif