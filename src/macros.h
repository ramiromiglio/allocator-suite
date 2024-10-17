#define FORCE_INLINE inline
#define UNLIKELY(...) (__VA_ARGS__)

#ifdef DEBUG_POOL_ALLOCATOR
#define DEBUG_ERR(fmt, ...) fprintf(stderr, fmt "\n", ## __VA_ARGS__)
#define DEBUG_LOG(fmt, ...) fprintf(stdout, fmt "\n", ## __VA_ARGS__)
#else
#define DEBUG_ERR(fmt, ...)
#define DEBUG_LOG(fmt, ...)
#endif

#ifdef UNIT_TEST_ENABLED
#define PRIVATE_BUT_PUBLIC_FOR_TESTING public
#else
#define PRIVATE_BUT_PUBLIC_FOR_TESTING private
#endif

#define ERR_FAIL(fmt, ...) { fprintf(stderr, fmt, ## __VA_ARGS__); assert(0); }
#define ERR_FAIL_IF(cond, fmt, ...) if (cond) { ERR_FAIL(fmt, ## __VA_ARGS__) } else {}
#define ERR_VERIFY(cond) ERR_FAIL_IF(!(cond), "")