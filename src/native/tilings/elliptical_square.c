#include "htb/tiling.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { ES_LEFT = 0, ES_RIGHT = 1, ES_BOTTOM = 2, ES_TOP = 3 };
enum { ES_UNRESOLVED_STRAIGHT_DIRECTION = 1 };

typedef struct { htb_vec2 center; } es_state;

static int64_t tile_i(const htb_tile_id *tile) { return (int64_t)tile->provider_words[0]; }
static int64_t tile_j(const htb_tile_id *tile) { return (int64_t)tile->provider_words[1]; }

static void make_tile(htb_tile_id *tile, int64_t i, int64_t j) {
    memset(tile, 0, sizeof(*tile));
    snprintf(tile->key, sizeof(tile->key), "elliptical-square:%lld,%lld", (long long)i, (long long)j);
    snprintf(tile->label, sizeof(tile->label), "(%lld, %lld)", (long long)i, (long long)j);
    snprintf(tile->type, sizeof(tile->type), "square");
    tile->provider_words[0] = (uint64_t)i; tile->provider_words[1] = (uint64_t)j; tile->provider_word_count = 2;
    tile->coordinates[0]=i;tile->coordinates[1]=j;tile->coordinate_count=2;
}

static const char *edge_label(int edge) {
    static const char *labels[] = {"left", "right", "bottom", "top"};
    return edge >= 0 && edge < 4 ? labels[edge] : "unknown";
}
static int opposite_edge(int edge) {
    static const int opposite[] = {ES_RIGHT, ES_LEFT, ES_TOP, ES_BOTTOM};
    return edge >= 0 && edge < 4 ? opposite[edge] : -1;
}
static void make_edge(const htb_tile_id *tile, int edge, htb_edge_ref *out) {
    int64_t i=tile_i(tile),j=tile_j(tile),line;
    memset(out,0,sizeof(*out));out->local_index=edge;out->owner_normal_sign=(edge==ES_LEFT||edge==ES_BOTTOM)?1:-1;snprintf(out->label,sizeof(out->label),"%s",edge_label(edge));
    if(edge==ES_LEFT||edge==ES_RIGHT){line=edge==ES_LEFT?i:i+1;snprintf(out->boundary_key,sizeof(out->boundary_key),"elliptical-square:v:%lld",(long long)line);}
    else{line=edge==ES_BOTTOM?j:j+1;snprintf(out->boundary_key,sizeof(out->boundary_key),"elliptical-square:h:%lld",(long long)line);}
}

static int forward_map(const htb_tiling *tiling, const htb_vec2 *plane, htb_vec2 *disk) {
    const es_state*s=tiling->state;htb_arith_ctx*ctx=tiling->arith;htb_scalar dx,dy,den,t,one;
    htb_scalar_init(ctx,&dx);htb_scalar_init(ctx,&dy);htb_scalar_init(ctx,&den);htb_scalar_init(ctx,&t);htb_scalar_init(ctx,&one);htb_scalar_set_si(&one,1);
    int ok=htb_scalar_sub(&dx,&plane->x,&s->center.x)&&htb_scalar_sub(&dy,&plane->y,&s->center.y)&&
      htb_scalar_sqr(&den,&dx)&&htb_scalar_sqr(&t,&dy)&&htb_scalar_add(&den,&den,&t)&&htb_scalar_add(&den,&den,&one)&&
      htb_scalar_sqrt(&den,&den)&&htb_scalar_div(&disk->x,&dx,&den)&&htb_scalar_div(&disk->y,&dy,&den);
    htb_scalar_clear(&dx);htb_scalar_clear(&dy);htb_scalar_clear(&den);htb_scalar_clear(&t);htb_scalar_clear(&one);return ok;
}

