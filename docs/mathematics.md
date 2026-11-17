# Mathematical reference

## Elliptically distorted square grid

Disk space ((u,v)) is an ordinary Euclidean subset of \(\mathbb R^2\). Square-grid space ((x,y)) supplies tile identities and a second representation. The map (F_c\) is a smooth bijection from the plane to the open disk. Thus four image tiles meet at every mapped integer-grid vertex, exactly as in the source grid.

The disk trajectory is authoritative. A straight disk segment from (P) to (Q) is shown in the square-grid window by sampling

\[
F_c^{-1}((1-t)P+tQ),\qquad 0\le t\le1.
\]

It is generally curved and is not independently refracted in the square-grid view.

## Analytic collision equation

For a ray (R(t)=P+tV\), substitution in a noncentral vertical boundary produces

\[
A=(1+a^2)d_u^2+a^2d_v^2,
\]
\[
B=2((1+a^2)u_0d_u+a^2v_0d_v),
\]
\[
C=(1+a^2)u_0^2+a^2v_0^2-a^2.
\]

The engine solves (At^2+Bt+C=0\) in the selected backend, with a linear fallback. Each root must be positive, strictly inside the disk, on the correct ellipse branch, on the current finite edge under the inverse map, and transverse to the edge.

At a just-processed curved edge, the exact point is mathematically known to be a root even when an Arb enclosure cannot re-prove (C=0\). The engine factors out that known (t=0\) event and tests the genuine second root (-B/A\). This is why it can still detect a future hit on the same supporting curve without a spatial offset.

## Certified ordering

Arb accepts (t_1<t_2\) only when their balls are disjoint in that order. Otherwise the entire trajectory is replayed at doubled precision from rational input. Calcium accepts an ordering only on `T_TRUE`. Equal candidates on two incident edges are a vertex event; an unknown result is never treated as equal.

## Tile and bounce convention

For an interior launch, the inverse point is compared against certified integer bounds to find its tile. Its first crossing is (P_0\), and requested bounces are (P_1,…,P_N\). Tile changes are discrete, explicit operations; the engine never pushes a point across an edge using an epsilon.

Disk displacement is \(\|P_n-P_0\|_2\), inverse-plane displacement is \(\|F_c^{-1}(P_n)-F_c^{-1}(P_0)\|_2\), and default cumulative length excludes the launch segment:

\[
L(n)=\sum_{k=1}^n\|P_k-P_{k-1}\|_2.
\]

All of these are Euclidean quantities.

## Regular order-5 square tiling \(\{4,5\}\)

The second provider uses the Coxeter/Wythoff reflection construction for the triangle group \(\Delta(2,4,5)\). Its exact group data lives in

\[
\mathbb Q(\sqrt2,\sqrt\varphi),\qquad \varphi=\frac{1+\sqrt5}{2}.
\]

Four reflected copies of one side bound the base square, and reflecting the square across each side recursively produces the infinite tessellation. Exact group coordinates identify coincident tiles and boundaries, so the adjacency structure does not depend on a screen-space tolerance. A test walks around the configurable central vertex and verifies that the fifth edge transition returns to the starting tile.

In hyperboloid coordinates, an edge plane has normal \(n=(n_t,n_x,n_y)\). Its Poincaré-disk image satisfies

\[
n_t(1+u^2+v^2)-2n_xu-2n_yv=0.
\]

This is a Euclidean circle orthogonal to the unit circle when \(n_t\ne0\), and a diameter when \(n_t=0\). Substituting \(P+tV\) gives a polynomial of degree at most two, so collisions are analytic rather than polyline based. The ordinary Euclidean disk normal used for billiards is proportional to

\[
(n_tu-n_x,\;n_tv-n_y).
\]

The reflection-group and Poincaré machinery construct the tiling only. The moving particle is not a hyperbolic geodesic: it follows straight Euclidean disk segments and applies the same Euclidean negative-refraction rule as every other provider.

The placement transform sends a distinguished tiling vertex to the exact rational parameter pair `centerU`, `centerV`. This changes only the disk placement, not the \(\{4,5\}\) incidence structure.

### Canonical order-5 side coding

For every order-5 tile, draw the straight Euclidean disk segment from the configured distinguished vertex to that tile's canonical center. Side 1 is the first finite tile edge met by this segment. If the segment meets a vertex, rotate counterclockwise from the segment's outward direction and take the first incident edge. Sides 2, 3, and 4 then follow counterclockwise around the square. All intersection, equal-time, orientation, and ordering tests use the selected certified scalar backend; the provider caches only the resulting local-edge permutation, never an approximate edge length.

