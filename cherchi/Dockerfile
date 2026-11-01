# Cherchi et al.'s InteractiveAndRobustMeshBooleans (MIT, with Attene's LGPL predicates), built from
# its pinned commit on a GCC image pinned by digest. Built by this repository's workflow and never
# pushed.
FROM gcc:13.3.0@sha256:1d71f0f3450214bef38fe09e6f610fb6cca90cf97b43f4ce845bfc32a4168818
RUN apt-get update && apt-get install -y --no-install-recommends cmake libtbb-dev python3
RUN git init /src && cd /src \
    && git remote add origin https://github.com/gcherchi/InteractiveAndRobustMeshBooleans \
    && git fetch --depth 1 origin 7bd6c2697695f555f9a15d7e154e084d841316f0 \
    && git checkout FETCH_HEAD \
    && git submodule update --init --recursive \
    && mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j4 mesh_booleans
COPY harness.py /oracle/
ENTRYPOINT ["python3", "/oracle/harness.py"]
