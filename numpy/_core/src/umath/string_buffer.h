#ifndef _NPY_CORE_SRC_UMATH_STRING_BUFFER_H_
#define _NPY_CORE_SRC_UMATH_STRING_BUFFER_H_

#include <Python.h>
#include <cstddef>
#include <wchar.h>

#define NPY_NO_DEPRECATED_API NPY_API_VERSION
#define _MULTIARRAYMODULE
#define _UMATHMODULE

#include "numpy/ndarraytypes.h"
#include "stringdtype/utf8_utils.h"
#include "string_fastsearch.h"
#include "gil_utils.h"

#define CHECK_OVERFLOW(index) if (buf + (index) >= after) return 0
#define MSB(val) ((val) >> 7 & 1)


enum class ENCODING {
    ASCII, UTF32, UTF8
};

enum class IMPLEMENTED_UNARY_FUNCTIONS {
    ISALPHA,
    ISDECIMAL,
    ISDIGIT,
    ISSPACE,
    ISALNUM,
    ISLOWER,
    ISUPPER,
    ISTITLE,
    ISNUMERIC,
    STR_LEN,
};

template <ENCODING enc>
inline npy_ucs4
getchar(const unsigned char *buf, int *bytes);


template <>
inline npy_ucs4
getchar<ENCODING::ASCII>(const unsigned char *buf, int *bytes)
{
    *bytes = 1;
    return (npy_ucs4) *buf;
}


template <>
inline npy_ucs4
getchar<ENCODING::UTF32>(const unsigned char *buf, int *bytes)
{
    *bytes = 4;
    return *(npy_ucs4 *)buf;
}

template <>
inline npy_ucs4
getchar<ENCODING::UTF8>(const unsigned char *buf, int *bytes)
{
    Py_UCS4 codepoint;
    *bytes = utf8_char_to_ucs4_code(buf, &codepoint);
    return (npy_ucs4)codepoint;
}

template<ENCODING enc>
inline bool
codepoint_isalpha(npy_ucs4 code);

template<>
inline bool
codepoint_isalpha<ENCODING::ASCII>(npy_ucs4 code)
{
    return NumPyOS_ascii_isalpha(code);
}

template<>
inline bool
codepoint_isalpha<ENCODING::UTF32>(npy_ucs4 code)
{
    return Py_UNICODE_ISALPHA(code);
}

template<>
inline bool
codepoint_isalpha<ENCODING::UTF8>(npy_ucs4 code)
{
    return Py_UNICODE_ISALPHA(code);
}

template<ENCODING enc>
inline bool
codepoint_isdigit(npy_ucs4 code);

template<>
inline bool
codepoint_isdigit<ENCODING::ASCII>(npy_ucs4 code)
{
    return NumPyOS_ascii_isdigit(code);
}

template<>
inline bool
codepoint_isdigit<ENCODING::UTF32>(npy_ucs4 code)
{
    return Py_UNICODE_ISDIGIT(code);
}

template<>
inline bool
codepoint_isdigit<ENCODING::UTF8>(npy_ucs4 code)
{
    return Py_UNICODE_ISDIGIT(code);
}

template<ENCODING enc>
inline bool
codepoint_isspace(npy_ucs4 code);

template<>
inline bool
codepoint_isspace<ENCODING::ASCII>(npy_ucs4 code)
{
    return NumPyOS_ascii_isspace(code);
}

template<>
inline bool
codepoint_isspace<ENCODING::UTF32>(npy_ucs4 code)
{
    return Py_UNICODE_ISSPACE(code);
}

template<>
inline bool
codepoint_isspace<ENCODING::UTF8>(npy_ucs4 code)
{
    return Py_UNICODE_ISSPACE(code);
}

template<ENCODING enc>
inline bool
codepoint_isalnum(npy_ucs4 code);

template<>
inline bool
codepoint_isalnum<ENCODING::ASCII>(npy_ucs4 code)
{
    return NumPyOS_ascii_isalnum(code);
}

template<>
inline bool
codepoint_isalnum<ENCODING::UTF32>(npy_ucs4 code)
{
    return Py_UNICODE_ISALNUM(code);
}

template<>
inline bool
codepoint_isalnum<ENCODING::UTF8>(npy_ucs4 code)
{
    return Py_UNICODE_ISALNUM(code);
}

template<ENCODING enc>
inline bool
codepoint_islower(npy_ucs4 code);

template<>
inline bool
codepoint_islower<ENCODING::ASCII>(npy_ucs4 code)
{
    return NumPyOS_ascii_islower(code);
}

template<>
inline bool
codepoint_islower<ENCODING::UTF32>(npy_ucs4 code)
{
    return Py_UNICODE_ISLOWER(code);
}

template<>
inline bool
codepoint_islower<ENCODING::UTF8>(npy_ucs4 code)
{
    return Py_UNICODE_ISLOWER(code);
}

template<ENCODING enc>
inline bool
codepoint_isupper(npy_ucs4 code);

template<>
inline bool
codepoint_isupper<ENCODING::ASCII>(npy_ucs4 code)
{
    return NumPyOS_ascii_isupper(code);
}

template<>
inline bool
codepoint_isupper<ENCODING::UTF32>(npy_ucs4 code)
{
    return Py_UNICODE_ISUPPER(code);
}

template<>
inline bool
codepoint_isupper<ENCODING::UTF8>(npy_ucs4 code)
{
    return Py_UNICODE_ISUPPER(code);
}

template<ENCODING enc>
inline bool
codepoint_istitle(npy_ucs4);

template<>
inline bool
codepoint_istitle<ENCODING::ASCII>(npy_ucs4 code)
{
    return false;
}

template<>
inline bool
codepoint_istitle<ENCODING::UTF32>(npy_ucs4 code)
{
    return Py_UNICODE_ISTITLE(code);
}

