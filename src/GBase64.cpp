

#include "../include/GBase64.h"
#include "../include/GData.h"

static char encode_6bits(unsigned c) {
    const char gTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                          "abcdefghijklmnopqrstuvwxyz"
                          "0123456789"
                          "+/";
    return gTable[c & 0x3F];
}

std::shared_ptr<GData> GEncodeToBase64(const void* srcPtr, size_t size, const char prefix[]) {
    if (size == 0) {
        return GData::Empty();
    }

    const size_t prefixSize = prefix ? strlen(prefix) : 0;

    // 3 bytes --> 4 chars
    // pad the tail with '=' chars
    const uint8_t* src = static_cast<const uint8_t*>(srcPtr);
    const size_t nCeilTriples = (size + 2) / 3; // including the tail if any
    const size_t totalBytes = nCeilTriples * 4;
    auto data = GData::Uninitialized(prefixSize + totalBytes + 1);
    char* dst = static_cast<char*>(data->data());

    if (prefixSize) {
        memcpy(dst, prefix, prefixSize);
        dst += prefixSize;
    }

    size_t i = 0;
    for (; i + 2 < size; i += 3) {
        const auto a = src[i],
                   b = src[i+1],
                   c = src[i+2];
        *dst++ = encode_6bits(a >> 2);
        *dst++ = encode_6bits((a << 4) | (b >> 4));
        *dst++ = encode_6bits((b << 2) | (c >> 6));
        *dst++ = encode_6bits(c);
    }
    const size_t tail = size - i;
    assert(tail <= 2);
    if (tail == 1) {
        const auto a = src[size - 1];
        *dst++ = encode_6bits(a >> 2);
        *dst++ = encode_6bits(a << 4);
        *dst++ = '=';
        *dst++ = '=';
    } else if (tail == 2) {
        const auto a = src[size - 2],
                   b = src[size - 1];
        *dst++ = encode_6bits(a >> 2);
        *dst++ = encode_6bits((a << 4) | (b >> 4));
        *dst++ = encode_6bits(b << 2);
        *dst++ = '=';
    }
    *dst++ = 0;   // null terminate string
    assert(dst - (const char*)data->data() == data->size());
    return data;
}

static int decode_to6bits(char c) {
    const struct {
        char first, last;
        char offset;
    } pairs[] = {
        'A', 'Z', 0,
        'a', 'z', 26,
        '0', '9', 52,
        '+', '+', 62,
        '/', '/', 63,
    };
    for (auto p : pairs) {
        if (c >= p.first && c <= p.last) {
            return c - p.first + p.offset;
        }
    }
    return -1;
}

std::shared_ptr<GData> GDecodeFromBase64(const void* srcPtr, size_t size) {
    auto signal_bad_input = []() {
        return nullptr;
    };
    if (size < 4) {
        return signal_bad_input();
    }

    const uint8_t* src = static_cast<const uint8_t*>(srcPtr);
    const size_t worstCaseSize = (size + 3) * 3 / 4;
    auto data = GData::Uninitialized(worstCaseSize);
    uint8_t* dst = (uint8_t*)data->data();

    auto is_ws = [](uint8_t c) {
        return c <= 32;
    };

    size_t localIndex = 0;
    char local[4];

    size_t i = 0;
    for (; i < size; ++i) {
        if (is_ws(src[i])) {
            continue;
        }
        if (src[i] == '=') {
            // we're at the logical end
            break;
        }
        int c = decode_to6bits(src[i]);
        if (c < 0) {
            return signal_bad_input();
        }
        assert(localIndex < 4);
        local[localIndex++] = (char)c;
        if (localIndex == 4) {
            *dst++ = (local[0] << 2) | (local[1] >> 4);
            *dst++ = (local[1] << 4) | (local[2] >> 2);
            *dst++ = (local[2] << 6) | local[3];
            localIndex = 0;
        }
    }
    if (i < size) {
        assert(src[i] == '=');
        if (localIndex == 2) {
            if (i + 1 >= size || src[i+1] != '=') {
                return signal_bad_input();
            }
            *dst++ = (local[0] << 2) | (local[1] >> 4);
        } else if (localIndex == 3) {
            *dst++ = (local[0] << 2) | (local[1] >> 4);
            *dst++ = (local[1] << 4) | (local[2] >> 2);
        } else {
            return signal_bad_input();
        }
    }

    size_t realSize = dst - (uint8_t*)data->data();
    assert(realSize <= data->size());
    // todo: shared subset on GData
    return GData::Copy(data->data(), realSize);
}
