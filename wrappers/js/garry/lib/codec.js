const TAG_NULL      = 0x00;
const TAG_BOOL      = 0x01;
const TAG_INT32     = 0x06;
const TAG_UINT32    = 0x07;
const TAG_FLOAT64   = 0x0B;
const TAG_STRING    = 0x0C;
const TAG_BYTES     = 0x0D;
const TAG_INT64     = 0x08;

function encode(value) {
    if (value === null || value === undefined) return Buffer.from([TAG_NULL]);
    if (typeof value === 'boolean') {
        const buf = Buffer.alloc(2);
        buf[0] = TAG_BOOL;
        buf[1] = value ? 1 : 0;
        return buf;
    }
    if (Number.isInteger(value) && value >= -2147483648 && value <= 2147483647) {
        const buf = Buffer.alloc(5);
        buf[0] = TAG_INT32;
        buf.writeInt32LE(value, 1);
        return buf;
    }
    if (Number.isInteger(value) && value >= 0 && value <= 4294967295) {
        const buf = Buffer.alloc(5);
        buf[0] = TAG_UINT32;
        buf.writeUInt32LE(value, 1);
        return buf;
    }
    if (typeof value === 'number') {
        const buf = Buffer.alloc(9);
        buf[0] = TAG_FLOAT64;
        buf.writeDoubleLE(value, 1);
        return buf;
    }
    if (typeof value === 'string') {
        const strBuf = Buffer.from(value, 'utf8');
        const buf = Buffer.alloc(1 + strBuf.length);
        buf[0] = TAG_STRING;
        strBuf.copy(buf, 1);
        return buf;
    }
    if (Buffer.isBuffer(value)) {
        const buf = Buffer.alloc(5 + value.length);
        buf[0] = TAG_BYTES;
        buf.writeInt32LE(value.length, 1);
        value.copy(buf, 5);
        return buf;
    }
    if (typeof value === 'bigint') {
        const buf = Buffer.alloc(9);
        buf[0] = TAG_INT64;
        buf.writeBigInt64LE(value, 1);
        return buf;
    }
    throw new TypeError(`Unsupported type: ${typeof value}`);
}

function decode(data) {
    if (!Buffer.isBuffer(data) || data.length === 0) {
        throw new Error('Data must be a non-empty Buffer');
    }
    const tag = data[0];
    switch (tag) {
        case TAG_NULL: return null;
        case TAG_BOOL: return data[1] !== 0;
        case TAG_INT32: return data.readInt32LE(1);
        case TAG_UINT32: return data.readUInt32LE(1);
        case TAG_FLOAT64: return data.readDoubleLE(1);
        case TAG_STRING: return data.slice(1).toString('utf8');
        case TAG_BYTES: {
            const len = data.readInt32LE(1);
            return data.slice(5, 5 + len);
        }
        case TAG_INT64: return data.readBigInt64LE(1);
        default: throw new Error(`Unknown tag: 0x${tag.toString(16)}`);
    }
}

module.exports = { encode, decode, TAG_NULL, TAG_BOOL, TAG_INT32, TAG_UINT32, TAG_FLOAT64, TAG_STRING, TAG_BYTES, TAG_INT64 };
