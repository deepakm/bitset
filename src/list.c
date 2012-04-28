#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "bitset/bitset.h"
#include "bitset/list.h"
#include "bitset/operation.h"

bitset_list *bitset_list_new() {
    bitset_list *c = (bitset_list *) malloc(sizeof(bitset_list));
    if (!c) {
        bitset_oom();
    }
    c->length = c->size = c->count = c->tail_offset = 0;
    c->buffer = c->tail = NULL;
    return c;
}

void bitset_list_free(bitset_list *c) {
    if (c->length) {
        free(c->buffer);
    }
    free(c);
}

static inline void bitset_list_resize(bitset_list *c, unsigned length) {
    if (length > c->size) {
        unsigned next_size;
        BITSET_NEXT_POW2(next_size, length);
        if (!c->length) {
            c->buffer = (char *) malloc(sizeof(char) * next_size);
        } else {
            c->buffer = (char *) realloc(c->buffer, sizeof(char) * next_size);
        }
        if (!c->buffer) {
            bitset_oom();
        }
        c->size = next_size;
    }
    c->length = length;
}

unsigned bitset_list_length(bitset_list *c) {
    return c->length;
}

unsigned bitset_list_count(bitset_list *c) {
    return c->count;
}

static inline unsigned bitset_encoded_length_required_bytes(unsigned length) {
    if (length < (1 << 6)) {
        return 1;
    } else if (length < (1 << 14)) {
        return 2;
    } else if (length < (1 << 22)) {
        return 3;
    } else {
        return 4;
    }
}

static inline void bitset_encoded_length_bytes(unsigned length, char *buffer) {
    if (length < (1 << 6)) {
        buffer[0] = (unsigned char)length;
    } else if (length < (1 << 14)) {
        buffer[0] = 0x40 | ((unsigned char)length >> 8);
        buffer[1] = (unsigned char)length & 0xFF;
    } else if (length < (1 << 22)) {
        buffer[0] = 0x80 | ((unsigned char)length >> 16);
        buffer[1] = ((unsigned char)length >> 8) & 0xFF;
        buffer[2] = (unsigned char)length & 0xFF;
    } else {
        buffer[0] = 0xC0 | ((unsigned char)length >> 24);
        buffer[1] = ((unsigned char)length >> 16) & 0xFF;
        buffer[2] = ((unsigned char)length >> 8) & 0xFF;
        buffer[3] = (unsigned char)length & 0xFF;
    }
}

static inline unsigned bitset_encoded_length_size(const char *buffer) {
    unsigned type = buffer[0] & 0xC0;
    switch (type) {
        case 0x00: return 1;
        case 0x40: return 2;
        case 0x80: return 3;
        default:   return 4;
    }
}

static inline unsigned bitset_encoded_length(const char *buffer) {
    unsigned length, type = buffer[0] & 0xC0;
    switch (type) {
        case 0x00:
            length = (unsigned char)buffer[0];
            break;
        case 0x40:
            length = (((unsigned char)buffer[0] & 0x3F) << 8);
            length += (unsigned char)buffer[1];
            break;
        case 0x80:
            length = (((unsigned char)buffer[0] & 0x3F) << 16);
            length += ((unsigned char)buffer[1] << 8);
            length += (unsigned char)buffer[2];
            break;
        default:
            length = (((unsigned char)buffer[0] & 0x3F) << 24);
            length += ((unsigned char)buffer[1] << 16);
            length += ((unsigned char)buffer[2] << 8);
            length += (unsigned char)buffer[3];
            break;
    }
    return length;
}

bitset_list *bitset_list_new_buffer(unsigned length, const char *buffer) {
    bitset_list *c = (bitset_list *) malloc(sizeof(bitset_list));
    if (!c) {
        bitset_oom();
    }
    c->buffer = (char *) malloc(length * sizeof(char));
    if (!c->buffer) {
        bitset_oom();
    }
    memcpy(c->buffer, buffer, length * sizeof(char));
    c->length = c->size = length;
    c->count = c->tail_offset = 0;
    unsigned length_bytes, offset_bytes;
    length = 0;
    char *buf = c->buffer;
    for (unsigned i = 0; i < c->length; c->count++) {
        c->tail = buf;
        c->tail_offset += bitset_encoded_length(buf);
        offset_bytes = bitset_encoded_length_size(buf);
        buf += offset_bytes;
        length = bitset_encoded_length(buf);
        length_bytes = bitset_encoded_length_size(buf);
        buf += length_bytes;
        length *= sizeof(bitset_word);
        i += offset_bytes + length_bytes + length;
        buf += length;
    }
    return c;
}

