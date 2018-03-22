#ifndef HS_PCRE_MNRL_H_
#define HS_PCRE_MNRL_H_

#include "hs_compile.h"
#include <mnrl.hpp>

#ifdef __cplusplus

// only available for c++
hs_error_t hs_pcre_mnrl_multi(const char * const *expressions,
                                     MNRL::MNRLNetwork &mnrl,
                                     const unsigned *flags, const unsigned *ids,
                                     unsigned elements,
                                     hs_compile_error_t **error);

#endif

#endif