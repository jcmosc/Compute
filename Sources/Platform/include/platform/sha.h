#pragma once

#if __APPLE__

#include <CommonCrypto/CommonDigest.h>

typedef CC_SHA1_CTX PLATFORM_SHA1_CTX;

#define PLATFORM_SHA1_Init CC_SHA1_Init
#define PLATFORM_SHA1_Update CC_SHA1_Update
#define PLATFORM_SHA1_Final CC_SHA1_Final
#define PLATFORM_SHA1_DIGEST_LENGTH CC_SHA1_DIGEST_LENGTH

#else

#include <openssl/sha.h>

typedef SHA_CTX PLATFORM_SHA1_CTX;

#define PLATFORM_SHA1_Init SHA1_Init
#define PLATFORM_SHA1_Update SHA1_Update
#define PLATFORM_SHA1_Final SHA1_Final
#define PLATFORM_SHA1_DIGEST_LENGTH SHA_DIGEST_LENGTH

#endif
