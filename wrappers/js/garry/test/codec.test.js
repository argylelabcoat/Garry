const { describe, it } = require('node:test');
const assert = require('node:assert/strict');
const { encode, decode, TAG_NULL, TAG_BOOL, TAG_INT32, TAG_UINT32, TAG_FLOAT64, TAG_STRING, TAG_BYTES, TAG_INT64 } = require('../lib/codec');

describe('BinaryCodec', () => {
    it('null', () => {
        const buf = encode(null);
        assert.equal(buf[0], TAG_NULL);
        assert.equal(decode(buf), null);
    });

    it('boolean true', () => {
        const buf = encode(true);
        assert.equal(buf[0], TAG_BOOL);
        assert.equal(buf[1], 1);
        assert.equal(decode(buf), true);
    });

    it('boolean false', () => {
        assert.equal(decode(encode(false)), false);
    });

    it('int32', () => {
        const buf = encode(42);
        assert.equal(buf[0], TAG_INT32);
        assert.equal(decode(buf), 42);
    });

    it('int32 negative', () => {
        assert.equal(decode(encode(-100)), -100);
    });

    it('uint32', () => {
        const buf = encode(3000000000);
        assert.equal(buf[0], TAG_UINT32);
        assert.equal(decode(buf), 3000000000);
    });

    it('float64', () => {
        const buf = encode(3.14);
        assert.equal(buf[0], TAG_FLOAT64);
        assert.ok(Math.abs(decode(buf) - 3.14) < 1e-10);
    });

    it('string', () => {
        const buf = encode('hello');
        assert.equal(buf[0], TAG_STRING);
        assert.equal(decode(buf), 'hello');
    });

    it('string empty', () => {
        assert.equal(decode(encode('')), '');
    });

    it('bytes', () => {
        const data = Buffer.from([1, 2, 3, 4]);
        const buf = encode(data);
        assert.equal(buf[0], TAG_BYTES);
        const decoded = decode(buf);
        assert.ok(Buffer.isBuffer(decoded));
        assert.deepEqual([...decoded], [1, 2, 3, 4]);
    });

    it('bigint', () => {
        const val = BigInt('9007199254740993');
        const buf = encode(val);
        assert.equal(buf[0], TAG_INT64);
        assert.equal(decode(buf), val);
    });

    it('rejects empty buffer', () => {
        assert.throws(() => decode(Buffer.alloc(0)));
    });

    it('rejects unknown tag', () => {
        assert.throws(() => decode(Buffer.from([0xFF])));
    });
});
