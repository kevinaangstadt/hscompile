#ifndef HS_COMPILE_MNRL_H_
#define HS_COMPILE_MNRL_H_

#include "hs_compile.h"
#include "ht.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * The basic MNRL compiler.
 *
 * This is the function call with which an expression is compiled into a
 * Hyperscan database which can be passed to the runtime functions (such as
 * @ref hs_scan(), @ref hs_open_stream(), etc.)
 *
 * @param expression
 *      The NULL-terminated expression to parse. Note that this string must
 *      represent ONLY the pattern to be matched, with no delimiters or flags;
 *      any global flags should be specified with the @a flags argument. For
 *      example, the expression `/abc?def/i` should be compiled by providing
 *      `abc?def` as the @a expression, and @ref HS_FLAG_CASELESS as the @a
 *      flags.
 *
 * @param flags
 *      Flags which modify the behaviour of the expression. Multiple flags may
 *      be used by ORing them together. Valid values are:
 *       - HS_FLAG_CASELESS - Matching will be performed case-insensitively.
 *       - HS_FLAG_DOTALL - Matching a `.` will not exclude newlines.
 *       - HS_FLAG_MULTILINE - `^` and `$` anchors match any newlines in data.
 *       - HS_FLAG_SINGLEMATCH - Only one match will be generated for the
 *                               expression per stream.
 *       - HS_FLAG_ALLOWEMPTY - Allow expressions which can match against an
 *                              empty string, such as `.*`.
 *       - HS_FLAG_UTF8 - Treat this pattern as a sequence of UTF-8 characters.
 *       - HS_FLAG_UCP - Use Unicode properties for character classes.
 *       - HS_FLAG_PREFILTER - Compile pattern in prefiltering mode.
 *       - HS_FLAG_SOM_LEFTMOST - Report the leftmost start of match offset
 *                                when a match is found.
 *
 * @param mode
 *      Compiler mode flags that affect the database as a whole. One of @ref
 *      HS_MODE_STREAM or @ref HS_MODE_BLOCK or @ref HS_MODE_VECTORED must be
 *      supplied, to select between the generation of a streaming, block or
 *      vectored database. In addition, other flags (beginning with HS_MODE_)
 *      may be supplied to enable specific features. See @ref HS_MODE_FLAG for
 *      more details.
 *
 * @param platform
 *      If not NULL, the platform structure is used to determine the target
 *      platform for the database. If NULL, a database suitable for running
 *      on the current host platform is produced.
 *
 * @param db
 *      On success, a pointer to the generated database will be returned in
 *      this parameter, or NULL on failure. The caller is responsible for
 *      deallocating the buffer using the @ref hs_free_database() function.
 *
 * @param error
 *      If the compile fails, a pointer to a @ref hs_compile_error_t will be
 *      returned, providing details of the error condition. The caller is
 *      responsible for deallocating the buffer using the @ref
 *      hs_free_compile_error() function.
 *
 * @return
 *      @ref HS_SUCCESS is returned on successful compilation; @ref
 *      HS_COMPILER_ERROR on failure, with details provided in the error
 *
 */

hs_error_t hs_compile_mnrl(const char *graphFN,
		hs_database_t **db, hs_compile_error_t **error,
        r_map **report_map);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HS_COMPILE_MNRL_H_ */
