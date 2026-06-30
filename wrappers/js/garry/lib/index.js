const addon = require('../build/Release/garry.node');

class Database {
    constructor(path, config) {
        if (config) {
            this.db = new addon.GarryDatabase(path, config);
        } else {
            this.db = new addon.GarryDatabase(path);
        }
    }

    get(key) {
        const buf = this.db.get(toBuffer(key));
        return buf ? fromBuffer(buf) : null;
    }

    set(key, value) {
        this.db.set(toBuffer(key), toBuffer(value));
    }

    delete(key) {
        this.db.delete(toBuffer(key));
    }

    exists(key) {
        return this.db.exists(toBuffer(key));
    }

    getData(key) {
        return this.db.getData(toBuffer(key));
    }

    beginTransaction() {
        return new Transaction(this.db.beginTransaction());
    }

    cursor(prefix) {
        const buf = prefix ? toBuffer(prefix) : null;
        return new Cursor(this.db.openCursor(buf));
    }

    close() {
        this.db.close();
    }
}

class Transaction {
    constructor(handle) {
        this.txn = handle;
    }

    get(key) {
        const buf = this.txn.get(toBuffer(key));
        return buf ? fromBuffer(buf) : null;
    }

    set(key, value) {
        this.txn.set(toBuffer(key), toBuffer(value));
    }

    delete(key) {
        this.txn.delete(toBuffer(key));
    }

    exists(key) {
        return this.txn.exists(toBuffer(key));
    }

    getData(key) {
        return this.txn.getData(toBuffer(key));
    }

    commit() {
        this.txn.commit();
    }

    rollback() {
        this.txn.rollback();
    }
}

class Cursor {
    constructor(handle) {
        this.cursor = handle;
    }

    next() {
        const entry = this.cursor.next();
        return entry ? { key: fromBuffer(entry.key), value: fromBuffer(entry.value) } : null;
    }

    close() {
        this.cursor.close();
    }

    [Symbol.iterator]() {
        return {
            next: () => {
                const entry = this.cursor.next();
                if (!entry) return { done: true };
                return { value: { key: fromBuffer(entry.key), value: fromBuffer(entry.value) } };
            },
            return: () => {
                this.cursor.close();
                return { done: true };
            }
        };
    }
}

function toBuffer(val) {
    if (Buffer.isBuffer(val)) return val;
    if (typeof val === 'string') return Buffer.from(val, 'utf8');
    if (val instanceof Uint8Array) return Buffer.from(val);
    throw new TypeError('Expected string or Buffer');
}

function fromBuffer(buf) {
    return buf.toString('utf8');
}

module.exports = { Database, Transaction, Cursor, toBuffer, fromBuffer };
