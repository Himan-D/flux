# ==============================================================================
# FLUX MULTI-STAGE HIGH-PERFORMANCE PRODUCTION DOCKERFILE
# ==============================================================================

# Stage 1: Build C++20 and Rust Engine Binaries
FROM rust:1.77-bookworm AS rust-cpp-builder
WORKDIR /build

RUN apt-get update && apt-get install -y clang cmake

COPY core-engine/ ./core-engine/
WORKDIR /build/core-engine
RUN clang++ -std=c++20 -O3 -Iinclude tests/test_pricing.cpp -o run_tests && ./run_tests
RUN cargo build --release

# Stage 2: Build Go Gateway & CLI Binary
FROM golang:1.22-bookworm AS go-builder
WORKDIR /build

COPY go.mod go.sum* ./
COPY saas-control/ ./saas-control/
COPY cli/ ./cli/

RUN go build -o /bin/flux-server ./saas-control/*.go
RUN go build -o /bin/flux ./cli/*.go

# Stage 3: Minimal Production Container (Alpine Linux)
FROM alpine:3.19
RUN apk add --no-cache ca-certificates tzdata

WORKDIR /app

COPY --from=go-builder /bin/flux-server /usr/local/bin/flux-server
COPY --from=go-builder /bin/flux /usr/local/bin/flux
COPY --from=rust-cpp-builder /build/core-engine/target/release/flux_core_runner /usr/local/bin/flux_core_runner

EXPOSE 8080

ENTRYPOINT ["flux-server"]
