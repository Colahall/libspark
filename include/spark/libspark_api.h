/*
 * Copyright (c) 2025 Colahall, LLC <about@colahall.io>.
 *
 * This File is part of libspark (see https://colahall.io/libspark).
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * “Software”), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#ifndef LIBSPARK_API_H_
#define LIBSPARK_API_H_

#include <spark/version.h>

/* Cross-platform export macro */
#ifndef LIBSPARK_API
# if defined(_WIN32) || defined(__CYGWIN__)
#   ifdef LIBSPARK_BUILD
#     define LIBSPARK_API __declspec(dllexport)
#   else
#     define LIBSPARK_API __declspec(dllimport)
#   endif /* LIBSPARK_BUILD */
# else
#   define LIBSPARK_API __attribute__((visibility("default")))
# endif /* defined(_WIN32) || defined(__CYGWIN__) */
#endif /* LIBSPARK_API */


#ifdef __cplusplus
extern "C" {
#endif

LIBSPARK_API const char *spark_version(void);

#ifdef __cplusplus
}
#endif


#endif /* LIBSPARK_API_H_ */