template<>
inline bool
codepoint_istitle<ENCODING::UTF8>(npy_ucs4 code)
{
    return Py_UNICODE_ISTITLE(code);
}

inline bool
codepoint_isnumeric(npy_ucs4 code)
{
    return Py_UNICODE_ISNUMERIC(code);
}

inline bool
codepoint_isdecimal(npy_ucs4 code)
{
    return Py_UNICODE_ISDECIMAL(code);
}


template <IMPLEMENTED_UNARY_FUNCTIONS f, ENCODING enc, typename T>
struct call_buffer_member_function;

template <ENCODING enc>
struct Buffer {
    char *buf;
    char *after;

    inline Buffer<enc>()
    {
        buf = after = NULL;
    }

    inline Buffer<enc>(char *buf_, npy_int64 elsize_)
    {
        buf = buf_;
        after = buf_ + elsize_;
    }

    inline size_t
    num_codepoints()
    {
        Buffer tmp(after, 0);
        size_t num_codepoints;
        switch (enc) {
            case ENCODING::ASCII:
            case ENCODING::UTF32:
            {
                tmp--;
                while (tmp >= *this && *tmp == '\0') {
                    tmp--;
                }
                num_codepoints = (size_t) (tmp - *this + 1);
                break;
            }
            case ENCODING::UTF8:
            {
                num_codepoints_for_utf8_bytes((unsigned char *)buf, &num_codepoints, (size_t)(after - buf));
            }
        }
        return num_codepoints;
    }

    inline Buffer<enc>&
    operator+=(npy_int64 rhs)
    {
        switch (enc) {
        case ENCODING::ASCII:
            buf += rhs;
            break;
        case ENCODING::UTF32:
            buf += rhs * sizeof(npy_ucs4);
            break;
        case ENCODING::UTF8:
            for (int i=0; i<rhs; i++) {
                buf += num_bytes_for_utf8_character((unsigned char *)buf);
            }
            break;
        }
        return *this;
    }

    inline Buffer<enc>&
    operator-=(npy_int64 rhs)
    {
        switch (enc) {
        case ENCODING::ASCII:
            buf -= rhs;
            break;
        case ENCODING::UTF32:
            buf -= rhs * sizeof(npy_ucs4);
            break;
        case ENCODING::UTF8:
            buf = (char *) find_previous_utf8_character((unsigned char *)buf, (size_t) rhs);
        }
        return *this;
    }

    inline Buffer<enc>&
    operator++()
    {
        *this += 1;
        return *this;
    }

    inline Buffer<enc>
    operator++(int)
    {
        Buffer<enc> old = *this;
        operator++();
        return old;
    }

    inline Buffer<enc>&
    operator--()
    {
        *this -= 1;
        return *this;
    }

    inline Buffer<enc>
    operator--(int)
    {
        Buffer<enc> old = *this;
        operator--();
        return old;
    }

    inline npy_ucs4
    operator*()
    {
        int bytes;
        return getchar<enc>((unsigned char *) buf, &bytes);
    }

    inline int
    buffer_memcmp(Buffer<enc> other, size_t len)
    {
        if (len == 0) {
            return 0;
        }
        switch (enc) {
            case ENCODING::ASCII:
            case ENCODING::UTF8:
                // note that len is in bytes for ASCII and UTF8 but
                // characters for UTF32
                return memcmp(buf, other.buf, len);
            case ENCODING::UTF32:
                return memcmp(buf, other.buf, len * sizeof(npy_ucs4));
        }
    }

    inline void
    buffer_memcpy(Buffer<enc> out, size_t n_chars)
    {
        if (n_chars == 0) {
            return;
        }
        switch (enc) {
            case ENCODING::ASCII:
            case ENCODING::UTF8:
                // for UTF8 we treat n_chars as number of bytes
                memcpy(out.buf, buf, n_chars);
                break;
            case ENCODING::UTF32:
                memcpy(out.buf, buf, n_chars * sizeof(npy_ucs4));
                break;
        }
    }

    inline npy_intp
    buffer_memset(npy_ucs4 fill_char, size_t n_chars)
    {
        if (n_chars == 0) {
            return 0;
        }
        switch (enc) {
            case ENCODING::ASCII:
                memset(this->buf, fill_char, n_chars);
                return n_chars;
            case ENCODING::UTF32:
            {
                char *tmp = this->buf;
                for (size_t i = 0; i < n_chars; i++) {
                    *(npy_ucs4 *)tmp = fill_char;
                    tmp += sizeof(npy_ucs4);
                }
                return n_chars;
            }
            case ENCODING::UTF8:
            {
                char utf8_c[4] = {0};
                char *tmp = this->buf;
                size_t num_bytes = ucs4_code_to_utf8_char(fill_char, utf8_c);
                for (size_t i = 0; i < n_chars; i++) {
                    memcpy(tmp, utf8_c, num_bytes);
                    tmp += num_bytes;
                }
                return num_bytes * n_chars;
            }
        }
    }

    inline void
    buffer_fill_with_zeros_after_index(size_t start_index)
    {
        Buffer<enc> offset = *this + start_index;
        for (char *tmp = offset.buf; tmp < after; tmp++) {
            *tmp = 0;
        }
    }

    inline void
    advance_chars_or_bytes(size_t n) {
        switch (enc) {
            case ENCODING::ASCII:
            case ENCODING::UTF32:
                *this += n;
                break;
            case ENCODING::UTF8:
                this->buf += n;
                break;
        }
    }

    inline size_t
    num_bytes_next_character(void) {
        switch (enc) {
            case ENCODING::ASCII:
                return 1;
            case ENCODING::UTF32:
                return 4;
            case ENCODING::UTF8:
                return num_bytes_for_utf8_character((unsigned char *)(*this).buf);
        }
    }

