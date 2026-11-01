# fiksi (Linebender, Apache-2.0 OR MIT), the sketch solver at the commit the kernel's chapter 15.16
# cites, from its repository as this folder's Cargo.lock pins it (crates.io dependencies checked by
# their SHA-256, fiksi's own crates by that commit), on a Rust image pinned by digest. Its licence
# texts are LICENSE-APACHE and LICENSE-MIT here. Built by this repository's workflow and never pushed.
FROM rust:1.99.0-slim-bookworm@sha256:2c3a22f0a5533ea2dd5a16627bc841228151faa2d4de2644ac9987e4a2f1f2fa
COPY Cargo.toml Cargo.lock /oracle/
COPY src /oracle/src
RUN cargo build --release --locked --manifest-path /oracle/Cargo.toml \
    && cp /oracle/target/release/fiksi-oracle /oracle/harness \
    && rm -rf /oracle/target
ENTRYPOINT ["/oracle/harness"]
