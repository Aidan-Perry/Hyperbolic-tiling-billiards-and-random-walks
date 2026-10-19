#include "htb/tiling.h"

#include <flint/flint.h>
#include <flint/fmpq.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Exact coefficient field Q(sqrt(2), sqrt(phi)), with
   a^2 = 2 and b^4 = b^2 + 1.  The eight-element basis is a^i b^j,
   i in {0,1}, j in {0,1,2,3}.  This representation is used only to
   canonicalize the reflection-group topology. Simulation scalars still go
   through the selected htb_arith_ctx. */
typedef struct { fmpq_t c[8]; } q5e;
typedef struct { q5e v[3]; } q5v;
typedef struct { q5e a[3][3]; } q5m;
enum { Q5_UNRESOLVED_LINEARITY = 1 };

static void e_init(q5e*x){for(int i=0;i<8;i++)fmpq_init(x->c[i]);}
static void e_clear(q5e*x){for(int i=0;i<8;i++)fmpq_clear(x->c[i]);}
static void e_zero(q5e*x){for(int i=0;i<8;i++)fmpq_zero(x->c[i]);}
static void e_set(q5e*z,const q5e*x){for(int i=0;i<8;i++)fmpq_set(z->c[i],x->c[i]);}
static void e_set_si(q5e*z,long n){e_zero(z);fmpq_set_si(z->c[0],n,1);}
static void e_set_basis(q5e*z,int i,long num,long den){e_zero(z);fmpq_set_si(z->c[i],num,den);}
static void e_add(q5e*z,const q5e*x,const q5e*y){for(int i=0;i<8;i++)fmpq_add(z->c[i],x->c[i],y->c[i]);}
static void e_sub(q5e*z,const q5e*x,const q5e*y){for(int i=0;i<8;i++)fmpq_sub(z->c[i],x->c[i],y->c[i]);}
static void e_neg(q5e*z,const q5e*x){for(int i=0;i<8;i++)fmpq_neg(z->c[i],x->c[i]);}
static int e_is_zero(const q5e*x){for(int i=0;i<8;i++)if(!fmpq_is_zero(x->c[i]))return 0;return 1;}
static int e_equal(const q5e*x,const q5e*y){for(int i=0;i<8;i++)if(!fmpq_equal(x->c[i],y->c[i]))return 0;return 1;}
static void e_accumulate(q5e*z,int ia,int jb,const fmpq_t value,long factor){
    fmpq_t t;fmpq_init(t);fmpq_mul_si(t,value,factor);fmpq_add(z->c[ia*4+jb],z->c[ia*4+jb],t);fmpq_clear(t);
}
static void e_mul(q5e*z,const q5e*x,const q5e*y){
    q5e out;e_init(&out);e_zero(&out);fmpq_t product;fmpq_init(product);
    for(int ix=0;ix<8;ix++)if(!fmpq_is_zero(x->c[ix]))for(int iy=0;iy<8;iy++)if(!fmpq_is_zero(y->c[iy])){
        int ai=ix/4+iy/4,bj=ix%4+iy%4;long af=ai==2?2:1;ai%=2;fmpq_mul(product,x->c[ix],y->c[iy]);
        if(bj<4)e_accumulate(&out,ai,bj,product,af);
        else if(bj==4){e_accumulate(&out,ai,2,product,af);e_accumulate(&out,ai,0,product,af);}
        else if(bj==5){e_accumulate(&out,ai,3,product,af);e_accumulate(&out,ai,1,product,af);}
        else{e_accumulate(&out,ai,2,product,2*af);e_accumulate(&out,ai,0,product,af);}
    }
    fmpq_clear(product);e_set(z,&out);e_clear(&out);
}

static void v_init(q5v*x){for(int i=0;i<3;i++)e_init(&x->v[i]);}
static void v_clear(q5v*x){for(int i=0;i<3;i++)e_clear(&x->v[i]);}
static void v_set(q5v*z,const q5v*x){for(int i=0;i<3;i++)e_set(&z->v[i],&x->v[i]);}
static int v_equal(const q5v*x,const q5v*y){return e_equal(&x->v[0],&y->v[0])&&e_equal(&x->v[1],&y->v[1])&&e_equal(&x->v[2],&y->v[2]);}
static int v_equal_up_to_sign(const q5v*x,const q5v*y){if(v_equal(x,y))return 1;q5v n;v_init(&n);for(int i=0;i<3;i++)e_neg(&n.v[i],&y->v[i]);int same=v_equal(x,&n);v_clear(&n);return same;}
static void m_init(q5m*x){for(int i=0;i<3;i++)for(int j=0;j<3;j++)e_init(&x->a[i][j]);}
static void m_clear(q5m*x){for(int i=0;i<3;i++)for(int j=0;j<3;j++)e_clear(&x->a[i][j]);}
static void m_identity(q5m*x){for(int i=0;i<3;i++)for(int j=0;j<3;j++)e_set_si(&x->a[i][j],i==j);}
static void m_set(q5m*z,const q5m*x){for(int i=0;i<3;i++)for(int j=0;j<3;j++)e_set(&z->a[i][j],&x->a[i][j]);}
static void m_mul(q5m*z,const q5m*x,const q5m*y){
    q5m out;m_init(&out);for(int i=0;i<3;i++)for(int j=0;j<3;j++){e_zero(&out.a[i][j]);for(int k=0;k<3;k++){q5e t,s;e_init(&t);e_init(&s);e_mul(&t,&x->a[i][k],&y->a[k][j]);e_add(&s,&out.a[i][j],&t);e_set(&out.a[i][j],&s);e_clear(&t);e_clear(&s);}}m_set(z,&out);m_clear(&out);
}
static void mv_mul(q5v*z,const q5m*x,const q5v*y){
    q5v out;v_init(&out);for(int i=0;i<3;i++){e_zero(&out.v[i]);for(int k=0;k<3;k++){q5e t,s;e_init(&t);e_init(&s);e_mul(&t,&x->a[i][k],&y->v[k]);e_add(&s,&out.v[i],&t);e_set(&out.v[i],&s);e_clear(&t);e_clear(&s);}}v_set(z,&out);v_clear(&out);
}
static void v_cross_planes(q5v*z,const q5v*n,const q5v*m){
    q5e nr[3],mr[3],a,b;for(int i=0;i<3;i++){e_init(&nr[i]);e_init(&mr[i]);e_set(&nr[i],&n->v[i]);e_set(&mr[i],&m->v[i]);}e_neg(&nr[0],&nr[0]);e_neg(&mr[0],&mr[0]);e_init(&a);e_init(&b);
    e_mul(&a,&nr[1],&mr[2]);e_mul(&b,&nr[2],&mr[1]);e_sub(&z->v[0],&a,&b);
    e_mul(&a,&nr[2],&mr[0]);e_mul(&b,&nr[0],&mr[2]);e_sub(&z->v[1],&a,&b);
    e_mul(&a,&nr[0],&mr[1]);e_mul(&b,&nr[1],&mr[0]);e_sub(&z->v[2],&a,&b);
    e_clear(&a);e_clear(&b);for(int i=0;i<3;i++){e_clear(&nr[i]);e_clear(&mr[i]);}
}
static void v_lorentz_dot(q5e*z,const q5v*x,const q5v*y){
    q5e a,b;e_init(&a);e_init(&b);
    e_mul(&a,&x->v[0],&y->v[0]);e_neg(&a,&a);
    e_mul(&b,&x->v[1],&y->v[1]);e_add(&a,&a,&b);
    e_mul(&b,&x->v[2],&y->v[2]);e_add(z,&a,&b);
    e_clear(&a);e_clear(&b);
}
static void reflection(q5m*r,const q5v*n){
    q5e t,two;e_init(&t);e_init(&two);e_set_si(&two,2);
    for(int i=0;i<3;i++)for(int j=0;j<3;j++){e_mul(&t,&n->v[i],&n->v[j]);e_mul(&t,&t,&two);if(j==0)e_neg(&t,&t);e_set_si(&r->a[i][j],i==j);e_sub(&r->a[i][j],&r->a[i][j],&t);}
    e_clear(&t);e_clear(&two);
}

