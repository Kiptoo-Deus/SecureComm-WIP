package main

import (
	"encoding/json"
	"flag"
	"log"
	"net/http"
	"sync"
	"time"

	"github.com/gorilla/websocket"
)

var (
	addr       = flag.String("addr", ":8080", "HTTP service address")
	mpesaAPI   = flag.String("mpesa-api", "https://sandbox.safaricom.co.ke", "M-Pesa API endpoint")
	consumerKey = flag.String("consumer-key", "", "M-Pesa consumer key")
	consumerSecret = flag.String("consumer-secret", "", "M-Pesa consumer secret")
)

var upgrader = websocket.Upgrader{
	CheckOrigin: func(r *http.Request) bool { return true },
}

type Message struct {
	Type      string    `json:"type"`      // "message", "payment", "ack"
	MessageID string    `json:"message_id"`
	SenderID  string    `json:"sender_id"`
	Recipient string    `json:"recipient"`
	Payload   []byte    `json:"payload"`
	Timestamp int64     `json:"timestamp"`
}

type PaymentRequest struct {
	Type         string `json:"type"` // "stk_push"
	PhoneNumber  string `json:"phone_number"`
	Amount       int    `json:"amount"`
	Reference    string `json:"reference"`
	Description  string `json:"description"`
	CallbackURL  string `json:"callback_url"`
}

type Client struct {
	ID     string
	Conn   *websocket.Conn
	Send   chan []byte
	Online bool
}

type Hub struct {
	mu      sync.RWMutex
	clients map[string]*Client
	queue   map[string][]Message // Offline message queue
}

func NewHub() *Hub {
	return &Hub{
		clients: make(map[string]*Client),
		queue:   make(map[string][]Message),
	}
}

func (h *Hub) register(client *Client) {
	h.mu.Lock()
	defer h.mu.Unlock()
	h.clients[client.ID] = client
	client.Online = true
	
	log.Printf("[Hub] Client registered: %s", client.ID)
	
	// Deliver queued messages
	if queued, ok := h.queue[client.ID]; ok {
		log.Printf("[Hub] Delivering %d queued messages to %s", len(queued), client.ID)
		for _, msg := range queued {
			if data, err := json.Marshal(msg); err == nil {
				select {
				case client.Send <- data:
				default:
					log.Printf("[Hub] Failed to deliver queued message to %s", client.ID)
				}
			}
		}
		delete(h.queue, client.ID)
	}
}

func (h *Hub) unregister(id string) {
	h.mu.Lock()
	defer h.mu.Unlock()
	if client, ok := h.clients[id]; ok {
		client.Online = false
		close(client.Send)
		log.Printf("[Hub] Client unregistered: %s", id)
	}
	delete(h.clients, id)
}

func (h *Hub) send(id string, data []byte) bool {
	h.mu.RLock()
	client, ok := h.clients[id]
	h.mu.RUnlock()
	
	if ok && client.Online {
		select {
		case client.Send <- data:
			return true
		default:
			log.Printf("[Hub] Send channel full for %s", id)
			return false
		}
	}
	return false
}

func (h *Hub) queueMessage(msg Message) {
	h.mu.Lock()
	defer h.mu.Unlock()
	h.queue[msg.Recipient] = append(h.queue[msg.Recipient], msg)
	log.Printf("[Hub] Queued message for offline client: %s", msg.Recipient)
}

func (h *Hub) processMessage(data []byte) {
	var msg Message
	if err := json.Unmarshal(data, &msg); err != nil {
		log.Printf("[Hub] Failed to unmarshal message: %v", err)
		return
	}
	
	msg.Timestamp = time.Now().Unix()
	
	// Handle payment requests
	if msg.Type == "payment" {
		var payment PaymentRequest
		if err := json.Unmarshal(msg.Payload, &payment); err == nil {
			go h.processPayment(payment, msg.SenderID)
		}
		return
	}
	
	// Try to send immediately
	msgData, _ := json.Marshal(msg)
	if !h.send(msg.Recipient, msgData) {
		// Queue for later delivery
		h.queueMessage(msg)
		log.Printf("[Hub] Message from %s to %s queued (offline)", msg.SenderID, msg.Recipient)
	} else {
		log.Printf("[Hub] Message delivered from %s to %s", msg.SenderID, msg.Recipient)
	}
}

