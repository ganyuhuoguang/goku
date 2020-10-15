package id

import (
	"crypto/md5"
	"encoding/binary"
	"fmt"
	"math/rand"
	"os"
	"time"
)

const IdByteLength = 16

var host [4]byte

func init() {
	// Set the seed of rand by timestamp
	rand.Seed(time.Now().UnixNano())
	h, _ := os.Hostname()
	sum := md5.Sum([]byte(h))
	copy(host[:], sum[:])
}

// Generate real random bytes with length 16
func realRandomBytes() [16]byte {
	var b [16]byte
	// timestamp, 4 bytes
	binary.BigEndian.PutUint32(b[:4], uint32(time.Now().UnixNano()))
	// random int32
	binary.BigEndian.PutUint32(b[4:8], rand.Uint32())
	// pid
	binary.BigEndian.PutUint32(b[8:12], uint32(os.Getpid()))
	copy(b[12:16], host[:])
	return md5.Sum(b[:])
}

func GenerateByteId() [IdByteLength]byte {
	rrb := realRandomBytes()
	var id [IdByteLength]byte
	copy(id[:], rrb[:IdByteLength])
	for i := IdByteLength; i < len(rrb); i++ {
		id[i%IdByteLength] ^= rrb[i]
	}
	return id
}

func GenerateId() string {
	return fmt.Sprintf("%x", GenerateByteId())
}
