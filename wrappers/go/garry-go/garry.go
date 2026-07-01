package garry

/*
#cgo LDFLAGS: -L../../../build -lgarry
#cgo CFLAGS: -I../../../include
#include <stdlib.h>
#include <stdint.h>
#include <garry/api.h>

extern void goGarryVisitorCB(void *key, int klen, void *val, int vlen, long long user_data);

static void call_go_visitor(garry_database *db, garry_txn txn, const garry_u8 *prefix, garry_i32 plen, long long id) {
    garry_for_each(db, txn, prefix, plen, (garry_visitor)goGarryVisitorCB, (void*)id);
}
*/
import "C"

import (
	"runtime"
	"unsafe"
)

type Database struct {
	db *C.garry_database
}

type Txn struct {
	db   *Database
	handle C.garry_txn
}

type Cursor struct {
	cur *C.garry_cursor
}

type Status int

const (
	OK                  Status = Status(C.GARRY_OK)
	ErrNotFound         Status = Status(C.GARRY_ERR_NOT_FOUND)
	ErrLockConflict     Status = Status(C.GARRY_ERR_LOCK_CONFLICT)
	ErrIO               Status = Status(C.GARRY_ERR_IO)
	ErrCorrupt          Status = Status(C.GARRY_ERR_CORRUPT)
	ErrInvalidArg       Status = Status(C.GARRY_ERR_INVALID_ARG)
	ErrNoMem            Status = Status(C.GARRY_ERR_NOMEM)
	ErrBufferTooSmall   Status = Status(C.GARRY_ERR_BUFFER_TOO_SMALL)
)

func (s Status) Error() string {
	return C.GoString(C.garry_strerror(C.garry_status_t(s)))
}

func (s Status) Is(target error) bool {
	if t, ok := target.(Status); ok {
		return s == t
	}
	return false
}

type Config struct {
	PoolSize      int32
	MaxRecordSize int32
	PageSize      int32
	MaxTxns       int32
	MaxVersions   int32
	MaxKeySize    int32
	MaxSubscripts int32
	Compression   int32
	BTreeFlags    int32
}

func DefaultConfig() Config {
	cc := C.garry_config_default()
	return Config{
		PoolSize:      int32(cc.pool_size),
		MaxRecordSize: int32(cc.max_record_size),
		PageSize:      int32(cc.page_size),
		MaxTxns:       int32(cc.max_txns),
		MaxVersions:   int32(cc.max_versions),
		MaxKeySize:    int32(cc.max_key_size),
		MaxSubscripts: int32(cc.max_subscripts),
		Compression:   int32(cc.compression),
		BTreeFlags:    int32(cc.btree_flags),
	}
}

func toCConfig(cfg Config) C.garry_config {
	return C.garry_config{
		pool_size:       C.garry_i32(cfg.PoolSize),
		max_record_size: C.garry_i32(cfg.MaxRecordSize),
		page_size:       C.garry_i32(cfg.PageSize),
		max_txns:        C.garry_i32(cfg.MaxTxns),
		max_versions:    C.garry_i32(cfg.MaxVersions),
		max_key_size:    C.garry_i32(cfg.MaxKeySize),
		max_subscripts:  C.garry_i32(cfg.MaxSubscripts),
		compression:     C.garry_i32(cfg.Compression),
		btree_flags:     C.garry_i32(cfg.BTreeFlags),
	}
}

func Create(path string) (*Database, error) {
	cpath := C.CString(path)
	defer C.free(unsafe.Pointer(cpath))
	db := C.garry_database_create(cpath)
	if db == nil {
		return nil, ErrIO
	}
	d := &Database{db: db}
	runtime.SetFinalizer(d, (*Database).Close)
	return d, nil
}

func CreateWithConfig(path string, cfg Config) (*Database, error) {
	cpath := C.CString(path)
	defer C.free(unsafe.Pointer(cpath))
	ccfg := toCConfig(cfg)
	db := C.garry_database_create_with_config(cpath, ccfg)
	if db == nil {
		return nil, ErrIO
	}
	d := &Database{db: db}
	runtime.SetFinalizer(d, (*Database).Close)
	return d, nil
}

func Open(path string) (*Database, error) {
	cpath := C.CString(path)
	defer C.free(unsafe.Pointer(cpath))
	db := C.garry_database_open(cpath)
	if db == nil {
		return nil, ErrIO
	}
	d := &Database{db: db}
	runtime.SetFinalizer(d, (*Database).Close)
	return d, nil
}

func (d *Database) Close() {
	if d.db != nil {
		runtime.SetFinalizer(d, nil)
		C.garry_database_close(d.db)
		d.db = nil
	}
}

func (d *Database) Begin() Txn {
	txn := C.garry_txn_begin(d.db)
	return Txn{db: d, handle: txn}
}

func (t Txn) Commit() {
	C.garry_txn_commit(t.db.db, t.handle)
}