    template<IMPLEMENTED_UNARY_FUNCTIONS f>
    inline bool
    unary_loop()
    {
        size_t len = num_codepoints();
        if (len == 0) {
            return false;
        }

        Buffer<enc> tmp = *this;

        for (size_t i=0; i<len; i++) {
            bool result;

            call_buffer_member_function<f, enc, bool> cbmf;

            result = cbmf(tmp);

            if (!result) {
                return false;
            }
            tmp++;
        }
        return true;
    }

    inline bool
    isalpha()
    {
        return unary_loop<IMPLEMENTED_UNARY_FUNCTIONS::ISALPHA>();
    }

    inline bool
    first_character_isspace()
    {
        switch (enc) {
            case ENCODING::ASCII:
                return NumPyOS_ascii_isspace(**this);
            case ENCODING::UTF32:
            case ENCODING::UTF8:
                return Py_UNICODE_ISSPACE(**this);
        }
    }

    inline bool
    isspace()
    {
        return unary_loop<IMPLEMENTED_UNARY_FUNCTIONS::ISSPACE>();
    }

    inline bool
    isdigit()
    {
        return unary_loop<IMPLEMENTED_UNARY_FUNCTIONS::ISDIGIT>();
    }

    inline bool
    isalnum()
    {
        return unary_loop<IMPLEMENTED_UNARY_FUNCTIONS::ISALNUM>();
    }

    inline bool
    islower()
    {
        size_t len = num_codepoints();
        if (len == 0) {
            return false;
        }

        Buffer<enc> tmp = *this;
        bool cased = 0;
        for (size_t i = 0; i < len; i++) {
            if (codepoint_isupper<enc>(*tmp) || codepoint_istitle<enc>(*tmp)) {
                return false;
            }
            else if (!cased && codepoint_islower<enc>(*tmp)) {
                cased = true;
            }
            tmp++;
        }
        return cased;
    }

    inline bool
    isupper()
    {
        size_t len = num_codepoints();
        if (len == 0) {
            return false;
        }

        Buffer<enc> tmp = *this;
        bool cased = 0;
        for (size_t i = 0; i < len; i++) {
            if (codepoint_islower<enc>(*tmp) || codepoint_istitle<enc>(*tmp)) {
                return false;
            }
            else if (!cased && codepoint_isupper<enc>(*tmp)) {
                cased = true;
            }
            tmp++;
        }
        return cased;
    }

    inline bool
    istitle()
    {
        size_t len = num_codepoints();
        if (len == 0) {
            return false;
        }

        Buffer<enc> tmp = *this;
        bool cased = false;
        bool previous_is_cased = false;
        for (size_t i = 0; i < len; i++) {
            if (codepoint_isupper<enc>(*tmp) || codepoint_istitle<enc>(*tmp)) {
                if (previous_is_cased) {
                    return false;
                }
                previous_is_cased = true;
                cased = true;
            }
            else if (codepoint_islower<enc>(*tmp)) {
                if (!previous_is_cased) {
                    return false;
                }
                cased = true;
            }
            else {
                previous_is_cased = false;
            }
            tmp++;
        }
        return cased;
    }

    inline bool
    isnumeric()
    {
        return unary_loop<IMPLEMENTED_UNARY_FUNCTIONS::ISNUMERIC>();
    }

    inline bool
    isdecimal()
    {
        return unary_loop<IMPLEMENTED_UNARY_FUNCTIONS::ISDECIMAL>();
    }

    inline Buffer<enc>
    rstrip()
    {
        Buffer<enc> tmp(after, 0);
        tmp--;
        while (tmp >= *this && (*tmp == '\0' || NumPyOS_ascii_isspace(*tmp))) {
            tmp--;
        }
        tmp++;

        after = tmp.buf;
        return *this;
    }

    inline int
    strcmp(Buffer<enc> other, bool rstrip)
    {
        Buffer tmp1 = rstrip ? this->rstrip() : *this;
        Buffer tmp2 = rstrip ? other.rstrip() : other;

        while (tmp1.buf < tmp1.after && tmp2.buf < tmp2.after) {
            if (*tmp1 < *tmp2) {
                return -1;
            }
            if (*tmp1 > *tmp2) {
                return 1;
            }
            tmp1++;
            tmp2++;
        }
        while (tmp1.buf < tmp1.after) {
            if (*tmp1) {
                return 1;
            }
            tmp1++;
        }
        while (tmp2.buf < tmp2.after) {
            if (*tmp2) {
                return -1;
            }
            tmp2++;
        }
        return 0;
    }

    inline int
    strcmp(Buffer<enc> other)
    {
        return strcmp(other, false);
    }
};


template <IMPLEMENTED_UNARY_FUNCTIONS f, ENCODING enc, typename T>
struct call_buffer_member_function {
    T operator()(Buffer<enc> buf) {
        switch (f) {
            case IMPLEMENTED_UNARY_FUNCTIONS::ISALPHA:
                return codepoint_isalpha<enc>(*buf);
            case IMPLEMENTED_UNARY_FUNCTIONS::ISDIGIT:
                return codepoint_isdigit<enc>(*buf);
            case IMPLEMENTED_UNARY_FUNCTIONS::ISSPACE:
                return codepoint_isspace<enc>(*buf);
            case IMPLEMENTED_UNARY_FUNCTIONS::ISALNUM:
                return codepoint_isalnum<enc>(*buf);
            case IMPLEMENTED_UNARY_FUNCTIONS::ISNUMERIC:
                return codepoint_isnumeric(*buf);
            case IMPLEMENTED_UNARY_FUNCTIONS::ISDECIMAL:
                return codepoint_isdecimal(*buf);
        }
    }
};

