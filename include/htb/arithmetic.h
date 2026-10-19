#ifndef HTB_ARITHMETIC_H
#define HTB_ARITHMETIC_H

#include <stddef.h>
#include <stdint.h>

#ifndef HTB_TEXT
#define HTB_TEXT 160
#endif

typedef enum {
    HTB_BACKEND_ARB = 0,
    HTB_BACKEND_CALCIUM = 1
} htb_backend_kind;

typedef enum {
    HTB_CMP_LESS = -1,
    HTB_CMP_EQUAL = 0,
    HTB_CMP_GREATER = 1,
    HTB_CMP_UNRESOLVED = 2,
    HTB_CMP_ERROR = 3
} htb_cmp;

typedef struct htb_arith_ctx htb_arith_ctx;

typedef struct {
    htb_arith_ctx *ctx;
    void *value;
} htb_scalar;

typedef struct {
    void (*value_init)(htb_scalar *x);
    void (*value_clear)(htb_scalar *x);
    int (*set)(htb_scalar *z, const htb_scalar *x);
    int (*set_si)(htb_scalar *z, int64_t x);
    int (*set_rational)(htb_scalar *z, const char *text, char *error, size_t error_size);
    int (*set_sin_cos_degrees)(htb_scalar *sine, htb_scalar *cosine, const char *text, char *error, size_t error_size);
    int (*add)(htb_scalar *z, const htb_scalar *x, const htb_scalar *y);
    int (*sub)(htb_scalar *z, const htb_scalar *x, const htb_scalar *y);
    int (*mul)(htb_scalar *z, const htb_scalar *x, const htb_scalar *y);
    int (*sqr)(htb_scalar *z, const htb_scalar *x);
    int (*hull)(htb_scalar *z, const htb_scalar *x, const htb_scalar *y);
    int (*div)(htb_scalar *z, const htb_scalar *x, const htb_scalar *y);
    int (*neg)(htb_scalar *z, const htb_scalar *x);
    int (*sqrt)(htb_scalar *z, const htb_scalar *x);
    htb_cmp (*compare)(const htb_scalar *x, const htb_scalar *y);
    int (*is_finite)(const htb_scalar *x);
    int (*get_display)(const htb_scalar *x, int digits, char *out, size_t out_size);
    int (*get_enclosure)(const htb_scalar *x, int digits, char *out, size_t out_size);
} htb_arith_ops;

struct htb_arith_ctx {
    htb_backend_kind kind;
    long precision;
    long max_precision;
    void *backend;
    const htb_arith_ops *ops;
    uint64_t comparisons;
    uint64_t unresolved_comparisons;
    uint64_t scalar_count;
    uint64_t peak_scalar_count;
};

int htb_arith_ctx_init(htb_arith_ctx *ctx, htb_backend_kind kind, long precision,
                       long max_precision, char *error, size_t error_size);
void htb_arith_ctx_clear(htb_arith_ctx *ctx);
const char *htb_backend_name(htb_backend_kind kind);

void htb_scalar_init(htb_arith_ctx *ctx, htb_scalar *x);
void htb_scalar_clear(htb_scalar *x);
int htb_scalar_set(htb_scalar *z, const htb_scalar *x);
int htb_scalar_set_si(htb_scalar *z, int64_t x);
int htb_scalar_parse_rational(htb_scalar *z, const char *text, char *error, size_t error_size);
int htb_scalar_set_sin_cos_degrees(htb_scalar *sine, htb_scalar *cosine, const char *text, char *error, size_t error_size);
int htb_scalar_add(htb_scalar *z, const htb_scalar *x, const htb_scalar *y);
int htb_scalar_sub(htb_scalar *z, const htb_scalar *x, const htb_scalar *y);
int htb_scalar_mul(htb_scalar *z, const htb_scalar *x, const htb_scalar *y);
int htb_scalar_sqr(htb_scalar *z, const htb_scalar *x);
int htb_scalar_hull(htb_scalar *z, const htb_scalar *x, const htb_scalar *y);
int htb_scalar_div(htb_scalar *z, const htb_scalar *x, const htb_scalar *y);
int htb_scalar_neg(htb_scalar *z, const htb_scalar *x);
int htb_scalar_sqrt(htb_scalar *z, const htb_scalar *x);
htb_cmp htb_scalar_compare(const htb_scalar *x, const htb_scalar *y);
htb_cmp htb_scalar_compare_si(const htb_scalar *x, int64_t y);
int htb_scalar_is_finite(const htb_scalar *x);
int htb_scalar_get_display(const htb_scalar *x, int digits, char *out, size_t out_size);
int htb_scalar_get_enclosure(const htb_scalar *x, int digits, char *out, size_t out_size);
double htb_scalar_get_approx(const htb_scalar *x);

/* Backend constructors and the shared exact decimal/fraction parser. */
int htb_arb_backend_init(htb_arith_ctx *ctx, char *error, size_t error_size);
void htb_arb_backend_clear(htb_arith_ctx *ctx);
int htb_calcium_backend_init(htb_arith_ctx *ctx, char *error, size_t error_size);
void htb_calcium_backend_clear(htb_arith_ctx *ctx);

#endif