func (t Txn) Rollback() {
	C.garry_txn_rollback(t.db.db, t.handle)
}

func (d *Database) Get(txn Txn, key []byte) ([]byte, error) {
	if len(key) == 0 {
		return nil, ErrInvalidArg
	}
	var vlen C.garry_i32
	ckey := (*C.garry_u8)(unsafe.Pointer(&key[0]))
	buf := make([]byte, 256)
	vlen = C.garry_i32(len(buf))
	ret := C.garry_get(d.db, txn.handle, ckey, C.garry_i32(len(key)), (*C.garry_u8)(unsafe.Pointer(&buf[0])), &vlen)
	if Status(ret) == ErrNotFound {
		return nil, ErrNotFound
	}
	if Status(ret) == ErrBufferTooSmall {
		buf = make([]byte, int(vlen))
		vlen = C.garry_i32(len(buf))
		ret = C.garry_get(d.db, txn.handle, ckey, C.garry_i32(len(key)), (*C.garry_u8)(unsafe.Pointer(&buf[0])), &vlen)
	}
	if Status(ret) != OK {
		return nil, Status(ret)
	}
	return buf[:int(vlen)], nil
}

func (d *Database) Set(txn Txn, key, value []byte) error {
	if len(key) == 0 {
		return ErrInvalidArg
	}
	ckey := (*C.garry_u8)(unsafe.Pointer(&key[0]))
	var cval *C.garry_u8
	if len(value) > 0 {
		cval = (*C.garry_u8)(unsafe.Pointer(&value[0]))
	}
	ret := C.garry_set(d.db, txn.handle, ckey, C.garry_i32(len(key)), cval, C.garry_i32(len(value)))
	if Status(ret) == OK {
		return nil
	}
	return Status(ret)
}

func (d *Database) Delete(txn Txn, key []byte) error {
	if len(key) == 0 {
		return ErrInvalidArg
	}
	ckey := (*C.garry_u8)(unsafe.Pointer(&key[0]))
	ret := C.garry_delete(d.db, txn.handle, ckey, C.garry_i32(len(key)))
	if Status(ret) == OK {
		return nil
	}
	return Status(ret)
}

func (d *Database) GetDefault(txn Txn, key, def []byte) ([]byte, error) {
	if len(key) == 0 {
		return nil, ErrInvalidArg
	}
	ckey := (*C.garry_u8)(unsafe.Pointer(&key[0]))
	var cdef *C.garry_u8
	if len(def) > 0 {
		cdef = (*C.garry_u8)(unsafe.Pointer(&def[0]))
	}
	var vlen C.garry_i32
	ret := C.garry_get_default(d.db, txn.handle, ckey, C.garry_i32(len(key)), cdef, C.garry_i32(len(def)), nil, &vlen)
	if Status(ret) != OK {
		return nil, Status(ret)
	}
	buf := make([]byte, int(vlen))
	ret = C.garry_get_default(d.db, txn.handle, ckey, C.garry_i32(len(key)), cdef, C.garry_i32(len(def)), (*C.garry_u8)(unsafe.Pointer(&buf[0])), &vlen)
	if Status(ret) != OK {
		return nil, Status(ret)
	}
	return buf[:int(vlen)], nil
}

func (d *Database) Data(txn Txn, key []byte) int32 {
	if len(key) == 0 {
		return 0
	}
	ckey := (*C.garry_u8)(unsafe.Pointer(&key[0]))
	return int32(C.garry_data(d.db, txn.handle, ckey, C.garry_i32(len(key))))
}

func (d *Database) SetStr(txn Txn, key, value string) error {
	ckey := C.CString(key)
	defer C.free(unsafe.Pointer(ckey))
	cval := C.CString(value)
	defer C.free(unsafe.Pointer(cval))
	ret := C.garry_set_str(d.db, txn.handle, ckey, cval)
	if Status(ret) == OK {
		return nil
	}
	return Status(ret)
}

func (d *Database) GetStr(txn Txn, key string) (string, error) {
	ckey := C.CString(key)
	defer C.free(unsafe.Pointer(ckey))
	buf := make([]byte, 65536)
	ret := C.garry_get_str(d.db, txn.handle, ckey, (*C.char)(unsafe.Pointer(&buf[0])), C.garry_i32(len(buf)))
	if Status(ret) == ErrNotFound {
		return "", ErrNotFound
	}
	if Status(ret) != OK {
		return "", Status(ret)
	}
	n := 0
	for n < len(buf) && buf[n] != 0 {
		n++
	}
	return string(buf[:n]), nil
}

func (d *Database) CursorOpen(txn Txn, prefix []byte) *Cursor {
	var cprefix *C.garry_u8
	var plen C.garry_i32
	if len(prefix) > 0 {
		cprefix = (*C.garry_u8)(unsafe.Pointer(&prefix[0]))
		plen = C.garry_i32(len(prefix))
	}
	cur := C.garry_cursor_open(d.db, txn.handle, cprefix, plen)
	if cur == nil {
		return nil
	}
	return &Cursor{cur: cur}
}

