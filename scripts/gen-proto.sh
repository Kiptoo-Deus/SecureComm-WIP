#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROTO_DIR="${ROOT}/proto"

echo "🔧 Generating protobuf code..."

if ! command -v protoc >/dev/null 2>&1; then
  echo " protoc not found. Install it first."
  exit 1
fi

echo "➡️  Go codegen..."
if command -v protoc-gen-go >/dev/null 2>&1 && command -v protoc-gen-go-grpc >/dev/null 2>&1; then
  protoc --proto_path="${PROTO_DIR}" \
    --go_out=paths=source_relative:"${PROTO_DIR}" \
    --go-grpc_out=paths=source_relative:"${PROTO_DIR}" \
    "${PROTO_DIR}/securecomm.proto"
  echo " Go done."
else
  echo " Go plugins missing. Skipping."
fi

echo "  TS codegen..."
TS_PROTO="${ROOT}/node_modules/.bin/protoc-gen-ts_proto"
if [ -f "${TS_PROTO}" ]; then
  protoc --proto_path="${PROTO_DIR}" \
    --plugin="protoc-gen-ts_proto=${TS_PROTO}" \
    --ts_proto_out="${PROTO_DIR}" \
    --ts_proto_opt=outputServices=grpc-js,esModuleInterop=true \
    "${PROTO_DIR}/securecomm.proto"
  echo " TS done."
else
  echo "  ts-proto not installed. Skipping."
fi

echo " Done."
