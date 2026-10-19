#include "htb/arithmetic.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int htb_arith_ctx_init(htb_arith_ctx *ctx, htb_backend_kind kind, long precision,
                       long max_precision, char *error, size_t error_size) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->kind = kind;
    ctx->precision = precision > 0 ? precision : 128;
    ctx->max_precision = max_precision >= ctx->precision ? max_precision : ctx->precision;
    if (kind == HTB_BACKEND_ARB)
        return htb_arb_backend_init(ctx, error, error_size);
    if (kind == HTB_BACKEND_CALCIUM)
        return htb_calcium_backend_init(ctx, error, error_size);
    snprintf(error, error_size, "unknown arithmetic backend");
    return 0;
}

void htb_arith_ctx_clear(htb_arith_ctx *ctx) {
    if (!ctx || !ctx->backend) return;
    if (ctx->kind == HTB_BACKEND_ARB) htb_arb_backend_clear(ctx);
    else if (ctx->kind == HTB_BACKEND_CALCIUM) htb_calcium_backend_clear(ctx);
    memset(ctx, 0, sizeof(*ctx));
}

const char *htb_backend_name(htb_backend_kind kind) {
    return kind == HTB_BACKEND_CALCIUM ? "Exact Algebra" : "Scientific Computing";
}

void htb_scalar_init(htb_arith_ctx *ctx, htb_scalar *x) {
    x->ctx = ctx;
    x->value = NULL;
    ctx->ops->value_init(x);
    ctx->scalar_count++;
    if (ctx->scalar_count > ctx->peak_scalar_count) ctx->peak_scalar_count = ctx->scalar_count;
}

void htb_scalar_clear(htb_scalar *x) {
    if (!x || !x->ctx || !x->value) return;
    htb_arith_ctx *ctx = x->ctx;
    ctx->ops->value_clear(x);
    if (ctx->scalar_count) ctx->scalar_count--;
    x->ctx = NULL;
    x->value = NULL;
}

#define WRAP2(name) \
int htb_scalar_##name(htb_scalar *z, const htb_scalar *x, const htb_scalar *y) { \
    return z && z->ctx && z->ctx == x->ctx && x->ctx == y->ctx && z->ctx->ops->name(z, x, y); \
}

int htb_scalar_set(htb_scalar *z, const htb_scalar *x) {
    return z && x && z->ctx == x->ctx && z->ctx->ops->set(z, x);
}
int htb_scalar_set_si(htb_scalar *z, int64_t x) { return z->ctx->ops->set_si(z, x); }
int htb_scalar_parse_rational(htb_scalar *z, const char *text, char *error, size_t error_size) {
    return z->ctx->ops->set_rational(z, text, error, error_size);
}
int htb_scalar_set_sin_cos_degrees(htb_scalar *sine, htb_scalar *cosine, const char *text, char *error, size_t error_size) {
    return sine && cosine && sine->ctx == cosine->ctx && sine->ctx->ops->set_sin_cos_degrees(sine, cosine, text, error, error_size);
}
WRAP2(add)
WRAP2(sub)
WRAP2(mul)
int htb_scalar_sqr(htb_scalar *z, const htb_scalar *x) { return z->ctx == x->ctx && z->ctx->ops->sqr(z, x); }
int htb_scalar_hull(htb_scalar *z, const htb_scalar *x, const htb_scalar *y) { return z->ctx == x->ctx && x->ctx == y->ctx && z->ctx->ops->hull(z, x, y); }
WRAP2(div)
int htb_scalar_neg(htb_scalar *z, const htb_scalar *x) { return z->ctx == x->ctx && z->ctx->ops->neg(z, x); }
int htb_scalar_sqrt(htb_scalar *z, const htb_scalar *x) { return z->ctx == x->ctx && z->ctx->ops->sqrt(z, x); }

htb_cmp htb_scalar_compare(const htb_scalar *x, const htb_scalar *y) {
    if (!x || !y || x->ctx != y->ctx) return HTB_CMP_ERROR;
    x->ctx->comparisons++;
    htb_cmp c = x->ctx->ops->compare(x, y);
    if (c == HTB_CMP_UNRESOLVED) x->ctx->unresolved_comparisons++;
    return c;
}

htb_cmp htb_scalar_compare_si(const htb_scalar *x, int64_t y) {
    htb_scalar t;
    htb_scalar_init(x->ctx, &t);
    htb_scalar_set_si(&t, y);
    htb_cmp c = htb_scalar_compare(x, &t);
    htb_scalar_clear(&t);
    return c;
}

int htb_scalar_is_finite(const htb_scalar *x) { return x && x->ctx->ops->is_finite(x); }
int htb_scalar_get_display(const htb_scalar *x, int digits, char *out, size_t out_size) {
    return x->ctx->ops->get_display(x, digits, out, out_size);
}
int htb_scalar_get_enclosure(const htb_scalar *x, int digits, char *out, size_t out_size) {
    return x->ctx->ops->get_enclosure(x, digits, out, out_size);
}
double htb_scalar_get_approx(const htb_scalar *x) {
    char buffer[256];
    if (!htb_scalar_get_display(x, 30, buffer, sizeof(buffer))) return 0.0;
    char *end = NULL;
    errno = 0;
    double value = strtod(buffer, &end);
    return errno == 0 && end != buffer ? value : 0.0;
}