func (c *Cursor) Next() (key, value []byte, ok bool) {
	kbuf := make([]byte, 4096)
	vbuf := make([]byte, 65536)
	var klen, vlen C.garry_i32
	klen = C.garry_i32(len(kbuf))
	vlen = C.garry_i32(len(vbuf))
	ret := C.garry_cursor_next(c.cur, (*C.garry_u8)(unsafe.Pointer(&kbuf[0])), &klen, (*C.garry_u8)(unsafe.Pointer(&vbuf[0])), &vlen)
	if ret == 0 {
		return nil, nil, false
	}
	return kbuf[:int(klen)], vbuf[:int(vlen)], true
}

func (c *Cursor) NextKey() (key []byte, ok bool) {
	kbuf := make([]byte, 4096)
	var klen C.garry_i32
	klen = C.garry_i32(len(kbuf))
	ret := C.garry_cursor_next_key(c.cur, (*C.garry_u8)(unsafe.Pointer(&kbuf[0])), &klen)
	if ret == 0 {
		return nil, false
	}
	return kbuf[:int(klen)], true
}

func (c *Cursor) Close() {
	if c.cur != nil {
		C.garry_cursor_close(c.cur)
		c.cur = nil
	}
}

func (d *Database) First(txn Txn) ([]byte, error) {
	key := make([]byte, 4096)
	var klen C.garry_i32
	ret := C.garry_first(d.db, txn.handle, (*C.garry_u8)(unsafe.Pointer(&key[0])), &klen)
	if ret == 0 {
		return nil, ErrNotFound
	}
	return key[:int(klen)], nil
}

func (d *Database) Last(txn Txn) ([]byte, error) {
	key := make([]byte, 4096)
	var klen C.garry_i32
	ret := C.garry_last(d.db, txn.handle, (*C.garry_u8)(unsafe.Pointer(&key[0])), &klen)
	if ret == 0 {
		return nil, ErrNotFound
	}
	return key[:int(klen)], nil
}

func (d *Database) NextKey(txn Txn, after []byte) ([]byte, error) {
	var cafter *C.garry_u8
	var alen C.garry_i32
	if len(after) > 0 {
		cafter = (*C.garry_u8)(unsafe.Pointer(&after[0]))
		alen = C.garry_i32(len(after))
	}
	key := make([]byte, 4096)
	var klen C.garry_i32
	ret := C.garry_next_key(d.db, txn.handle, cafter, alen, (*C.garry_u8)(unsafe.Pointer(&key[0])), &klen)
	if ret == 0 {
		return nil, ErrNotFound
	}
	return key[:int(klen)], nil
}

func (d *Database) PrevKey(txn Txn, before []byte) ([]byte, error) {
	var cbefore *C.garry_u8
	var blen C.garry_i32
	if len(before) > 0 {
		cbefore = (*C.garry_u8)(unsafe.Pointer(&before[0]))
		blen = C.garry_i32(len(before))
	}
	key := make([]byte, 4096)
	var klen C.garry_i32
	ret := C.garry_prev_key(d.db, txn.handle, cbefore, blen, (*C.garry_u8)(unsafe.Pointer(&key[0])), &klen)
	if ret == 0 {
		return nil, ErrNotFound
	}
	return key[:int(klen)], nil
}

func (d *Database) Exists(txn Txn, key []byte) bool {
	if len(key) == 0 {
		return false
	}
	ckey := (*C.garry_u8)(unsafe.Pointer(&key[0]))
	return C.garry_exists(d.db, txn.handle, ckey, C.garry_i32(len(key))) != 0
}

func (d *Database) Count(txn Txn) int32 {
	return int32(C.garry_count(d.db, txn.handle))
}

type Visitor func(key, value []byte)

var (
	visitors   = map[int64]Visitor{}
	visitorSeq int64
)

//export goGarryVisitorCB
func goGarryVisitorCB(key unsafe.Pointer, klen C.int, val unsafe.Pointer, vlen C.int, userData C.longlong) {
	id := int64(userData)
	if fn, ok := visitors[id]; ok {
		gkey := C.GoBytes(key, klen)
		gval := C.GoBytes(val, vlen)
		fn(gkey, gval)
	}
}

func (d *Database) ForEach(txn Txn, prefix []byte, visitor Visitor) {
	var cprefix *C.garry_u8
	var plen C.garry_i32
	if len(prefix) > 0 {
		cprefix = (*C.garry_u8)(unsafe.Pointer(&prefix[0]))
		plen = C.garry_i32(len(prefix))
	}
	visitorSeq++
	id := visitorSeq
	visitors[id] = visitor
	defer delete(visitors, id)
	C.call_go_visitor(d.db, txn.handle, cprefix, plen, C.longlong(id))
}
