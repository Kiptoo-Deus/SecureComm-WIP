package main

import (
	"flag"
	"log"
	"net/http"
	"sync"

	"github.com/gorilla/websocket"
)

var addr = flag.String("addr", ":8080", "http service address")

type Client struct {
	conn *websocket.Conn
	send chan []byte
}

var upgrader = websocket.Upgrader{
	CheckOrigin: func(r *http.Request) bool { return true },
}

type Hub struct {
	clients map[string]*Client
	mu sync.RWMutex
}

func NewHub() *Hub {
	return &Hub{clients: make(map[string]*Client)}
}

func (h *Hub) register(id string, c *Client) {
	h.mu.Lock(); defer h.mu.Unlock()
	h.clients[id] = c
}

func (h *Hub) unregister(id string) {
	h.mu.Lock(); defer h.mu.Unlock()
	delete(h.clients, id)
}

func (h *Hub) sendTo(id string, msg []byte) {
	h.mu.RLock(); defer h.mu.RUnlock()
	if c, ok := h.clients[id]; ok {
		select {
		case c.send <- msg:
		default:
		}
	}
}

func serveWS(h *Hub, w http.ResponseWriter, r *http.Request) {
	id := r.URL.Query().Get("id")
	if id == "" {
		http.Error(w, "missing id", http.StatusBadRequest)
		return
	}
	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		return
	}
	client := &Client{conn: conn, send: make(chan []byte, 256)}
	h.register(id, client)

	go func() {
		defer func(){ h.unregister(id); conn.Close() }()
		for {
			_, msg, err := conn.ReadMessage()
			if err != nil {
				return
			}
			// assume the first bytes of msg include recipient id length + recipient id
			// simple framing: first byte = idlen, next idlen bytes = recipient id, rest = payload
			if len(msg) < 1 { continue }
			idlen := int(msg[0])
			if len(msg) < 1+idlen { continue }
			recipient := string(msg[1:1+idlen])
			payload := msg[1+idlen:]
			h.sendTo(recipient, payload)
		}
	}()

	go func() {
		defer conn.Close()
		for m := range client.send {
			conn.WriteMessage(websocket.BinaryMessage, m)
		}
	}()
}

func main() {
	flag.Parse()
	h := NewHub()
	http.HandleFunc("/ws", func(w http.ResponseWriter, r *http.Request){ serveWS(h, w, r) })
	log.Println("server listening on", *addr)
	log.Fatal(http.ListenAndServe(*addr, nil))
}
