const { describe, it } = require('node:test');
const assert = require('node:assert/strict');
const { serialize, deserialize } = require('../lib/serializer');

describe('Serializer', () => {
    it('serializes flat object', () => {
        const pairs = serialize('user', { name: 'matthew', age: 30 });
        assert.ok(pairs.length >= 3);
        assert.ok(pairs[0].value.length === 0);
    });

    it('round-trips flat object', () => {
        const obj = { name: 'matthew', age: 30 };
        const pairs = serialize('user', obj);
        const result = deserialize('user', pairs);
        assert.equal(result.name, 'matthew');
        assert.equal(result.age, 30);
    });

    it('serializes nested object', () => {
        const obj = { name: 'matthew', address: { city: 'Seattle' } };
        const pairs = serialize('user', obj);
        const result = deserialize('user', pairs);
        assert.equal(result.name, 'matthew');
        assert.deepEqual(result.address, { city: 'Seattle' });
    });

    it('serializes array', () => {
        const obj = { tags: ['admin', 'user'] };
        const pairs = serialize('user', obj);
        const result = deserialize('user', pairs);
        assert.deepEqual(result.tags, ['admin', 'user']);
    });

    it('serializes mixed types', () => {
        const obj = { str: 'hi', num: 42, flag: true };
        const pairs = serialize('item', obj);
        const result = deserialize('item', pairs);
        assert.equal(result.str, 'hi');
        assert.equal(result.num, 42);
        assert.equal(result.flag, true);
    });

    it('skips undefined values', () => {
        const obj = { a: 'yes', b: undefined };
        const pairs = serialize('r', obj);
        const result = deserialize('r', pairs);
        assert.equal(result.a, 'yes');
        assert.equal(result.b, undefined);
    });

    it('deserialize returns empty for no pairs', () => {
        assert.deepEqual(deserialize('x', []), {});
    });

    describe('Deep nesting', () => {
        it('round-trips 10+ levels of nested objects', () => {
            const original = {
                child: {
                    child: {
                        child: {
                            child: {
                                child: {
                                    child: {
                                        child: {
                                            child: {
                                                child: {
                                                    deepValue: 'found it'
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            };
            const pairs = serialize('root', original);
            const restored = deserialize('root', pairs);
            assert.equal(restored.child.child.child.child.child.child.child.child.child.deepValue, 'found it');
        });
    });
});
