#ifndef HTB_TILING_H
#define HTB_TILING_H

#include "htb/geometry.h"
#include <stddef.h>
#include <stdint.h>

#define HTB_TILING_ID_MAX 64
#define HTB_TILING_VERSION_MAX 24
#define HTB_TILING_PARAM_MAX 24
#define HTB_TILING_KEY_MAX 512
#define HTB_TILING_LABEL_MAX 96
#define HTB_TILING_TYPE_MAX 48
#define HTB_TILING_OPAQUE_WORDS 8
#define HTB_TILING_COORDINATES_MAX 8

typedef enum {
    HTB_PARAM_SCALAR,
    HTB_PARAM_INTEGER,
    HTB_PARAM_CHOICE,
    HTB_PARAM_BOOLEAN
} htb_parameter_kind;

typedef struct {
    const char *id;
    const char *label;
    htb_parameter_kind kind;
    const char *default_value;
    const char *description;
} htb_parameter_spec;

typedef struct {
    const char *id;
    const char *display_name;
    size_t side_count;
} htb_tile_type_spec;

typedef struct {
    char key[HTB_TILING_ID_MAX];
    char value[HTB_TEXT];
} htb_tiling_parameter;

typedef struct {
    char id[HTB_TILING_ID_MAX];
    char version[HTB_TILING_VERSION_MAX];
    int parameter_count;
    htb_tiling_parameter parameters[HTB_TILING_PARAM_MAX];
} htb_tiling_config;

/* Tile identities are opaque to dynamics. The canonical key is stable for
   hashing, equality, JSON, exports, and future adjacency caches. Provider
   words are an optional compact representation interpreted only by the
   owning tiling provider. */
typedef struct {
    char key[HTB_TILING_KEY_MAX];
    char label[HTB_TILING_LABEL_MAX];
    char type[HTB_TILING_TYPE_MAX];
    uint64_t provider_words[HTB_TILING_OPAQUE_WORDS];
    size_t provider_word_count;
    int64_t coordinates[HTB_TILING_COORDINATES_MAX];
    size_t coordinate_count;
} htb_tile_id;

typedef struct {
    int local_index;
    int owner_normal_sign;
    char label[HTB_TILING_LABEL_MAX];
    char boundary_key[HTB_TILING_KEY_MAX];
} htb_edge_ref;

typedef struct {
    htb_edge_ref *items;
    size_t count;
    size_t capacity;
} htb_edge_list;

typedef struct {
    htb_tile_id source;
    htb_edge_ref source_edge;
    htb_tile_id neighbor;
    htb_edge_ref neighbor_edge;
} htb_adjacency;

typedef struct {
    htb_adjacency *items;
    size_t count;
    size_t capacity;
} htb_adjacency_list;

typedef enum {
    HTB_INTERSECTION_INVALID,
    HTB_INTERSECTION_VALID,
    HTB_INTERSECTION_UNRESOLVED,
    HTB_INTERSECTION_TANGENT
} htb_intersection_state;

typedef struct {
    htb_scalar time;
    htb_vec2 point;
    htb_edge_ref edge;
    htb_intersection_state state;
    int has_time;
    uint32_t provider_flags;
} htb_intersection;

typedef struct {
    htb_intersection *items;
    size_t count;
    size_t capacity;
    htb_arith_ctx *arith;
} htb_intersection_list;

typedef struct {
    htb_tile_id tile;
    htb_edge_list incident_edges;
} htb_point_location;

typedef enum {
    HTB_MEASURE_DISK_EUCLIDEAN_AREA,
    HTB_MEASURE_REFERENCE_EUCLIDEAN_AREA,
    HTB_MEASURE_DISK_EUCLIDEAN_PERIMETER
} htb_measure_kind;

typedef struct htb_tiling htb_tiling;
typedef struct htb_tiling_definition htb_tiling_definition;

