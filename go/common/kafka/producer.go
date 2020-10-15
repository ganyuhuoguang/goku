package kafka

import (
	"github.com/confluentinc/confluent-kafka-go/kafka"
)

type Producer struct {
	Producer *kafka.Producer
	Sync     bool
}

// address: kakfa ip:port; sync: send the message sync or async
func NewProducer(brokerAddr string, sync bool) (*Producer, error) {
	p, err := kafka.NewProducer(&kafka.ConfigMap{"bootstrap.servers": brokerAddr})
	if err != nil {
		return nil, err
	}
	producer := &Producer{
		Producer: p,
		Sync:     sync,
	}
	return producer, nil
}

// If sync, return partition else nil
func (p *Producer) Push(topic string, msgBytes []byte) (*kafka.TopicPartition, error) {
	err := p.Producer.Produce(&kafka.Message{
		TopicPartition: kafka.TopicPartition{
			Topic:     &topic,
			Partition: kafka.PartitionAny,
		},
		Value: msgBytes,
	}, nil)
	if err != nil {
		return nil, err
	}
	if p.Sync {
		return p.GetDeliveryReports()
	}
	return nil, nil
}

func (p *Producer) GetDeliveryReports() (*kafka.TopicPartition, error) {
	for e := range p.Producer.Events() {
		switch ev := e.(type) {
		case *kafka.Message:
			if ev.TopicPartition.Error != nil {
				return nil, ev.TopicPartition.Error
			} else {
				return &ev.TopicPartition, nil
			}
		case kafka.Error:
			return nil, nil
		default:
			continue
		}
	}
	return nil, nil
}

func (p *Producer) Close() {
	p.Producer.Close()
}