static int inverse_map(const htb_tiling *tiling, const htb_vec2 *disk, htb_vec2 *plane) {
    const es_state*s=tiling->state;htb_arith_ctx*ctx=tiling->arith;htb_scalar r2,den,one,t,tmp;
    htb_scalar_init(ctx,&r2);htb_scalar_init(ctx,&den);htb_scalar_init(ctx,&one);htb_scalar_init(ctx,&t);htb_scalar_init(ctx,&tmp);htb_scalar_set_si(&one,1);
    int ok=htb_scalar_sqr(&r2,&disk->x)&&htb_scalar_sqr(&tmp,&disk->y)&&htb_scalar_add(&r2,&r2,&tmp)&&htb_scalar_sub(&den,&one,&r2);
    if(ok&&htb_scalar_compare_si(&den,0)!=HTB_CMP_GREATER)ok=0;
    if(ok)ok=htb_scalar_sqrt(&den,&den)&&htb_scalar_div(&t,&disk->x,&den)&&htb_scalar_add(&plane->x,&s->center.x,&t)&&
      htb_scalar_div(&t,&disk->y,&den)&&htb_scalar_add(&plane->y,&s->center.y,&t);
    htb_scalar_clear(&r2);htb_scalar_clear(&den);htb_scalar_clear(&one);htb_scalar_clear(&t);htb_scalar_clear(&tmp);return ok;
}

static int initialize(htb_tiling *tiling,const htb_tiling_config*config,char*error,size_t error_size){
    es_state*s=calloc(1,sizeof(*s));if(!s){snprintf(error,error_size,"failed to allocate elliptical tiling state");return 0;}
    tiling->state=s;htb_vec2_init(tiling->arith,&s->center);
    const char*x=htb_tiling_parameter_value(config,"centerX","0"),*y=htb_tiling_parameter_value(config,"centerY","0");
    if(!htb_scalar_parse_rational(&s->center.x,x,error,error_size)||!htb_scalar_parse_rational(&s->center.y,y,error,error_size)){
        htb_vec2_clear(&s->center);free(s);tiling->state=NULL;return 0;
    }return 1;
}
static void clear_tiling(htb_tiling*tiling){if(tiling->state){es_state*s=tiling->state;htb_vec2_clear(&s->center);free(s);tiling->state=NULL;}}

static int coordinate_index(const htb_scalar*x,int64_t*index,int*on,char*error,size_t error_size){
    double a=htb_scalar_get_approx(x);if(!isfinite(a)||a<(double)INT64_MIN+2||a>(double)INT64_MAX-2){snprintf(error,error_size,"inverse coordinate is outside supported tile-index range");return 0;}
    int64_t k=(int64_t)floor(a);htb_cmp lo=htb_scalar_compare_si(x,k),hi=htb_scalar_compare_si(x,k+1);
    if(lo==HTB_CMP_UNRESOLVED||hi==HTB_CMP_UNRESOLVED){snprintf(error,error_size,"tile index could not be certified at current precision");return 0;}
    if(lo==HTB_CMP_EQUAL){*index=k;*on=1;return 1;}if(lo==HTB_CMP_GREATER&&hi==HTB_CMP_LESS){*index=k;*on=0;return 1;}
    for(int delta=-1;delta<=1;delta++){int64_t q=k+delta;lo=htb_scalar_compare_si(x,q);hi=htb_scalar_compare_si(x,q+1);
      if(lo==HTB_CMP_EQUAL){*index=q;*on=1;return 1;}if(lo==HTB_CMP_GREATER&&hi==HTB_CMP_LESS){*index=q;*on=0;return 1;}}
    snprintf(error,error_size,"inverse coordinate is not certifiably contained in a grid interval");return 0;
}

static int locate_point(const htb_tiling*tiling,const htb_vec2*point,htb_point_location*location,char*error,size_t error_size){
    htb_vec2 plane;htb_vec2_init(tiling->arith,&plane);int64_t i=0,j=0;int on_v=0,on_h=0;
    int ok=inverse_map(tiling,point,&plane);if(!ok)snprintf(error,error_size,"point is not certified strictly inside the open disk");
    if(ok)ok=coordinate_index(&plane.x,&i,&on_v,error,error_size);if(ok)ok=coordinate_index(&plane.y,&j,&on_h,error,error_size);
    if(ok){make_tile(&location->tile,i,j);htb_edge_ref edge;if(on_v){make_edge(&location->tile,ES_LEFT,&edge);ok=htb_edge_list_append(&location->incident_edges,&edge);}if(ok&&on_h){make_edge(&location->tile,ES_BOTTOM,&edge);ok=htb_edge_list_append(&location->incident_edges,&edge);}}
    htb_vec2_clear(&plane);return ok;
}