template <ENCODING enc>
inline Buffer<enc>
operator+(Buffer<enc> lhs, npy_int64 rhs)
{
    switch (enc) {
        case ENCODING::ASCII:
            return Buffer<enc>(lhs.buf + rhs, lhs.after - lhs.buf - rhs);
        case ENCODING::UTF32:
            return Buffer<enc>(lhs.buf + rhs * sizeof(npy_ucs4),
                          lhs.after - lhs.buf - rhs * sizeof(npy_ucs4));
        case ENCODING::UTF8:
            char* buf = lhs.buf;
            for (int i=0; i<rhs; i++) {
                buf += num_bytes_for_utf8_character((unsigned char *)buf);
            }
            return Buffer<enc>(buf, (npy_int64)(lhs.after - buf));
    }
}


template <ENCODING enc>
inline std::ptrdiff_t
operator-(Buffer<enc> lhs, Buffer<enc> rhs)
{
    switch (enc) {
    case ENCODING::ASCII:
    case ENCODING::UTF8:
        // note for UTF8 strings this is nonsense unless we're comparing
        // two points in the same string
        return lhs.buf - rhs.buf;
    case ENCODING::UTF32:
        return (lhs.buf - rhs.buf) / (std::ptrdiff_t) sizeof(npy_ucs4);
    }
}


template <ENCODING enc>
inline Buffer<enc>
operator-(Buffer<enc> lhs, npy_int64 rhs)
{
    switch (enc) {
        case ENCODING::ASCII:
            return Buffer<enc>(lhs.buf - rhs, lhs.after - lhs.buf + rhs);
        case ENCODING::UTF32:
            return Buffer<enc>(lhs.buf - rhs * sizeof(npy_ucs4),
                          lhs.after - lhs.buf + rhs * sizeof(npy_ucs4));
        case ENCODING::UTF8:
            char* buf = lhs.buf;
            buf = (char *)find_previous_utf8_character((unsigned char *)buf, rhs);
            return Buffer<enc>(buf, (npy_int64)(lhs.after - buf));
    }

}


template <ENCODING enc>
inline bool
operator==(Buffer<enc> lhs, Buffer<enc> rhs)
{
    return lhs.buf == rhs.buf;
}


template <ENCODING enc>
inline bool
operator!=(Buffer<enc> lhs, Buffer<enc> rhs)
{
    return !(rhs == lhs);
}


template <ENCODING enc>
inline bool
operator<(Buffer<enc> lhs, Buffer<enc> rhs)
{
    return lhs.buf < rhs.buf;
}


template <ENCODING enc>
inline bool
operator>(Buffer<enc> lhs, Buffer<enc> rhs)
{
    return rhs < lhs;
}


template <ENCODING enc>
inline bool
operator<=(Buffer<enc> lhs, Buffer<enc> rhs)
{
    return !(lhs > rhs);
}


template <ENCODING enc>
inline bool
operator>=(Buffer<enc> lhs, Buffer<enc> rhs)
{
    return !(lhs < rhs);
}

/*
 * Helper to fixup start/end slice values.
 *
 * This function is taken from CPython's unicode module
 * (https://github.com/python/cpython/blob/0b718e6407da65b838576a2459d630824ca62155/Objects/bytes_methods.c#L495)
 * in order to remain compatible with how CPython handles
 * start/end arguments to str function like find/rfind etc.
 */
static inline void
adjust_offsets(npy_int64 *start, npy_int64 *end, size_t len)
{
    if (*end > static_cast<npy_int64>(len)) {
        *end = len;
    }
    else if (*end < 0) {
        *end += len;
        if (*end < 0) {
            *end = 0;
        }
    }

    if (*start < 0) {
        *start += len;
        if (*start < 0) {
            *start = 0;
        }
    }
}

template <ENCODING enc>
static inline npy_intp
string_find(Buffer<enc> buf1, Buffer<enc> buf2, npy_int64 start, npy_int64 end)
{
    size_t len1 = buf1.num_codepoints();
    size_t len2 = buf2.num_codepoints();

    adjust_offsets(&start, &end, len1);
    if (end - start < static_cast<npy_int64>(len2)) {
        return (npy_intp) -1;
    }
    if (len2 == 0) {
        return (npy_intp) start;
    }

    char *start_loc = NULL;
    char *end_loc = NULL;
    if (enc == ENCODING::UTF8) {
        find_start_end_locs(buf1.buf, (buf1.after - buf1.buf), start, end,
                            &start_loc, &end_loc);
    }
    else {
        start_loc = (buf1 + start).buf;
        end_loc = (buf1 + end).buf;
    }

    if (len2 == 1) {
        npy_intp result;
        switch (enc) {
            case ENCODING::UTF8:
            {
                if (num_bytes_for_utf8_character((const unsigned char *)buf2.buf) > 1) {
                    goto multibyte_search;
                }
                // fall through to the ASCII case because this is a one-byte character
            }
            case ENCODING::ASCII:
            {
                char ch = *buf2;
                CheckedIndexer<char> ind(start_loc, end_loc - start_loc);
                result = (npy_intp) findchar(ind, end_loc - start_loc, ch);
                if (enc == ENCODING::UTF8 && result > 0) {
                    result = utf8_character_index(
                            start_loc, start_loc - buf1.buf, start, result,
                            buf1.after - start_loc);
                }
                break;
            }
            case ENCODING::UTF32:
            {
                npy_ucs4 ch = *buf2;
                CheckedIndexer<npy_ucs4> ind((npy_ucs4 *)(buf1 + start).buf, end-start);
                result = (npy_intp) findchar(ind, end - start, ch);
                break;
            }
        }
        if (result == -1) {
            return (npy_intp) -1;
        }
        else {
            return result + (npy_intp) start;
        }
    }

  multibyte_search:

    npy_intp pos;
    switch(enc) {
        case ENCODING::UTF8:
            pos = fastsearch(start_loc, end_loc - start_loc, buf2.buf, buf2.after - buf2.buf, -1, FAST_SEARCH);
            // pos is the byte index, but we need the character index
            if (pos > 0) {
                pos = utf8_character_index(start_loc, start_loc - buf1.buf, start, pos, buf1.after - start_loc);
            }
            break;
        case ENCODING::ASCII:
            pos = fastsearch(start_loc, end - start, buf2.buf, len2, -1, FAST_SEARCH);
            break;
        case ENCODING::UTF32:
            pos = fastsearch((npy_ucs4 *)start_loc, end - start,
                             (npy_ucs4 *)buf2.buf, len2, -1, FAST_SEARCH);
            break;
    }

    if (pos >= 0) {
        pos += start;
    }
    return pos;
}

