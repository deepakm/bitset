#ifndef BITSET_BITSET_H_
#define BITSET_BITSET_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The bitset structure uses a form of word-aligned run-length encoding.
 *
 * The compression technique is optimised for sparse bitsets where runs of
 * empty words are typically followed by a word with only one set bit. We
 * can exploit the fact that runs are usually less than 2^25 words long and
 * use the extra space in the previous word to encode the position of this bit.
 *
 * There are two types of words identified by the most significant bit
 *
 *     Literal word: 0XXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX
 *        Fill word: 1PPPPPLL LLLLLLLL LLLLLLLL LLLLLLLL
 *
 * X = Uncompressed bits
 * L = represents the length of the span of clean words
 * C = the colour of the span (all 1's or 0's)
 * P = if the word proceeding the span contains only 1 bit, this 5-bit length
 *     stores the position of the bit so that the next literal can be omitted
 */

#define bitset_word                    uint32_t

#define BITSET_WORD_LENGTH             (sizeof(bitset_word) * 8)
#define BITSET_POSITION_LENGTH         BITSET_LOG2(BITSET_WORD_LENGTH)
#define BITSET_FILL_BIT                (1 << (BITSET_WORD_LENGTH - 1))
#define BITSET_SPAN_LENGTH             (BITSET_WORD_LENGTH - BITSET_POSITION_LENGTH - 1)
#define BITSET_POSITION_MASK           (((1 << (BITSET_POSITION_LENGTH)) - 1) << (BITSET_SPAN_LENGTH))
#define BITSET_LENGTH_MASK             ((1 << (BITSET_SPAN_LENGTH)) - 1)
#define BITSET_LITERAL_LENGTH          (BITSET_WORD_LENGTH - 1)

#define BITSET_IS_FILL_WORD(word)      ((word) & BITSET_FILL_BIT)
#define BITSET_IS_LITERAL_WORD(word)   (((word) & BITSET_FILL_BIT) == 0)
#define BITSET_GET_LENGTH(word)        ((word) & BITSET_LENGTH_MASK)
#define BITSET_SET_LENGTH(word, len)   ((word) | (len))
#define BITSET_GET_POSITION(word)      (((word) & BITSET_POSITION_MASK) >> BITSET_SPAN_LENGTH)
#define BITSET_SET_POSITION(word, pos) ((word) | ((pos) << BITSET_SPAN_LENGTH))
#define BITSET_UNSET_POSITION(word)    ((word) & ~BITSET_POSITION_MASK)
#define BITSET_CREATE_FILL(len, pos)   BITSET_SET_POSITION(BITSET_FILL_BIT | (len), (pos) + 1)
#define BITSET_CREATE_EMPTY_FILL(len)  (BITSET_FILL_BIT | (len))
#define BITSET_CREATE_LITERAL(bit)     ((1 << (BITSET_WORD_LENGTH - 2)) >> (bit))
#define BITSET_MAX_LENGTH              BITSET_LENGTH_MASK

#define BITSET_TMPVAR(i, line)         BITSET_TMPVAR_(i, line)
#define BITSET_TMPVAR_(a,b)            a##b

#define BITSET_IS_TAGGED_POINTER(p)    ((uintptr_t)p & 1)
#define BITSET_TAG_POINTER(p)          ((uintptr_t)p | 1)
#define BITSET_UNTAG_POINTER(p)        ((uintptr_t)p & ~((uintptr_t)1))
#define BITSET_UINT_IN_POINTER(u)      (((uintptr_t)u << 1) | 1)
#define BITSET_UINT_FROM_POINTER(u)    ((uintptr_t)u >> 1)
#define BITSET_UINT_CAN_TAG(u)         (u < (sizeof(void*)==4 ? 1U<<31 : 1LLU<<63))