static int enumerate_edges(const htb_tiling*tiling,const htb_tile_id*tile,htb_edge_list*edges,char*error,size_t error_size){
    (void)tiling;(void)error;(void)error_size;for(int e=0;e<4;e++){htb_edge_ref edge;make_edge(tile,e,&edge);if(!htb_edge_list_append(edges,&edge))return 0;}return 1;
}

static int normal_at(const htb_tiling*tiling,const htb_edge_ref*edge,const htb_vec2*p,htb_vec2*n,char*error,size_t error_size){
    (void)error;(void)error_size;htb_scalar one,t;htb_scalar_init(tiling->arith,&one);htb_scalar_init(tiling->arith,&t);htb_scalar_set_si(&one,1);
    int vertical=edge->local_index==ES_LEFT||edge->local_index==ES_RIGHT;
    int ok=htb_scalar_sqr(&t,vertical?&p->y:&p->x)&&htb_scalar_sub(vertical?&n->x:&n->y,&one,&t)&&htb_scalar_mul(vertical?&n->y:&n->x,&p->x,&p->y);
    htb_scalar_clear(&one);htb_scalar_clear(&t);return ok;
}

static int adjacent(const htb_tiling*tiling,const htb_tile_id*tile,const htb_edge_ref*edge,htb_adjacency*a,char*error,size_t error_size){
    (void)tiling;(void)error;(void)error_size;int64_t i=tile_i(tile),j=tile_j(tile);a->source=*tile;a->source_edge=*edge;
    if(edge->local_index==ES_LEFT)i--;else if(edge->local_index==ES_RIGHT)i++;else if(edge->local_index==ES_BOTTOM)j--;else if(edge->local_index==ES_TOP)j++;else return 0;
    make_tile(&a->neighbor,i,j);make_edge(&a->neighbor,opposite_edge(edge->local_index),&a->neighbor_edge);return 1;
}

static htb_cmp branch_match(const htb_vec2*point,const htb_scalar*offset,int vertical){
    htb_cmp os=htb_scalar_compare_si(offset,0),ps=htb_scalar_compare_si(vertical?&point->x:&point->y,0);
    if(os==HTB_CMP_ERROR||ps==HTB_CMP_ERROR)return HTB_CMP_ERROR;if(os==HTB_CMP_UNRESOLVED||ps==HTB_CMP_UNRESOLVED)return HTB_CMP_UNRESOLVED;
    if(os==HTB_CMP_EQUAL)return ps==HTB_CMP_EQUAL?HTB_CMP_GREATER:HTB_CMP_LESS;return os==ps?HTB_CMP_GREATER:HTB_CMP_LESS;
}