void bitset_list_push(bitset_list *c, bitset *b, unsigned offset) {
    if (offset < c->tail_offset) {
        fprintf(stderr, "Can only append to a bitset list\n");
        exit(1);
    }

    unsigned length = c->length;
    unsigned relative_offset = offset - c->tail_offset;
    c->count++;

    //Resize the list to accommodate the bitset
    unsigned length_bytes = bitset_encoded_length_required_bytes(b->length);
    unsigned offset_bytes = bitset_encoded_length_required_bytes(relative_offset);
    bitset_list_resize(c, length + length_bytes + offset_bytes + b->length * sizeof(bitset_word));

    //Keep a reference to the tail bitset
    char *buffer = c->tail = c->buffer + length;
    c->tail_offset = offset;

    //Encode the offset and length
    bitset_encoded_length_bytes(relative_offset, buffer);
    buffer += offset_bytes;
    bitset_encoded_length_bytes(b->length, buffer);
    buffer += length_bytes;

    //Copy the bitset
    if (b->length) {
        memcpy(buffer, b->words, b->length * sizeof(bitset_word));
    }
}

static inline void bitset_list_iterator_resize(bitset_list_iterator *i, unsigned length) {
    if (length > i->size) {
        unsigned next_size;
        BITSET_NEXT_POW2(next_size, length);
        if (!i->length) {
            i->bitsets = (bitset **) malloc(sizeof(bitset*) * next_size);
            i->offsets = (unsigned *) malloc(sizeof(unsigned) * next_size);
        } else {
            i->bitsets = (bitset **) realloc(i->bitsets, sizeof(bitset*) * next_size);
            i->offsets = (unsigned *) realloc(i->offsets, sizeof(unsigned) * next_size);
        }
        if (!i->offsets || !i->bitsets) {
            bitset_oom();
        }
        i->size = next_size;
    }
    i->length = length;
}

bitset_list_iterator *bitset_list_iterator_new(bitset_list *c, unsigned start, unsigned end) {
    bitset_list_iterator *i = (bitset_list_iterator *) malloc(sizeof(bitset_list_iterator));
    if (!i) {
        bitset_oom();
    }
    if (start == BITSET_LIST_START && end == BITSET_LIST_END) {
        i->bitsets = (bitset **) malloc(sizeof(bitset*) * c->count);
        i->offsets = (unsigned *) malloc(sizeof(unsigned) * c->count);
        if (!i->bitsets || !i->offsets) {
            bitset_oom();
        }
        i->length = i->size = c->count;
    } else {
        i->length = i->size = 0;
        i->bitsets = NULL;
        i->offsets = NULL;
    }
    unsigned length = 0, length_bytes, offset = 0, offset_bytes;
    char *buffer = c->buffer;
    for (unsigned j = 0, b = 0; j < c->length; ) {
        offset += bitset_encoded_length(buffer);
        offset_bytes = bitset_encoded_length_size(buffer);
        buffer += offset_bytes;
        length = bitset_encoded_length(buffer);
        length_bytes = bitset_encoded_length_size(buffer);
        buffer += length_bytes;
        if (offset >= start && (!end || offset < end)) {
            bitset_list_iterator_resize(i, b + 1);
            i->bitsets[b] = (bitset *) malloc(sizeof(bitset));
            if (!i->bitsets[b]) {
                bitset_oom();
            }
            i->bitsets[b]->words = (bitset_word *) buffer;
            i->bitsets[b]->length = i->bitsets[b]->size = length;
            i->offsets[b] = offset;
            b++;
        }
        length *= sizeof(bitset_word);
        j += offset_bytes + length_bytes + length;
        buffer += length;
    }
    return i;
}

void bitset_list_iterator_concat(bitset_list_iterator *i, bitset_list_iterator *c, unsigned offset) {
    if (c->length) {
        unsigned previous_length = i->length;
        bitset_list_iterator_resize(i, previous_length + c->length);
        for (unsigned j = 0; j < c->length; j++) {
            i->bitsets[j + previous_length] = c->bitsets[j];
            i->offsets[j + previous_length] = c->offsets[j] + offset;
        }
        free(c->bitsets);
        free(c->offsets);
    }
    free(c);
}

void bitset_list_iterator_count(bitset_list_iterator *i, unsigned *raw, unsigned *unique) {
    unsigned raw_count = 0, offset;
    bitset *b;
    bitset_operation *o = bitset_operation_new(NULL);
    BITSET_LIST_FOREACH(i, b, offset) {
        raw_count += bitset_count(b);
        bitset_operation_add(o, b, BITSET_OR);
    }
    *raw = raw_count;
    *unique = bitset_operation_count(o);
    bitset_operation_free(o);
}

void bitset_list_iterator_free(bitset_list_iterator *i) {
    for (unsigned j = 0; j < i->length; j++) {
        free(i->bitsets[j]);
    }
    if (i->bitsets) free(i->bitsets);
    if (i->offsets) free(i->offsets);
    free(i);
}

