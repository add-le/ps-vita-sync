#!/bin/sh
# Build every benchmark VPK with the VitaSDK docker image. Output: build-vita/*.vpk
set -e
cd "$(dirname "$0")"
REPO="$(cd ../.. && pwd)"
docker run --rm -v "$REPO":/src -w /src/benchmarks/ui-stack vitasdk/vitasdk:latest sh -c \
  'cmake -S . -B build-vita -DCMAKE_BUILD_TYPE=Release >/dev/null && cmake --build build-vita -j4'