typedef struct {q5m transform;q5v center;uint64_t hash_a,hash_b;} q5_tile;
typedef struct {size_t owner,other;int owner_edge,other_edge;q5v normal;uint64_t hash_a,hash_b;} q5_boundary;
typedef struct {
    q5m side_reflection[4];q5v base_normal[4],base_vertex[4],base_center;
    htb_scalar basis[8],h[3][3],hinv[3][3],base_normal_scalar[4][3];
    q5_tile*tiles;size_t tile_count,tile_capacity;
    q5_boundary*boundaries;size_t boundary_count,boundary_capacity;
    int central_vertex_at_origin;
    htb_vec2 central_vertex_disk;
} q5_state;

static int scalar_from_e(const q5_state*s,htb_scalar*out,const q5e*x){
    htb_arith_ctx*ctx=out->ctx;htb_scalar term,coef,sum;htb_scalar_init(ctx,&term);htb_scalar_init(ctx,&coef);htb_scalar_init(ctx,&sum);htb_scalar_set_si(&sum,0);int ok=1;char error[80]={0};
    for(int i=0;i<8&&ok;i++)if(!fmpq_is_zero(x->c[i])){char*text=fmpq_get_str(NULL,10,x->c[i]);ok=text&&htb_scalar_parse_rational(&coef,text,error,sizeof(error))&&htb_scalar_mul(&term,&coef,&s->basis[i])&&htb_scalar_add(&sum,&sum,&term);if(text)flint_free(text);}
    if(ok)ok=htb_scalar_set(out,&sum);htb_scalar_clear(&term);htb_scalar_clear(&coef);htb_scalar_clear(&sum);return ok;
}
static int scalar_from_v(const q5_state*s,htb_scalar out[3],const q5v*x){for(int i=0;i<3;i++)if(!scalar_from_e(s,&out[i],&x->v[i]))return 0;return 1;}
static int sm_apply(htb_scalar out[3],htb_scalar m[3][3],htb_scalar in[3]){
    htb_arith_ctx*ctx=in[0].ctx;htb_scalar t,sum;htb_scalar_init(ctx,&t);htb_scalar_init(ctx,&sum);int ok=1;
    for(int i=0;i<3&&ok;i++){htb_scalar_set_si(&sum,0);for(int j=0;j<3&&ok;j++)ok=htb_scalar_mul(&t,&m[i][j],&in[j])&&htb_scalar_add(&sum,&sum,&t);if(ok)ok=htb_scalar_set(&out[i],&sum);}htb_scalar_clear(&t);htb_scalar_clear(&sum);return ok;
}
static int sm_mul(htb_scalar out[3][3],htb_scalar a[3][3],htb_scalar b[3][3]){
    htb_arith_ctx*ctx=a[0][0].ctx;htb_scalar t,sum,tmp[3][3];for(int i=0;i<3;i++)for(int j=0;j<3;j++)htb_scalar_init(ctx,&tmp[i][j]);htb_scalar_init(ctx,&t);htb_scalar_init(ctx,&sum);int ok=1;
    for(int i=0;i<3&&ok;i++)for(int j=0;j<3&&ok;j++){htb_scalar_set_si(&sum,0);for(int k=0;k<3&&ok;k++)ok=htb_scalar_mul(&t,&a[i][k],&b[k][j])&&htb_scalar_add(&sum,&sum,&t);if(ok)ok=htb_scalar_set(&tmp[i][j],&sum);}
    if(ok)for(int i=0;i<3;i++)for(int j=0;j<3;j++)htb_scalar_set(&out[i][j],&tmp[i][j]);for(int i=0;i<3;i++)for(int j=0;j<3;j++)htb_scalar_clear(&tmp[i][j]);htb_scalar_clear(&t);htb_scalar_clear(&sum);return ok;
}
static int disk_to_hyper(const htb_vec2*p,htb_scalar out[3]){
    htb_arith_ctx*ctx=p->x.ctx;htb_scalar r2,t,one,two,den;htb_scalar_init(ctx,&r2);htb_scalar_init(ctx,&t);htb_scalar_init(ctx,&one);htb_scalar_init(ctx,&two);htb_scalar_init(ctx,&den);htb_scalar_set_si(&one,1);htb_scalar_set_si(&two,2);
    int ok=htb_scalar_sqr(&r2,&p->x)&&htb_scalar_sqr(&t,&p->y)&&htb_scalar_add(&r2,&r2,&t)&&htb_scalar_sub(&den,&one,&r2)&&htb_scalar_add(&t,&one,&r2)&&htb_scalar_div(&out[0],&t,&den)&&htb_scalar_mul(&t,&two,&p->x)&&htb_scalar_div(&out[1],&t,&den)&&htb_scalar_mul(&t,&two,&p->y)&&htb_scalar_div(&out[2],&t,&den);
    htb_scalar_clear(&r2);htb_scalar_clear(&t);htb_scalar_clear(&one);htb_scalar_clear(&two);htb_scalar_clear(&den);return ok;
}
static int hyper_to_disk(htb_scalar in[3],htb_vec2*p){htb_scalar den,one;htb_scalar_init(in[0].ctx,&den);htb_scalar_init(in[0].ctx,&one);htb_scalar_set_si(&one,1);int ok=htb_scalar_add(&den,&in[0],&one)&&htb_scalar_div(&p->x,&in[1],&den)&&htb_scalar_div(&p->y,&in[2],&den);htb_scalar_clear(&den);htb_scalar_clear(&one);return ok;}
static int lorentz_dot(htb_scalar*out,htb_scalar x[3],htb_scalar y[3]){htb_arith_ctx*ctx=x[0].ctx;htb_scalar a,b,c;htb_scalar_init(ctx,&a);htb_scalar_init(ctx,&b);htb_scalar_init(ctx,&c);int ok=htb_scalar_mul(&a,&x[0],&y[0])&&htb_scalar_neg(&a,&a)&&htb_scalar_mul(&b,&x[1],&y[1])&&htb_scalar_add(&a,&a,&b)&&htb_scalar_mul(&c,&x[2],&y[2])&&htb_scalar_add(out,&a,&c);htb_scalar_clear(&a);htb_scalar_clear(&b);htb_scalar_clear(&c);return ok;}
static int boost_matrix(htb_scalar out[3][3],htb_scalar p[3],int inverse){
    htb_arith_ctx*ctx=p[0].ctx;htb_scalar one,den,t;htb_scalar_init(ctx,&one);htb_scalar_init(ctx,&den);htb_scalar_init(ctx,&t);htb_scalar_set_si(&one,1);int ok=htb_scalar_set(&out[0][0],&p[0])&&htb_scalar_add(&den,&p[0],&one);
    for(int i=1;i<3&&ok;i++){if(inverse)ok=htb_scalar_neg(&out[0][i],&p[i])&&htb_scalar_neg(&out[i][0],&p[i]);else ok=htb_scalar_set(&out[0][i],&p[i])&&htb_scalar_set(&out[i][0],&p[i]);}
    for(int i=1;i<3&&ok;i++)for(int j=1;j<3&&ok;j++){ok=htb_scalar_mul(&t,&p[i],&p[j])&&htb_scalar_div(&t,&t,&den);if(ok&&i==j)ok=htb_scalar_add(&out[i][j],&one,&t);else if(ok)ok=htb_scalar_set(&out[i][j],&t);}
    htb_scalar_clear(&one);htb_scalar_clear(&den);htb_scalar_clear(&t);return ok;
}
static int normalize_hyper(htb_scalar x[3]){
    htb_arith_ctx*ctx=x[0].ctx;htb_scalar n,scale,t;htb_scalar_init(ctx,&n);htb_scalar_init(ctx,&scale);htb_scalar_init(ctx,&t);int ok=lorentz_dot(&n,x,x)&&htb_scalar_neg(&n,&n)&&htb_scalar_sqrt(&scale,&n);for(int i=0;i<3&&ok;i++)ok=htb_scalar_div(&t,&x[i],&scale)&&htb_scalar_set(&x[i],&t);if(ok&&htb_scalar_get_approx(&x[0])<0)for(int i=0;i<3&&ok;i++)ok=htb_scalar_neg(&x[i],&x[i]);htb_scalar_clear(&n);htb_scalar_clear(&scale);htb_scalar_clear(&t);return ok;
}

