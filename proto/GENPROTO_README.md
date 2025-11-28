# Protobuf code generation (SecureComm)

## Prereqs
- `protoc` (>= 3.20)
- Go: `protoc-gen-go` and `protoc-gen-go-grpc`
- Rust: `prost` + `tonic-build` (used from build script)
- TypeScript: `ts-proto` (or `protobufjs`)

## Go (example)
Install:

go install google.golang.org/protobuf/cmd/protoc-gen-go@latest
go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@latest


Generate:
protoc --proto_path=proto
--go_out=paths=source_relative:proto
--go-grpc_out=paths=source_relative:proto
proto/securecomm.proto


## Rust (prost + tonic)
Common approach: add `tonic-build` to your `build.rs` in the crate that needs generated types.

Example `build.rs` snippet:
```rust
fn main() {
  tonic_build::configure()
    .compile(&["proto/securecomm.proto"], &["proto"])
    .unwrap();
}

TypeScript (ts-proto)

Install:
npm install -D ts-proto
npm install -D protoc

Generate:

protoc --plugin=./node_modules/.bin/protoc-gen-ts_proto \
  --ts_proto_out=./proto \
  --ts_proto_opt=outputServices=grpc-js,esModuleInterop=true,outputEncodeMethods=false \
  -I proto proto/securecomm.proto



Notes

Keep proto/ as the single source of truth.

Use paths=source_relative for Go to place files alongside proto file imports.

For CI, install protoc and the generators in the pipeline.

---

### `scripts/gen-proto.sh`
Make executable (`chmod +x scripts/gen-proto.sh`)

```bash
#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "Generating protobuf stubs..."

# Go
if command -v protoc >/dev/null 2>&1; then
  protoc -I "${ROOT}/proto" \
    --go_out=paths=source_relative:"${ROOT}/proto" \
    --go-grpc_out=paths=source_relative:"${ROOT}/proto" \
    "${ROOT}/proto/securecomm.proto"
  echo "Generated Go protobuf files in proto/ (go)"
else
  echo "protoc not found. Please install protoc >= 3.20"
fi

# TypeScript/ts-proto (if installed)
if [ -f "${ROOT}/node_modules/.bin/protoc-gen-ts_proto" ]; then
  protoc -I "${ROOT}/proto" \
    --plugin="${ROOT}/node_modules/.bin/protoc-gen-ts_proto" \
    --ts_proto_out="${ROOT}/proto" \
    --ts_proto_opt=outputServices=grpc-js,esModuleInterop=true \
    "${ROOT}/proto/securecomm.proto"
  echo "Generated TypeScript protobuf files in proto/ (ts-proto)"
else
  echo "ts-proto plugin not found. Skipping TS generation."
fi

echo "Done."