static htb_intersection_state validate_candidate(const htb_tiling*tiling,htb_intersection*c,const htb_vec2*p,const htb_vec2*v,
                                                  const htb_tile_id*tile,const htb_scalar*offset){
    htb_cmp cmp=htb_scalar_compare_si(&c->time,0);if(cmp==HTB_CMP_LESS||cmp==HTB_CMP_EQUAL)return HTB_INTERSECTION_INVALID;
    if(cmp==HTB_CMP_UNRESOLVED||cmp==HTB_CMP_ERROR)return HTB_INTERSECTION_UNRESOLVED;if(!htb_ray_point(&c->point,p,v,&c->time))return HTB_INTERSECTION_INVALID;
    int vertical=c->edge.local_index==ES_LEFT||c->edge.local_index==ES_RIGHT;if(htb_scalar_compare_si(offset,0)==HTB_CMP_EQUAL)htb_scalar_set_si(vertical?&c->point.x:&c->point.y,0);
    htb_cmp inside;if(!htb_disk_contains(&c->point,&inside))return HTB_INTERSECTION_INVALID;if(inside==HTB_CMP_UNRESOLVED)return HTB_INTERSECTION_UNRESOLVED;if(inside!=HTB_CMP_LESS)return HTB_INTERSECTION_INVALID;
    htb_cmp branch=branch_match(&c->point,offset,vertical);if(branch==HTB_CMP_UNRESOLVED)return HTB_INTERSECTION_UNRESOLVED;if(branch!=HTB_CMP_GREATER)return HTB_INTERSECTION_INVALID;
    htb_vec2 plane;htb_vec2_init(tiling->arith,&plane);if(!inverse_map(tiling,&c->point,&plane)){htb_vec2_clear(&plane);return HTB_INTERSECTION_UNRESOLVED;}
    const htb_scalar*along=vertical?&plane.y:&plane.x;int64_t lo=vertical?tile_j(tile):tile_i(tile);htb_cmp clo=htb_scalar_compare_si(along,lo),chi=htb_scalar_compare_si(along,lo+1);htb_vec2_clear(&plane);
    if(clo==HTB_CMP_UNRESOLVED||chi==HTB_CMP_UNRESOLVED)return HTB_INTERSECTION_UNRESOLVED;if(clo==HTB_CMP_LESS||chi==HTB_CMP_GREATER)return HTB_INTERSECTION_INVALID;
    htb_vec2 n;htb_vec2_init(tiling->arith,&n);htb_scalar a,b,dot;htb_scalar_init(tiling->arith,&a);htb_scalar_init(tiling->arith,&b);htb_scalar_init(tiling->arith,&dot);
    int ok=normal_at(tiling,&c->edge,&c->point,&n,NULL,0)&&htb_scalar_mul(&a,&n.x,&v->x)&&htb_scalar_mul(&b,&n.y,&v->y)&&htb_scalar_add(&dot,&a,&b);
    htb_cmp tangent=ok?htb_scalar_compare_si(&dot,0):HTB_CMP_ERROR;htb_vec2_clear(&n);htb_scalar_clear(&a);htb_scalar_clear(&b);htb_scalar_clear(&dot);
    if(tangent==HTB_CMP_EQUAL)return HTB_INTERSECTION_TANGENT;if(tangent==HTB_CMP_UNRESOLVED||tangent==HTB_CMP_ERROR)return HTB_INTERSECTION_UNRESOLVED;return HTB_INTERSECTION_VALID;
}

static int add_root(const htb_tiling*tiling,htb_intersection_list*list,const htb_scalar*t,const htb_edge_ref*edge,
                    const htb_vec2*p,const htb_vec2*v,const htb_tile_id*tile,const htb_scalar*offset){
    htb_intersection*c=htb_intersection_list_append(list);if(!c)return 0;c->edge=*edge;c->has_time=1;htb_scalar_set(&c->time,t);c->state=validate_candidate(tiling,c,p,v,tile,offset);return 1;
}