static void hash_text(uint64_t*h,const char*text,uint64_t prime){for(const unsigned char*p=(const unsigned char*)text;*p;p++){*h^=*p;*h*=prime;}*h^=255;*h*=prime;}
static void hash_vector(const q5v*v,int up_to_sign,uint64_t*a,uint64_t*b){
    *a=UINT64_C(1469598103934665603);*b=UINT64_C(1099511628211);int sign=1;
    if(up_to_sign)for(int i=0;i<3;i++)for(int j=0;j<8;j++)if(!fmpq_is_zero(v->v[i].c[j])){sign=fmpq_sgn(v->v[i].c[j])<0?-1:1;i=3;break;}
    for(int i=0;i<3;i++)for(int j=0;j<8;j++){fmpq_t value;fmpq_init(value);if(sign<0)fmpq_neg(value,v->v[i].c[j]);else fmpq_set(value,v->v[i].c[j]);char*text=fmpq_get_str(NULL,10,value);hash_text(a,text,UINT64_C(1099511628211));hash_text(b,text,UINT64_C(14029467366897019727));flint_free(text);fmpq_clear(value);}
}
static void make_tile_id(const q5_state*s,htb_tile_id*out,size_t id){memset(out,0,sizeof(*out));snprintf(out->key,sizeof(out->key),"order5-square:%016llx%016llx",(unsigned long long)s->tiles[id].hash_a,(unsigned long long)s->tiles[id].hash_b);snprintf(out->label,sizeof(out->label),"{4,5} square %zu",id);snprintf(out->type,sizeof(out->type),"square");out->provider_words[0]=id;out->provider_word_count=1;}
static int tile_index(const q5_state*s,const htb_tile_id*t,size_t*out){if(t->provider_word_count<1||t->provider_words[0]>=s->tile_count)return 0;*out=(size_t)t->provider_words[0];return 1;}
static int add_tile(q5_state*s,const q5m*g,size_t*out){
    q5v center;v_init(&center);mv_mul(&center,g,&s->base_center);for(size_t i=0;i<s->tile_count;i++)if(v_equal(&center,&s->tiles[i].center)){*out=i;v_clear(&center);return 1;}
    if(s->tile_count==s->tile_capacity){size_t next=s->tile_capacity?s->tile_capacity*2:32;void*p=realloc(s->tiles,next*sizeof(*s->tiles));if(!p){v_clear(&center);return 0;}s->tiles=p;s->tile_capacity=next;}
    q5_tile*t=&s->tiles[s->tile_count];m_init(&t->transform);v_init(&t->center);m_set(&t->transform,g);v_set(&t->center,&center);hash_vector(&center,0,&t->hash_a,&t->hash_b);*out=s->tile_count++;v_clear(&center);return 1;
}
static void transformed_normal(q5v*out,const q5_state*s,size_t tile,int edge){mv_mul(out,&s->tiles[tile].transform,&s->base_normal[edge]);}
static int find_local_edge(const q5_state*s,size_t tile,const q5v*normal){q5v n;v_init(&n);for(int k=0;k<4;k++){transformed_normal(&n,s,tile,k);if(v_equal_up_to_sign(&n,normal)){v_clear(&n);return k;}}v_clear(&n);return -1;}
static int ensure_neighbor(q5_state*s,size_t tile,int edge,size_t*neighbor,int*neighbor_edge,size_t*boundary){
    q5m g;m_init(&g);m_mul(&g,&s->tiles[tile].transform,&s->side_reflection[edge]);if(!add_tile(s,&g,neighbor)){m_clear(&g);return 0;}q5v n;v_init(&n);transformed_normal(&n,s,tile,edge);*neighbor_edge=find_local_edge(s,*neighbor,&n);if(*neighbor_edge<0){v_clear(&n);m_clear(&g);return 0;}
    for(size_t i=0;i<s->boundary_count;i++)if((s->boundaries[i].owner==tile&&s->boundaries[i].other==*neighbor)||(s->boundaries[i].owner==*neighbor&&s->boundaries[i].other==tile)){*boundary=i;v_clear(&n);m_clear(&g);return 1;}
    if(s->boundary_count==s->boundary_capacity){size_t next=s->boundary_capacity?s->boundary_capacity*2:64;void*p=realloc(s->boundaries,next*sizeof(*s->boundaries));if(!p){v_clear(&n);m_clear(&g);return 0;}s->boundaries=p;s->boundary_capacity=next;}
    q5_boundary*b=&s->boundaries[s->boundary_count];b->owner=tile;b->other=*neighbor;b->owner_edge=edge;b->other_edge=*neighbor_edge;v_init(&b->normal);v_set(&b->normal,&n);hash_vector(&n,1,&b->hash_a,&b->hash_b);*boundary=s->boundary_count++;v_clear(&n);m_clear(&g);return 1;
}
static int make_edge(q5_state*s,size_t tile,int local,htb_edge_ref*out){size_t neighbor,boundary;int reverse;if(!ensure_neighbor(s,tile,local,&neighbor,&reverse,&boundary))return 0;memset(out,0,sizeof(*out));out->local_index=local;out->owner_normal_sign=s->boundaries[boundary].owner==tile?1:-1;snprintf(out->label,sizeof(out->label),"side %d",local);snprintf(out->boundary_key,sizeof(out->boundary_key),"order5-square:boundary:%016llx%016llx",(unsigned long long)s->boundaries[boundary].hash_a,(unsigned long long)s->boundaries[boundary].hash_b);return 1;}
static q5_boundary*boundary_for(q5_state*s,const htb_edge_ref*edge){for(size_t id=0;id<s->boundary_count;id++){q5_boundary*b=&s->boundaries[id];char key[HTB_TILING_KEY_MAX];snprintf(key,sizeof(key),"order5-square:boundary:%016llx%016llx",(unsigned long long)b->hash_a,(unsigned long long)b->hash_b);if(strcmp(key,edge->boundary_key)==0)return b;}return NULL;}

