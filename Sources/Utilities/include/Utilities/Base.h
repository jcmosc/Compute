#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef __has_feature
#define __has_feature(x) 0
#endif
#ifndef __has_attribute
#define __has_attribute(x) 0
#endif
#ifndef __has_extension
#define __has_extension(x) 0
#endif

#if __has_feature(assume_nonnull)
#define UTIL_ASSUME_NONNULL_BEGIN _Pragma("clang assume_nonnull begin")
#define UTIL_ASSUME_NONNULL_END   _Pragma("clang assume_nonnull end")
#else
#define UTIL_ASSUME_NONNULL_BEGIN
#define UTIL_ASSUME_NONNULL_END
#endif

#if !__has_feature(nullability)
#ifndef _Nullable
#define _Nullable
#endif
#ifndef _Nonnull
#define _Nonnull
#endif
#ifndef _Null_unspecified
#define _Null_unspecified
#endif
#endif