static int solve_edge(const htb_tiling*tiling,htb_intersection_list*list,const htb_vec2*p,const htb_vec2*v,const htb_tile_id*tile,
                      const htb_edge_ref*edge,int known_current){
    int vertical=edge->local_index==ES_LEFT||edge->local_index==ES_RIGHT;int64_t grid=vertical?(edge->local_index==ES_LEFT?tile_i(tile):tile_i(tile)+1):(edge->local_index==ES_BOTTOM?tile_j(tile):tile_j(tile)+1);
    htb_arith_ctx*ctx=tiling->arith;const es_state*s=tiling->state;htb_scalar k,offset,a2,one,onepa2,A,B,C,t1,t2,tmp,q,disc,two,four;
    htb_scalar*all[]={&k,&offset,&a2,&one,&onepa2,&A,&B,&C,&t1,&t2,&tmp,&q,&disc,&two,&four};for(size_t z=0;z<sizeof(all)/sizeof(all[0]);z++)htb_scalar_init(ctx,all[z]);
    htb_scalar_set_si(&k,grid);htb_scalar_set_si(&one,1);htb_scalar_set_si(&two,2);htb_scalar_set_si(&four,4);htb_scalar_sub(&offset,&k,vertical?&s->center.x:&s->center.y);htb_cmp offset_zero=htb_scalar_compare_si(&offset,0);int ok=1;
    if(offset_zero==HTB_CMP_UNRESOLVED){htb_intersection*c=htb_intersection_list_append(list);if(!c){ok=0;goto done;}c->edge=*edge;c->state=HTB_INTERSECTION_UNRESOLVED;goto done;}
    if(offset_zero==HTB_CMP_EQUAL){if(known_current)goto done;const htb_scalar*coord=vertical?&p->x:&p->y,*dir=vertical?&v->x:&v->y;htb_cmp dz=htb_scalar_compare_si(dir,0);if(dz==HTB_CMP_EQUAL)goto done;
      if(dz==HTB_CMP_UNRESOLVED){htb_intersection*c=htb_intersection_list_append(list);if(!c){ok=0;goto done;}c->edge=*edge;c->state=HTB_INTERSECTION_UNRESOLVED;c->provider_flags=ES_UNRESOLVED_STRAIGHT_DIRECTION;goto done;}
      htb_scalar_neg(&tmp,coord);htb_scalar_div(&t1,&tmp,dir);ok=add_root(tiling,list,&t1,edge,p,v,tile,&offset);goto done;}
    htb_scalar_mul(&a2,&offset,&offset);htb_scalar_add(&onepa2,&one,&a2);const htb_scalar*pc=vertical?&p->x:&p->y,*po=vertical?&p->y:&p->x,*vc=vertical?&v->x:&v->y,*vo=vertical?&v->y:&v->x;
    htb_scalar_sqr(&A,vc);htb_scalar_mul(&A,&A,&onepa2);htb_scalar_sqr(&tmp,vo);htb_scalar_mul(&tmp,&tmp,&a2);htb_scalar_add(&A,&A,&tmp);
    htb_scalar_mul(&B,pc,vc);htb_scalar_mul(&B,&B,&onepa2);htb_scalar_mul(&tmp,po,vo);htb_scalar_mul(&tmp,&tmp,&a2);htb_scalar_add(&B,&B,&tmp);htb_scalar_mul(&B,&B,&two);
    htb_scalar_sqr(&C,pc);htb_scalar_mul(&C,&C,&onepa2);htb_scalar_sqr(&tmp,po);htb_scalar_mul(&tmp,&tmp,&a2);htb_scalar_add(&C,&C,&tmp);htb_scalar_sub(&C,&C,&a2);
    htb_cmp az=htb_scalar_compare_si(&A,0);if(az==HTB_CMP_UNRESOLVED){htb_intersection*c=htb_intersection_list_append(list);if(!c){ok=0;goto done;}c->edge=*edge;c->state=HTB_INTERSECTION_UNRESOLVED;goto done;}
    if(az==HTB_CMP_EQUAL){htb_cmp bz=htb_scalar_compare_si(&B,0);if(bz==HTB_CMP_EQUAL)goto done;if(bz==HTB_CMP_UNRESOLVED){htb_intersection*c=htb_intersection_list_append(list);if(!c){ok=0;goto done;}c->edge=*edge;c->state=HTB_INTERSECTION_UNRESOLVED;goto done;}htb_scalar_neg(&tmp,&C);htb_scalar_div(&t1,&tmp,&B);ok=add_root(tiling,list,&t1,edge,p,v,tile,&offset);goto done;}
    if(known_current){htb_scalar_neg(&tmp,&B);htb_scalar_div(&t1,&tmp,&A);ok=add_root(tiling,list,&t1,edge,p,v,tile,&offset);goto done;}
    htb_scalar_sqr(&disc,&B);htb_scalar_mul(&tmp,&four,&A);htb_scalar_mul(&tmp,&tmp,&C);htb_scalar_sub(&disc,&disc,&tmp);htb_cmp ds=htb_scalar_compare_si(&disc,0);if(ds==HTB_CMP_LESS)goto done;
    if(ds==HTB_CMP_UNRESOLVED){htb_intersection*c=htb_intersection_list_append(list);if(!c){ok=0;goto done;}c->edge=*edge;c->state=HTB_INTERSECTION_UNRESOLVED;goto done;}if(!htb_scalar_sqrt(&q,&disc)){ok=0;goto done;}
    htb_scalar_neg(&tmp,&B);htb_scalar_mul(&A,&A,&two);htb_scalar_sub(&t1,&tmp,&q);htb_scalar_div(&t1,&t1,&A);htb_scalar_add(&t2,&tmp,&q);htb_scalar_div(&t2,&t2,&A);ok=add_root(tiling,list,&t1,edge,p,v,tile,&offset)&&add_root(tiling,list,&t2,edge,p,v,tile,&offset);
done:for(size_t z=0;z<sizeof(all)/sizeof(all[0]);z++)htb_scalar_clear(all[z]);return ok;
}

