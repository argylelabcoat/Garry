package garry

import "errors"

var (
	ErrKeyPartTooLong = errors.New("garry: key part exceeds 255 bytes")
	ErrKeyTruncated   = errors.New("garry: key data truncated")
)

func EncodeKey(parts ...string) []byte {
	total := 0
	for _, p := range parts {
		b := []byte(p)
		if len(b) > 255 {
			panic(ErrKeyPartTooLong)
		}
		total += 1 + len(b)
	}
	out := make([]byte, 0, total)
	for _, p := range parts {
		b := []byte(p)
		out = append(out, byte(len(b)))
		out = append(out, b...)
	}
	return out
}

func DecodeKey(key []byte) []string {
	var parts []string
	off := 0
	for off < len(key) {
		if off+1 > len(key) {
			panic(ErrKeyTruncated)
		}
		n := int(key[off])
		off++
		if off+n > len(key) {
			panic(ErrKeyTruncated)
		}
		parts = append(parts, string(key[off:off+n]))
		off += n
	}
	return parts
}

func CombineKey(prefix []byte, additionalParts ...string) []byte {
	existing := DecodeKey(prefix)
	all := make([]string, 0, len(existing)+len(additionalParts))
	all = append(all, existing...)
	all = append(all, additionalParts...)
	return EncodeKey(all...)
}
