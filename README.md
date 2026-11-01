# Spare Kit oracles

The outside programs the Spare Kit geometry kernel is compared with, each run as a separate program in its own container. Differential testing finds what a kernel's own tests cannot: the same job goes through another system, and every difference is investigated. See chapter 28.10 of the kernel architecture in [spareware-co-op/spareware](https://github.com/spareware-co-op/spareware).

- **Never linked, never shipped.** Nothing here is compiled into the kernel or its tests. The kernel's repository holds only the jobs and the comparison code; this one holds the harnesses, each under the licence its library requires.
- **Never published.** Each image is built from pinned source by this repository's own workflow, and its layers are kept in this repository's Actions cache. No image is pushed to any registry, private ones included. The kernel's `oracle check --repository` holds this repository to that.
- **Never trusted as truth.** An oracle raises a difference; ground truth settles it.
- **Each under its program's licence.** Every oracle's folder has its own licence file, the one its program requires. Nothing here is licensed as a whole; this README and the workflow are MIT.
- **No test data stored under GPL.** Data under the GPL is fetched at test time and never kept here.
- **Numerical Recipes is never compiled.** Any build of SMLib stubs out the routines derived from it.

Each folder is one oracle: a `Dockerfile` whose base image is pinned by digest, a harness that reads one JSON job per line and answers one JSON line per job, the pins of what it installs, and a `LICENSE` file: the licence that oracle's program requires (decision 32.6), so there is no licence for the repository as a whole. The nightly workflow writes the night's jobs with the kernel's `oracle jobs`, runs the image, and judges the answers with `oracle compare`, which fails only on a difference nobody has triaged.

| Oracle | Licence of the program | Checks |
| --- | --- | --- |
| [geomdl](geomdl) | MIT | NURBS basis, evaluation, knot operations |
| [smlib-nlib](smlib-nlib) | Apache-2.0 | NLib's NURBS evaluation, knot insertion and removal and degree elevation; run twice, its answers compared bit for bit |
| [manifold](manifold) | Apache-2.0 | Mesh booleans |
| [cgal](cgal) | GPL-3.0-or-later | Mesh booleans on Nef polyhedra, compared exactly |
| [cherchi](cherchi) | MIT, with LGPL-3.0-or-later predicates | Mesh booleans |
