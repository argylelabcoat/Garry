function encodeKey(...parts) {
    const bufs = parts.map(p => {
        if (typeof p === 'string') return Buffer.from(p, 'utf8');
        if (Buffer.isBuffer(p)) return p;
        throw new TypeError('Key part must be string or Buffer');
    });
    const lens = bufs.map(b => {
        if (b.length > 255) throw new RangeError(`Key part exceeds 255 bytes (${b.length})`);
        return b.length;
    });
    const totalLen = lens.reduce((s, l) => s + 1 + l, 0);
    const out = Buffer.alloc(totalLen);
    let off = 0;
    for (let i = 0; i < bufs.length; i++) {
        out[off++] = lens[i];
        bufs[i].copy(out, off);
        off += lens[i];
    }
    return out;
}

function decodeKey(key) {
    const buf = Buffer.isBuffer(key) ? key : Buffer.from(key, 'utf8');
    const parts = [];
    let off = 0;
    while (off < buf.length) {
        const len = buf[off++];
        if (off + len > buf.length) throw new Error('Key data truncated');
        parts.push(buf.slice(off, off + len).toString('utf8'));
        off += len;
    }
    return parts;
}

function combineKey(prefix, ...additionalParts) {
    const prefixParts = decodeKey(prefix);
    return encodeKey(...prefixParts, ...additionalParts);
}

module.exports = { encodeKey, decodeKey, combineKey };
