const { encodeKey, decodeKey } = require('./encoder');
const codec = require('./codec');

function serialize(rootKey, obj) {
    const pairs = [];
    const rootParts = [rootKey];
    pairs.push({ key: encodeKey(...rootParts), value: Buffer.alloc(0) });
    serializeObject(rootParts, obj, pairs);
    return pairs;
}

function serializeObject(prefix, obj, pairs) {
    for (const [propName, propValue] of Object.entries(obj)) {
        if (propValue === undefined) continue;
        const keyParts = [...prefix, propName];
        if (propValue === null) continue;
        if (isPrimitive(propValue)) {
            pairs.push({ key: encodeKey(...keyParts), value: codec.encode(propValue) });
        } else if (Array.isArray(propValue)) {
            pairs.push({ key: encodeKey(...keyParts), value: Buffer.alloc(0) });
            serializeArray(keyParts, propValue, pairs);
        } else if (typeof propValue === 'object') {
            pairs.push({ key: encodeKey(...keyParts), value: Buffer.alloc(0) });
            serializeObject(keyParts, propValue, pairs);
        }
    }
}

function serializeArray(prefix, arr, pairs) {
    for (let i = 0; i < arr.length; i++) {
        const element = arr[i];
        const keyParts = [...prefix, String(i)];
        if (isPrimitive(element)) {
            pairs.push({ key: encodeKey(...keyParts), value: codec.encode(element) });
        } else if (element !== null && element !== undefined) {
            pairs.push({ key: encodeKey(...keyParts), value: Buffer.alloc(0) });
            serializeObject(keyParts, element, pairs);
        }
    }
}

function deserialize(rootKey, pairs) {
    if (!pairs || pairs.length === 0) return {};
    const result = {};
    for (const { key, value } of pairs) {
        const parts = decodeKey(key);
        if (parts.length < 2 || value.length === 0) continue;
        if (parts[0] !== rootKey) continue;
        if (parts.length === 2) {
            result[parts[1]] = codec.decode(value);
        } else if (parts.length > 2) {
            setNested(result, parts.slice(1), value);
        }
    }
    return arraysFromNumericKeys(result);
}

function arraysFromNumericKeys(obj) {
    if (Array.isArray(obj)) return obj.map(arraysFromNumericKeys);
    if (obj !== null && typeof obj === 'object') {
        const keys = Object.keys(obj);
        const allNumeric = keys.length > 0 && keys.every(k => /^\d+$/.test(k));
        const out = allNumeric ? [] : {};
        for (const k of keys) {
            const val = arraysFromNumericKeys(obj[k]);
            if (allNumeric) out[parseInt(k, 10)] = val;
            else out[k] = val;
        }
        return out;
    }
    return obj;
}

function setNested(obj, parts, value) {
    if (parts.length === 1) {
        obj[parts[0]] = codec.decode(value);
    } else {
        const key = parts[0];
        if (!obj[key] || typeof obj[key] !== 'object') obj[key] = {};
        setNested(obj[key], parts.slice(1), value);
    }
}

function isPrimitive(val) {
    const t = typeof val;
    return t === 'string' || t === 'number' || t === 'boolean' || t === 'bigint'
        || val === null || Buffer.isBuffer(val);
}

module.exports = { serialize, deserialize };
