/*
 * Copyright (c) 1993-1994 by Xerox Corporation.  All rights reserved.
 *
 * THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED
 * OR IMPLIED.  ANY USE IS AT YOUR OWN RISK.
 *
 * Permission is hereby granted to use or copy this program
 * for any purpose,  provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 */

/*
 * Cords are immutable character strings.  A number of operations
 * on long cords are much more efficient than their strings.h counterpart.
 * In particular, concatenation takes constant time independent of the length
 * of the arguments.  (Cords are represented as trees, with internal
 * nodes representing concatenation and leaves consisting of either C
 * strings or a functional description of the string.)
 *
 * The following are reasonable applications of cords.  They would perform
 * unacceptably if C strings were used:
 * - A compiler that produces assembly language output by repeatedly
 *   concatenating instructions onto a cord representing the output file.
 * - A text editor that converts the input file to a cord, and then
 *   performs editing operations by producing a new cord representing
 *   the file after each character change (and keeping the old ones in an
 *   edit history)
 *
 * For optimal performance, cords should be built by
 * concatenating short sections.
 * This interface is designed for maximum compatibility with C strings.
 * ASCII NUL characters may be embedded in cords using CORD_from_fn.
 * This is handled correctly, but CORD_to_char_star will produce a string
 * with embedded NULs when given such a cord.
 *
 * This interface is fairly big, largely for performance reasons.
 * The most basic constants and functions:
 *
 * CORD - the type of a cord;
 * CORD_EMPTY - empty cord;
 * CORD_len(cord) - length of a cord;
 * CORD_cat(cord1,cord2) - concatenation of two cords;
 * CORD_substr(cord, start, len) - substring (or subcord);
 * CORD_pos i;  CORD_FOR(i, cord) {  ... CORD_pos_fetch(i) ... } -
 *    examine each character in a cord.  CORD_pos_fetch(i) is the char.
 * CORD_fetch(int i) - Retrieve i'th character (slowly).
 * CORD_cmp(cord1, cord2) - compare two cords.
 * CORD_from_file(FILE * f) - turn a read-only file into a cord.
 * CORD_to_char_star(cord) - convert to C string.
 *   (Non-NULL C constant strings are cords.)
 * CORD_printf (etc.) - cord version of printf. Use %r for cords.
 */
#ifndef CORD_H
#define CORD_H

#include <stddef.h>
#include <stdio.h>

#ifdef GC_DLL
  /* Same as for GC_API in gc_config_macros.h.  */
# ifdef CORD_BUILD
#   if defined(__MINGW32__) || defined(__CEGCC__)
#     define CORD_API __declspec(dllexport)
#   elif defined(_MSC_VER) || defined(__DMC__) || defined(__BORLANDC__) \
         || defined(__CYGWIN__) || defined(__WATCOMC__)
#     define CORD_API extern __declspec(dllexport)
#   elif defined(__GNUC__) && !defined(GC_NO_VISIBILITY) \
         && (__GNUC__ >= 4 || defined(GC_VISIBILITY_HIDDEN_SET))
    /* Only matters if used in conjunction with -fvisibility=hidden option. */
#     define CORD_API extern __attribute__((__visibility__("default")))
#   endif
# else
#   if defined(__MINGW32__) || defined(__CEGCC__) || defined(_MSC_VER) \
       || defined(__DMC__) || defined(__BORLANDC__) || defined(__CYGWIN__)
#     define CORD_API __declspec(dllimport)
#   elif defined(__WATCOMC__)
#     define CORD_API extern __declspec(dllimport)
#   endif
# endif /* !CORD_BUILD */
#endif /* GC_DLL */

#ifndef CORD_API
# define CORD_API extern
#endif

#if defined(BDW_GC)
    #include "gc.h"

    typedef const char* CORD;
    typedef const char* CORD_IN;

    typedef const char* PCHAR;
    typedef const char* PCHAR_IN;

    #define CORD_EMPTY 0
    #define CORD_IS_EMPTY(x) (x == 0)
#elif defined(SHARED_PTR)
    #include <memory>

    typedef std::shared_ptr<const char> CORD;
    typedef const std::shared_ptr<const char>& CORD_IN;

    typedef std::shared_ptr<const char> PCHAR;
    typedef const std::shared_ptr<const char>& PCHAR_IN;

    #define CORD_EMPTY nullptr
    #define CORD_IS_EMPTY(x) (x == nullptr)