/* string_index returns -2 to signify a raised exception */
template <ENCODING enc>
static inline npy_intp
string_index(Buffer<enc> buf1, Buffer<enc> buf2, npy_int64 start, npy_int64 end)
{
    npy_intp pos = string_find(buf1, buf2, start, end);
    if (pos == -1) {
        npy_gil_error(PyExc_ValueError, "substring not found");
        return -2;
    }
    return pos;
}

template <ENCODING enc>
static inline npy_intp
string_rfind(Buffer<enc> buf1, Buffer<enc> buf2, npy_int64 start, npy_int64 end)
{
    size_t len1 = buf1.num_codepoints();
    size_t len2 = buf2.num_codepoints();

    adjust_offsets(&start, &end, len1);
    if (end - start < static_cast<npy_int64>(len2)) {
        return (npy_intp) -1;
    }

    if (len2 == 0) {
        return (npy_intp) end;
    }

    char *start_loc = NULL;
    char *end_loc = NULL;
    if (enc == ENCODING::UTF8) {
        find_start_end_locs(buf1.buf, (buf1.after - buf1.buf), start, end,
                            &start_loc, &end_loc);
    }
    else {
        start_loc = (buf1 + start).buf;
        end_loc = (buf1 + end).buf;
    }

    if (len2 == 1) {
        npy_intp result;
        switch (enc) {
            case ENCODING::UTF8:
            {
                if (num_bytes_for_utf8_character((const unsigned char *)buf2.buf) > 1) {
                    goto multibyte_search;
                }
                // fall through to the ASCII case because this is a one-byte character
            }
            case ENCODING::ASCII:
            {
                char ch = *buf2;
                CheckedIndexer<char> ind(start_loc, end_loc - start_loc);
                result = (npy_intp) rfindchar(ind, end_loc - start_loc, ch);
                if (enc == ENCODING::UTF8 && result > 0) {
                    result = utf8_character_index(
                            start_loc, start_loc - buf1.buf, start, result,
                            buf1.after - start_loc);
                }
                break;
            }
            case ENCODING::UTF32:
            {
                npy_ucs4 ch = *buf2;
                CheckedIndexer<npy_ucs4> ind((npy_ucs4 *)(buf1 + start).buf, end - start);
                result = (npy_intp) rfindchar(ind, end - start, ch);
                break;
            }
        }
        if (result == -1) {
            return (npy_intp) -1;
        }
        else {
            return result + (npy_intp) start;
        }
    }

  multibyte_search:

    npy_intp pos;
    switch (enc) {
        case ENCODING::UTF8:
            pos = fastsearch(start_loc, end_loc - start_loc, buf2.buf, buf2.after - buf2.buf, -1, FAST_RSEARCH);
            // pos is the byte index, but we need the character index
            if (pos > 0) {
                pos = utf8_character_index(start_loc, start_loc - buf1.buf, start, pos, buf1.after - start_loc);
            }
            break;
        case ENCODING::ASCII:
            pos = (npy_intp) fastsearch(start_loc, end - start, buf2.buf, len2, -1, FAST_RSEARCH);
            break;
        case ENCODING::UTF32:
            pos = (npy_intp) fastsearch((npy_ucs4 *)start_loc, end - start,
                                        (npy_ucs4 *)buf2.buf, len2, -1, FAST_RSEARCH);
            break;
    }
    if (pos >= 0) {
        pos += start;
    }
    return pos;
}


/* string_rindex returns -2 to signify a raised exception */
template <ENCODING enc>
static inline npy_intp
string_rindex(Buffer<enc> buf1, Buffer<enc> buf2, npy_int64 start, npy_int64 end)
{
    npy_intp pos = string_rfind(buf1, buf2, start, end);
    if (pos == -1) {
        npy_gil_error(PyExc_ValueError, "substring not found");
        return -2;
    }
    return pos;
}


/*
 * Count the number of occurrences of buf2 in buf1 between
 * start (inclusive) and end (exclusive)
 */