static int intersections(const htb_tiling*tiling,const htb_vec2*p,const htb_vec2*v,const htb_tile_id*tile,const char*entry_boundary,
                         htb_intersection_list*list,char*error,size_t error_size){
    (void)error;(void)error_size;htb_edge_list edges;htb_edge_list_init(&edges);if(!enumerate_edges(tiling,tile,&edges,error,error_size))return 0;
    for(size_t i=0;i<edges.count;i++)if(!solve_edge(tiling,list,p,v,tile,&edges.items[i],entry_boundary&&strcmp(entry_boundary,edges.items[i].boundary_key)==0)){htb_edge_list_clear(&edges);return 0;}
    htb_edge_list_clear(&edges);
    for(size_t i=0;i<list->count;i++){
      if(list->items[i].provider_flags!=ES_UNRESOLVED_STRAIGHT_DIRECTION)continue;
      for(size_t j=0;j<list->count;j++){
        htb_intersection*other=&list->items[j];
        if(i==j||!other->has_time||(other->state!=HTB_INTERSECTION_VALID&&other->state!=HTB_INTERSECTION_TANGENT))continue;
        htb_vec2 endpoint;htb_vec2_init(tiling->arith,&endpoint);
        if(!htb_ray_point(&endpoint,p,v,&other->time)){htb_vec2_clear(&endpoint);continue;}
        int vertical=list->items[i].edge.local_index==ES_LEFT||list->items[i].edge.local_index==ES_RIGHT;
        const htb_scalar*start=vertical?&p->x:&p->y,*finish=vertical?&endpoint.x:&endpoint.y;
        htb_cmp a=htb_scalar_compare_si(start,0),b=htb_scalar_compare_si(finish,0);
        htb_vec2_clear(&endpoint);
        if((a==HTB_CMP_LESS&&b==HTB_CMP_LESS)||(a==HTB_CMP_GREATER&&b==HTB_CMP_GREATER)){
          list->items[i].state=HTB_INTERSECTION_INVALID;break;
        }
      }
    }
    return 1;
}

