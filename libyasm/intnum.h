#ifndef YASM_INTNUM_H
#define YASM_INTNUM_H
///
/// @file libyasm/intnum.h
/// @brief YASM integer number interface.
///
/// @license
///  Copyright (C) 2001-2007  Peter Johnson
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions
/// are met:
///  - Redistributions of source code must retain the above copyright
///    notice, this list of conditions and the following disclaimer.
///  - Redistributions in binary form must reproduce the above copyright
///    notice, this list of conditions and the following disclaimer in the
///    documentation and/or other materials provided with the distribution.
///
/// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
/// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
/// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
/// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
/// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
/// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
/// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
/// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
/// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
/// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
/// POSSIBILITY OF SUCH DAMAGE.
/// @endlicense
///
#include <cstdlib>
#include <iosfwd>

#include "bitvect.h"
#include "operator.h"


namespace yasm {

class IntNum {
    friend std::ostream& operator<< (std::ostream &os, const IntNum &intn);
    friend int compare(const IntNum& intn1, const IntNum& intn2);
    friend bool operator==(const IntNum& lhs, const IntNum& rhs);
    friend bool operator<(const IntNum& lhs, const IntNum& rhs);
    friend bool operator>(const IntNum& lhs, const IntNum& rhs);

public:
    /// Default constructor.  Initializes value to 0.
    IntNum() : m_type(INTNUM_L) { m_val.l = 0; }

    /// Create a new intnum from a decimal/binary/octal/hexidecimal string.
    explicit IntNum(char *str, int base=10);

    /// Create a new intnum from an unsigned integer value.
    /// @param i        unsigned integer value
    IntNum(unsigned long i) : m_type(INTNUM_L) { set(i); }

    /// Create a new intnum from an signed integer value.
    /// @param i        signed integer value
    IntNum(long i) : m_type(INTNUM_L) { m_val.l = i; }

    /// Create a new intnum from an unsigned integer value.
    /// @param i        unsigned integer value
    IntNum(unsigned int i) : m_type(INTNUM_L) { set(i); }

    /// Create a new intnum from an signed integer value.
    /// @param i        signed integer value
    IntNum(int i) : m_type(INTNUM_L) { m_val.l = (long)i; }

    /// Create a new intnum from LEB128-encoded form.
    /// @param ptr      pointer to start of LEB128 encoded form
    /// @param sign     signed (true) or unsigned (false) LEB128 format
    /// @param size     number of bytes read from ptr (output)
    /// @return Number of bytes read returned into size parameter.
    IntNum(const unsigned char *ptr, bool sign, /*@out@*/ unsigned long &size);

    /// Create a new intnum from a little-endian or big-endian buffer.
    /// In little endian, the LSB is in ptr[0].
    /// @param ptr          pointer to start of buffer
    /// @param sign         signed (true) or unsigned (false) source
    /// @param srcsize      source buffer size (in bytes)
    /// @param bigendian    endianness (true=big, false=little)
    IntNum(const unsigned char *ptr, bool sign, size_t srcsize,
           bool bigendian);

    /// Copy constructor.
    IntNum(const IntNum &rhs);
    /// Assignment operators.
    IntNum& operator= (const IntNum& rhs);

    /// Get an allocated copy.
    IntNum *clone() const { return new IntNum(*this); }

    /// Destructor.
    ~IntNum()
    {
        if (m_type == INTNUM_BV)
            BitVector::Destroy(m_val.bv);
    }

#if 0
    /// Convert character constant to integer value, using NASM rules.
    /// NASM syntax supports automatic conversion from strings such as
    /// 'abcd' to a 32-bit integer value.  This function performs those
    /// conversions.
    /// @param str      character constant string
    /// @return Newly allocated intnum.
    /*@only@*/ yasm_intnum *yasm_intnum_create_charconst_nasm(const char *str);
#endif

    /// Floating point calculation function: acc = acc op operand.
    /// @note Not all operations in Op::Op may be supported; unsupported
    ///       operations will result in an error.
    /// @param op       operation
    /// @param operand  intnum operand
    void calc(Op::Op op, const IntNum &operand) { calc(op, &operand); }
    void calc(Op::Op op, /*@null@*/ const IntNum *operand = 0);

    /// Zero an intnum.
    void zero() { set(0); }

    /// Set an intnum to an unsigned integer.
    /// @param val      integer value
    void set(unsigned long val);

    /// Set an intnum to an signed integer.
    /// @param val      integer value
    void set(long val)
    {
        if (m_type == INTNUM_BV)
            BitVector::Destroy(m_val.bv);
        m_type = INTNUM_L;
        m_val.l = val;
    }

    /// Set an intnum to an unsigned integer.
    /// @param val      integer value
    void set(unsigned int val) { set((unsigned long)val); }

    /// Set an intnum to an signed integer.
    /// @param val      integer value
    void set(int val) { set(long(val)); }

