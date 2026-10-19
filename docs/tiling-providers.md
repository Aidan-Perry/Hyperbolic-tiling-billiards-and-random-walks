# Tiling providers

A tiling is a versioned package of geometry, topology, metadata, and presentation behavior. The billiard engine operates on this contract and does not assume square tiles, four sides, integer coordinate pairs, or a Euclidean square-grid reference view. Native definitions and JSON manifests both declare tile types and side counts; manifests may additionally carry descriptive topology such as vertex valence.

## Native contract

Register one `htb_tiling_definition` in `src/native/tilings/registry.c`. Its operations must provide:

- `initialize` / `clear`: parse exact provider parameters and own run-local state.
- `locate_point`: return a canonical tile identity and any incident edges for a disk point.
- `enumerate_edges`: list every finite boundary edge owned by a tile.
- `intersections`: analytically produce valid, tangent, or unresolved ray/edge candidates. Rendering polylines are not valid collision geometry.
- `normal_at`: provide a Euclidean disk-space normal used by the shared negative-refraction operation.
- `adjacent`: map a tile/edge pair to the neighboring tile and its reverse edge.
- `edge_vertices`: provide endpoints when vertex-clearance diagnostics are supported.
- `to_reference` / `from_reference`: provide an optional secondary representation.
- `tile_measure`: provide supported geometric measures for diagnostics or stochastic weighting.

Tile identity is an opaque `htb_tile_id`. Its canonical `key` must be stable and unique within the provider version; the engine uses that key for equality, hashing, visited sets, periodicity, exports, and future caches. `provider_words` are an optional compact form that only the provider interprets. A label and tile-type string are presentation metadata.

Boundary identity follows the same rule. The `boundary_key` identifies the same geometric boundary from either adjacent tile; `local_index` is only meaningful to its provider. This is how the engine detects a true shared-vertex event without assuming names such as `left` or `top`.

`htb_tiling_neighbors` is the first-class adjacency lookup. It enumerates the tile's edges and resolves each neighbor through the provider. The lookup is independent of billiard propagation, so a future random-walk runner can use it directly. `tile_measure` already distinguishes disk Euclidean area, reference-space Euclidean area, and disk perimeter; a random-walk policy can request a supported measure and normalize its own probability distribution. The current provider reports reference area exactly as 1 and honestly reports disk area as unsupported.

## Metadata manifest

Add a manifest under `src/tilings/<provider-id>/manifest.json`. It declares:

- stable ID and version;
- display name and scientific description;
- all tile types and their side counts;
- supported representations;
- typed parameters and exact-string defaults;
- capabilities such as adjacency and available area measures.

The server publishes these manifests at `/api/tilings`; the diagnostics UI is generated from their parameter list. Unknown IDs, versions, and parameter names are rejected before a process starts and are validated again natively.

## Browser renderer

Register the corresponding presentation module in `src/web/tilings`. It supplies `drawDiskGrid`, `drawReferenceGrid`, and the disk-to-reference display transform. Add its script to the two geometry pages. These functions are deliberately non-authoritative: the native provider remains the sole source of tile crossings, refraction points, adjacency, and event state.

## Adding an order-5 square tiling

An order-5 square provider can therefore use a square tile type with `sideCount: 4` while giving each vertex valence 5 through its adjacency/topology implementation. It will need canonical combinatorial tile identities, analytic disk-edge intersections, Euclidean normals to those disk curves, robust point location, reverse-edge adjacency, a manifest, and a browser renderer. No Arb/Calcium code, trajectory loop, server lifecycle, or billiard refraction code should be duplicated.

If it lacks a useful global plane representation, its reference-map operations may report unsupported. The generic event record preserves disk results, and the reference page and reference-space diagnostics must show unavailable data instead of inventing a square-grid map.
