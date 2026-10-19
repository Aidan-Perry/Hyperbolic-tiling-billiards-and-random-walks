# Hyperbolic Tiling Billiards

Research software for ordinary **Euclidean tiling billiards** on selectable curved tilings of the open unit disk. The installed definitions are the elliptically distorted square grid and the regular order-5 square tiling \(\{4,5\}\). The name describes the disk representation, not the dynamics: trajectories are straight Euclidean segments, and all boundary angles, normals, distances, and refractions are Euclidean. The order-5 provider uses hyperbolic geometry only to construct its fixed tiling boundaries—not to propagate the billiard.

The application has a C17/FLINT native engine, a small Node.js process supervisor, and three synchronized p5.js pages. It supports one or two independently initialized trajectories in rigorous Arb **Scientific Computing** mode or Calcium **Exact Algebra** mode.

## Mathematics

For a configurable center (c=(c_x,c_y)), the ordinary square grid is mapped into the disk by

\[
F_c(x,y)=\frac{(x-c_x,y-c_y)}{\sqrt{1+(x-c_x)^2+(y-c_y)^2}}.
\]

Its inverse is

\[
F_c^{-1}(u,v)=\left(c_x+\frac{u}{\sqrt{1-u^2-v^2}},c_y+\frac{v}{\sqrt{1-u^2-v^2}}\right).
\]

The image of a vertical line (x=k), where (a=k-c_x\neq0), lies on

\[
(1+a^2)u^2+a^2v^2=a^2,
\]

but only on the branch with \(\operatorname{sign}(u)=\operatorname{sign}(a)\). The native solver enforces that restriction and then verifies membership in the current finite edge through the inverse map. Horizontal boundaries are treated analogously. Lines with (a=0) or (b=0) are handled as straight lines.

At a crossing, the native engine uses

\[
V_{\mathrm{out}}=2\frac{V_{\mathrm{in}}\cdot N}{N\cdot N}N-V_{\mathrm{in}},
\]

with (N_x=(1-v^2,uv)) or (N_y=(uv,1-u^2)). This preserves the normal component and negates the tangential component; it is **not** ordinary specular reflection.

See [Mathematical reference](docs/mathematics.md), [Architecture](docs/architecture.md), and [Tiling providers](docs/tiling-providers.md) for implementation details.

### Order-5 square tiling

The `order5-square@1` provider constructs the regular Poincaré-disk tessellation \(\{4,5\}\): every tile has four geodesic sides and exactly five tiles meet at every vertex. It uses the reflection triangle group \(\Delta(2,4,5)\) for exact topology and adjacency. The distinguished vertex defaults to \((0,0)\), and its disk coordinates are configurable as exact rational `centerU` and `centerV` parameters. This placement is a disk isometry of the same tiling.

Although these tile edges are Poincaré geodesics, collision detection treats each as its exact Euclidean circle or diameter. Between crossings, the particle still follows a straight Euclidean chord and uses the same Euclidean negative-refraction formula shown above.

## Dependencies

- C17 compiler
- CMake 3.20+
- FLINT 3.x with Calcium and Arb
- Jansson
- Node.js 20+
- npm

macOS with Homebrew:

```bash
brew install cmake flint jansson
npm install
```

Ubuntu/Debian package names vary by release. When FLINT 3.x is available from the configured repositories:

```bash
sudo apt install build-essential cmake pkg-config libflint-dev libjansson-dev nodejs npm
npm install
```

Confirm that `pkg-config --modversion flint` reports 3.x. Calcium is part of current FLINT; do not install a separate legacy Calcium package.

