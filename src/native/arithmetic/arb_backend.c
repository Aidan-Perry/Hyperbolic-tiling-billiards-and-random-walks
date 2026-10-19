#include "htb/arithmetic.h"
#include <flint/arb.h>
#include <flint/fmpq.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int htb_parse_fmpq(fmpq_t result, const char *text, char *error, size_t error_size);

static arb_ptr value(const htb_scalar *x) { return (arb_ptr)x->value; }
static void init_value(htb_scalar *x) { x->value = malloc(sizeof(arb_struct)); arb_init(value(x)); }
static void clear_value(htb_scalar *x) { arb_clear(value(x)); free(x->value); }
static int set_value(htb_scalar *z, const htb_scalar *x) { arb_set(value(z), value(x)); return 1; }
static int set_si_value(htb_scalar *z, int64_t x) { arb_set_si(value(z), (slong)x); return 1; }
static int set_rational(htb_scalar *z, const char *text, char *error, size_t error_size) {
    fmpq_t q; fmpq_init(q);
    int ok = htb_parse_fmpq(q, text, error, error_size);
    if (ok) arb_set_fmpq(value(z), q, z->ctx->precision);
    fmpq_clear(q); return ok;
}
static int set_sin_cos_degrees(htb_scalar*sine,htb_scalar*cosine,const char*text,char*error,size_t error_size){
    fmpq_t turns;fmpq_init(turns);int ok=htb_parse_fmpq(turns,text,error,error_size);
    if(ok){fmpz_mul_ui(fmpq_denref(turns),fmpq_denref(turns),180);fmpq_canonicalise(turns);
      arb_sin_cos_pi_fmpq(value(sine),value(cosine),turns,sine->ctx->precision);ok=arb_is_finite(value(sine))&&arb_is_finite(value(cosine));}
    if(!ok&&error[0]=='\0')snprintf(error,error_size,"could not evaluate the direction angle");fmpq_clear(turns);return ok;
}
#define BIN(name, fn) static int name(htb_scalar *z,const htb_scalar*x,const htb_scalar*y){fn(value(z),value(x),value(y),z->ctx->precision);return 1;}
BIN(add_value, arb_add)
BIN(sub_value, arb_sub)
BIN(mul_value, arb_mul)
static int sqr_value(htb_scalar*z,const htb_scalar*x){arb_sqr(value(z),value(x),z->ctx->precision);return 1;}
static int hull_value(htb_scalar*z,const htb_scalar*x,const htb_scalar*y){arb_union(value(z),value(x),value(y),z->ctx->precision);return 1;}
static int div_value(htb_scalar*z,const htb_scalar*x,const htb_scalar*y){arb_div(value(z),value(x),value(y),z->ctx->precision);return arb_is_finite(value(z));}
static int neg_value(htb_scalar *z,const htb_scalar*x){arb_neg(value(z),value(x));return 1;}
static int sqrt_value(htb_scalar *z,const htb_scalar*x){
    if(arb_is_negative(value(x)))return 0;arb_t domain;arb_init(domain);arb_nonnegative_part(domain,value(x));arb_sqrt(value(z),domain,z->ctx->precision);arb_clear(domain);return arb_is_finite(value(z));
}
static htb_cmp compare_value(const htb_scalar *x,const htb_scalar*y){
    if (arb_lt(value(x), value(y))) return HTB_CMP_LESS;
    if (arb_gt(value(x), value(y))) return HTB_CMP_GREATER;
    if (arb_is_exact(value(x)) && arb_is_exact(value(y)) && arb_equal(value(x),value(y))) return HTB_CMP_EQUAL;
    return HTB_CMP_UNRESOLVED;
}
static int finite_value(const htb_scalar*x){return arb_is_finite(value(x));}
static int stringify(const htb_scalar*x,int digits,char*out,size_t n,ulong flags){
    char *s=arb_get_str(value(x),digits,flags); if(!s)return 0;
    int ok=snprintf(out,n,"%s",s)>=0 && strlen(s)<n; flint_free(s); return ok;
}
static int display_value(const htb_scalar*x,int d,char*out,size_t n){return stringify(x,d,out,n,ARB_STR_NO_RADIUS);}
static int enclosure_value(const htb_scalar*x,int d,char*out,size_t n){return stringify(x,d,out,n,0);}

static const htb_arith_ops OPS={init_value,clear_value,set_value,set_si_value,set_rational,set_sin_cos_degrees,
 add_value,sub_value,mul_value,sqr_value,hull_value,div_value,neg_value,sqrt_value,compare_value,finite_value,
 display_value,enclosure_value};
int htb_arb_backend_init(htb_arith_ctx *ctx,char*error,size_t error_size){
    (void)error;(void)error_size; ctx->backend=calloc(1,1); ctx->ops=&OPS; return ctx->backend!=NULL;
}
void htb_arb_backend_clear(htb_arith_ctx *ctx){free(ctx->backend);ctx->backend=NULL;}
