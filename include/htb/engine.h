#ifndef HTB_ENGINE_H
#define HTB_ENGINE_H

#include "htb/tiling.h"
#include <jansson.h>
#include <stddef.h>
#include <stdint.h>

#define HTB_STATUS_TEXT 512
#define HTB_MAX_COMPETING_CANDIDATES 16

typedef enum {
    HTB_STATUS_READY,
    HTB_STATUS_COMPUTING,
    HTB_STATUS_COMPLETED,
    HTB_STATUS_CANCELLED,
    HTB_STATUS_UNRESOLVED,
    HTB_STATUS_VERTEX,
    HTB_STATUS_DEGENERATE,
    HTB_STATUS_RESOURCE_LIMIT,
    HTB_STATUS_INVALID_INPUT,
    HTB_STATUS_INTERNAL_ERROR
} htb_status;

typedef struct {
    char id[2];
    char start_u[HTB_TEXT], start_v[HTB_TEXT];
    char direction_u[HTB_TEXT], direction_v[HTB_TEXT];
    char angle_degrees[HTB_TEXT];
    int uses_angle;
    int bounces;
} htb_trajectory_input;

typedef struct {
    char run_id[80];
    htb_backend_kind backend;
    htb_tiling_config tiling;
    /* Version-1 compatibility fields. New requests use tiling parameters. */
    char center_x[HTB_TEXT], center_y[HTB_TEXT];
    long initial_precision, max_precision;
    size_t memory_limit_mb;
    int trajectory_count;
    htb_trajectory_input trajectories[2];
} htb_run_request;

typedef struct {
    int index;
    htb_tile_id tile_before, tile_after;
    htb_edge_ref edge;
    htb_vec2 point, reference_point, incoming, outgoing;
    htb_scalar time, disk_displacement, reference_displacement, path_length;
    htb_scalar disk_clearance, reference_clearance;
    long precision;
    int certified;
    int reference_available;
    int clearance_available;
    int unique_tiles;
    int repeated_visits;
    double runtime_seconds;
    char time_enclosure[HTB_TEXT];
} htb_event;

typedef struct {
    char id[2];
    htb_status status;
    char status_detail[HTB_STATUS_TEXT];
    int requested_bounces, completed_bounces;
    htb_vec2 launch, initial_direction;
    htb_vec2 true_start;
    htb_tile_id initial_tile;
    htb_event *events;
    size_t event_count, event_capacity;
    int total_tile_crossings, unique_tiles, repeated_visits;
    int periodicity_period, periodicity_start;
    char periodicity[80];
    uint64_t candidates_examined, comparison_retries, precision_refinements;
    long maximum_precision;
    double runtime_seconds;
    int uncertainty_bounce;
    htb_tile_id uncertainty_tile;
    int competing_count;
    char competing_edges[HTB_STATUS_TEXT];
    char competing_edge_labels[HTB_MAX_COMPETING_CANDIDATES][HTB_TILING_LABEL_MAX];
    char competing_times[HTB_MAX_COMPETING_CANDIDATES][HTB_TEXT];
    char competing_states[HTB_MAX_COMPETING_CANDIDATES][40];
    char uncertainty_reason[160];
    htb_arith_ctx *arith;
} htb_trajectory_result;

typedef struct {
    char run_id[80];
    htb_backend_kind backend;
    htb_status status;
    char status_detail[HTB_STATUS_TEXT];
    htb_tiling_config tiling;
    htb_trajectory_result trajectories[2];
    int trajectory_count;
    double runtime_seconds;
    long peak_memory_kb;
    uint64_t comparisons, unresolved_comparisons, candidates_examined;
    long maximum_precision;
} htb_run_result;

typedef void (*htb_progress_fn)(json_t *event, void *user);

int htb_parse_request_json(json_t *root, htb_run_request *request, char *error, size_t error_size);
int htb_run(const htb_run_request *request, htb_run_result *result,
            htb_progress_fn progress, void *progress_user);
void htb_run_result_clear(htb_run_result *result);
json_t *htb_run_result_json(const htb_run_request *request, const htb_run_result *result);
const char *htb_status_name(htb_status status);

#endif