typedef struct {
    int (*initialize)(htb_tiling *tiling, const htb_tiling_config *config,
                      char *error, size_t error_size);
    void (*clear)(htb_tiling *tiling);
    int (*locate_point)(const htb_tiling *tiling, const htb_vec2 *point,
                        htb_point_location *location, char *error, size_t error_size);
    int (*enumerate_edges)(const htb_tiling *tiling, const htb_tile_id *tile,
                           htb_edge_list *edges, char *error, size_t error_size);
    int (*intersections)(const htb_tiling *tiling, const htb_vec2 *point,
                         const htb_vec2 *direction, const htb_tile_id *tile,
                         const char *entry_boundary_key, htb_intersection_list *candidates,
                         char *error, size_t error_size);
    int (*normal_at)(const htb_tiling *tiling, const htb_edge_ref *edge,
                     const htb_vec2 *point, htb_vec2 *normal,
                     char *error, size_t error_size);
    int (*adjacent)(const htb_tiling *tiling, const htb_tile_id *tile,
                    const htb_edge_ref *edge, htb_adjacency *adjacency,
                    char *error, size_t error_size);
    int (*edge_vertices)(const htb_tiling *tiling, const htb_tile_id *tile,
                         const htb_edge_ref *edge, htb_vec2 *disk_a, htb_vec2 *disk_b,
                         htb_vec2 *reference_a, htb_vec2 *reference_b,
                         char *error, size_t error_size);
    int (*to_reference)(const htb_tiling *tiling, const htb_vec2 *disk,
                        htb_vec2 *reference, char *error, size_t error_size);
    int (*from_reference)(const htb_tiling *tiling, const htb_vec2 *reference,
                          htb_vec2 *disk, char *error, size_t error_size);
    int (*tile_measure)(const htb_tiling *tiling, const htb_tile_id *tile,
                        htb_measure_kind kind, htb_scalar *measure,
                        char *error, size_t error_size);
} htb_tiling_ops;

struct htb_tiling_definition {
    const char *id;
    const char *version;
    const char *display_name;
    const char *description;
    const char *reference_id;
    const char *reference_name;
    const htb_tile_type_spec *tile_types;
    size_t tile_type_count;
    const htb_parameter_spec *parameters;
    size_t parameter_count;
    htb_tiling_ops ops;
};

struct htb_tiling {
    const htb_tiling_definition *definition;
    htb_arith_ctx *arith;
    void *state;
};

void htb_tile_id_clear(htb_tile_id *tile);
int htb_tile_id_equal(const htb_tile_id *a, const htb_tile_id *b);
uint64_t htb_tile_id_hash(const htb_tile_id *tile);

void htb_edge_list_init(htb_edge_list *list);
void htb_edge_list_clear(htb_edge_list *list);
int htb_edge_list_append(htb_edge_list *list, const htb_edge_ref *edge);
void htb_adjacency_list_init(htb_adjacency_list *list);
void htb_adjacency_list_clear(htb_adjacency_list *list);
int htb_adjacency_list_append(htb_adjacency_list *list, const htb_adjacency *adjacency);
void htb_intersection_list_init(htb_intersection_list *list, htb_arith_ctx *arith);
void htb_intersection_list_clear(htb_intersection_list *list);
htb_intersection *htb_intersection_list_append(htb_intersection_list *list);
void htb_point_location_init(htb_point_location *location);
void htb_point_location_clear(htb_point_location *location);

const htb_tiling_definition *htb_tiling_definition_find(const char *id);
size_t htb_tiling_definition_count(void);
const htb_tiling_definition *htb_tiling_definition_at(size_t index);
int htb_tiling_config_normalize(htb_tiling_config *config, char *error, size_t error_size);
int htb_tiling_init(htb_tiling *tiling, htb_arith_ctx *arith,
                    const htb_tiling_config *config, char *error, size_t error_size);
void htb_tiling_clear(htb_tiling *tiling);
const char *htb_tiling_parameter_value(const htb_tiling_config *config,
                                       const char *key, const char *fallback);
int htb_tiling_neighbors(const htb_tiling *tiling, const htb_tile_id *tile,
                         htb_adjacency_list *neighbors, char *error, size_t error_size);

extern const htb_tiling_definition htb_elliptical_square_definition;
extern const htb_tiling_definition htb_order5_square_definition;

#endif