static int world_normal(const htb_tiling*t,const q5v*n,htb_scalar out[3]){
    q5_state*s=t->state;htb_scalar local[3];for(int i=0;i<3;i++)htb_scalar_init(t->arith,&local[i]);
    int ok=scalar_from_v(s,local,n)&&sm_apply(out,s->h,local);
    /* Arb cannot prove that a cancellation is exactly zero.  When the
       distinguished vertex is at the disk origin, every exact geodesic
       through it is a diameter, so establish its zero time coefficient from
       the exact reflection-group field rather than from a numerical ball. */
    if(ok&&s->central_vertex_at_origin){q5e incidence;e_init(&incidence);v_lorentz_dot(&incidence,&s->base_vertex[0],n);if(e_is_zero(&incidence))ok=htb_scalar_set_si(&out[0],0);e_clear(&incidence);}
    for(int i=0;i<3;i++)htb_scalar_clear(&local[i]);return ok;
}
static int halfspace_disk(htb_scalar*out,const htb_vec2*p,htb_scalar n[3]){
    htb_arith_ctx*ctx=p->x.ctx;htb_scalar r2,tmp,one,two;htb_scalar_init(ctx,&r2);htb_scalar_init(ctx,&tmp);htb_scalar_init(ctx,&one);htb_scalar_init(ctx,&two);htb_scalar_set_si(&one,1);htb_scalar_set_si(&two,2);
    int ok=htb_scalar_sqr(&r2,&p->x)&&htb_scalar_sqr(&tmp,&p->y)&&htb_scalar_add(&r2,&r2,&tmp)&&htb_scalar_add(&r2,&r2,&one)&&htb_scalar_mul(out,&n[0],&r2)&&htb_scalar_mul(&tmp,&n[1],&p->x)&&htb_scalar_mul(&tmp,&tmp,&two)&&htb_scalar_sub(out,out,&tmp)&&htb_scalar_mul(&tmp,&n[2],&p->y)&&htb_scalar_mul(&tmp,&tmp,&two)&&htb_scalar_sub(out,out,&tmp);
    htb_scalar_clear(&r2);htb_scalar_clear(&tmp);htb_scalar_clear(&one);htb_scalar_clear(&two);return ok;
}
static int tile_normal_world(const htb_tiling*t,size_t tile,int edge,htb_scalar out[3]){q5_state*s=t->state;q5v n;v_init(&n);transformed_normal(&n,s,tile,edge);int ok=world_normal(t,&n,out);v_clear(&n);return ok;}

