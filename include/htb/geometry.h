#ifndef HTB_GEOMETRY_H
#define HTB_GEOMETRY_H

#include "htb/arithmetic.h"
#include <stddef.h>
#include <stdint.h>

typedef struct { htb_scalar x, y; } htb_vec2;

void htb_vec2_init(htb_arith_ctx *ctx, htb_vec2 *v);
void htb_vec2_clear(htb_vec2 *v);
int htb_vec2_set(htb_vec2 *dst, const htb_vec2 *src);

int htb_disk_contains(const htb_vec2 *p, htb_cmp *inside_cmp);
int htb_negative_refract(htb_vec2 *out, const htb_vec2 *incoming, const htb_vec2 *normal);
int htb_ray_point(htb_vec2 *out, const htb_vec2 *point, const htb_vec2 *direction,
                  const htb_scalar *time);
int htb_point_distance(htb_scalar *out, const htb_vec2 *a, const htb_vec2 *b);

#endif