    /// Simple value check for 0.
    /// @return True if acc==0.
    bool is_zero() const
    { return (m_type == INTNUM_L && m_val.l == 0); }

    /// Simple value check for 1.
    /// @param acc      intnum
    /// @return True if acc==1.
    bool is_pos1() const
    { return (m_type == INTNUM_L && m_val.l == 1); }

    /// Simple value check for -1.
    /// @return True if acc==-1.
    bool is_neg1() const
    { return (m_type == INTNUM_L && m_val.l == -1); }

    /// Simple sign check.
    /// @return -1 if negative, 0 if zero, +1 if positive
    int sign() const;

    /// Convert an intnum to an unsigned 32-bit value.
    /// The value is in "standard" C format (eg, of unknown endian).
    /// @note Parameter intnum is truncated to fit into 32 bits.  Use
    ///       intnum_check_size() to check for overflow.
    /// @return Unsigned 32-bit value of intn.
    unsigned long get_uint() const;

    /// Convert an intnum to a signed 32-bit value.
    /// The value is in "standard" C format (eg, of unknown endian).
    /// @note Parameter intnum is truncated to fit into 32 bits.  Use
    ///       intnum_check_size() to check for overflow.
    /// @return Signed 32-bit value of intn.
    long get_int() const;

    /// Output #yasm_intnum to buffer in little-endian or big-endian.
    /// Puts the value into the least significant bits of the destination,
    /// or may be shifted into more significant bits by the shift parameter.
    /// The destination bits are cleared before being set.
    /// [0] should be the first byte output to the file.
    /// @param ptr          pointer to storage for size bytes of output
    /// @param destsize     destination size (in bytes)
    /// @param valsize      size (in bits)
    /// @param shift        left shift (in bits); may be negative to specify
    ///                     right shift (standard warnings include truncation
    ///                     to boundary)
    /// @param bigendian    endianness (true=big, false=little)
    /// @param warn         enables standard warnings (value doesn't fit into
    ///                     valsize bits): <0=signed warnings,
    ///                     >0=unsigned warnings, 0=no warn
    void get_sized(unsigned char *ptr,
                   size_t destsize,
                   size_t valsize,
                   int shift,
                   bool bigendian,
                   int warn) const;

    /// Check to see if intnum will fit without overflow into size bits.
    /// @param size         number of bits of output space
    /// @param rshift       right shift
    /// @param rangetype    signed/unsigned range selection:
    ///                     0 => (0, unsigned max);
    ///                     1 => (signed min, signed max);
    ///                     2 => (signed min, unsigned max)
    /// @return True if intnum will fit.
    bool ok_size(size_t size, size_t rshift, int rangetype) const;

    /// Check to see if intnum will fit into a particular numeric range.
    /// @param low      low end of range (inclusive)
    /// @param high     high end of range (inclusive)
    /// @return True if intnum is within range.
    bool in_range(long low, long high) const;

    /// Output intnum to buffer in LEB128-encoded form.
    /// @param ptr      pointer to storage for output bytes
    /// @param sign     signedness of LEB128 encoding (false=unsigned,
    ///                 true=signed)
    /// @return Number of bytes generated.
    unsigned long get_leb128(unsigned char *ptr, bool sign) const;

    /// Calculate number of bytes LEB128-encoded form of intnum will take.
    /// @param intn     intnum
    /// @param sign     signedness of LEB128 encoding (false=unsigned,
    ///                 true=signed)
    /// @return Number of bytes.
    unsigned long size_leb128(bool sign) const;

    /// Get an intnum as a signed decimal string.  The returned string will
    /// contain a leading '-' if the intnum is negative.
    /// @return String containing the decimal representation of the intnum.
    /*@only@*/ char *get_str() const;

    /// Overloaded unary operators:

    /// Prefix increment.
    IntNum& operator++();
    /// Postfix increment.
    IntNum operator++(int) { IntNum old(*this); ++*this; return old; }
    /// Prefix decrement.
    IntNum& operator--();
    /// Postfix decrement.
    IntNum operator--(int) { IntNum old(*this); --*this; return old; }

private:
    // Compress a bitvector into intnum storage.
    // If saved as a bitvector, clones the passed bitvector.
    // Can modify the passed bitvector.
    // @param bv        bitvector
    void from_bv(BitVector::wordptr bv);

    // If intnum is a BV, returns its bitvector directly.
    // If not, converts into passed bv and returns that instead.
    // @param bv        bitvector to use if intnum is not bitvector.
    // @return Passed bv or intnum internal bitvector.
    BitVector::wordptr to_bv(/*@returned@*/ BitVector::wordptr bv) const;

