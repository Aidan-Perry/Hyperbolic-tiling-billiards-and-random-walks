# Architecture

The application is organized around independent simulation, arithmetic, and tiling abstractions. A simulation runner determines which operation is performed, an arithmetic backend determines how billiard scalar facts are proved, and a tiling provider determines the geometry and topology consumed by both billiards and random walks.

## Native layers

1. `src/native/arithmetic` owns FLINT-specific values and contexts. Arb and Calcium expose the same operations and explicit unresolved comparisons.
2. `src/native/geometry` contains only tiling-independent vector operations, ray evaluation, disk containment, Euclidean distance, and negative refraction. It does not know that a tile is a square.
3. `src/native/tilings` is the provider registry. A provider owns point location, tile and boundary identities, edge enumeration, analytic intersections, normals, adjacency, optional reference maps, vertices, and tile measurements. `elliptical_square.c` implements the radial grid image; `order5_square.c` implements the regular \(\{4,5\}\) reflection tiling.
4. `src/native/billiards` asks the selected provider for candidate crossings, certifies their ordering through the selected arithmetic backend, applies the generic negative-refraction operation, follows the provider's adjacency result, and records diagnostics. It never derives a neighboring tile by modifying grid coordinates.
5. `src/native/random_walk` asks only for point location, adjacency, tile centers, and numerical disk areas. It applies seeded stochastic policies without knowing tile coordinate schemes or billiard rules.
6. `src/native/io` validates versioned tiling configurations and serializes opaque canonical tile/boundary identities. Coordinate arrays are provider metadata, not the engine's identity scheme.

The two main dependency directions are therefore:

```text
Arb / Calcium -> scalar interface <- generic billiard runner
tiling provider -> tiling interface <- generic billiard runner
                                    <- generic random-walk runner
```

The contract is defined in `include/htb/tiling.h`. See [Tiling providers](tiling-providers.md) for the complete extension path.

## Server and protocol

`src/server/server.js` validates request structure and limits, supervises one shell-free native subprocess, stores the current snapshot, synchronizes pages, cancels jobs, and serves export files. It exposes provider manifests at `GET /api/tilings`.

Schema 3 requests select a simulation type and a versioned provider with provider-owned parameters:

```json
{
  "mode": "scientific",
  "tiling": {
    "id": "elliptical-square",
    "version": "1",
    "parameters": {"centerX": "0", "centerY": "0"}
  },
  "trajectories": []
}
```

Mathematical scalar inputs remain strings at the browser/server/native boundary. The native protocol is JSON Lines: progress records precede exactly one final result. Results carry the provider ID, version, canonical tile keys, canonical boundary keys, human-readable labels, and optional provider coordinates. This makes exported runs reproducible without teaching consumers the provider's internal representation.

## Browser layers

The diagnostics page obtains provider manifests from `/api/tilings`, builds the selector and scalar parameter fields, and submits the selected ID/version. The disk and secondary pages use the registry in `src/web/tilings`. Each visual provider supplies only presentation transforms and grid drawing. It cannot choose collisions, choose random transitions, or feed values back to native computation. Random-walk graph layout is likewise presentational: topology, node identity, traversal counts, and the one-edge halo all arrive from the native result.

The server snapshot is reload-safe. A `BroadcastChannel` makes bounce selection and previews immediate, while polling ensures independently opened pages converge on the current run ID and result.

## Cancellation

Cancel and reset send `SIGTERM` to the current child and escalate to `SIGKILL` after one second if it remains alive. State updates are guarded by run ID, so a superseded child cannot publish stale results.

## Rendering boundary

Disk renderers sample provider-defined curves solely for display. When a provider exposes a reference representation, the reference renderer samples the provider's inverse image of each already computed disk segment. Providers without such a representation produce a clear unavailable state. Browser drawing uses JavaScript numbers because screen coordinates are presentation values; none of it enters the native simulation.