#elif defined(PRECISE_GC_SERIAL)
    #include "liballocgc/liballocgc.hpp"

    typedef allocgc::serial::gc_ptr<const char[]> CORD;
    typedef const allocgc::serial::gc_ptr<const char[]>& CORD_IN;

    typedef allocgc::serial::gc_ptr<const char[]> PCHAR;
    typedef const allocgc::serial::gc_ptr<const char[]>& PCHAR_IN;

    #define CORD_EMPTY nullptr
    #define CORD_IS_EMPTY(x) (x == nullptr)
#elif defined(PRECISE_GC_CMS)
    #include "liballocgc/liballocgc.hpp"

    typedef allocgc::cms::gc_ptr<const char[]> CORD;
    typedef const allocgc::cms::gc_ptr<const char[]>& CORD_IN;

    typedef allocgc::cms::gc_ptr<const char[]> PCHAR;
    typedef const allocgc::cms::gc_ptr<const char[]>& PCHAR_IN;

    #define CORD_EMPTY nullptr
    #define CORD_IS_EMPTY(x) (x == nullptr)
#endif

/* Is a nonempty cord represented as a C string? */
#define CORD_IS_STRING(s) (*(s) != '\0')

/* Concatenate two cords.  If the arguments are C strings, they may     */
/* not be subsequently altered.                                         */
CORD_API CORD CORD_cat(CORD_IN x, CORD_IN y);

/* Concatenate a cord and a C string with known length.  Except for the */
/* empty string case, this is a special case of CORD_cat.  Since the    */
/* length is known, it can be faster.                                   */
/* The string y is shared with the resulting CORD.  Hence it should     */
/* not be altered by the caller.                                        */
CORD_API CORD CORD_cat_char_star(CORD x, PCHAR y, size_t leny);

/* Compute the length of a cord */
CORD_API size_t CORD_len(CORD_IN x);

/* Cords may be represented by functions defining the ith character */
typedef char (* CORD_fn)(size_t i, void * client_data);

/* Turn a functional description into a cord.   */
//CORD_API CORD CORD_from_fn(CORD_fn fn, void * client_data, size_t len);

/* Return the substring (subcord really) of x with length at most n,    */
/* starting at position i.  (The initial character has position 0.)     */
CORD_API CORD CORD_substr(CORD_IN x, size_t i, size_t n);

/* Return the argument, but rebalanced to allow more efficient          */
/* character retrieval, substring operations, and comparisons.          */
/* This is useful only for cords that were built using repeated         */
/* concatenation.  Guarantees log time access to the result, unless     */
/* x was obtained through a large number of repeated substring ops      */
/* or the embedded functional descriptions take longer to evaluate.     */
/* May reallocate significant parts of the cord.  The argument is not   */
/* modified; only the result is balanced.                               */
CORD_API CORD CORD_balance(CORD_IN x);

/* The following traverse a cord by applying a function to each         */
/* character.  This is occasionally appropriate, especially where       */
/* speed is crucial.  But, since C doesn't have nested functions,       */
/* clients of this sort of traversal are clumsy to write.  Consider     */
/* the functions that operate on cord positions instead.                */

/* Function to iteratively apply to individual characters in cord.      */
typedef int (* CORD_iter_fn)(char c, void * client_data);

/* Function to apply to substrings of a cord.  Each substring is a      */
/* a C character string, not a general cord.                            */
typedef int (* CORD_batched_iter_fn)(const char * s, void * client_data);

#define CORD_NO_FN ((CORD_batched_iter_fn)0)

/* Apply f1 to each character in the cord, in ascending order,          */
/* starting at position i. If                                           */
/* f2 is not CORD_NO_FN, then multiple calls to f1 may be replaced by   */
/* a single call to f2.  The parameter f2 is provided only to allow     */
/* some optimization by the client.  This terminates when the right     */
/* end of this string is reached, or when f1 or f2 return != 0.  In the */
/* latter case CORD_iter returns != 0.  Otherwise it returns 0.         */
/* The specified value of i must be < CORD_len(x).                      */
CORD_API int CORD_iter5(CORD_IN x, size_t i, CORD_iter_fn f1,
                        CORD_batched_iter_fn f2, void * client_data);

/* A simpler version that starts at 0, and without f2:  */
CORD_API int CORD_iter(CORD x, CORD_iter_fn f1, void * client_data);

/* Similar to CORD_iter5, but end-to-beginning. No provisions for       */
/* CORD_batched_iter_fn.                                                */
//CORD_API int CORD_riter4(CORD x, size_t i, CORD_iter_fn f1, void * client_data);

/* A simpler version that starts at the end:    */
//CORD_API int CORD_riter(CORD x, CORD_iter_fn f1, void * client_data);

