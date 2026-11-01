# SMLib's NLib layer (Apache-2.0), compiled from usd-brep's pinned commit by build.sh, the routines
# decision 32.23 bars cut out first, with nlohmann/json 3.12.0 (MIT) pinned by SHA-256, on a compiler
# image pinned by digest. Built by this repository's workflow and never pushed.
FROM gcc:14.3.0@sha256:4e2d22617a3b4af1ad0150ba6ca3cb8a975c44c1abac6f9a09870b93cb5c363b
ADD --checksum=sha256:aaf127c04cb31c406e5b04a63f1ae89369fccde6d8fa7cdda1ed4f32dfc5de63 https://github.com/nlohmann/json/releases/download/v3.12.0/json.hpp /oracle/json.hpp
COPY build.sh harness.cpp stubs.cpp /oracle/
RUN sh /oracle/build.sh
ENTRYPOINT ["/oracle/harness"]
