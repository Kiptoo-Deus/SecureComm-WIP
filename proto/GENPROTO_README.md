# SecureComm Protobuf Codegen

## Go
Install:
  go install google.golang.org/protobuf/cmd/protoc-gen-go@latest
  go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@latest

Generate:
  protoc --proto_path=proto \
    --go_out=paths=source_relative:proto \
    --go-grpc_out=paths=source_relative:proto \
    proto/securecomm.proto

## TypeScript
Install:
  npm install -D ts-proto

Generate:
  protoc --proto_path=proto \
    --plugin=./node_modules/.bin/protoc-gen-ts_proto \
    --ts_proto_out=proto \
    --ts_proto_opt=outputServices=grpc-js,esModuleInterop=true \
    proto/securecomm.proto
