#include "htb/tiling.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const htb_tiling_definition *definitions[] = {
    &htb_elliptical_square_definition,
    &htb_order5_square_definition
};

void htb_tile_id_clear(htb_tile_id *tile) { memset(tile, 0, sizeof(*tile)); }
int htb_tile_id_equal(const htb_tile_id *a, const htb_tile_id *b) { return strcmp(a->key, b->key) == 0; }
uint64_t htb_tile_id_hash(const htb_tile_id *tile) {
    uint64_t hash = UINT64_C(1469598103934665603);
    for (const unsigned char *p = (const unsigned char *)tile->key; *p; p++) {
        hash ^= *p; hash *= UINT64_C(1099511628211);
    }
    return hash;
}

void htb_edge_list_init(htb_edge_list *list) { memset(list, 0, sizeof(*list)); }
void htb_edge_list_clear(htb_edge_list *list) { free(list->items); memset(list, 0, sizeof(*list)); }
int htb_edge_list_append(htb_edge_list *list, const htb_edge_ref *edge) {
    if (list->count == list->capacity) {
        size_t next = list->capacity ? list->capacity * 2 : 4;
        void *items = realloc(list->items, next * sizeof(*list->items));
        if (!items) return 0; list->items = items; list->capacity = next;
    }
    list->items[list->count++] = *edge; return 1;
}

void htb_adjacency_list_init(htb_adjacency_list *list) { memset(list, 0, sizeof(*list)); }
void htb_adjacency_list_clear(htb_adjacency_list *list) { free(list->items); memset(list, 0, sizeof(*list)); }
int htb_adjacency_list_append(htb_adjacency_list *list, const htb_adjacency *adjacency) {
    if (list->count == list->capacity) {
        size_t next = list->capacity ? list->capacity * 2 : 4;
        void *items = realloc(list->items, next * sizeof(*list->items));
        if (!items) return 0; list->items = items; list->capacity = next;
    }
    list->items[list->count++] = *adjacency; return 1;
}

void htb_intersection_list_init(htb_intersection_list *list, htb_arith_ctx *arith) {
    memset(list, 0, sizeof(*list)); list->arith = arith;
}
void htb_intersection_list_clear(htb_intersection_list *list) {
    for (size_t i = 0; i < list->count; i++) {
        htb_scalar_clear(&list->items[i].time); htb_vec2_clear(&list->items[i].point);
    }
    free(list->items); memset(list, 0, sizeof(*list));
}
htb_intersection *htb_intersection_list_append(htb_intersection_list *list) {
    if (list->count == list->capacity) {
        size_t next = list->capacity ? list->capacity * 2 : 8;
        void *items = realloc(list->items, next * sizeof(*list->items));
        if (!items) return NULL; list->items = items; list->capacity = next;
    }
    htb_intersection *item = &list->items[list->count++]; memset(item, 0, sizeof(*item));
    htb_scalar_init(list->arith, &item->time); htb_vec2_init(list->arith, &item->point);
    item->state = HTB_INTERSECTION_INVALID; return item;
}

void htb_point_location_init(htb_point_location *location) {
    memset(location, 0, sizeof(*location)); htb_edge_list_init(&location->incident_edges);
}
void htb_point_location_clear(htb_point_location *location) {
    htb_edge_list_clear(&location->incident_edges); memset(location, 0, sizeof(*location));
}

size_t htb_tiling_definition_count(void) { return sizeof(definitions) / sizeof(definitions[0]); }
const htb_tiling_definition *htb_tiling_definition_at(size_t index) {
    return index < htb_tiling_definition_count() ? definitions[index] : NULL;
}
const htb_tiling_definition *htb_tiling_definition_find(const char *id) {
    if (!id || !*id) id = "elliptical-square";
    for (size_t i = 0; i < htb_tiling_definition_count(); i++)
        if (strcmp(definitions[i]->id, id) == 0) return definitions[i];
    return NULL;
}

