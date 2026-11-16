# Hyperbolic Tiling Billiards

Research software for ordinary **Euclidean tiling billiards** and seeded, area-weighted **random walks** on selectable curved tilings of the open unit disk. The installed definitions are the elliptically distorted square grid, the regular order-5 square tiling \(\{4,5\}\), and the horocyclic binary (Böröczky) tiling. The name describes the disk representation, not the billiard dynamics: trajectories are straight Euclidean segments, and all boundary angles, normals, distances, and refractions are Euclidean. Hyperbolic constructions are used only to define applicable tiling boundaries—not to propagate the billiard.

The application has a C17/FLINT native engine, a small Node.js process supervisor, and three synchronized p5.js pages. Billiards supports one or two independently initialized trajectories in rigorous Arb **Scientific Computing** mode or Calcium **Exact Algebra** mode. Random walk is a separate native simulation type with its own controls and diagnostics.

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

### Binary tiling

The `binary@1` provider is the formal horocyclic binary or Böröczky tiling. In the Poincaré upper half-plane it uses the complete dyadic family

\[
T_{n,k}=[k2^n,(k+1)2^n]\times[2^n,2^{n+1}],
\qquad n,k\in\mathbb Z.
\]

The Cayley map sends this family to the disk with its common ideal point at \((1,0)\). A tile has a left neighbor, a right neighbor, one parent, and two children. Its lower horocycle is consequently represented by two finite edges, giving five positive-length neighbors. The secondary geometry window shows the exact upper-half-plane representation. See the [mathematical reference](docs/mathematics.md#horocyclic-binary-b%C3%B6r%C3%B6czky-tiling) for the map, analytic circle equations, and adjacency formulas.

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

Windows uses the MSYS2 UCRT64 toolchain because the native engine depends on FLINT, GMP, MPFR, and Jansson. Install [MSYS2](https://www.msys2.org/), open its **UCRT64** terminal, and run:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-flint mingw-w64-ucrt-x86_64-jansson
```

Install Node.js 20 or newer separately. The Windows launcher recognizes the default `C:\msys64` installation. For a different location, set `MSYS2_ROOT` to the MSYS2 installation directory before launching.

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

For a double-click launch, use the platform-specific file in the repository root:

- macOS: `Start_sim_mac.command`
- Windows: `Start_sim_windows.cmd`

Each launcher checks dependencies, installs the local p5.js package when needed, builds the native engine, starts the local server, and opens the three synchronized pages. Keep its terminal window open while using the application. The Windows launcher keeps its native artifacts in `build-windows` so they cannot conflict with a macOS or Linux build directory.

To start the already-built application manually:

```bash
npm start
```

Open <http://127.0.0.1:8765>. The launcher opens three separate pages:

- <http://127.0.0.1:8765/disk> — Poincaré disk image, actual straight billiard segments, or random-walk tile-center path
- <http://127.0.0.1:8765/plane> — provider reference representation for billiards, or the numbered adjacency graph for random walk
- <http://127.0.0.1:8765/diagnostics> — authoritative controls, playback, graphs, event history, uncertainty, and export

If the engine is installed elsewhere, set `HTB_ENGINE` to its absolute path. Set `PORT` to change the local port.

## Using the controls

The visually separate **Simulation** selector switches between Billiards and Random Walk. Each choice has its own input panels, run labels, diagnostics, event history, graphs, and secondary visualization. Changing the selection while a result is displayed requires a reset, like any other mathematical input.

Coordinate and angle fields accept exact integers, terminating decimals, and fractions such as `-17/64`. Strings are parsed directly into FLINT rationals; they never pass through JavaScript binary floating point before native computation.

The tiling selector is populated from the server's installed provider manifests. It contains **Elliptically Distorted Square Grid**, **Order-5 Square Tiling {4,5}**, and **Binary Tiling (Böröczky)**. Selecting a tiling chooses a versioned native geometry/topology provider and its provider-specific parameters; it is not a cosmetic rendering choice.

The launch point must lie strictly inside the disk. Initial direction is entered in ordinary Euclidean degrees: (0^\circ) points along (+u), (90^\circ) along (+v), and positive angles turn counterclockwise. The native arithmetic backend evaluates the unit direction ((\cos(\pi\theta/180),\sin(\pi\theta/180))); browser trigonometry is used only for the green preview. Arb produces rigorous enclosures and Calcium retains the exact algebraic values for rational degree inputs.

The coordinate steppers change (u) and (v) by `0.001`; the angle stepper changes the direction by `0.25` degrees. Holding a stepper waits 500 ms and then repeats every 45 ms. Values can always be typed directly.

An interior launch is followed until the first certified tile-boundary intersection, recorded as (P_0). Negative refraction is applied there, and a request for (N) bounces then attempts (P_1,\ldots,P_N). The launch-to-(P_0) segment is initialization and is excluded from cumulative billiard path length.

An exact launch on one non-vertex edge uses that point as (P_0); the supplied direction must point unambiguously into one tile, and no artificial refraction is applied at time zero. Vertex and tangent launches are rejected.

Enable Trajectory B for an independent blue trajectory. Both trajectories share the arithmetic backend and selected tiling configuration, but have independent launch, direction, and bounce values. A failure in one preserves its certified prefix and does not discard the other result.

Inputs lock as soon as a run starts. **Reset Trajectory** cancels an active subprocess if necessary, clears run-specific data, unlocks inputs, and preserves current field values. Green arrows are presentation-only Euclidean unit-direction indicators and disappear while a run is active or displayed.

Each viewer has a **Hide tiling** control that removes the tiling or adjacency backdrop without hiding the billiard trajectory or random-walk path. Use **Show tiling** to restore it. Mouse-wheel zoom stays anchored to the pointer and has no application-imposed maximum magnification.

For the order-5 square tiling, the disk viewer also offers **Show tenths**. Its ten green rays alternate between the five square edges incident to the distinguished vertex and their five angular bisectors. Because the Poincaré model preserves angles, the construction remains tied to the tiling when that vertex is moved away from the disk center.

## Random walks

Random walk accepts an interior disk point and a number of steps. When a run begins, the server generates a fresh uniformly random unsigned 64-bit seed and records it in the authoritative request and result. The native tiling provider locates the starting tile. At every step the candidate set consists of the current tile together with every tile sharing a positive-length edge with it; vertex-only contacts are excluded. For the installed distribution,

\[
\Pr(T\to S)=\frac{\operatorname{area}_{\mathbb D}(S)}{\sum_{R\in\{T\}\cup N(T)}\operatorname{area}_{\mathbb D}(R)}.
\]

Including \(T\) itself means that a step can stay in the current tile. Such stays are recorded explicitly rather than discarded. The generated seed is shown in the completed-run summary and included in JSON and CSV exports. It drives a fixed native SplitMix64 generator, so retaining that recorded seed together with the application version, tiling parameters, distribution, starting point, and step count preserves a reproducible experiment. Direct native or structured API requests may supply a recorded seed when an exact replay is required; ordinary browser runs always generate one automatically.

This mode intentionally has no Arb/Calcium selector. Tile location still begins from exact rational text and uses certified native geometry, but transition weights are numerical Euclidean disk-area estimates supplied by each tiling provider. The result labels those areas and their refinement-difference error estimates as approximate; they are not certified balls and are never used for billiard decisions.

The disk window joins successive tile centers with a red line. The secondary window displays the canonical adjacency graph: gray edges are tile adjacencies, red edges are traversed transitions, node/edge labels record step indices, and the exported graph includes every visited tile plus its complete one-edge neighbor halo. Playback synchronizes the selected step across both views.

Random-walk diagnostics include center displacement, unique tiles, cumulative center-to-center path length, current tile area, selected transition probability, repeat count, stay count, and a complete step table. JSON and CSV exports include the seed, draws, probabilities, approximate area metadata, canonical tile identities, and adjacency graph.

## Arithmetic modes

### Scientific Computing

Arb balls rigorously enclose every native scalar. The initial precision is 128 bits by default. If a collision decision cannot be certified, the engine doubles precision and replays the trajectory from the original rational inputs, up to the configured maximum (8192 bits by default). It never chooses by midpoint or epsilon.

### Exact Algebra

Calcium `ca_t` values retain exact algebraic expressions through intersections and refractions. Comparisons use Calcium truth values; `T_UNKNOWN` becomes an unresolved decision, never equality. Exact mode never silently switches to numerical propagation. Certified numerical enclosures are produced only for display and export.

Each enabled trajectory owns an independent mutable arithmetic context.

## Diagnostics and export

The native result records every certified event: disk and provider-reference coordinates when available, canonical before/after tile keys, canonical boundary key, crossed-edge label, incoming/outgoing direction, collision-time enclosure, precision, displacement, cumulative Euclidean disk path length, and endpoint clearance where supported. Unique visits use opaque provider-owned tile identities rather than inferred coordinates. Exact periodicity requires a repeated exact crossing state; an Arb proximity label is explicitly not a proof.

The diagnostics page offers full JSON and mode-appropriate CSV data: per-bounce/crossing exports for billiards or per-step exports for random walk. Rounded `approx` values are presentation values. `display` and `enclosure` fields preserve the distinction between display conversion and the authoritative native state.

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

The server permits one active native run (containing up to two trajectories), validates its inputs, and uses `spawn` without a shell. There is no wall-clock time budget: a computation continues until it finishes or the user presses **Cancel computation** (reset also cancels active work). Peak resident memory is measured with `getrusage`; the configured memory threshold is a checked budget rather than an OS hard limit.

## Known limitations

- Exact algebra can exceed practical time or memory long before 50 bounces on algebraically complicated orbits. Partial certified output is intentional.
- The browser's A–B head-separation plot currently uses the centers of the native certified display enclosures. It is labeled as a presentation comparison and is not an exact or certified separation claim. Collision decisions never use it.
- Browser drawing uses polygonal sampling, including adaptive-looking dense inverse-map samples; drawing never feeds back into collision calculations.
- The visible disk grid is a finite selection of mathematically infinite grid lines. The native trajectory has no corresponding rendering boundary.
- The order-5 provider has no global Euclidean reference-plane representation. Its disk results, topology, diagnostics, and exports are complete, while reference-space graphs and the reference page are explicitly unavailable.
- Exact order-5 trajectories can accumulate algebraic complexity rapidly. Short rational-direction runs are practical; long generic runs may require cancellation even though no mathematical approximation fallback occurs.
- The binary provider fixes its common ideal point at the right-hand disk boundary \((1,0)\). A placement/rotation parameter is not exposed in version 1.
- Binary tile indices are limited to levels \(-4096\) through \(4096\). At much smaller disk-area scales, random-walk weighting can also stop with an explicit numerical-range error rather than underflow silently.
- Memory enforcement is portable and cooperative; the native memory limit is checked against measured resident usage. Long exact computations have no automatic wall-clock deadline and must be cancelled explicitly when no longer wanted.
- Random-walk disk areas are deterministic numerical estimates rather than Arb-certified measures. All installed providers expose an estimated quadrature/sampling error, and exports state `certified: false`.

## Extending the engine

The generic geometry layer calls only the scalar abstraction in `include/htb/arithmetic.h`; FLINT calls are isolated in `src/native/arithmetic`. A replacement backend implements initialization, exact-rational parsing, field operations, square root, comparison, finiteness, and display/enclosure conversion. Unresolved comparison must remain a first-class outcome.

Additional curved tilings implement the contract in `include/htb/tiling.h`: opaque tile identity, finite-edge enumeration and analytic intersections, normals, adjacency, tile centers, approximate disk area, optional reference maps, vertices, and measurements. Add a manifest and a presentation-only browser renderer. `htb_tiling_neighbors` drives both billiard transitions and generic random walks; `tile_center` and `disk_area_approx` make the stochastic runner independent of any particular tiling. Do not duplicate either simulation runner or put authoritative state transitions in the browser.