func (h *Hub) processPayment(payment PaymentRequest, senderID string) {
	log.Printf("[Hub] Processing payment: %+v from %s", payment, senderID)
	
	// Simulate M-Pesa STK Push
	// In production, call actual M-Pesa API:
	// POST https://api.safaricom.co.ke/mpesa/stkpush/v1/processrequest
	// Headers: Authorization: Bearer <token>
	// Body: {
	//   "BusinessShortCode": "174379",
	//   "Password": "<encoded>",
	//   "Timestamp": "20191219102345",
	//   "TransactionType": "CustomerPayBillOnline",
	//   "Amount": "<amount>",
	//   "PartyA": "<phone>",
	//   "PartyB": "174379",
	//   "PhoneNumber": "<phone>",
	//   "CallBackURL": "<callback>",
	//   "AccountReference": "<reference>",
	//   "TransactionDesc": "<description>"
	// }
	
	time.Sleep(2 * time.Second)
	
	// Send payment confirmation
	response := map[string]interface{}{
		"type":        "payment_confirmation",
		"success":     true,
		"reference":   payment.Reference,
		"amount":      payment.Amount,
		"timestamp":   time.Now().Unix(),
		"description": "Payment processed successfully (simulated)",
	}
	
	responseData, _ := json.Marshal(response)
	
	msg := Message{
		Type:      "payment_ack",
		SenderID:  "server",
		Recipient: senderID,
		Payload:   responseData,
		Timestamp: time.Now().Unix(),
	}
	
	msgData, _ := json.Marshal(msg)
	if !h.send(senderID, msgData) {
		h.queueMessage(msg)
		log.Printf("[Hub] Payment ACK queued for %s", senderID)
	}
}

func serveWS(hub *Hub, w http.ResponseWriter, r *http.Request) {
	id := r.URL.Query().Get("id")
	if id == "" {
		http.Error(w, "missing id", http.StatusBadRequest)
		return
	}
	
	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Printf("[Hub] Upgrade error: %v", err)
		return
	}
	
	client := &Client{
		ID:   id,
		Conn: conn,
		Send: make(chan []byte, 256),
	}
	
	hub.register(client)
	
	// Read pump
	go func() {
		defer func() {
			hub.unregister(id)
			conn.Close()
		}()
		
		for {
			_, message, err := conn.ReadMessage()
			if err != nil {
				if websocket.IsUnexpectedCloseError(err, 
					websocket.CloseGoingAway, 
					websocket.CloseAbnormalClosure) {
					log.Printf("[Hub] Read error: %v", err)
				}
				break
			}
			hub.processMessage(message)
		}
	}()
	
	// Write pump
	go func() {
		defer conn.Close()
		
		for message := range client.Send {
			if err := conn.WriteMessage(websocket.TextMessage, message); err != nil {
				log.Printf("[Hub] Write error: %v", err)
				break
			}
		}
	}()
}

func main() {
	flag.Parse()
	
	hub := NewHub()
	
	http.HandleFunc("/ws", func(w http.ResponseWriter, r *http.Request) {
		serveWS(hub, w, r)
	})
	
	http.HandleFunc("/health", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(map[string]interface{}{
			"status":    "healthy",
			"timestamp": time.Now().Unix(),
			"clients":   len(hub.clients),
		})
	})
	
	http.HandleFunc("/stats", func(w http.ResponseWriter, r *http.Request) {
		hub.mu.RLock()
		defer hub.mu.RUnlock()
		
		queued_count := 0
		for _, msgs := range hub.queue {
			queued_count += len(msgs)
		}
		
		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(map[string]interface{}{
			"online_clients":  len(hub.clients),
			"queued_messages": queued_count,
			"timestamp":       time.Now().Unix(),
		})
	})
	
	log.Printf("Server starting on %s", *addr)
	if err := http.ListenAndServe(*addr, nil); err != nil {
		log.Fatal("ListenAndServe: ", err)
	}
}
