#include "htb/geometry.h"

void htb_vec2_init(htb_arith_ctx *ctx, htb_vec2 *v) {
    htb_scalar_init(ctx, &v->x); htb_scalar_init(ctx, &v->y);
}
void htb_vec2_clear(htb_vec2 *v) { htb_scalar_clear(&v->x); htb_scalar_clear(&v->y); }
int htb_vec2_set(htb_vec2 *dst, const htb_vec2 *src) {
    return htb_scalar_set(&dst->x, &src->x) && htb_scalar_set(&dst->y, &src->y);
}

static int squared_norm(htb_scalar *out, const htb_vec2 *v) {
    htb_scalar a, b; htb_scalar_init(out->ctx, &a); htb_scalar_init(out->ctx, &b);
    int ok = htb_scalar_sqr(&a, &v->x) && htb_scalar_sqr(&b, &v->y) && htb_scalar_add(out, &a, &b);
    htb_scalar_clear(&a); htb_scalar_clear(&b); return ok;
}

int htb_disk_contains(const htb_vec2 *p, htb_cmp *inside_cmp) {
    htb_scalar r2, one; htb_scalar_init(p->x.ctx, &r2); htb_scalar_init(p->x.ctx, &one);
    htb_scalar_set_si(&one, 1); int ok = squared_norm(&r2, p);
    *inside_cmp = ok ? htb_scalar_compare(&r2, &one) : HTB_CMP_ERROR;
    htb_scalar_clear(&r2); htb_scalar_clear(&one); return ok;
}

int htb_negative_refract(htb_vec2 *out, const htb_vec2 *v, const htb_vec2 *n) {
    htb_arith_ctx *ctx = out->x.ctx; htb_scalar dot, nn, t, a, b, two;
    htb_scalar_init(ctx,&dot); htb_scalar_init(ctx,&nn); htb_scalar_init(ctx,&t);
    htb_scalar_init(ctx,&a); htb_scalar_init(ctx,&b); htb_scalar_init(ctx,&two); htb_scalar_set_si(&two,2);
    int ok=htb_scalar_mul(&a,&v->x,&n->x)&&htb_scalar_mul(&b,&v->y,&n->y)&&htb_scalar_add(&dot,&a,&b)&&
      htb_scalar_sqr(&a,&n->x)&&htb_scalar_sqr(&b,&n->y)&&htb_scalar_add(&nn,&a,&b)&&
      htb_scalar_mul(&t,&two,&dot)&&htb_scalar_div(&t,&t,&nn)&&
      htb_scalar_mul(&a,&t,&n->x)&&htb_scalar_sub(&out->x,&a,&v->x)&&
      htb_scalar_mul(&b,&t,&n->y)&&htb_scalar_sub(&out->y,&b,&v->y);
    htb_scalar_clear(&dot); htb_scalar_clear(&nn); htb_scalar_clear(&t);
    htb_scalar_clear(&a); htb_scalar_clear(&b); htb_scalar_clear(&two); return ok;
}

int htb_ray_point(htb_vec2 *out, const htb_vec2 *p, const htb_vec2 *v, const htb_scalar *t) {
    htb_scalar q; htb_scalar_init(t->ctx, &q);
    int ok=htb_scalar_mul(&q,&v->x,t)&&htb_scalar_add(&out->x,&p->x,&q)&&
      htb_scalar_mul(&q,&v->y,t)&&htb_scalar_add(&out->y,&p->y,&q);
    htb_scalar_clear(&q); return ok;
}

int htb_point_distance(htb_scalar *out, const htb_vec2 *a, const htb_vec2 *b) {
    htb_vec2 d; htb_vec2_init(out->ctx, &d); htb_scalar x2, y2;
    htb_scalar_init(out->ctx, &x2); htb_scalar_init(out->ctx, &y2);
    int ok=htb_scalar_sub(&d.x,&a->x,&b->x)&&htb_scalar_sub(&d.y,&a->y,&b->y)&&
      htb_scalar_sqr(&x2,&d.x)&&htb_scalar_sqr(&y2,&d.y)&&htb_scalar_add(out,&x2,&y2)&&
      htb_scalar_sqrt(out,out);
    htb_scalar_clear(&x2); htb_scalar_clear(&y2); htb_vec2_clear(&d); return ok;
}