template <ENCODING enc>
static inline npy_intp
string_count(Buffer<enc> buf1, Buffer<enc> buf2, npy_int64 start, npy_int64 end)
{
    size_t len1 = buf1.num_codepoints();
    size_t len2 = buf2.num_codepoints();

    adjust_offsets(&start, &end, len1);
    if (end < start || end - start < static_cast<npy_int64>(len2)) {
        return (npy_intp) 0;
    }

    if (len2 == 0) {
        return (end - start) < PY_SSIZE_T_MAX ? end - start + 1 : PY_SSIZE_T_MAX;
    }

    char *start_loc = NULL;
    char *end_loc = NULL;
    if (enc == ENCODING::UTF8) {
        find_start_end_locs(buf1.buf, (buf1.after - buf1.buf), start, end,
                            &start_loc, &end_loc);
    }
    else {
        start_loc = (buf1 + start).buf;
        end_loc = (buf1 + end).buf;
    }
    npy_intp count;
    switch (enc) {
        case ENCODING::UTF8:
            count = fastsearch(start_loc, end_loc - start_loc, buf2.buf,
                               buf2.after - buf2.buf, PY_SSIZE_T_MAX,
                               FAST_COUNT);
            break;
        case ENCODING::ASCII:
            count = (npy_intp) fastsearch(start_loc, end - start, buf2.buf, len2,
                                          PY_SSIZE_T_MAX, FAST_COUNT);
            break;
        case ENCODING::UTF32:
            count = (npy_intp) fastsearch((npy_ucs4 *)start_loc, end - start,
                                          (npy_ucs4 *)buf2.buf, len2,
                                          PY_SSIZE_T_MAX, FAST_COUNT);
            break;
    }
    if (count < 0) {
        return 0;
    }
    return count;
}

enum class STARTPOSITION {
    FRONT, BACK
};

template <ENCODING enc>
inline npy_bool
tailmatch(Buffer<enc> buf1, Buffer<enc> buf2, npy_int64 start, npy_int64 end,
          STARTPOSITION direction)
{
    size_t len1 = buf1.num_codepoints();
    size_t len2 = buf2.num_codepoints();

    adjust_offsets(&start, &end, len1);
    end -= len2;
    if (end < start) {
        return 0;
    }

    if (len2 == 0) {
        return 1;
    }

    size_t offset;
    size_t end_sub = len2 - 1;
    if (direction == STARTPOSITION::BACK) {
        offset = end;
    }
    else {
        offset = start;
    }

    size_t size2 = len2;
    if (enc == ENCODING::UTF8) {
        size2 = (buf2.after - buf2.buf);
    }

    Buffer start_buf = (buf1 + offset);
    Buffer end_buf = start_buf + end_sub;
    if (*start_buf == *buf2 && *end_buf == *(buf2 + end_sub)) {
        return !start_buf.buffer_memcmp(buf2, size2);
    }

    return 0;
}

enum class STRIPTYPE {
    LEFTSTRIP, RIGHTSTRIP, BOTHSTRIP
};


template <ENCODING enc>
static inline size_t
string_lrstrip_whitespace(Buffer<enc> buf, Buffer<enc> out, STRIPTYPE striptype)
{
    size_t len = buf.num_codepoints();
    if (len == 0) {
        if (enc != ENCODING::UTF8) {
            out.buffer_fill_with_zeros_after_index(0);
        }
        return 0;
    }

    size_t i = 0;

    size_t num_bytes = (buf.after - buf.buf);
    Buffer traverse_buf = Buffer<enc>(buf.buf, num_bytes);

    if (striptype != STRIPTYPE::RIGHTSTRIP) {
        while (i < len) {
            if (!traverse_buf.first_character_isspace()) {
                break;
            }
            num_bytes -= traverse_buf.num_bytes_next_character();
            traverse_buf++;
            i++;
        }
    }

    npy_intp j = len - 1;  // Could also turn negative if we're stripping the whole string
    if (enc == ENCODING::UTF8) {
        traverse_buf = Buffer<enc>(buf.after, 0) - 1;
    }
    else {
        traverse_buf = buf + j;
    }

    if (striptype != STRIPTYPE::LEFTSTRIP) {
        while (j >= static_cast<npy_intp>(i)) {
            if (*traverse_buf != 0 && !traverse_buf.first_character_isspace()) {
                break;
            }
            num_bytes -= traverse_buf.num_bytes_next_character();
            traverse_buf--;
            j--;
        }
    }

    Buffer offset_buf = buf + i;
    if (enc == ENCODING::UTF8) {
        offset_buf.buffer_memcpy(out, num_bytes);
        return num_bytes;
    }
    offset_buf.buffer_memcpy(out, j - i + 1);
    out.buffer_fill_with_zeros_after_index(j - i + 1);
    return j - i + 1;
}


template <ENCODING enc>
static inline size_t
string_lrstrip_chars(Buffer<enc> buf1, Buffer<enc> buf2, Buffer<enc> out, STRIPTYPE striptype)
{
    size_t len1 = buf1.num_codepoints();
    if (len1 == 0) {
        if (enc != ENCODING::UTF8) {
            out.buffer_fill_with_zeros_after_index(0);
        }
        return 0;
    }

    size_t len2 = buf2.num_codepoints();
    if (len2 == 0) {
        if (enc == ENCODING::UTF8) {
            buf1.buffer_memcpy(out, (buf1.after - buf1.buf));
            return buf1.after - buf1.buf;
        }
        buf1.buffer_memcpy(out, len1);
        out.buffer_fill_with_zeros_after_index(len1);
        return len1;
    }

    size_t i = 0;

    size_t num_bytes = (buf1.after - buf1.buf);
    Buffer traverse_buf = Buffer<enc>(buf1.buf, num_bytes);

    if (striptype != STRIPTYPE::RIGHTSTRIP) {
        while (i < len1) {
            Py_ssize_t res;
            switch (enc) {
                case ENCODING::ASCII:
                case ENCODING::UTF8:
                {
                    CheckedIndexer<char> ind(buf2.buf, len2);
                    res = findchar<char>(ind, len2, *traverse_buf);
                    break;
                }
                case ENCODING::UTF32:
                {
                    CheckedIndexer<npy_ucs4> ind((npy_ucs4 *)buf2.buf, len2);
                    res = findchar<npy_ucs4>(ind, len2, *traverse_buf);
                    break;
                }
            }
            if (res < 0) {
                break;
            }
            num_bytes -= traverse_buf.num_bytes_next_character();
            traverse_buf++;
            i++;
        }
    }

    npy_intp j = len1 - 1;
    if (enc == ENCODING::UTF8) {
        traverse_buf = Buffer<enc>(buf1.after, 0) - 1;
    }
    else {
        traverse_buf = buf1 + j;
    }

    if (striptype != STRIPTYPE::LEFTSTRIP) {
        while (j >= static_cast<npy_intp>(i)) {
            Py_ssize_t res;
            switch (enc) {
                case ENCODING::ASCII:
                case ENCODING::UTF8:
                {
                    CheckedIndexer<char> ind(buf2.buf, len2);
                    res = findchar<char>(ind, len2, *traverse_buf);
                    break;
                }
                case ENCODING::UTF32:
                {
                    CheckedIndexer<npy_ucs4> ind((npy_ucs4 *)buf2.buf, len2);
                    res = findchar<npy_ucs4>(ind, len2, *traverse_buf);
                    break;
                }
            }
            if (res < 0) {
                break;
            }
            num_bytes -= traverse_buf.num_bytes_next_character();
            j--;
            if (j > 0) {
                traverse_buf--;
            }
        }
    }

    Buffer offset_buf = buf1 + i;
    if (enc == ENCODING::UTF8) {
        offset_buf.buffer_memcpy(out, num_bytes);
        return num_bytes;
    }
    offset_buf.buffer_memcpy(out, j - i + 1);
    out.buffer_fill_with_zeros_after_index(j - i + 1);
    return j - i + 1;
}

