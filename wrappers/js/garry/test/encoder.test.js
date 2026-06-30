const { describe, it } = require('node:test');
const assert = require('node:assert/strict');
const { encodeKey, decodeKey, combineKey } = require('../lib/encoder');

describe('KeyEncoder', () => {
    it('encodes a single part', () => {
        const key = encodeKey('hello');
        assert.ok(Buffer.isBuffer(key));
        assert.equal(key[0], 5);
        assert.equal(key.slice(1).toString('utf8'), 'hello');
    });

    it('encodes multiple parts', () => {
        const key = encodeKey('users', 'matthew');
        assert.equal(key[0], 5);
        assert.equal(key[1], 0x75); // 'u'
        assert.equal(key[6], 7);
        assert.equal(key.slice(7, 14).toString('utf8'), 'matthew');
    });

    it('round-trips encode/decode', () => {
        const parts = ['a', 'bb', 'ccc'];
        const key = encodeKey(...parts);
        const decoded = decodeKey(key);
        assert.deepEqual(decoded, parts);
    });

    it('decodes empty buffer', () => {
        const decoded = decodeKey(Buffer.alloc(0));
        assert.deepEqual(decoded, []);
    });

    it('combineKey appends parts', () => {
        const prefix = encodeKey('users');
        const combined = combineKey(prefix, 'matthew', 'email');
        const parts = decodeKey(combined);
        assert.deepEqual(parts, ['users', 'matthew', 'email']);
    });

    it('rejects >255 byte segments', () => {
        assert.throws(() => encodeKey('x'.repeat(256)), RangeError);
    });

    it('throws on truncated data', () => {
        assert.throws(() => decodeKey(Buffer.from([5, 1, 2])));
    });
});