    union {
        long l;                 ///< integer value (for integers <=32 bits)
        BitVector::wordptr bv;  ///< bit vector (for integers >32 bits)
    } m_val;
    enum { INTNUM_L, INTNUM_BV } m_type;
};

/// Overloaded assignment binary operators.
inline IntNum& operator+=(IntNum& lhs, const IntNum& rhs)
{ lhs.calc(Op::ADD, &rhs); return lhs; }
inline IntNum& operator-=(IntNum& lhs, const IntNum& rhs)
{ lhs.calc(Op::SUB, &rhs); return lhs; }
inline IntNum& operator*=(IntNum& lhs, const IntNum& rhs)
{ lhs.calc(Op::MUL, &rhs); return lhs; }
inline IntNum& operator/=(IntNum& lhs, const IntNum& rhs)
{ lhs.calc(Op::DIV, &rhs); return lhs; }
inline IntNum& operator%=(IntNum& lhs, const IntNum& rhs)
{ lhs.calc(Op::MOD, &rhs); return lhs; }
inline IntNum& operator^=(IntNum& lhs, const IntNum& rhs)
{ lhs.calc(Op::XOR, &rhs); return lhs; }
inline IntNum& operator&=(IntNum& lhs, const IntNum& rhs)
{ lhs.calc(Op::AND, &rhs); return lhs; }
inline IntNum& operator|=(IntNum& lhs, const IntNum& rhs)
{ lhs.calc(Op::OR, &rhs); return lhs; }
inline IntNum& operator>>=(IntNum& lhs, const IntNum& rhs)
{ lhs.calc(Op::SHR, &rhs); return lhs; }
inline IntNum& operator<<=(IntNum& lhs, const IntNum& rhs)
{ lhs.calc(Op::SHL, &rhs); return lhs; }

/// Overloaded unary operators.
inline IntNum operator-(IntNum rhs)
{ rhs.calc(Op::NEG, 0); return rhs; }
inline IntNum operator+(const IntNum& rhs)
{ return IntNum(rhs); }
inline IntNum operator~(IntNum rhs)
{ rhs.calc(Op::NOT, 0); return rhs; }
inline bool operator!(const IntNum& rhs)
{ return rhs.is_zero(); }

/// Overloaded binary operators.
inline const IntNum operator+(IntNum lhs, const IntNum& rhs)
{ return lhs += rhs; }
inline const IntNum operator-(IntNum lhs, const IntNum& rhs)
{ return lhs -= rhs; }
inline const IntNum operator*(IntNum lhs, const IntNum& rhs)
{ return lhs *= rhs; }
inline const IntNum operator/(IntNum lhs, const IntNum& rhs)
{ return lhs /= rhs; }
inline const IntNum operator%(IntNum lhs, const IntNum& rhs)
{ return lhs %= rhs; }
inline const IntNum operator^(IntNum lhs, const IntNum& rhs)
{ return lhs ^= rhs; }
inline const IntNum operator&(IntNum lhs, const IntNum& rhs)
{ return lhs &= rhs; }
inline const IntNum operator|(IntNum lhs, const IntNum& rhs)
{ return lhs |= rhs; }
inline const IntNum operator>>(IntNum lhs, const IntNum& rhs)
{ return lhs >>= rhs; }
inline const IntNum operator<<(IntNum lhs, const IntNum& rhs)
{ return lhs <<= rhs; }

/// Compare two intnums.
/// @param lhs      first intnum
/// @param rhs      second intnum
/// @return -1 if lhs < rhs, 0 if lhs == rhs, 1 if lhs > rhs.
int compare(const IntNum& lhs, const IntNum& rhs);

/// Overloaded comparison operators.
bool operator==(const IntNum& lhs, const IntNum& rhs);
bool operator<(const IntNum& lhs, const IntNum& rhs);
bool operator>(const IntNum& lhs, const IntNum& rhs);

inline bool operator!=(const IntNum& lhs, const IntNum& rhs)
{ return !(lhs == rhs); }
inline bool operator<=(const IntNum& lhs, const IntNum& rhs)
{ return !(lhs > rhs); }
inline bool operator>=(const IntNum& lhs, const IntNum& rhs)
{ return !(lhs < rhs); }

/// Output integer to buffer in signed LEB128-encoded form.
/// @param v        integer
/// @param ptr      pointer to storage for output bytes
/// @return Number of bytes generated.
unsigned long get_sleb128(long v, unsigned char *ptr);

/// Calculate number of bytes signed LEB128-encoded form of integer will take.
/// @param v        integer
/// @return Number of bytes.
unsigned long size_sleb128(long v);

/// Output integer to buffer in unsigned LEB128-encoded form.
/// @param v        integer
/// @param ptr      pointer to storage for output bytes
/// @return Number of bytes generated.
unsigned long get_uleb128(unsigned long v, unsigned char *ptr);

/// Calculate number of bytes unsigned LEB128-encoded form of integer will
/// take.
/// @param v        integer
/// @return Number of bytes.
unsigned long size_uleb128(unsigned long v);

std::ostream& operator<< (std::ostream &os, const IntNum &intn);

} // namespace yasm

#endif