static int edge_vertices(const htb_tiling*tiling,const htb_tile_id*tile,const htb_edge_ref*edge,htb_vec2*da,htb_vec2*db,htb_vec2*ra,htb_vec2*rb,char*error,size_t error_size){
    (void)error;(void)error_size;int64_t i=tile_i(tile),j=tile_j(tile);if(edge->local_index==ES_LEFT||edge->local_index==ES_RIGHT){int64_t x=edge->local_index==ES_LEFT?i:i+1;htb_scalar_set_si(&ra->x,x);htb_scalar_set_si(&rb->x,x);htb_scalar_set_si(&ra->y,j);htb_scalar_set_si(&rb->y,j+1);}else{int64_t y=edge->local_index==ES_BOTTOM?j:j+1;htb_scalar_set_si(&ra->y,y);htb_scalar_set_si(&rb->y,y);htb_scalar_set_si(&ra->x,i);htb_scalar_set_si(&rb->x,i+1);}return forward_map(tiling,ra,da)&&forward_map(tiling,rb,db);
}
static int to_reference(const htb_tiling*t,const htb_vec2*d,htb_vec2*r,char*e,size_t n){if(!inverse_map(t,d,r)){snprintf(e,n,"point is not inside the disk");return 0;}return 1;}
static int from_reference(const htb_tiling*t,const htb_vec2*r,htb_vec2*d,char*e,size_t n){(void)e;(void)n;return forward_map(t,r,d);}
static int tile_measure(const htb_tiling*t,const htb_tile_id*tile,htb_measure_kind kind,htb_scalar*measure,char*error,size_t error_size){
    (void)t;(void)tile;if(kind==HTB_MEASURE_REFERENCE_EUCLIDEAN_AREA)return htb_scalar_set_si(measure,1);snprintf(error,error_size,"requested tile measure is not implemented by elliptical-square@1");return 0;
}
static int tile_center(const htb_tiling*t,const htb_tile_id*tile,htb_vec2*center,char*error,size_t error_size){
    (void)error;(void)error_size;htb_vec2 plane;htb_vec2_init(t->arith,&plane);htb_scalar half;htb_scalar_init(t->arith,&half);char parse_error[32]={0};int ok=htb_scalar_parse_rational(&half,"1/2",parse_error,sizeof(parse_error))&&htb_scalar_set_si(&plane.x,tile_i(tile))&&htb_scalar_add(&plane.x,&plane.x,&half)&&htb_scalar_set_si(&plane.y,tile_j(tile))&&htb_scalar_add(&plane.y,&plane.y,&half)&&forward_map(t,&plane,center);htb_scalar_clear(&half);htb_vec2_clear(&plane);return ok;
}
static double area_gauss(const es_state*s,const htb_tile_id*tile,const double*nodes,const double*weights,int count,int subdivisions){
    double cx=htb_scalar_get_approx(&s->center.x),cy=htb_scalar_get_approx(&s->center.y),half=.5/subdivisions,total=0;
    for(int sx=0;sx<subdivisions;sx++)for(int sy=0;sy<subdivisions;sy++){double mx=(double)tile_i(tile)+(sx+.5)/subdivisions,my=(double)tile_j(tile)+(sy+.5)/subdivisions;for(int i=0;i<count;i++)for(int j=0;j<count;j++){double x=mx+half*nodes[i]-cx,y=my+half*nodes[j]-cy,d=1+x*x+y*y;total+=weights[i]*weights[j]/(d*d);}}return half*half*total;
}
static int disk_area_approx(const htb_tiling*t,const htb_tile_id*tile,double*area,double*estimated_error,char*error,size_t error_size){
    static const double n4[]={-.8611363115940526,-.3399810435848563,.3399810435848563,.8611363115940526};
    static const double w4[]={.3478548451374539,.6521451548625461,.6521451548625461,.3478548451374539};
    const es_state*s=t->state;double hi=area_gauss(s,tile,n4,w4,4,4),lo=area_gauss(s,tile,n4,w4,4,2);if(!isfinite(hi)||hi<=0){snprintf(error,error_size,"could not numerically evaluate disk area for tile %s",tile->label);return 0;}*area=hi;*estimated_error=fabs(hi-lo);return 1;
}

static const htb_parameter_spec parameters[]={
    {"centerX","Tiling center c_x",HTB_PARAM_SCALAR,"0","Horizontal placement of the source square grid."},
    {"centerY","Tiling center c_y",HTB_PARAM_SCALAR,"0","Vertical placement of the source square grid."}
};
static const htb_tile_type_spec tile_types[]={{"square","Square",4}};

const htb_tiling_definition htb_elliptical_square_definition={
    .id="elliptical-square",.version="1",.display_name="Elliptically Distorted Square Grid",
    .description="Image of the Euclidean square grid under the radial map to the open unit disk.",
    .reference_id="square-grid",.reference_name="Euclidean Square Grid",.tile_types=tile_types,.tile_type_count=1,.parameters=parameters,.parameter_count=2,
    .ops={.initialize=initialize,.clear=clear_tiling,.locate_point=locate_point,.enumerate_edges=enumerate_edges,
      .intersections=intersections,.normal_at=normal_at,.adjacent=adjacent,.edge_vertices=edge_vertices,
      .to_reference=to_reference,.from_reference=from_reference,.tile_measure=tile_measure,
      .tile_center=tile_center,.disk_area_approx=disk_area_approx}
};
