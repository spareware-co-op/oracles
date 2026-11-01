# Manifold v3.5.4 (Apache-2.0), built from its tag's commit on a GCC image pinned by digest. Built by
# this repository's workflow and never pushed.
FROM gcc:13.3.0@sha256:1d71f0f3450214bef38fe09e6f610fb6cca90cf97b43f4ce845bfc32a4168818
RUN apt-get update && apt-get install -y --no-install-recommends cmake nlohmann-json3-dev
RUN git init /src && cd /src \
    && git remote add origin https://github.com/elalish/manifold \
    && git fetch --depth 1 origin ce50d78021d64507f89e8c9fc2c2e51018117857 \
    && git checkout FETCH_HEAD \
    && cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
        -DMANIFOLD_TEST=OFF -DMANIFOLD_CBIND=OFF -DMANIFOLD_PYBIND=OFF \
        -DMANIFOLD_CROSS_SECTION=OFF -DMANIFOLD_PAR=OFF \
    && cmake --build build -j4 && cmake --install build
COPY harness.cpp /oracle/
RUN g++ -std=c++17 -O2 /oracle/harness.cpp -o /oracle/harness -lmanifold
ENTRYPOINT ["/oracle/harness"]
