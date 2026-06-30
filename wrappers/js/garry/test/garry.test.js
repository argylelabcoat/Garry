const { describe, it, before, after } = require('node:test');
const assert = require('node:assert/strict');
const path = require('path');
const fs = require('fs');
const { Database } = require('../lib/index');

const TEST_DB = path.join(__dirname, '.test_garry.db');

function cleanup() {
    try { fs.unlinkSync(TEST_DB); } catch {}
}

describe('Database', () => {
    before(cleanup);
    after(cleanup);

    it('creates and opens a database', () => {
        const db = new Database(TEST_DB);
        assert.ok(db);
        db.close();
    });

    it('set and get', () => {
        const db = new Database(TEST_DB);
        db.set('hello', 'world');
        assert.equal(db.get('hello'), 'world');
        db.close();
    });

    it('overwrite existing key', () => {
        const db = new Database(TEST_DB);
        db.set('key', 'v1');
        assert.equal(db.get('key'), 'v1');
        db.set('key', 'v2');
        assert.equal(db.get('key'), 'v2');
        db.close();
    });

    it('returns null for missing key', () => {
        const db = new Database(TEST_DB);
        assert.equal(db.get('nonexistent'), null);
        db.close();
    });

    it('delete removes key', () => {
        const db = new Database(TEST_DB);
        db.set('to_delete', 'val');
        assert.equal(db.get('to_delete'), 'val');
        db.delete('to_delete');
        assert.equal(db.get('to_delete'), null);
        db.close();
    });

    it('exists checks key presence', () => {
        const db = new Database(TEST_DB);
        db.set('exists_key', 'val');
        assert.equal(db.exists('exists_key'), true);
        assert.equal(db.exists('no_key'), false);
        db.close();
    });

    it('accepts Buffer keys and values', () => {
        const db = new Database(TEST_DB);
        const key = Buffer.from('buf_key');
        const val = Buffer.from('binary_data');
        db.set(key, val);
        const got = db.get(key);
        assert.equal(got, 'binary_data');
        db.close();
    });

    it('creates with custom config', () => {
        const cfgPath = path.join(__dirname, '.test_garry_cfg.db');
        try {
            const db = new Database(cfgPath, { pool_size: 4, page_size: 2048 });
            db.set('cfg', 'ok');
            assert.equal(db.get('cfg'), 'ok');
            db.close();
        } finally {
            try { fs.unlinkSync(cfgPath); } catch {}
        }
    });
});
