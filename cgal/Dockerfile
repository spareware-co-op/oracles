# CGAL 6.0.1 (its packages under GPL-3.0-or-later or LGPL-3.0-or-later), used header-only from its
# tag's commit on a GCC image pinned by digest. Built by this repository's workflow and never
# pushed.
FROM gcc:13.3.0@sha256:1d71f0f3450214bef38fe09e6f610fb6cca90cf97b43f4ce845bfc32a4168818
RUN apt-get update && apt-get install -y --no-install-recommends \
    libboost-dev libgmp-dev libmpfr-dev nlohmann-json3-dev
RUN git init /src && cd /src \
    && git remote add origin https://github.com/CGAL/cgal \
    && git fetch --depth 1 origin 50cfbde3b84dbeae8338268db2d78fe4fcb522de \
    && git checkout FETCH_HEAD
COPY harness.cpp /oracle/
RUN cd /oracle && includes=$(for d in /src/*/include; do printf -- '-I%s ' "$d"; done) \
    && g++ -std=c++17 -O2 $includes -DCGAL_DISABLE_ROUNDING_MATH_CHECK harness.cpp -o harness \
        -lmpfr -lgmp
ENTRYPOINT ["/oracle/harness"]
