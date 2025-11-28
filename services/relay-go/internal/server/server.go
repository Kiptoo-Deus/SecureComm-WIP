package server

import (
    "context"
    "sync"

    pb "github.com/you/securecomm/proto"
)

type RelayServer struct {
    pb.UnimplementedRelayServer

    mu    sync.Mutex
    inbox map[string][]*pb.EncryptedMessage
}

func New() *RelayServer {
    return &RelayServer{
        inbox: make(map[string][]*pb.EncryptedMessage),
    }
}

func (s *RelayServer) RegisterDevice(ctx context.Context, req *pb.RegisterDeviceRequest) (*pb.RegisterDeviceResponse, error) {
    return &pb.RegisterDeviceResponse{
        Ack: &pb.Ack{Ok: true, Message: "device registered"},
    }, nil
}

func (s *RelayServer) SendMessage(ctx context.Context, req *pb.SendMessageRequest) (*pb.SendMessageResponse, error) {
    msg := req.GetMessage()

    s.mu.Lock()
    s.inbox[msg.ToDevice] = append(s.inbox[msg.ToDevice], msg)
    s.mu.Unlock()

    return &pb.SendMessageResponse{
        Ack: &pb.Ack{Ok: true, Message: "message delivered"},
    }, nil
}

func (s *RelayServer) PollMessages(req *pb.PollRequest, stream pb.Relay_PollMessagesServer) error {
    deviceID := req.Device.DeviceId

    s.mu.Lock()
    messages := s.inbox[deviceID]
    delete(s.inbox, deviceID)
    s.mu.Unlock()

    for _, m := range messages {
        if err := stream.Send(m); err != nil {
            return err
        }
    }

    return nil
}
