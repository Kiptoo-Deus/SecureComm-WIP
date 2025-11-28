package main

import (
    "log"
    "net"

    pb "github.com/you/securecomm/proto"
    "github.com/you/securecomm/services/relay-go/internal/server"
    "google.golang.org/grpc"
)

func main() {
    lis, err := net.Listen("tcp", ":50051")
    if err != nil {
        log.Fatalf("failed to listen: %v", err)
    }

    grpcServer := grpc.NewServer()

    srv := server.New()
    pb.RegisterRelayServer(grpcServer, srv)

    log.Println("Relay server running on :50051")
    if err := grpcServer.Serve(lis); err != nil {
        log.Fatalf("failed to serve: %v", err)
    }
}
