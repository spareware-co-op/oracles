# ezpz 0.2.29 (MIT), Zoo's sketch solver, from crates.io as this folder's Cargo.lock pins it (every
# crate checked by its SHA-256), on a Rust image pinned by digest. Built by this repository's workflow
# and never pushed.
FROM rust:1.99.0-slim-bookworm@sha256:2c3a22f0a5533ea2dd5a16627bc841228151faa2d4de2644ac9987e4a2f1f2fa
COPY Cargo.toml Cargo.lock /oracle/
COPY src /oracle/src
RUN cargo build --release --locked --manifest-path /oracle/Cargo.toml \
    && cp /oracle/target/release/ezpz-oracle /oracle/harness \
    && rm -rf /oracle/target
ENTRYPOINT ["/oracle/harness"]