/* Functions that operate on cord positions.  The easy way to traverse  */
/* cords.  A cord position is logically a pair consisting of a cord     */
/* and an index into that cord.  But it is much faster to retrieve a    */
/* character based on a position than on an index.  Unfortunately,      */
/* positions are big (order of a few 100 bytes), so allocate them with  */
/* caution.                                                             */
/* Things in cord_pos.h should be treated as opaque, except as          */
/* described below.  Also note that                                     */
/* CORD_pos_fetch, CORD_next and CORD_prev have both macro and function */
/* definitions.  The former may evaluate their argument more than once. */
#include "cord_pos.hpp"

/*
        Visible definitions from above:

        typedef <OPAQUE but fairly big> CORD_pos[1];

        * Extract the cord from a position:
        CORD CORD_pos_to_cord(CORD_pos p);

        * Extract the current index from a position:
        size_t CORD_pos_to_index(CORD_pos p);

        * Fetch the character located at the given position:
        char CORD_pos_fetch(CORD_pos p);

        * Initialize the position to refer to the given cord and index.
        * Note that this is the most expensive function on positions:
        void CORD_set_pos(CORD_pos p, CORD x, size_t i);

        * Advance the position to the next character.
        * P must be initialized and valid.
        * Invalidates p if past end:
        void CORD_next(CORD_pos p);

        * Move the position to the preceding character.
        * P must be initialized and valid.
        * Invalidates p if past beginning:
        void CORD_prev(CORD_pos p);

        * Is the position valid, i.e. inside the cord?
        int CORD_pos_valid(CORD_pos p);
*/
#define CORD_FOR(pos, cord) \
    for (CORD_set_pos(pos, cord, 0); CORD_pos_valid(pos); CORD_next(pos))


/* An out of memory handler to call.  May be supplied by client.        */
/* Must not return.                                                     */
extern void (* CORD_oom_fn)(void);

/* Dump the representation of x to stdout in an implementation defined  */
/* manner.  Intended for debugging only.                                */
CORD_API void CORD_dump(CORD x);

/* The following could easily be implemented by the client.  They are   */
/* provided in cordxtra.c for convenience.                              */

/* Concatenate a character to the end of a cord.        */
//CORD_API CORD CORD_cat_char(CORD x, char c);

/* Concatenate n cords. */
//CORD_API CORD CORD_catn(int n, /* CORD */ ...);

/* Return the character in CORD_substr(x, i, 1)         */
//CORD_API char CORD_fetch(CORD x, size_t i);

/* Return < 0, 0, or > 0, depending on whether x < y, x = y, x > y      */
//CORD_API int CORD_cmp(CORD x, CORD y);

/* A generalization that takes both starting positions for the          */
/* comparison, and a limit on the number of characters to be compared.  */
//CORD_API int CORD_ncmp(CORD x, size_t x_start, CORD y, size_t y_start,
//                       size_t len);

/* Find the first occurrence of s in x at position start or later.      */
/* Return the position of the first character of s in x, or             */
/* CORD_NOT_FOUND if there is none.                                     */
//CORD_API size_t CORD_str(CORD x, size_t start, CORD s);

/* Return a cord consisting of i copies of (possibly NUL) c.  Dangerous */
/* in conjunction with CORD_to_char_star.                               */
/* The resulting representation takes constant space, independent of i. */
//CORD_API CORD CORD_chars(char c, size_t i);
//#define CORD_nul(i) CORD_chars('\0', (i))

/* Turn a cord into a C string. The result shares no structure with     */
/* x, and is thus modifiable.                                           */
CORD_API PCHAR CORD_to_char_star(CORD_IN x);

/* Turn a C string into a CORD.  The C string is copied, and so may     */
/* subsequently be modified.                                            */
CORD_API CORD CORD_from_char_star(const char *s);

/* Identical to the above, but the result may share structure with      */
/* the argument and is thus not modifiable.                             */
CORD_API const char * CORD_to_const_char_star(CORD x);

/* "Not found" result for the following two functions.                  */
//#define CORD_NOT_FOUND ((size_t)(-1))

/* A vague analog of strchr.  Returns the position (an integer, not     */
/* a pointer) of the first occurrence of (char) c inside x at position  */
/* i or later. The value i must be < CORD_len(x).                       */
//CORD_API size_t CORD_chr(CORD x, size_t i, int c);

/* A vague analog of strrchr.  Returns index of the last occurrence     */
/* of (char) c inside x at position i or earlier. The value i           */
/* must be < CORD_len(x).                                               */
//CORD_API size_t CORD_rchr(CORD x, size_t i, int c);

#endif /* CORD_H */
