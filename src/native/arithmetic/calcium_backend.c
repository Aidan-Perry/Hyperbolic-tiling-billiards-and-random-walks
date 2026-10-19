#include "htb/arithmetic.h"
#include <flint/ca.h>
#include <flint/acb.h>
#include <flint/fmpq.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int htb_parse_fmpq(fmpq_t result, const char *text, char *error, size_t error_size);
typedef struct { ca_ctx_t ca; } calcium_state;
static calcium_state *state(const htb_scalar*x){return (calcium_state*)x->ctx->backend;}
static ca_ptr value(const htb_scalar*x){return (ca_ptr)x->value;}
static htb_cmp compare_value(const htb_scalar*x,const htb_scalar*y);
static void init_value(htb_scalar*x){x->value=malloc(sizeof(ca_struct));ca_init(value(x),state(x)->ca);}
static void clear_value(htb_scalar*x){ca_clear(value(x),state(x)->ca);free(x->value);}
static int set_value(htb_scalar*z,const htb_scalar*x){ca_set(value(z),value(x),state(z)->ca);return 1;}
static int set_si_value(htb_scalar*z,int64_t x){ca_set_si(value(z),(slong)x,state(z)->ca);return 1;}
static int set_rational(htb_scalar*z,const char*text,char*error,size_t error_size){
 fmpq_t q;fmpq_init(q);int ok=htb_parse_fmpq(q,text,error,error_size);if(ok)ca_set_fmpq(value(z),q,state(z)->ca);fmpq_clear(q);return ok;
}
static int set_sin_cos_degrees(htb_scalar*sine,htb_scalar*cosine,const char*text,char*error,size_t error_size){
 fmpq_t q;fmpq_init(q);int ok=htb_parse_fmpq(q,text,error,error_size);ca_t angle,pi;ca_init(angle,state(sine)->ca);ca_init(pi,state(sine)->ca);
 if(ok){ca_set_fmpq(angle,q,state(sine)->ca);ca_div_si(angle,angle,180,state(sine)->ca);ca_pi(pi,state(sine)->ca);ca_mul(angle,angle,pi,state(sine)->ca);ca_sin_cos(value(sine),value(cosine),angle,state(sine)->ca);ok=!ca_is_special(value(sine),state(sine)->ca)&&!ca_is_special(value(cosine),state(sine)->ca);}
 if(!ok&&error[0]=='\0')snprintf(error,error_size,"could not evaluate the exact direction angle");ca_clear(angle,state(sine)->ca);ca_clear(pi,state(sine)->ca);fmpq_clear(q);return ok;
}
#define BIN(name,fn) static int name(htb_scalar*z,const htb_scalar*x,const htb_scalar*y){fn(value(z),value(x),value(y),state(z)->ca);return !ca_is_special(value(z),state(z)->ca);}
BIN(add_value,ca_add)
BIN(sub_value,ca_sub)
BIN(mul_value,ca_mul)
static int sqr_value(htb_scalar*z,const htb_scalar*x){ca_mul(value(z),value(x),value(x),state(z)->ca);return !ca_is_special(value(z),state(z)->ca);}
static int hull_value(htb_scalar*z,const htb_scalar*x,const htb_scalar*y){htb_cmp c=compare_value(x,y);if(c==HTB_CMP_LESS||c==HTB_CMP_EQUAL)return set_value(z,x);if(c==HTB_CMP_GREATER)return set_value(z,y);return 0;}
BIN(div_value,ca_div)
static int neg_value(htb_scalar*z,const htb_scalar*x){ca_neg(value(z),value(x),state(z)->ca);return 1;}
static int sqrt_value(htb_scalar*z,const htb_scalar*x){ca_sqrt(value(z),value(x),state(z)->ca);return !ca_is_special(value(z),state(z)->ca);}
static htb_cmp compare_value(const htb_scalar*x,const htb_scalar*y){
 truth_t eq=ca_check_equal(value(x),value(y),state(x)->ca);if(eq==T_TRUE)return HTB_CMP_EQUAL;
 truth_t lt=ca_check_lt(value(x),value(y),state(x)->ca);if(lt==T_TRUE)return HTB_CMP_LESS;
 truth_t gt=ca_check_gt(value(x),value(y),state(x)->ca);if(gt==T_TRUE)return HTB_CMP_GREATER;
 return HTB_CMP_UNRESOLVED;
}
static int finite_value(const htb_scalar*x){return !ca_is_special(value(x),state(x)->ca)&&ca_check_is_real(value(x),state(x)->ca)==T_TRUE;}
static int display_value(const htb_scalar*x,int digits,char*out,size_t n){
 char*s=ca_get_decimal_str(value(x),digits,0,state(x)->ca);if(!s)return 0;int ok=snprintf(out,n,"%s",s)>=0&&strlen(s)<n;flint_free(s);return ok;
}
static int enclosure_value(const htb_scalar*x,int digits,char*out,size_t n){
 acb_t a;acb_init(a);ca_get_acb(a,value(x),(slong)(digits*4+32),state(x)->ca);char*s=arb_get_str(acb_realref(a),digits,0);
 int ok=s&&snprintf(out,n,"%s",s)>=0&&strlen(s)<n;if(s)flint_free(s);acb_clear(a);return ok;
}
static const htb_arith_ops OPS={init_value,clear_value,set_value,set_si_value,set_rational,set_sin_cos_degrees,
 add_value,sub_value,mul_value,sqr_value,hull_value,div_value,neg_value,sqrt_value,compare_value,finite_value,
 display_value,enclosure_value};
int htb_calcium_backend_init(htb_arith_ctx*ctx,char*error,size_t error_size){
 calcium_state*s=malloc(sizeof(*s));if(!s){snprintf(error,error_size,"cannot allocate Calcium context");return 0;}ca_ctx_init(s->ca);
 ca_ctx_set_option(s->ca,CA_OPT_PREC_LIMIT,ctx->max_precision);ctx->backend=s;ctx->ops=&OPS;return 1;
}
void htb_calcium_backend_clear(htb_arith_ctx*ctx){calcium_state*s=ctx->backend;ca_ctx_clear(s->ca);free(s);ctx->backend=NULL;}