static int initialize(htb_tiling*t,const htb_tiling_config*config,char*error,size_t error_size){
    q5_state*s=calloc(1,sizeof(*s));if(!s){snprintf(error,error_size,"failed to allocate order-5 tiling state");return 0;}t->state=s;htb_arith_ctx*ctx=t->arith;
    for(int k=0;k<4;k++){m_init(&s->side_reflection[k]);v_init(&s->base_normal[k]);v_init(&s->base_vertex[k]);for(int i=0;i<3;i++)htb_scalar_init(ctx,&s->base_normal_scalar[k][i]);}v_init(&s->base_center);htb_vec2_init(ctx,&s->central_vertex_disk);for(int i=0;i<8;i++)htb_scalar_init(ctx,&s->basis[i]);for(int i=0;i<3;i++)for(int j=0;j<3;j++){htb_scalar_init(ctx,&s->h[i][j]);htb_scalar_init(ctx,&s->hinv[i][j]);}
    q5v mirrors[3];q5m reflectors[3],q,qpow,qinv,temp;for(int i=0;i<3;i++){v_init(&mirrors[i]);m_init(&reflectors[i]);}m_init(&q);m_init(&qpow);m_init(&qinv);m_init(&temp);
    for(int i=0;i<3;i++)for(int j=0;j<3;j++)e_zero(&mirrors[i].v[j]);e_set_basis(&mirrors[0].v[1],0,1,1);e_set_basis(&mirrors[1].v[2],0,1,1);e_zero(&mirrors[2].v[0]);fmpq_set_si(mirrors[2].v[0].c[3],1,2);fmpq_set_si(mirrors[2].v[0].c[1],-1,2);e_set_basis(&mirrors[2].v[1],2,-1,2);e_set_basis(&mirrors[2].v[2],4,-1,2);
    for(int i=0;i<3;i++)reflection(&reflectors[i],&mirrors[i]);m_mul(&q,&reflectors[1],&reflectors[2]);m_identity(&qpow);
    for(int k=0;k<4;k++){mv_mul(&s->base_normal[k],&qpow,&mirrors[0]);m_identity(&qinv);for(int z=0;z<(4-k)%4;z++){m_mul(&temp,&qinv,&q);m_set(&qinv,&temp);}m_mul(&temp,&qpow,&reflectors[0]);m_mul(&s->side_reflection[k],&temp,&qinv);m_mul(&temp,&qpow,&q);m_set(&qpow,&temp);}
    for(int k=0;k<4;k++)v_cross_planes(&s->base_vertex[k],&s->base_normal[k],&s->base_normal[(k+1)%4]);e_set_basis(&s->base_center.v[0],3,1,1);e_set_si(&s->base_center.v[1],-1);e_set_si(&s->base_center.v[2],0);
    htb_scalar two,five,root5,phi,root2,b,b2,b3;htb_scalar_init(ctx,&two);htb_scalar_init(ctx,&five);htb_scalar_init(ctx,&root5);htb_scalar_init(ctx,&phi);htb_scalar_init(ctx,&root2);htb_scalar_init(ctx,&b);htb_scalar_init(ctx,&b2);htb_scalar_init(ctx,&b3);htb_scalar_set_si(&two,2);htb_scalar_set_si(&five,5);int ok=htb_scalar_sqrt(&root5,&five);
    /* Build (1 + sqrt(5))/2 without a backend escape hatch. */
    if(ok){htb_scalar one;htb_scalar_init(ctx,&one);htb_scalar_set_si(&one,1);ok=htb_scalar_add(&phi,&root5,&one)&&htb_scalar_div(&phi,&phi,&two);htb_scalar_clear(&one);}if(ok)ok=htb_scalar_sqrt(&root2,&two)&&htb_scalar_sqrt(&b,&phi)&&htb_scalar_mul(&b2,&b,&b)&&htb_scalar_mul(&b3,&b2,&b);
    for(int i=0;i<8&&ok;i++){htb_scalar_set_si(&s->basis[i],1);int j=i%4;if(j==1)ok=htb_scalar_set(&s->basis[i],&b);else if(j==2)ok=htb_scalar_set(&s->basis[i],&b2);else if(j==3)ok=htb_scalar_set(&s->basis[i],&b3);if(ok&&i>=4)ok=htb_scalar_mul(&s->basis[i],&s->basis[i],&root2);}
    for(int k=0;k<4&&ok;k++)ok=scalar_from_v(s,s->base_normal_scalar[k],&s->base_normal[k]);
    if(ok)ok=htb_scalar_parse_rational(&s->central_vertex_disk.x,htb_tiling_parameter_value(config,"centerU","0"),error,error_size)&&htb_scalar_parse_rational(&s->central_vertex_disk.y,htb_tiling_parameter_value(config,"centerV","0"),error,error_size);htb_cmp inside=HTB_CMP_ERROR;if(ok)ok=htb_disk_contains(&s->central_vertex_disk,&inside)&&inside==HTB_CMP_LESS;if(!ok&&inside!=HTB_CMP_ERROR)snprintf(error,error_size,"order-5 center must lie strictly inside the unit disk");
    if(ok)s->central_vertex_at_origin=htb_scalar_compare_si(&s->central_vertex_disk.x,0)==HTB_CMP_EQUAL&&htb_scalar_compare_si(&s->central_vertex_disk.y,0)==HTB_CMP_EQUAL;
    htb_scalar pv[3],qv[3],bp[3][3],bq[3][3];for(int i=0;i<3;i++){htb_scalar_init(ctx,&pv[i]);htb_scalar_init(ctx,&qv[i]);for(int j=0;j<3;j++){htb_scalar_init(ctx,&bp[i][j]);htb_scalar_init(ctx,&bq[i][j]);}}
    if(ok)ok=scalar_from_v(s,pv,&s->base_vertex[0])&&normalize_hyper(pv)&&disk_to_hyper(&s->central_vertex_disk,qv)&&boost_matrix(bp,pv,1)&&boost_matrix(bq,qv,0)&&sm_mul(s->h,bq,bp);
    if(ok)for(int i=0;i<3;i++)for(int j=0;j<3;j++){int sign=(i==0?-1:1)*(j==0?-1:1);if(sign<0)ok=htb_scalar_neg(&s->hinv[i][j],&s->h[j][i]);else ok=htb_scalar_set(&s->hinv[i][j],&s->h[j][i]);}
    q5m identity;m_init(&identity);m_identity(&identity);size_t base_id;if(ok)ok=add_tile(s,&identity,&base_id);m_clear(&identity);
    for(int i=0;i<3;i++){htb_scalar_clear(&pv[i]);htb_scalar_clear(&qv[i]);for(int j=0;j<3;j++){htb_scalar_clear(&bp[i][j]);htb_scalar_clear(&bq[i][j]);}}htb_scalar_clear(&two);htb_scalar_clear(&five);htb_scalar_clear(&root5);htb_scalar_clear(&phi);htb_scalar_clear(&root2);htb_scalar_clear(&b);htb_scalar_clear(&b2);htb_scalar_clear(&b3);
    for(int i=0;i<3;i++){v_clear(&mirrors[i]);m_clear(&reflectors[i]);}m_clear(&q);m_clear(&qpow);m_clear(&qinv);m_clear(&temp);if(!ok){if(!error[0])snprintf(error,error_size,"failed to initialize exact {4,5} construction");return 0;}return 1;
}

static void clear_tiling(htb_tiling*t){q5_state*s=t->state;if(!s)return;for(int k=0;k<4;k++){m_clear(&s->side_reflection[k]);v_clear(&s->base_normal[k]);v_clear(&s->base_vertex[k]);for(int i=0;i<3;i++)htb_scalar_clear(&s->base_normal_scalar[k][i]);}v_clear(&s->base_center);htb_vec2_clear(&s->central_vertex_disk);for(int i=0;i<8;i++)htb_scalar_clear(&s->basis[i]);for(int i=0;i<3;i++)for(int j=0;j<3;j++){htb_scalar_clear(&s->h[i][j]);htb_scalar_clear(&s->hinv[i][j]);}for(size_t i=0;i<s->tile_count;i++){m_clear(&s->tiles[i].transform);v_clear(&s->tiles[i].center);}for(size_t i=0;i<s->boundary_count;i++)v_clear(&s->boundaries[i].normal);free(s->tiles);free(s->boundaries);free(s);t->state=NULL;}

