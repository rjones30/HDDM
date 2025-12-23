#include <xstream/xdr.h>


/*
 * Minimal xdr implementation
 * should be endian agnostic
 *
 * XXX need to check for EOF, and have some kind of failbit
 * XXX throw exceptions
 */

namespace xstream {
    namespace xdr{

// OUTPUT
    
ostream& ostream::operator<<(const string &s) {
    static const char pad[4] = {0,0,0,0};
    size_t len =  s.size();
    if (len > UINT32_MAX) throw;
    *this << static_cast<uint32_t>(len);
    _sb->sputn(s.data(), len);
    size_t padlen = (-len) & 3;
    if (padlen > 0)
        _sb->sputn(pad, padlen);
    return *this;
}

ostream& ostream::operator<<(uint32_t _v) {
    uint32_t v = _v;
    unsigned char c[4];
    for (int i=0; i < 4; i++, v >>= 8) {
        c[i] = static_cast<unsigned char>(v & 0xff);
    }
    for (int i=0; i < 4; i++) {        // RFC1832 mandates msb...lsb order
        _sb->sputc(c[3 - i]);
    }
    return *this;
}

ostream& ostream::operator<<(int32_t v) {
    uint32_t n;
    std::memcpy(&n, &v, sizeof n);
    return (*this << n);
}

ostream& ostream::operator<<(int64_t v) {
    uint64_t n;
    std::memcpy(&n, &v, sizeof n);
    return (*this << n);
}

ostream& ostream::operator<<(uint64_t _v) {
    uint64_t v=_v;
    unsigned char c[8];
    for (int i=0; i < 8; i++, v >>= 8) {
        c[i] = static_cast<unsigned char>(v & 0xff);
    }
    for (int i=0; i < 8; i++) {        // RFC1832 mandates msb...lsb order
        _sb->sputc(c[7 - i]);
    }
    return *this;
}

// assume floats on this platform are IEEE-754 binary32
ostream& ostream::operator<<(float v) {
    uint32_t ui32;
    std::memcpy(&ui32, &v, sizeof ui32);
    return (*this << ui32);
}

// assume doubles on this platform are IEEE-754 binary32
ostream& ostream::operator<<(double v) {
   static_assert(sizeof(double) == 8, "Non-IEEE double");
    uint64_t bits;
    std::memcpy(&bits, &v, sizeof bits);  // portable, preserves all bits
    uint32_t hi = static_cast<uint32_t>(bits >> 32);
    uint32_t lo = static_cast<uint32_t>(bits & 0xffffffff);
    *this << hi;  // writes big-endian 32-bit word
    *this << lo;  // writes big-endian 32-bit word
    return *this;
}

// INPUT

istream& istream::operator>>(string &s) {
    uint32_t len;
    (*this) >> len;
    if (len > 0) {
        char *line = (char*)malloc(len);
        _sb->sgetn(line, len);
        s = string(line, line + len);
        free(line);

        size_t pad = (-static_cast<int32_t>(len)) & 3;  // correct XDR padding
        char dummy[4];
        if (pad) _sb->sgetn(dummy, pad);
    }
    return *this;
}

istream& istream::operator>>(uint32_t &v) {
    v=0;
    for (int i=0; i < 32; i += 8) {
        uint32_t c = static_cast<uint32_t>(_sb->sbumpc());
        v <<= 8;
        v |= c;
    }
    return *this;
}

istream& istream::operator>>(int32_t &v) {
    uint32_t _v;
    (*this) >> _v;
    v = static_cast<int32_t>(_v);
    return (*this);
}

istream& istream::operator>>(uint64_t &v) {
    v = 0;
    for (int i=0; i < 64; i += 8) {
        uint64_t c = static_cast<uint64_t>(_sb->sbumpc());
        v <<= 8;
        v |= c;
    }
    return *this;
}

istream& istream::operator>>(int64_t &v) {
    uint64_t _v;
    (*this) >> _v;
    v = static_cast<int64_t>(_v);
    return (*this);
}

istream& istream::operator>>(float &v) {
    uint32_t n;
    (*this) >> n;
    std::memcpy(&v, &n, sizeof(v));  // portable, preserves bits
    return *this;

istream& istream::operator>>(double &v) {
    uint32_t hi, lo;
    (*this) >> hi;  // most significant 32 bits
    (*this) >> lo;  // least significant 32 bits

    uint64_t bits = (static_cast<uint64_t>(hi) << 32) |
                     static_cast<uint64_t>(lo);

    std::memcpy(&v, &bits, sizeof(v));  // safe, preserves IEEE-754 bits
    return *this;
}

}//namespace xdr
}//namespace xstream
