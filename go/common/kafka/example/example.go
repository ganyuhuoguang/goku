package main

import (
	"log"
	msg "novumind/goku/go/common/kafka"
)

func main() {
	// Produce message
	p, err := msg.NewProducer("192.168.3.12:9092")
	if err != nil {
		log.Printf("Error: %v\n", err)
		return
	}
	defer p.Close()
	p.Produce("test", []byte("sfasfdsdfsafsafsadf"))

	// Consume message
	c, err := msg.NewConsumer("192.168.3.12:9092", "test", "groupid")
	if err != nil {
		log.Printf("Error: %v\n", err)
		return
	}
	defer c.Close()

	msg := c.FetchMsg()
	log.Printf("Receive message: %s, offset: %d\n", string(msg.Payload), msg.Offset)
	c.AckMsg(msg)
}