static int reflect_hyper(htb_scalar x[3],htb_scalar n[3]){htb_arith_ctx*ctx=x[0].ctx;htb_scalar dot,t,two;htb_scalar_init(ctx,&dot);htb_scalar_init(ctx,&t);htb_scalar_init(ctx,&two);htb_scalar_set_si(&two,2);int ok=lorentz_dot(&dot,x,n)&&htb_scalar_mul(&dot,&dot,&two);for(int i=0;i<3&&ok;i++)ok=htb_scalar_mul(&t,&dot,&n[i])&&htb_scalar_sub(&x[i],&x[i],&t);htb_scalar_clear(&dot);htb_scalar_clear(&t);htb_scalar_clear(&two);return ok;}
static int locate_point(const htb_tiling*t,const htb_vec2*point,htb_point_location*location,char*error,size_t error_size){
    q5_state*s=t->state;
    htb_cmp central_u=htb_scalar_compare(&point->x,&s->central_vertex_disk.x),central_v=htb_scalar_compare(&point->y,&s->central_vertex_disk.y);
    if(central_u==HTB_CMP_EQUAL&&central_v==HTB_CMP_EQUAL){make_tile_id(s,&location->tile,0);for(int k=0;k<2;k++){htb_edge_ref edge;if(!make_edge(s,0,k,&edge)||!htb_edge_list_append(&location->incident_edges,&edge)){snprintf(error,error_size,"failed to record central vertex incidence");return 0;}}return 1;}
    htb_scalar world[3],local[3],dot;for(int i=0;i<3;i++){htb_scalar_init(t->arith,&world[i]);htb_scalar_init(t->arith,&local[i]);}htb_scalar_init(t->arith,&dot);q5m g,temp;m_init(&g);m_init(&temp);m_identity(&g);int ok=disk_to_hyper(point,world)&&sm_apply(local,s->hinv,world),finished=0;
    for(int step=0;ok&&step<4096&&!finished;step++){finished=1;for(int k=0;k<4;k++){ok=lorentz_dot(&dot,local,s->base_normal_scalar[k]);if(!ok)break;htb_cmp c=htb_scalar_compare_si(&dot,0);if(c==HTB_CMP_UNRESOLVED||c==HTB_CMP_ERROR){snprintf(error,error_size,"tile location side test is unresolved");ok=0;break;}if(c==HTB_CMP_GREATER){finished=0;ok=reflect_hyper(local,s->base_normal_scalar[k]);if(ok){m_mul(&temp,&g,&s->side_reflection[k]);m_set(&g,&temp);}break;}}}
    if(ok&&!finished){snprintf(error,error_size,"tile location exceeded 4096 reflection steps");ok=0;}size_t id=0;if(ok)ok=add_tile(s,&g,&id);if(ok)make_tile_id(s,&location->tile,id);
    if(ok)for(int k=0;k<4;k++){ok=lorentz_dot(&dot,local,s->base_normal_scalar[k]);if(!ok)break;htb_cmp c=htb_scalar_compare_si(&dot,0);if(c==HTB_CMP_EQUAL){htb_edge_ref edge;if(!make_edge(s,id,k,&edge)||!htb_edge_list_append(&location->incident_edges,&edge)){ok=0;break;}}else if(c==HTB_CMP_UNRESOLVED||c==HTB_CMP_ERROR){snprintf(error,error_size,"boundary incidence is unresolved");ok=0;break;}}
    if(!ok&&!error[0])snprintf(error,error_size,"failed to locate point in the {4,5} tiling");m_clear(&g);m_clear(&temp);for(int i=0;i<3;i++){htb_scalar_clear(&world[i]);htb_scalar_clear(&local[i]);}htb_scalar_clear(&dot);return ok;
}
static int enumerate_edges(const htb_tiling*t,const htb_tile_id*tile,htb_edge_list*edges,char*error,size_t error_size){q5_state*s=t->state;size_t id;if(!tile_index(s,tile,&id)){snprintf(error,error_size,"invalid {4,5} tile identity");return 0;}for(int k=0;k<4;k++){htb_edge_ref edge;if(!make_edge(s,id,k,&edge)||!htb_edge_list_append(edges,&edge)){snprintf(error,error_size,"failed to enumerate {4,5} edge");return 0;}}return 1;}
static int adjacent(const htb_tiling*t,const htb_tile_id*tile,const htb_edge_ref*edge,htb_adjacency*out,char*error,size_t error_size){q5_state*s=t->state;size_t id,neighbor,boundary;int reverse;if(!tile_index(s,tile,&id)||edge->local_index<0||edge->local_index>=4||!ensure_neighbor(s,id,edge->local_index,&neighbor,&reverse,&boundary)){snprintf(error,error_size,"failed to resolve {4,5} adjacency");return 0;}out->source=*tile;make_edge(s,id,edge->local_index,&out->source_edge);make_tile_id(s,&out->neighbor,neighbor);make_edge(s,neighbor,reverse,&out->neighbor_edge);return 1;}
static int normal_at(const htb_tiling*t,const htb_edge_ref*edge,const htb_vec2*p,htb_vec2*n,char*error,size_t error_size){q5_state*s=t->state;q5_boundary*b=boundary_for(s,edge);if(!b){snprintf(error,error_size,"unknown {4,5} boundary");return 0;}htb_scalar w[3],tmp;for(int i=0;i<3;i++)htb_scalar_init(t->arith,&w[i]);htb_scalar_init(t->arith,&tmp);int ok=world_normal(t,&b->normal,w)&&htb_scalar_mul(&tmp,&w[0],&p->x)&&htb_scalar_sub(&n->x,&tmp,&w[1])&&htb_scalar_mul(&tmp,&w[0],&p->y)&&htb_scalar_sub(&n->y,&tmp,&w[2]);for(int i=0;i<3;i++)htb_scalar_clear(&w[i]);htb_scalar_clear(&tmp);return ok;}

