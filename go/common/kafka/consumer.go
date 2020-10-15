package kafka

import (
	"github.com/confluentinc/confluent-kafka-go/kafka"
)

// TODO: This version is just for demo. For the release version, Must consider the partitions.
type Consumer struct {
	Consumer   *kafka.Consumer
	AutoCommit bool
}

func NewConsumer(brokerAddr string, topics []string, groupId string, autoCommit bool) (*Consumer, error) {
	c, err := kafka.NewConsumer(&kafka.ConfigMap{
		"bootstrap.servers":  brokerAddr,
		"group.id":           groupId,
		"enable.auto.commit": autoCommit,
		"session.timeout.ms": 6000,
		"default.topic.config": kafka.ConfigMap{
			"auto.offset.reset": "earliest",
		},
	})
	if err != nil {
		return nil, err
	}
	if err = c.SubscribeTopics(topics, nil); err != nil {
		c.Close()
		return nil, err
	}
	return &Consumer{
		Consumer:   c,
		AutoCommit: autoCommit,
	}, nil
}

func (c *Consumer) Pull(interval int) (*kafka.Message, error) {
	for {
		e := c.Consumer.Poll(interval)
		if e == nil {
			continue
		}
		switch e := e.(type) {
		case *kafka.Message:
			return e, nil
		case kafka.Error:
			return nil, e
		default:
			continue
		}
	}
}

func (c *Consumer) GetMessageTopic(msg *kafka.Message) string {
	if msg == nil {
		return ""
	}
	return *msg.TopicPartition.Topic
}

func (c *Consumer) AckMsg(msg *kafka.Message) error {
	if c.AutoCommit {
		return nil
	}
	_, err := c.Consumer.CommitMessage(msg)
	return err
}

func (c *Consumer) Close() {
	c.Consumer.Close()
}
