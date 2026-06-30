package garry

import (
	"encoding/binary"
	"errors"
	"math"
	"time"
)

const (
	TagNull     byte = 0x00
	TagBool     byte = 0x01
	TagByte     byte = 0x02
	TagSByte    byte = 0x03
	TagInt16    byte = 0x04
	TagUint16   byte = 0x05
	TagInt32    byte = 0x06
	TagUint32   byte = 0x07
	TagInt64    byte = 0x08
	TagUint64   byte = 0x09
	TagSingle   byte = 0x0A
	TagFloat64  byte = 0x0B
	TagString   byte = 0x0C
	TagBytes    byte = 0x0D
	TagDateTime byte = 0x0E
	TagGuid     byte = 0x0F
)

var ErrUnknownTag = errors.New("garry: unknown codec tag")

func EncodeValue(v interface{}) []byte {
	if v == nil {
		return []byte{TagNull}
	}
	switch val := v.(type) {
	case bool:
		buf := make([]byte, 2)
		buf[0] = TagBool
		if val {
			buf[1] = 1
		}
		return buf
	case byte:
		return []byte{TagByte, val}
	case int8:
		return []byte{TagSByte, byte(val)}
	case int16:
		buf := make([]byte, 3)
		buf[0] = TagInt16
		binary.LittleEndian.PutUint16(buf[1:], uint16(val))
		return buf
	case uint16:
		buf := make([]byte, 3)
		buf[0] = TagUint16
		binary.LittleEndian.PutUint16(buf[1:], val)
		return buf
	case int32:
		buf := make([]byte, 5)
		buf[0] = TagInt32
		binary.LittleEndian.PutUint32(buf[1:], uint32(val))
		return buf
	case uint32:
		buf := make([]byte, 5)
		buf[0] = TagUint32
		binary.LittleEndian.PutUint32(buf[1:], val)
		return buf
	case int64:
		buf := make([]byte, 9)
		buf[0] = TagInt64
		binary.LittleEndian.PutUint64(buf[1:], uint64(val))
		return buf
	case uint64:
		buf := make([]byte, 9)
		buf[0] = TagUint64
		binary.LittleEndian.PutUint64(buf[1:], val)
		return buf
	case float32:
		buf := make([]byte, 5)
		buf[0] = TagSingle
		binary.LittleEndian.PutUint32(buf[1:], math.Float32bits(val))
		return buf
	case float64:
		buf := make([]byte, 9)
		buf[0] = TagFloat64
		binary.LittleEndian.PutUint64(buf[1:], math.Float64bits(val))
		return buf
	case string:
		b := []byte(val)
		buf := make([]byte, 1+len(b))
		buf[0] = TagString
		copy(buf[1:], b)
		return buf
	case []byte:
		buf := make([]byte, 5+len(val))
		buf[0] = TagBytes
		binary.LittleEndian.PutUint32(buf[1:], uint32(len(val)))
		copy(buf[5:], val)
		return buf
	case time.Time:
		buf := make([]byte, 9)
		buf[0] = TagDateTime
		binary.LittleEndian.PutUint64(buf[1:], uint64(val.UnixNano()))
		return buf
	case [16]byte:
		buf := make([]byte, 17)
		buf[0] = TagGuid
		copy(buf[1:], val[:])
		return buf
	default:
		panic("garry: unsupported type for EncodeValue")
	}
}

func DecodeValue(data []byte) (interface{}, error) {
	if len(data) == 0 {
		return nil, nil
	}
	tag := data[0]
	switch tag {
	case TagNull:
		return nil, nil
	case TagBool:
		return data[1] != 0, nil
	case TagByte:
		return data[1], nil
	case TagSByte:
		return int8(data[1]), nil
	case TagInt16:
		return int16(binary.LittleEndian.Uint16(data[1:])), nil
	case TagUint16:
		return binary.LittleEndian.Uint16(data[1:]), nil
	case TagInt32:
		return int32(binary.LittleEndian.Uint32(data[1:])), nil
	case TagUint32:
		return binary.LittleEndian.Uint32(data[1:]), nil
	case TagInt64:
		return int64(binary.LittleEndian.Uint64(data[1:])), nil
	case TagUint64:
		return binary.LittleEndian.Uint64(data[1:]), nil
	case TagSingle:
		return math.Float32frombits(binary.LittleEndian.Uint32(data[1:])), nil
	case TagFloat64:
		return math.Float64frombits(binary.LittleEndian.Uint64(data[1:])), nil
	case TagString:
		return string(data[1:]), nil
	case TagBytes:
		n := int(binary.LittleEndian.Uint32(data[1:]))
		return data[5 : 5+n], nil
	case TagDateTime:
		nanos := int64(binary.LittleEndian.Uint64(data[1:]))
		return time.Unix(0, nanos), nil
	case TagGuid:
		var g [16]byte
		copy(g[:], data[1:17])
		return g, nil
	default:
		return nil, ErrUnknownTag
	}
}