static htb_intersection_state validate_candidate(const htb_tiling*t,htb_intersection*c,const htb_vec2*p,const htb_vec2*v,size_t tile){
    htb_cmp time=htb_scalar_compare_si(&c->time,0);if(time==HTB_CMP_LESS||time==HTB_CMP_EQUAL)return HTB_INTERSECTION_INVALID;if(time==HTB_CMP_UNRESOLVED||time==HTB_CMP_ERROR)return HTB_INTERSECTION_UNRESOLVED;if(!htb_ray_point(&c->point,p,v,&c->time))return HTB_INTERSECTION_INVALID;htb_cmp inside;if(!htb_disk_contains(&c->point,&inside))return HTB_INTERSECTION_INVALID;if(inside==HTB_CMP_UNRESOLVED)return HTB_INTERSECTION_UNRESOLVED;if(inside!=HTB_CMP_LESS)return HTB_INTERSECTION_INVALID;
    htb_scalar n[3],f;for(int i=0;i<3;i++)htb_scalar_init(t->arith,&n[i]);htb_scalar_init(t->arith,&f);htb_intersection_state state=HTB_INTERSECTION_VALID;
    for(int k=0;k<4;k++){if(k==c->edge.local_index)continue;if(!tile_normal_world(t,tile,k,n)||!halfspace_disk(&f,&c->point,n)){state=HTB_INTERSECTION_INVALID;break;}htb_cmp side=htb_scalar_compare_si(&f,0);if(side==HTB_CMP_LESS){state=HTB_INTERSECTION_INVALID;break;}if((side==HTB_CMP_UNRESOLVED||side==HTB_CMP_ERROR)&&state==HTB_INTERSECTION_VALID)state=HTB_INTERSECTION_UNRESOLVED;}
    if(state==HTB_INTERSECTION_VALID){htb_vec2 normal;htb_vec2_init(t->arith,&normal);htb_scalar a,b,d;htb_scalar_init(t->arith,&a);htb_scalar_init(t->arith,&b);htb_scalar_init(t->arith,&d);if(!normal_at(t,&c->edge,&c->point,&normal,NULL,0)||!htb_scalar_mul(&a,&normal.x,&v->x)||!htb_scalar_mul(&b,&normal.y,&v->y)||!htb_scalar_add(&d,&a,&b))state=HTB_INTERSECTION_INVALID;else{htb_cmp tangent=htb_scalar_compare_si(&d,0);if(tangent==HTB_CMP_EQUAL)state=HTB_INTERSECTION_TANGENT;else if(tangent==HTB_CMP_UNRESOLVED||tangent==HTB_CMP_ERROR)state=HTB_INTERSECTION_UNRESOLVED;}htb_vec2_clear(&normal);htb_scalar_clear(&a);htb_scalar_clear(&b);htb_scalar_clear(&d);}
    for(int i=0;i<3;i++)htb_scalar_clear(&n[i]);htb_scalar_clear(&f);return state;
}
static int add_root(const htb_tiling*t,htb_intersection_list*list,const htb_scalar*root,const htb_edge_ref*edge,const htb_vec2*p,const htb_vec2*v,size_t tile){htb_intersection*c=htb_intersection_list_append(list);if(!c)return 0;c->edge=*edge;c->has_time=1;htb_scalar_set(&c->time,root);c->state=validate_candidate(t,c,p,v,tile);return 1;}
static int solve_edge(const htb_tiling*t,htb_intersection_list*list,const htb_vec2*p,const htb_vec2*v,size_t tile,const htb_edge_ref*edge,int known){
    q5_state*s=t->state;q5_boundary*boundary=boundary_for(s,edge);if(!boundary)return 0;htb_scalar n[3],A,B,C,disc,root,tmp,two,four,r2;for(int i=0;i<3;i++)htb_scalar_init(t->arith,&n[i]);htb_scalar*values[]={&A,&B,&C,&disc,&root,&tmp,&two,&four,&r2};for(size_t i=0;i<sizeof(values)/sizeof(values[0]);i++)htb_scalar_init(t->arith,values[i]);htb_scalar_set_si(&two,2);htb_scalar_set_si(&four,4);int ok=world_normal(t,&boundary->normal,n);
    if(ok)ok=htb_scalar_sqr(&A,&v->x)&&htb_scalar_sqr(&tmp,&v->y)&&htb_scalar_add(&A,&A,&tmp)&&htb_scalar_mul(&A,&A,&n[0]);
    if(ok)ok=htb_scalar_mul(&B,&p->x,&v->x)&&htb_scalar_mul(&tmp,&p->y,&v->y)&&htb_scalar_add(&B,&B,&tmp)&&htb_scalar_mul(&B,&B,&n[0])&&htb_scalar_mul(&tmp,&n[1],&v->x)&&htb_scalar_sub(&B,&B,&tmp)&&htb_scalar_mul(&tmp,&n[2],&v->y)&&htb_scalar_sub(&B,&B,&tmp)&&htb_scalar_mul(&B,&B,&two);
    if(ok)ok=htb_scalar_sqr(&r2,&p->x)&&htb_scalar_sqr(&tmp,&p->y)&&htb_scalar_add(&r2,&r2,&tmp);
    if(ok){htb_scalar one;htb_scalar_init(t->arith,&one);htb_scalar_set_si(&one,1);ok=htb_scalar_add(&r2,&r2,&one)&&htb_scalar_mul(&C,&n[0],&r2)&&htb_scalar_mul(&tmp,&n[1],&p->x)&&htb_scalar_mul(&tmp,&tmp,&two)&&htb_scalar_sub(&C,&C,&tmp)&&htb_scalar_mul(&tmp,&n[2],&p->y)&&htb_scalar_mul(&tmp,&tmp,&two)&&htb_scalar_sub(&C,&C,&tmp);htb_scalar_clear(&one);}
    htb_cmp az=ok?htb_scalar_compare_si(&A,0):HTB_CMP_ERROR;if(ok&&known){if(az==HTB_CMP_GREATER||az==HTB_CMP_LESS){htb_scalar_neg(&root,&B);ok=htb_scalar_div(&root,&root,&A)&&add_root(t,list,&root,edge,p,v,tile);}goto done;}
    if(ok&&az==HTB_CMP_EQUAL){htb_cmp bz=htb_scalar_compare_si(&B,0);if(bz==HTB_CMP_GREATER||bz==HTB_CMP_LESS){htb_scalar_neg(&root,&C);ok=htb_scalar_div(&root,&root,&B)&&add_root(t,list,&root,edge,p,v,tile);}goto done;}
    if(ok&&(az==HTB_CMP_UNRESOLVED||az==HTB_CMP_ERROR)){htb_intersection*c=htb_intersection_list_append(list);if(c){c->edge=*edge;c->state=HTB_INTERSECTION_UNRESOLVED;c->provider_flags=Q5_UNRESOLVED_LINEARITY;}else ok=0;goto done;}
    if(ok)ok=htb_scalar_sqr(&disc,&B)&&htb_scalar_mul(&tmp,&A,&C)&&htb_scalar_mul(&tmp,&tmp,&four)&&htb_scalar_sub(&disc,&disc,&tmp);if(ok){htb_cmp ds=htb_scalar_compare_si(&disc,0);if(ds==HTB_CMP_LESS)goto done;if(ds==HTB_CMP_UNRESOLVED||ds==HTB_CMP_ERROR){htb_intersection*c=htb_intersection_list_append(list);if(c){c->edge=*edge;c->state=HTB_INTERSECTION_UNRESOLVED;}else ok=0;goto done;}ok=htb_scalar_sqrt(&disc,&disc)&&htb_scalar_neg(&tmp,&B)&&htb_scalar_mul(&A,&A,&two)&&htb_scalar_sub(&root,&tmp,&disc)&&htb_scalar_div(&root,&root,&A)&&add_root(t,list,&root,edge,p,v,tile)&&htb_scalar_add(&root,&tmp,&disc)&&htb_scalar_div(&root,&root,&A)&&add_root(t,list,&root,edge,p,v,tile);}
done:for(int i=0;i<3;i++)htb_scalar_clear(&n[i]);for(size_t i=0;i<sizeof(values)/sizeof(values[0]);i++)htb_scalar_clear(values[i]);return ok;
}
static int intersections(const htb_tiling*t,const htb_vec2*p,const htb_vec2*v,const htb_tile_id*tile,const char*entry,htb_intersection_list*list,char*error,size_t error_size){
    q5_state*s=t->state;size_t id;if(!tile_index(s,tile,&id)){snprintf(error,error_size,"invalid {4,5} tile identity");return 0;}
    for(int k=0;k<4;k++){htb_edge_ref edge;if(!make_edge(s,id,k,&edge)||!solve_edge(t,list,p,v,id,&edge,entry&&strcmp(entry,edge.boundary_key)==0)){snprintf(error,error_size,"failed to solve a {4,5} geodesic edge");return 0;}}
    for(size_t i=0;i<list->count;i++)if(list->items[i].provider_flags==Q5_UNRESOLVED_LINEARITY){
        q5_boundary*b=boundary_for(s,&list->items[i].edge);if(!b)continue;htb_scalar n[3],start,finish;for(int z=0;z<3;z++)htb_scalar_init(t->arith,&n[z]);htb_scalar_init(t->arith,&start);htb_scalar_init(t->arith,&finish);
        if(world_normal(t,&b->normal,n)&&halfspace_disk(&start,p,n))for(size_t j=0;j<list->count;j++){htb_intersection*other=&list->items[j];if(i==j||!other->has_time||(other->state!=HTB_INTERSECTION_VALID&&other->state!=HTB_INTERSECTION_TANGENT))continue;if(!halfspace_disk(&finish,&other->point,n))continue;htb_cmp a=htb_scalar_compare_si(&start,0),c=htb_scalar_compare_si(&finish,0);if((a==HTB_CMP_LESS&&c==HTB_CMP_LESS)||(a==HTB_CMP_GREATER&&c==HTB_CMP_GREATER)){list->items[i].state=HTB_INTERSECTION_INVALID;break;}}
        for(int z=0;z<3;z++)htb_scalar_clear(&n[z]);htb_scalar_clear(&start);htb_scalar_clear(&finish);
    }
    return 1;
}

