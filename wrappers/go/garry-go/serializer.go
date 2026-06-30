package garry

import (
	"reflect"
	"strconv"
)

type KeyValuePair struct {
	Key   []byte
	Value []byte
}

func Serialize(rootKey string, obj interface{}) []KeyValuePair {
	var pairs []KeyValuePair
	rootParts := []string{rootKey}
	pairs = append(pairs, KeyValuePair{Key: EncodeKey(rootParts...), Value: []byte{}})
	serializeObject(rootParts, reflect.ValueOf(obj), &pairs)
	return pairs
}

func serializeObject(prefix []string, v reflect.Value, pairs *[]KeyValuePair) {
	t := v.Type()
	if t.Kind() == reflect.Ptr {
		v = v.Elem()
		t = v.Type()
	}
	for i := 0; i < t.NumField(); i++ {
		field := t.Field(i)
		fv := v.Field(i)

		tag := field.Tag.Get("garry")
		if tag == "-" {
			continue
		}
		name := field.Name
		if tag != "" {
			name = tag
		}

		if fv.Kind() == reflect.Ptr && fv.IsNil() {
			continue
		}

		keyParts := make([]string, len(prefix)+1)
		copy(keyParts, prefix)
		keyParts[len(prefix)] = name

		if isPrimitiveKind(fv.Kind()) {
			*pairs = append(*pairs, KeyValuePair{
				Key:   EncodeKey(keyParts...),
				Value: EncodeValue(primitiveInterface(fv)),
			})
		} else if fv.Kind() == reflect.Slice {
			*pairs = append(*pairs, KeyValuePair{
				Key:   EncodeKey(keyParts...),
				Value: []byte{},
			})
			serializeArray(keyParts, fv, pairs)
		} else if fv.Kind() == reflect.Struct || (fv.Kind() == reflect.Ptr && fv.Elem().Kind() == reflect.Struct) {
			*pairs = append(*pairs, KeyValuePair{
				Key:   EncodeKey(keyParts...),
				Value: []byte{},
			})
			serializeObject(keyParts, fv, pairs)
		}
	}
}

func serializeArray(prefix []string, v reflect.Value, pairs *[]KeyValuePair) {
	for i := 0; i < v.Len(); i++ {
		elem := v.Index(i)
		keyParts := make([]string, len(prefix)+1)
		copy(keyParts, prefix)
		keyParts[len(prefix)] = strconv.Itoa(i)

		if isPrimitiveKind(elem.Kind()) {
			*pairs = append(*pairs, KeyValuePair{
				Key:   EncodeKey(keyParts...),
				Value: EncodeValue(primitiveInterface(elem)),
			})
		} else if elem.Kind() == reflect.Slice {
			*pairs = append(*pairs, KeyValuePair{
				Key:   EncodeKey(keyParts...),
				Value: []byte{},
			})
			serializeArray(keyParts, elem, pairs)
		} else if elem.Kind() == reflect.Struct || (elem.Kind() == reflect.Ptr && elem.Elem().Kind() == reflect.Struct) {
			*pairs = append(*pairs, KeyValuePair{
				Key:   EncodeKey(keyParts...),
				Value: []byte{},
			})
			serializeObject(keyParts, elem, pairs)
		}
	}
}

func Deserialize(rootKey string, pairs []KeyValuePair, target interface{}) {
	rv := reflect.ValueOf(target)
	if rv.Kind() != reflect.Ptr || rv.Elem().Kind() != reflect.Struct {
		panic("garry: target must be a pointer to struct")
	}
	for _, p := range pairs {
		if len(p.Value) == 0 {
			continue
		}
		parts := DecodeKey(p.Key)
		if len(parts) < 2 || parts[0] != rootKey {
			continue
		}
		setNested(rv.Elem(), parts[1:], p.Value)
	}
}

func setNested(v reflect.Value, parts []string, value []byte) {
	if len(parts) == 0 {
		return
	}

	t := v.Type()
	fieldName := parts[0]

	fieldIdx := -1
	for i := 0; i < t.NumField(); i++ {
		f := t.Field(i)
		tag := f.Tag.Get("garry")
		name := f.Name
		if tag != "" && tag != "-" {
			name = tag
		}
		if name == fieldName {
			fieldIdx = i
			break
		}
	}

	if fieldIdx == -1 {
		return
	}

	fv := v.Field(fieldIdx)
	ft := t.Field(fieldIdx)

	if len(parts) == 1 {
		if isPrimitiveKind(fv.Kind()) {
			decoded, err := DecodeValue(value)
			if err == nil {
				setReflectValue(fv, decoded)
			}
		}
		return
	}

	if fv.Kind() == reflect.Ptr {
		if fv.IsNil() {
			fv.Set(reflect.New(ft.Type.Elem()))
		}
		setNested(fv.Elem(), parts[1:], value)
		return
	}

	if fv.Kind() == reflect.Struct {
		setNested(fv, parts[1:], value)
		return
	}

	if fv.Kind() == reflect.Slice {
		idx, err := strconv.Atoi(parts[0])
		if err != nil {
			return
		}
		if fv.Len() <= idx {
			newSlice := reflect.MakeSlice(ft.Type, idx+1, idx+1)
			reflect.Copy(newSlice, fv)
			fv.Set(newSlice)
		}
		elem := fv.Index(idx)
		if len(parts) == 1 {
			decoded, err := DecodeValue(value)
			if err == nil {
				setReflectValue(elem, decoded)
			}
		} else {
			if elem.Kind() == reflect.Struct {
				setNested(elem, parts[1:], value)
			} else if elem.Kind() == reflect.Ptr {
				if elem.IsNil() {
					elem.Set(reflect.New(ft.Type.Elem().Elem()))
				}
				setNested(elem.Elem(), parts[1:], value)
			}
		}
	}
}

func isPrimitiveKind(k reflect.Kind) bool {
	switch k {
	case reflect.Bool, reflect.Int32, reflect.Uint32, reflect.Int64,
		reflect.Float64, reflect.String:
		return true
	}
	return false
}

func primitiveInterface(v reflect.Value) interface{} {
	switch v.Kind() {
	case reflect.Bool:
		return v.Bool()
	case reflect.Int32:
		return int32(v.Int())
	case reflect.Uint32:
		return uint32(v.Uint())
	case reflect.Int64:
		return v.Int()
	case reflect.Float64:
		return v.Float()
	case reflect.String:
		return v.String()
	case reflect.Slice:
		if v.Type().Elem().Kind() == reflect.Uint8 {
			return v.Bytes()
		}
	}
	return nil
}

func setReflectValue(v reflect.Value, val interface{}) {
	if val == nil {
		return
	}
	switch v.Kind() {
	case reflect.Bool:
		v.SetBool(val.(bool))
	case reflect.Int32:
		v.SetInt(int64(val.(int32)))
	case reflect.Uint32:
		v.SetUint(uint64(val.(uint32)))
	case reflect.Int64:
		v.SetInt(val.(int64))
	case reflect.Float64:
		v.SetFloat(val.(float64))
	case reflect.String:
		v.SetString(val.(string))
	case reflect.Slice:
		if b, ok := val.([]byte); ok {
			v.SetBytes(b)
		}
	}
}