## Build and test

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
npm test
```

The engine consumes one JSON request on standard input and emits JSON Lines progress plus one final result. It can also read a file:

```bash
build/htb_engine --request request.json
```

## Run the application

```bash
npm start
```

Open <http://127.0.0.1:8765>. The launcher opens three separate pages:

- <http://127.0.0.1:8765/disk> — Poincaré disk image and actual straight billiard segments
- <http://127.0.0.1:8765/plane> — the selected provider's optional secondary representation (the order-5 provider clearly reports that none is defined)
- <http://127.0.0.1:8765/diagnostics> — authoritative controls, playback, graphs, event history, uncertainty, and export

If the engine is installed elsewhere, set `HTB_ENGINE` to its absolute path. Set `PORT` to change the local port.

## Using the controls

Coordinate and angle fields accept exact integers, terminating decimals, and fractions such as `-17/64`. Strings are parsed directly into FLINT rationals; they never pass through JavaScript binary floating point before native computation.

The tiling selector is populated from the server's installed provider manifests. It contains **Elliptically Distorted Square Grid** and **Order-5 Square Tiling {4,5}**. Selecting a tiling chooses a versioned native geometry/topology provider and its provider-specific parameters; it is not a cosmetic rendering choice.

The launch point must lie strictly inside the disk. Initial direction is entered in ordinary Euclidean degrees: (0^\circ) points along (+u), (90^\circ) along (+v), and positive angles turn counterclockwise. The native arithmetic backend evaluates the unit direction ((\cos(\pi\theta/180),\sin(\pi\theta/180))); browser trigonometry is used only for the green preview. Arb produces rigorous enclosures and Calcium retains the exact algebraic values for rational degree inputs.

The coordinate steppers change (u) and (v) by `0.001`; the angle stepper changes the direction by `0.25` degrees. Holding a stepper waits 500 ms and then repeats every 45 ms. Values can always be typed directly.

An interior launch is followed until the first certified tile-boundary intersection, recorded as (P_0). Negative refraction is applied there, and a request for (N) bounces then attempts (P_1,\ldots,P_N). The launch-to-(P_0) segment is initialization and is excluded from cumulative billiard path length.

An exact launch on one non-vertex edge uses that point as (P_0); the supplied direction must point unambiguously into one tile, and no artificial refraction is applied at time zero. Vertex and tangent launches are rejected.

Enable Trajectory B for an independent blue trajectory. Both trajectories share the arithmetic backend and selected tiling configuration, but have independent launch, direction, and bounce values. A failure in one preserves its certified prefix and does not discard the other result.

Inputs lock as soon as a run starts. **Reset Trajectory** cancels an active subprocess if necessary, clears run-specific data, unlocks inputs, and preserves current field values. Green arrows are presentation-only Euclidean unit-direction indicators and disappear while a run is active or displayed.

## Arithmetic modes

### Scientific Computing

Arb balls rigorously enclose every native scalar. The initial precision is 128 bits by default. If a collision decision cannot be certified, the engine doubles precision and replays the trajectory from the original rational inputs, up to the configured maximum (8192 bits by default). It never chooses by midpoint or epsilon.

### Exact Algebra

Calcium `ca_t` values retain exact algebraic expressions through intersections and refractions. Comparisons use Calcium truth values; `T_UNKNOWN` becomes an unresolved decision, never equality. Exact mode never silently switches to numerical propagation. Certified numerical enclosures are produced only for display and export.

Each enabled trajectory owns an independent mutable arithmetic context.

## Diagnostics and export

The native result records every certified event: disk and provider-reference coordinates when available, canonical before/after tile keys, canonical boundary key, crossed-edge label, incoming/outgoing direction, collision-time enclosure, precision, displacement, cumulative Euclidean disk path length, and endpoint clearance where supported. Unique visits use opaque provider-owned tile identities rather than inferred coordinates. Exact periodicity requires a repeated exact crossing state; an Arb proximity label is explicitly not a proof.

The diagnostics page offers full JSON, per-bounce diagnostics CSV, and crossing-state CSV. Rounded `approx` values are presentation values. `display` and `enclosure` fields preserve the distinction between display conversion and the authoritative native state.

## Benchmarks

Run the built-in native workload at (N\in\{10,25,50,100,200\}):

```bash
build/htb_engine --benchmark scientific
build/htb_engine --benchmark exact
# or both
npm run benchmark
```

The benchmark prints measured status, completed bounces, runtime, comparison count, candidate count, and maximum precision. Results depend strongly on the orbit and machine; exact algebraic complexity can grow far faster than this deliberately reproducible rational sample.

## Uncertainty and resource behavior

Every collision predicate has an explicit Less/Equal/Greater/Unresolved/Error outcome. A true equal-time incident-edge event stops as a vertex collision. Overlapping Arb balls cause replay at higher precision; an unresolved Calcium comparison or an Arb overlap at the precision ceiling stops with the last fully certified prefix, competing edges, time enclosures, tile, bounce, precision, and reason.

The server permits one active native run (containing up to two trajectories), imposes input and bounce limits, and uses `spawn` without a shell. There is no wall-clock time budget: a computation continues until it finishes or the user presses **Cancel computation** (reset also cancels active work). Peak resident memory is measured with `getrusage`; the configured memory threshold is a checked budget rather than an OS hard limit.

## Known limitations

- Exact algebra can exceed practical time or memory long before 50 bounces on algebraically complicated orbits. Partial certified output is intentional.
- The browser's A–B head-separation plot currently uses the centers of the native certified display enclosures. It is labeled as a presentation comparison and is not an exact or certified separation claim. Collision decisions never use it.
- Browser drawing uses polygonal sampling, including adaptive-looking dense inverse-map samples; drawing never feeds back into collision calculations.
- The visible disk grid is a finite selection of mathematically infinite grid lines. The native trajectory has no corresponding rendering boundary.
- The order-5 provider has no global Euclidean reference-plane representation. Its disk results, topology, diagnostics, and exports are complete, while reference-space graphs and the reference page are explicitly unavailable.
- Exact order-5 trajectories can accumulate algebraic complexity rapidly. Short rational-direction runs are practical; long generic runs may require cancellation even though no mathematical approximation fallback occurs.
- Memory enforcement is portable and cooperative; the native memory limit is checked against measured resident usage. Long exact computations have no automatic wall-clock deadline and must be cancelled explicitly when no longer wanted.

## Extending the engine

The generic geometry layer calls only the scalar abstraction in `include/htb/arithmetic.h`; FLINT calls are isolated in `src/native/arithmetic`. A replacement backend implements initialization, exact-rational parsing, field operations, square root, comparison, finiteness, and display/enclosure conversion. Unresolved comparison must remain a first-class outcome.

Additional curved tilings implement the contract in `include/htb/tiling.h`: opaque tile identity, finite-edge enumeration and analytic intersections, normals, adjacency, optional reference maps, vertices, and measurements. Add a manifest and a presentation-only browser renderer. `htb_tiling_neighbors` exposes adjacency independently of billiards for future random-walk runners, while the measurement operation provides a home for area-weighted policies. Do not duplicate the trajectory runner or arithmetic backends, and do not put authoritative collision logic in the browser.