static int project_vertex(const htb_tiling*t,const q5v*x,htb_vec2*out){q5_state*s=t->state;htb_scalar local[3],world[3];for(int i=0;i<3;i++){htb_scalar_init(t->arith,&local[i]);htb_scalar_init(t->arith,&world[i]);}int ok=scalar_from_v(s,local,x)&&sm_apply(world,s->h,local)&&normalize_hyper(world)&&hyper_to_disk(world,out);for(int i=0;i<3;i++){htb_scalar_clear(&local[i]);htb_scalar_clear(&world[i]);}return ok;}
static int edge_vertices(const htb_tiling*t,const htb_tile_id*tile,const htb_edge_ref*edge,htb_vec2*da,htb_vec2*db,htb_vec2*ra,htb_vec2*rb,char*error,size_t error_size){q5_state*s=t->state;size_t id;if(!tile_index(s,tile,&id)||edge->local_index<0||edge->local_index>=4){snprintf(error,error_size,"invalid {4,5} edge");return 0;}q5v a,b;v_init(&a);v_init(&b);mv_mul(&a,&s->tiles[id].transform,&s->base_vertex[(edge->local_index+3)%4]);mv_mul(&b,&s->tiles[id].transform,&s->base_vertex[edge->local_index]);int ok=project_vertex(t,&a,da)&&project_vertex(t,&b,db)&&htb_vec2_set(ra,da)&&htb_vec2_set(rb,db);v_clear(&a);v_clear(&b);return ok;}
static int tile_measure(const htb_tiling*t,const htb_tile_id*tile,htb_measure_kind kind,htb_scalar*measure,char*error,size_t error_size){(void)t;(void)tile;(void)kind;(void)measure;snprintf(error,error_size,"Euclidean tile-area measurement is not yet implemented for order5-square@1");return 0;}
static int tile_center(const htb_tiling*t,const htb_tile_id*tile,htb_vec2*center,char*error,size_t error_size){q5_state*s=t->state;size_t id;if(!tile_index(s,tile,&id)){snprintf(error,error_size,"invalid {4,5} tile identity");return 0;}q5v value;v_init(&value);mv_mul(&value,&s->tiles[id].transform,&s->base_center);int ok=project_vertex(t,&value,center);v_clear(&value);return ok;}
static void geodesic_sample(double ax,double ay,double bx,double by,double t,double*x,double*y){
    double nr=bx-ax,ni=by-ay,dr=1-ax*bx-ay*by,di=ay*bx-ax*by,dd=dr*dr+di*di,wr=(nr*dr+ni*di)/dd,wi=(ni*dr-nr*di)/dd;
    double qx=t*wr,qy=t*wi,er=1+ax*qx+ay*qy,ei=ax*qy-ay*qx,ed=er*er+ei*ei;
    *x=((qx+ax)*er+(qy+ay)*ei)/ed;*y=((qy+ay)*er-(qx+ax)*ei)/ed;
}
static double sampled_tile_area(const htb_tiling*t,size_t id,int segments,int*ok){
    q5_state*s=t->state;double vx[4],vy[4];for(int k=0;k<4;k++){q5v v;v_init(&v);htb_vec2 p;htb_vec2_init(t->arith,&p);mv_mul(&v,&s->tiles[id].transform,&s->base_vertex[k]);if(!project_vertex(t,&v,&p)){*ok=0;htb_vec2_clear(&p);v_clear(&v);return 0;}vx[k]=htb_scalar_get_approx(&p.x);vy[k]=htb_scalar_get_approx(&p.y);htb_vec2_clear(&p);v_clear(&v);}
    double first_x=0,first_y=0,previous_x=0,previous_y=0,sum=0;int have=0;for(int edge=0;edge<4;edge++){int a=(edge+3)%4,b=edge;for(int j=0;j<=segments;j++){if(edge&&j==0)continue;double x,y;geodesic_sample(vx[a],vy[a],vx[b],vy[b],(double)j/segments,&x,&y);if(!have){first_x=previous_x=x;first_y=previous_y=y;have=1;}else{sum+=previous_x*y-x*previous_y;previous_x=x;previous_y=y;}}}sum+=previous_x*first_y-first_x*previous_y;return .5*fabs(sum);
}
static int disk_area_approx(const htb_tiling*t,const htb_tile_id*tile,double*area,double*estimated_error,char*error,size_t error_size){q5_state*s=t->state;size_t id;if(!tile_index(s,tile,&id)){snprintf(error,error_size,"invalid {4,5} tile identity");return 0;}int ok=1;double low=sampled_tile_area(t,id,48,&ok),high=sampled_tile_area(t,id,96,&ok);if(!ok||!isfinite(high)||high<=0){snprintf(error,error_size,"could not numerically evaluate disk area for tile %s",tile->label);return 0;}*area=high;*estimated_error=fabs(high-low);return 1;}

static const htb_parameter_spec parameters[]={
    {"centerU","Central vertex u",HTB_PARAM_SCALAR,"0","Disk u-coordinate of a distinguished order-5 vertex."},
    {"centerV","Central vertex v",HTB_PARAM_SCALAR,"0","Disk v-coordinate of a distinguished order-5 vertex."}
};
static const htb_tile_type_spec tile_types[]={{"square","Regular hyperbolic square",4}};
const htb_tiling_definition htb_order5_square_definition={
    .id="order5-square",.version="1",.display_name="Order-5 Square Tiling {4,5}",
    .description="Regular {4,5} Poincare-disk tessellation, with five squares at every vertex.",
    .reference_id=NULL,.reference_name=NULL,.tile_types=tile_types,.tile_type_count=1,.parameters=parameters,.parameter_count=2,
    .ops={.initialize=initialize,.clear=clear_tiling,.locate_point=locate_point,.enumerate_edges=enumerate_edges,
      .intersections=intersections,.normal_at=normal_at,.adjacent=adjacent,.edge_vertices=edge_vertices,
      .to_reference=NULL,.from_reference=NULL,.tile_measure=tile_measure,
      .tile_center=tile_center,.disk_area_approx=disk_area_approx}
};