template <typename char_type>
static inline npy_intp
findslice_for_replace(CheckedIndexer<char_type> buf1, npy_intp len1,
                      CheckedIndexer<char_type> buf2, npy_intp len2)
{
    if (len2 == 0) {
        return 0;
    }
    if (len2 == 1) {
        return (npy_intp) findchar(buf1, len1, *buf2);
    }
    return (npy_intp) fastsearch(buf1.buffer, len1, buf2.buffer, len2, -1, FAST_SEARCH);
}


template <ENCODING enc>
static inline size_t
string_replace(Buffer<enc> buf1, Buffer<enc> buf2, Buffer<enc> buf3, npy_int64 count,
               Buffer<enc> out)
{
    size_t len1 = buf1.num_codepoints();
    size_t len2 = buf2.num_codepoints();
    size_t len3 = buf3.num_codepoints();
    char *start;
    size_t length = len1;
    if (enc == ENCODING::UTF8) {
        start = buf1.after;
        length = 0;
    }
    else if (enc == ENCODING::UTF32) {
        start = buf1.buf + sizeof(npy_ucs4) * len1;
    }
    else {
        start = buf1.buf + len1;
    }

    Buffer<enc> end1(start, length);
    size_t span2, span3;

    switch(enc) {
        case ENCODING::ASCII:
        case ENCODING::UTF32:
        {
            span2 = len2;
            span3 = len3;
            break;
        }
        case ENCODING::UTF8:
        {
            span2 = buf2.after - buf2.buf;
            span3 = buf3.after - buf3.buf;
            break;
        }
    }

    size_t ret = 0;

    // Only try to replace if replacements are possible.
    if (count <= 0                      // There's nothing to replace.
        || len1 < len2                  // Input is too small to have a match.
        || (len2 <= 0 && len3 <= 0)     // Match and replacement strings both empty.
        || (len2 == len3 && buf2.strcmp(buf3) == 0)) {  // Match and replacement are the same.

        goto copy_rest;
    }

    if (len2 > 0) {
        for (npy_int64 time = 0; time < count; time++) {
            npy_intp pos;
            switch (enc) {
                case ENCODING::ASCII:
                case ENCODING::UTF8:
                {
                    CheckedIndexer<char> ind1(buf1.buf, end1 - buf1);
                    CheckedIndexer<char> ind2(buf2.buf, span2);
                    pos = findslice_for_replace(ind1, end1 - buf1, ind2, span2);
                    break;
                }
                case ENCODING::UTF32:
                {
                    CheckedIndexer<npy_ucs4> ind1((npy_ucs4 *)buf1.buf, end1 - buf1);
                    CheckedIndexer<npy_ucs4> ind2((npy_ucs4 *)buf2.buf, span2);
                    pos = findslice_for_replace(ind1, end1 - buf1, ind2, span2);
                    break;
                }
            }
            if (pos < 0) {
                break;
            }

            buf1.buffer_memcpy(out, pos);
            ret += pos;
            out.advance_chars_or_bytes(pos);
            buf1.advance_chars_or_bytes(pos);

            buf3.buffer_memcpy(out, span3);
            ret += span3;
            out.advance_chars_or_bytes(span3);
            buf1.advance_chars_or_bytes(span2);
        }
    }
    else {  // If match string empty, interleave.
        while (count > 0) {
            buf3.buffer_memcpy(out, span3);
            ret += span3;
            out.advance_chars_or_bytes(span3);

            if (--count <= 0) {
                break;
            }

            switch (enc) {
                case ENCODING::ASCII:
                case ENCODING::UTF32:
                    buf1.buffer_memcpy(out, 1);
                    ret += 1;
                    break;
                case ENCODING::UTF8:
                    size_t n_bytes = buf1.num_bytes_next_character();
                    buf1.buffer_memcpy(out, n_bytes);
                    ret += n_bytes;
                    break;
            }
            buf1 += 1;
            out += 1;
        }
    }

copy_rest:
    buf1.buffer_memcpy(out, end1 - buf1);
    ret += end1 - buf1;
    if (enc == ENCODING::UTF8) {
        return ret;
    }
    out.buffer_fill_with_zeros_after_index(end1 - buf1);
    return ret;
}