int htb_tiling_config_normalize(htb_tiling_config *config, char *error, size_t error_size) {
    const htb_tiling_definition *definition=htb_tiling_definition_find(config?config->id:NULL);
    if(!config||!definition){if(error_size)snprintf(error,error_size,"unknown tiling definition: %s",config?config->id:"");return 0;}
    if(!config->id[0])snprintf(config->id,sizeof(config->id),"%s",definition->id);
    if(!config->version[0])snprintf(config->version,sizeof(config->version),"%s",definition->version);
    if(strcmp(config->version,definition->version)!=0){if(error_size)snprintf(error,error_size,"unsupported version %s for tiling %s",config->version,definition->id);return 0;}
    for(int i=0;i<config->parameter_count;i++){
        int known=0;for(size_t j=0;j<definition->parameter_count;j++)if(strcmp(config->parameters[i].key,definition->parameters[j].id)==0){known=1;break;}
        if(!known){if(error_size)snprintf(error,error_size,"unknown parameter %s for tiling %s",config->parameters[i].key,definition->id);return 0;}
        for(int j=0;j<i;j++)if(strcmp(config->parameters[i].key,config->parameters[j].key)==0){if(error_size)snprintf(error,error_size,"duplicate tiling parameter: %s",config->parameters[i].key);return 0;}
    }
    for(size_t i=0;i<definition->parameter_count;i++){
        int found=0;for(int j=0;j<config->parameter_count;j++)if(strcmp(config->parameters[j].key,definition->parameters[i].id)==0){found=1;break;}
        if(!found){if(config->parameter_count>=HTB_TILING_PARAM_MAX){if(error_size)snprintf(error,error_size,"too many tiling parameters");return 0;}htb_tiling_parameter*p=&config->parameters[config->parameter_count++];snprintf(p->key,sizeof(p->key),"%s",definition->parameters[i].id);snprintf(p->value,sizeof(p->value),"%s",definition->parameters[i].default_value);}
    }
    return 1;
}

const char *htb_tiling_parameter_value(const htb_tiling_config *config,
                                       const char *key, const char *fallback) {
    for (int i = 0; config && i < config->parameter_count; i++)
        if (strcmp(config->parameters[i].key, key) == 0) return config->parameters[i].value;
    return fallback;
}

int htb_tiling_init(htb_tiling *tiling, htb_arith_ctx *arith,
                    const htb_tiling_config *config, char *error, size_t error_size) {
    memset(tiling, 0, sizeof(*tiling));tiling->arith=arith;htb_tiling_config normalized;
    if(config)normalized=*config;else memset(&normalized,0,sizeof(normalized));
    if(!htb_tiling_config_normalize(&normalized,error,error_size))return 0;
    tiling->definition=htb_tiling_definition_find(normalized.id);
    return tiling->definition->ops.initialize(tiling,&normalized,error,error_size);
}

void htb_tiling_clear(htb_tiling *tiling) {
    if (tiling && tiling->definition && tiling->definition->ops.clear) tiling->definition->ops.clear(tiling);
    if (tiling) memset(tiling, 0, sizeof(*tiling));
}

int htb_tiling_neighbors(const htb_tiling *tiling, const htb_tile_id *tile,
                         htb_adjacency_list *neighbors, char *error, size_t error_size) {
    htb_edge_list edges; htb_edge_list_init(&edges);
    if (!tiling->definition->ops.enumerate_edges(tiling, tile, &edges, error, error_size)) {
        htb_edge_list_clear(&edges); return 0;
    }
    for (size_t i = 0; i < edges.count; i++) {
        htb_adjacency adjacency; memset(&adjacency, 0, sizeof(adjacency));
        if (!tiling->definition->ops.adjacent(tiling, tile, &edges.items[i], &adjacency, error, error_size) ||
            !htb_adjacency_list_append(neighbors, &adjacency)) {
            htb_edge_list_clear(&edges); return 0;
        }
    }
    htb_edge_list_clear(&edges); return 1;
}
