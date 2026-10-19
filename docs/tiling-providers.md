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

`htb_tiling_neighbors` is the first-class adjacency lookup. It enumerates the tile's edges and resolves each positive-side-length neighbor through the provider; vertex-only contacts never enter the list. The lookup is independent of billiard propagation and is used directly by the random-walk runner. Providers also expose `tile_center` and `disk_area_approx`, allowing the generic runner to draw canonical center paths and normalize stochastic weights. The area call returns an explicit numerical estimate and estimated error rather than pretending to be an exact scalar. The elliptical provider uses fixed Gauss-Legendre quadrature of the radial-map Jacobian; the order-5 provider numerically integrates its geodesic boundary.

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

Register the corresponding presentation module in `src/web/tilings`. It must supply `drawDiskGrid`; `drawReferenceGrid` and the disk-to-reference display transform are optional as a pair. Add its script to the two geometry pages. These functions are deliberately non-authoritative: the native provider remains the sole source of tile crossings, refraction points, adjacency, and event state.

## Installed order-5 square tiling

`order5-square@1` uses a square tile type with `sideCount: 4` and vertex valence 5. Its canonical topology is generated in the exact field \(\mathbb Q(\sqrt2,\sqrt\varphi)\) from the reflection group \(\Delta(2,4,5)\). Exact transformed tile centers merge duplicate group words, and shared boundary records give a reversible adjacency lookup. Disk point location folds a point through the same four side reflections until it lies in the base square.

The `centerU` and `centerV` parameters move one distinguished order-5 vertex to any rational point strictly inside the disk. The native provider applies a Lorentz/Poincaré isometry to every edge, while retaining the same exact combinatorial tiling. The default is the disk origin.

Each supporting edge is represented analytically as a Poincaré geodesic (a Euclidean circle orthogonal to the unit circle, or a diameter). Intersections are solved as degree-at-most-two equations through the shared Arb or Calcium abstraction, finite-edge membership is checked against all four tile half-spaces, and the returned normal is the ordinary Euclidean gradient used by the generic negative-refraction function. The browser independently samples the same reflection construction for black-line rendering only.

The provider has no useful global plane representation, so its reference-map operations report unsupported. The generic event record preserves disk results, and the reference page and reference-space diagnostics show unavailable data instead of inventing a square-grid map.
