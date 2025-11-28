package main

import (
	"bufio"
	"fmt"
	"net"
	"os"
	"strings"
)

func sendUDP(msg string, addr string) error {
	conn, err := net.Dial("udp", addr)
	if err != nil {
		return err
	}
	defer conn.Close()
	_, err = conn.Write([]byte(msg))
	return err
}

func main() {
	reader := bufio.NewReader(os.Stdin)
	fmt.Print("Enter username: ")
	username, _ := reader.ReadString('\n')
	username = strings.TrimSpace(username)

	broker := "127.0.0.1:55000"
	_ = sendUDP("REGISTER "+username, broker)
	fmt.Println("Registered with broker at", broker)
	fmt.Println("Type messages as: <recipient> <message>")

	for {
		line, _ := reader.ReadString('\n')
		line = strings.TrimSpace(line)
		if line == "" {
			continue
		}
		parts := strings.SplitN(line, " ", 2)
		if len(parts) < 2 {
			fmt.Println("Invalid format")
			continue
		}
		msg := "MESSAGE " + parts[0] + " " + parts[1]
		if err := sendUDP(msg, broker); err != nil {
			fmt.Println("send error:", err)
		}
	}
}