template <ENCODING enc>
static inline npy_intp
string_expandtabs_length(Buffer<enc> buf, npy_int64 tabsize)
{
    size_t len = buf.num_codepoints();

    npy_intp new_len = 0, line_pos = 0;

    Buffer<enc> tmp = buf;
    for (size_t i = 0; i < len; i++) {
        npy_ucs4 ch = *tmp;
        if (ch == '\t') {
            if (tabsize > 0) {
                npy_intp incr = tabsize - (line_pos % tabsize);
                line_pos += incr;
                new_len += incr;
            }
        }
        else {
            line_pos += 1;
            size_t n_bytes = tmp.num_bytes_next_character();
            new_len += n_bytes;
            if (ch == '\n' || ch == '\r') {
                line_pos = 0;
            }
        }
        if (new_len == PY_SSIZE_T_MAX || new_len < 0) {
            npy_gil_error(PyExc_OverflowError, "new string is too long");
            return -1;
        }
        tmp++;
    }
    return new_len;
}


template <ENCODING enc>
static inline npy_intp
string_expandtabs(Buffer<enc> buf, npy_int64 tabsize, Buffer<enc> out)
{
    size_t len = buf.num_codepoints();

    npy_intp new_len = 0, line_pos = 0;

    Buffer<enc> tmp = buf;
    for (size_t i = 0; i < len; i++) {
        npy_ucs4 ch = *tmp;
        if (ch == '\t') {
            if (tabsize > 0) {
                npy_intp incr = tabsize - (line_pos % tabsize);
                line_pos += incr;
                new_len += out.buffer_memset((npy_ucs4) ' ', incr);
                out += incr;
            }
        }
        else {
            line_pos++;
            new_len += out.buffer_memset(ch, 1);
            out++;
            if (ch == '\n' || ch == '\r') {
                line_pos = 0;
            }
        }
        tmp++;
    }
    return new_len;
}


enum class JUSTPOSITION {
    CENTER, LEFT, RIGHT
};

template <ENCODING enc>
static inline npy_intp
string_pad(Buffer<enc> buf, npy_int64 width, npy_ucs4 fill, JUSTPOSITION pos, Buffer<enc> out)
{
    size_t finalwidth = width > 0 ? width : 0;
    if (finalwidth > PY_SSIZE_T_MAX) {
        npy_gil_error(PyExc_OverflowError, "padded string is too long");
        return -1;
    }

    size_t len_codepoints = buf.num_codepoints();
    size_t len_bytes = buf.after - buf.buf;

    size_t len;
    if (enc == ENCODING::UTF8) {
        len = len_bytes;
    }
    else {
        len = len_codepoints;
    }

    if (len_codepoints >= finalwidth) {
        buf.buffer_memcpy(out, len);
        return (npy_intp) len;
    }

    size_t left, right;
    if (pos == JUSTPOSITION::CENTER) {
        size_t pad = finalwidth - len_codepoints;
        left = pad / 2 + (pad & finalwidth & 1);
        right = pad - left;
    }
    else if (pos == JUSTPOSITION::LEFT) {
        left = 0;
        right = finalwidth - len_codepoints;
    }
    else {
        left = finalwidth - len_codepoints;
        right = 0;
    }

    assert(left >= 0 || right >= 0);
    assert(left <= PY_SSIZE_T_MAX - len && right <= PY_SSIZE_T_MAX - (left + len));

    if (left > 0) {
        out.advance_chars_or_bytes(out.buffer_memset(fill, left));
    }

    buf.buffer_memcpy(out, len);
    out += len_codepoints;

    if (right > 0) {
        out.advance_chars_or_bytes(out.buffer_memset(fill, right));
    }

    return finalwidth;
}


template <ENCODING enc>
static inline npy_intp
string_zfill(Buffer<enc> buf, npy_int64 width, Buffer<enc> out)
{
    size_t finalwidth = width > 0 ? width : 0;

    npy_ucs4 fill = '0';
    npy_intp new_len = string_pad(buf, width, fill, JUSTPOSITION::RIGHT, out);
    if (new_len == -1) {
        return -1;
    }

    size_t offset = finalwidth - buf.num_codepoints();
    Buffer<enc> tmp = out + offset;

    npy_ucs4 c = *tmp;
    if (c == '+' || c == '-') {
        tmp.buffer_memset(fill, 1);
        out.buffer_memset(c, 1);
    }

    return new_len;
}


template <ENCODING enc>
static inline void
string_partition(Buffer<enc> buf1, Buffer<enc> buf2, npy_int64 idx,
                 Buffer<enc> out1, Buffer<enc> out2, Buffer<enc> out3,
                 npy_intp *final_len1, npy_intp *final_len2, npy_intp *final_len3,
                 STARTPOSITION pos)
{
    // StringDType uses a ufunc that implements the find-part as well
    assert(enc != ENCODING::UTF8);

    size_t len1 = buf1.num_codepoints();
    size_t len2 = buf2.num_codepoints();

    if (len2 == 0) {
        npy_gil_error(PyExc_ValueError, "empty separator");
        *final_len1 = *final_len2 = *final_len3 = -1;
        return;
    }

    if (idx < 0) {
        if (pos == STARTPOSITION::FRONT) {
            buf1.buffer_memcpy(out1, len1);
            *final_len1 = len1;
            *final_len2 = *final_len3 = 0;
        }
        else {
            buf1.buffer_memcpy(out3, len1);
            *final_len1 = *final_len2 = 0;
            *final_len3 = len1;
        }
        return;
    }

    buf1.buffer_memcpy(out1, idx);
    *final_len1 = idx;
    buf2.buffer_memcpy(out2, len2);
    *final_len2 = len2;
    (buf1 + idx + len2).buffer_memcpy(out3, len1 - idx - len2);
    *final_len3 = len1 - idx - len2;
}


#endif /* _NPY_CORE_SRC_UMATH_STRING_BUFFER_H_ */