#define BITSET_MAX(a, b)               ((a) > (b) ? (a) : (b));
#define BITSET_MIN(a, b)               ((a) < (b) ? (a) : (b));
#define BITSET_IS_POW2(word)           ((word & (word - 1)) == 0)
#define BITSET_LOG2(v)                 (8 - 90/(((v)/4+14)|1) - 2/((v)/2+1))
#define BITSET_NEXT_POW2(d,s)          d=s;d--;d|=d>>1;d|=d>>2;d|=d>>4;d|=d>>8;d|=d>>16;d++;
#define BITSET_POP_COUNT(c,w)          w&=P1;w-=(w>>1)&P2;w=(w&P3)+((w>>2)&P3);w=(w+(w>>4))\
                                       &P4;c+=(w*P5)>>(BITSET_WORD_LENGTH-8);
#define P1 0x7FFFFFFF
#define P2 0x55555555
#define P3 0x33333333
#define P4 0x0F0F0F0F
#define P5 0x01010101

/**
 * 64-bit offsets are supported.
 */

#ifndef BITSET_64BIT_OFFSETS
#  define bitset_offset uint32_t
#  define bitset_format "%u"
#else
#  define bitset_offset uint64_t
#  define bitset_format "%llu"
#endif

/**
 * Bitset types.
 */

typedef struct bitset_ {
    bitset_word *words;
    size_t length;
    size_t size;
    unsigned references;
} bitset;

typedef struct bitset_iterator_ {
    bitset_offset *offsets;
    size_t length;
} bitset_iterator;

/**
 * Custom out of memory behaviour.
 */

#ifndef bitset_oom
#  define bitset_oom() fprintf(stderr, "Out of memory\n"), exit(1)
#endif

/**
 * Create a new bitset.
 */

bitset *bitset_new();

/**
 * Clear the specified bitset.
 */

void bitset_clear(bitset *);

/**
 * Free the specified bitset.
 */

void bitset_free(bitset *);

/**
 * Resize the bitset buffer.
 */

void bitset_resize(bitset *, size_t);

/**
 * Get the byte length of the bitset buffer.
 */

size_t bitset_length(bitset *);

/**
 * Create a new bitset from an existing buffer.
 */

bitset *bitset_new_buffer(const char *, size_t);

/**
 * Create a new bitset from an array of bits.
 */

bitset *bitset_new_bits(bitset_offset *, size_t);

/**
 * A helper for creating bitsets: BITSET_NEW(b1, { 1, 10, 100 });
 */

#define BITSET_NEW(name, ...) \
    bitset_offset BITSET_TMPVAR(o, __LINE__)[] = __VA_ARGS__; \
    bitset *name = bitset_new_bits((bitset_offset*)BITSET_TMPVAR(o, __LINE__), \
      sizeof(BITSET_TMPVAR(o, __LINE__))/sizeof(BITSET_TMPVAR(o, __LINE__)[0]));

/**
 * Create a copy of the specified bitset.
 */

bitset *bitset_copy(bitset *);

/**
 * Check whether a bit is set.
 */

bool bitset_get(const bitset *, bitset_offset);

/**
 * Get the population count of the bitset (number of set bits).
 */

bitset_offset bitset_count(const bitset *);

/**
 * Set or unset the specified bit.
 */

bool bitset_set_to(bitset *, bitset_offset, bool);

/**
 * Set the specified bit.
 */

bool bitset_set(bitset *, bitset_offset);

/**
 * Unset the specified bit.
 */

bool bitset_unset(bitset *, bitset_offset);

/**
 * Find the lowest set bit in the bitset.
 */

bitset_offset bitset_min(const bitset *);

/**
 * Find the highest set bit in the bitset.
 */

bitset_offset bitset_max(const bitset *);

/**
 * Create a new bitset iterator.
 */

bitset_iterator *bitset_iterator_new(const bitset *);

/**
 * Iterate over all bits.
 */

#define BITSET_FOREACH(iterator, offset) \
    for (unsigned BITSET_TMPVAR(i, __LINE__) = 0; \
         offset = iterator->length ? iterator->offsets[BITSET_TMPVAR(i, __LINE__)] : 0, \
         BITSET_TMPVAR(i, __LINE__) < iterator->length; \
         BITSET_TMPVAR(i, __LINE__)++)

/**
 * Free the bitset iterator.
 */

void bitset_iterator_free(bitset_iterator *);

#ifdef __cplusplus
} //extern "C"
#endif

#endif

