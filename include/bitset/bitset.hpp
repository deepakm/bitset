#ifndef BITSET_BITSET_HPP_
#define BITSET_BITSET_HPP_

#include <cstddef>

#include "bitset/bitset.h"
#include "bitset/vector.h"
#include "bitset/operation.h"
#include "bitset/probabilistic.h"

/**
 * C++ wrapper classes for bitset structures.
 */

namespace compressedbitset {

class Bitset {
  public:
    Bitset() { b = bitset_new(); }
    Bitset(bitset *bs) { b = bs; }
    Bitset(const char *buffer, size_t len) {
        b = bitset_new_buffer(buffer, len);
    }
    Bitset(bitset_offset *bits, size_t len) {
        b = bitset_new_bits(bits, len);
    }
    ~Bitset() { bitset_free(b); }
    bool get(bitset_offset offset) const {
        return bitset_get(b, offset);
    }
    bool set(bitset_offset offset) {
        return bitset_set(b, offset);
    }
    bool unset(bitset_offset offset) {
        return bitset_unset(b, offset);
    }
    unsigned count() const {
        return bitset_count(b);
    }
    unsigned length() const {
        return bitset_length(b);
    }
    bitset_offset min() const {
        return bitset_min(b);
    }
    bitset_offset max() const {
        return bitset_max(b);
    }
    void clear() {
        bitset_clear(b);
    }
    char *getBuffer() const {
        return (char *) b->words;
    }
    bitset *getBitset() {
        return b;
    }
  protected:
    bitset *b;
};

class BitsetOperation {
  public:
    BitsetOperation() {
        o = bitset_operation_new(NULL);
    }
    BitsetOperation(Bitset& b) {
        o = bitset_operation_new(b.getBitset());
    }
    ~BitsetOperation() {
        bitset_operation_free(o);
    }
    BitsetOperation& add(Bitset& b, enum bitset_operation_type op) {
        bitset_operation_add(o, b.getBitset(), op);
        return *this;
    }
    BitsetOperation& add(BitsetOperation& nested, enum bitset_operation_type op) {
        bitset_operation_add_nested(o, nested.getBitsetOperation(), op);
        return *this;
    }
    BitsetOperation& And(Bitset& b) {
        return add(b, BitsetOperation::AND);
    }
    BitsetOperation& Or(Bitset& b) {
        return add(b, BitsetOperation::OR);
    }
    BitsetOperation& Xor(Bitset& b) {
        return add(b, BitsetOperation::XOR);
    }
    BitsetOperation& AndNot(Bitset& b) {
        return add(b, BitsetOperation::ANDNOT);
    }
    bitset_operation *getBitsetOperation() {
        return o;
    }
    Bitset *exec() {
        return new Bitset(bitset_operation_exec(o));
    }
    unsigned count() {
        return bitset_operation_count(o);
    }
    static enum bitset_operation_type AND;
    static enum bitset_operation_type OR;
    static enum bitset_operation_type XOR;
    static enum bitset_operation_type ANDNOT;
  protected:
    bitset_operation *o;
};

enum bitset_operation_type BitsetOperation::AND = BITSET_AND;
enum bitset_operation_type BitsetOperation::OR = BITSET_OR;
enum bitset_operation_type BitsetOperation::XOR = BITSET_XOR;
enum bitset_operation_type BitsetOperation::ANDNOT = BITSET_ANDNOT;

class Vector {
  public:
    Vector() { v = bitset_vector_new(); }
    Vector(bitset_vector *ve) { v = ve; };
    Vector(const char *buffer, unsigned length) {
        v = bitset_vector_new_buffer(buffer, length);
    }
    ~Vector() { bitset_vector_free(v); }
    void push(Bitset &b, unsigned offset) {
        bitset_vector_push(v, b.getBitset(), offset);
    }
    unsigned length() const {
        return bitset_vector_length(v);
    }
    unsigned count() const {
        return bitset_vector_count(v);
    }
    char *getBuffer() const {
        return v->buffer;
    }
    bitset_vector *getVector() const {
        return v;
    }
    static unsigned START;
    static unsigned END;
  protected:
    bitset_vector *v;
};

unsigned Vector::START = BITSET_VECTOR_START;
unsigned Vector::END = BITSET_VECTOR_END;

class VectorIterator {
  public:
    VectorIterator(bitset_vector_iterator *it) { i = it; }
    VectorIterator(Vector& list, unsigned start=Vector::START, unsigned end=Vector::END) {
        i = bitset_vector_iterator_new(list.getVector(), start, end);
    }
    ~VectorIterator() {
        bitset_vector_iterator_free(i);
    }
    VectorIterator& concat(VectorIterator& next, unsigned offset) {
        bitset_vector_iterator_concat(i, next.getVectorIterator(), offset);
        return *this;
    }
    void count(unsigned *raw, unsigned *unique) const {
        bitset_vector_iterator_count(i, raw, unique);
    }
    Bitset *merge() {
        return new Bitset(bitset_vector_iterator_merge(i));
    }
    Vector *compact() {
        return new Vector(bitset_vector_iterator_compact(i));
    }
    bitset_vector_iterator *getVectorIterator() const {
        return i;
    }
  protected:
    bitset_vector_iterator *i;
};

class VectorOperation {
    VectorOperation() {
        o = bitset_vector_operation_new(NULL);
    }
    VectorOperation(VectorIterator& i) {
        o = bitset_vector_operation_new(i.getVectorIterator());
    }
    ~VectorOperation() { bitset_vector_operation_free(o); }
    VectorOperation& add(VectorIterator& v, enum bitset_operation_type op) {
        bitset_vector_operation_add(o, v.getVectorIterator(), op);
        return *this;
    }
    VectorOperation& add(VectorOperation& nested, enum bitset_operation_type op) {
        bitset_vector_operation_add_nested(o, nested.getVectorOperation(), op);
        return *this;
    }
    VectorIterator *exec() {
        return new VectorIterator(bitset_vector_operation_exec(o));
    }
    bitset_vector_operation *getVectorOperation() {
        return o;
    }
  protected:
    bitset_vector_operation *o;
};

} //namespace compressedbitset

#endif