A certified crossing from canonical side \(a\) of the old tile into canonical side \(b\) of the new tile has directed symbol `a->b`. Event 0 is included, so a successful initialization crossing already gives a one-symbol word even when zero later bounces are requested. The convention identifier is `order5-radial-v1`. If a label cannot be certified, the event is not appended: Arb replays at higher precision, and a precision-ceiling or exact-backend uncertainty exports only the previously certified event and word prefix.

For a token word \(w\), its factor complexity is

\[
p(n)=\#\{w_iw_{i+1}\cdots w_{i+n-1}:0\le i\le |w|-n\},
\qquad 1\le n\le |w|.
\]

Thus \(p(n)\) counts distinct contiguous token substrings, not characters in the exported text. The browser computes all values in linear time from suffix-automaton state-length intervals and caches them for graph drawing.

The construction follows the standard reflection/Wythoff description of regular hyperbolic tilings; useful background appears in the [PRX Quantum discussion of triangle-group tilings](https://journals.aps.org/prxquantum/abstract/10.1103/PRXQuantum.5.040327) and the [Clark University Poincaré tiling reference](https://mathcs.clarku.edu/~djoyce/poincare/index.html). The formulas implemented here are stated above so a run does not depend on either external site.

## Horocyclic binary (Böröczky) tiling

The `binary@1` provider uses the conventional dyadic construction in the Poincaré upper half-plane

\[
\mathbb H=\{(x,y):y>0\}.
\]

Its tiles are the closed reference cells

\[
T_{n,k}=[k2^n,(k+1)2^n]\times[2^n,2^{n+1}],
\qquad n,k\in\mathbb Z,
\]

with the usual consistent ownership convention on shared boundaries. Scaling by two maps every tile to a congruent tile in hyperbolic geometry. The horizontal sides lie on horocycles; the two vertical sides lie on hyperbolic geodesics asymptotic to the same ideal point. This is the rectangle/cube construction used in the formal binary-tiling literature; see [A Quadtree for Hyperbolic Space](https://jocg.org/index.php/jocg/article/download/5499/3921/17748). The submitted reference image is the corresponding [Poincaré-disk rendering](https://commons.wikimedia.org/wiki/File:Hyperbolic_binary_tiling.png).

The exact Cayley map used by the provider is

\[
C(x,y)=\left(
\frac{x^2+y^2-1}{x^2+(y+1)^2},
\frac{-2x}{x^2+(y+1)^2}
\right),
\]

and its inverse is

\[
C^{-1}(u,v)=\left(
\frac{-2v}{(1-u)^2+v^2},
\frac{1-u^2-v^2}{(1-u)^2+v^2}
\right).
\]

Thus the common upper-half-plane point at infinity is shown at the right-hand ideal disk point \((1,0)\). This placement is fixed in provider version 1.

A vertical reference boundary \(x=a\) has disk equation

\[
a\big((1-u)^2+v^2\big)+2v=0,
\]

and a horizontal reference boundary \(y=h\), with \(h>0\), has disk equation

\[
(h+1)(u^2+v^2)-2hu+(h-1)=0.
\]

Both are Euclidean circle or line equations. Substitution of the Euclidean billiard ray \(P+tV\) therefore gives a polynomial of degree at most two. The native provider solves that polynomial in Arb or Calcium, checks the appropriate finite reference interval under \(C^{-1}\), rejects tangencies, and supplies the Euclidean gradient of the implicit disk circle to the shared negative-refraction operation. Browser curve sampling is never used to choose a collision.

Each cell has five positive-length adjacency edges. Writing a tile as \((n,k)\), these are

\[
\begin{aligned}
L(n,k)&=(n,k-1), & R(n,k)&=(n,k+1),\\
P(n,k)&=(n+1,\lfloor k/2\rfloor),\\
C_0(n,k)&=(n-1,2k), & C_1(n,k)&=(n-1,2k+1).
\end{aligned}
\]

The lower horocycle segment is split at its midpoint so the two child adjacencies are separate finite edges. Vertices at that midpoint have valence three; corner vertices have valence four. Random walks receive exactly these five neighbors through the same provider contract used for billiard transitions.

As with the order-5 provider, the hyperbolic model defines the tiling only. The simulated billiard still travels on straight Euclidean disk segments and refracts with Euclidean normals.
