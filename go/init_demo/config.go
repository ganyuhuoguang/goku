package main

import (
	"bufio"
	"fmt"
	"io"
	"os"
	"strconv"
	"strings"
)

type Config struct {
	Filename string
	Data     map[string]string
}

func NewConfig(filename string) (conf *Config, err error) {
	conf = &Config{
		Filename: filename,
		Data:     make(map[string]string, 100),
	}
	m, err := conf.Parse()
	if err != nil {
		return nil, err
	}
	conf.Data = m
	return conf, nil
}

func (c *Config) Parse() (m map[string]string, err error) {
	m = make(map[string]string, 100)
	file, err := os.Open(c.Filename)
	if err != nil {
		return nil, err
	}
	defer file.Close()
	var lineNo int
	reader := bufio.NewReader(file)
	for {
		line, errRet := reader.ReadString('\n')
		if errRet == io.EOF {
			break
		}
		if errRet != nil {
			err = errRet
			return nil, err
		}
		lineNo++
		line = strings.TrimSpace(line)
		if len(line) == 0 || line[0] == '\n' || line[0] == '+' || line[0] == ';' {
			continue
		}
		arr := strings.Split(line, "=")
		if len(arr) == 0 {
			fmt.Printf("invalid config,line:%d\n", lineNo)
			continue
		}
		key := strings.TrimSpace(arr[0])
		if len(key) == 0 {
			fmt.Printf("invalid config,line:%d\n", lineNo)
			continue
		}
		if len(arr) == 1 {
			m[key] = ""
			continue
		}
		value := strings.TrimSpace(arr[1])
		m[key] = value
	}
	return m, nil
}

func (c *Config) GetInt(key string) (int, error) {
	str, ok := c.Data[key]
	if !ok {
		err := fmt.Errorf("key[%s] not found", key)
		return -1, err
	}
	value, _ := strconv.Atoi(str)
	return value, nil
}

func (c *Config) GetString(key string) (string, error) {
	value, ok := c.Data[key]
	if !ok {
		err := fmt.Errorf("key[%s] not found", key)
		return "", err
	}
	return value, nil
}

func (c *Config) GetIntSlice(key string) ([]int, error) {
	str, ok := c.Data[key]
	if !ok {
		err := fmt.Errorf("key[%s] not found", key)
		return nil, err
	}
	strSlice := strings.Split(str, ",")
	if len(strSlice) == 0 {
		fmt.Printf("invalid config,line:\n")
		return nil, nil
	}
	values := make([]int, 0)
	for _, value := range strSlice {
		intValue, _ := strconv.Atoi(value)
		values = append(values, intValue)
	}
	return values, nil
}

func (c *Config) GetStringSlice(key string) ([]string, error) {
	value, ok := c.Data[key]
	if !ok {
		err := fmt.Errorf("key[%s] not found", key)
		return nil, err
	}
	values := strings.Split(value, ",")
	if len(values) == 0 {
		fmt.Printf("invalid config,line:\n")
		return nil, nil
	}
	return values, nil
}
