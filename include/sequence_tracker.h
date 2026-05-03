#ifndef SEQUENCE_TRACKER_H
#define SEQUENCE_TRACKER_H

#include <cstdint>
#include <cstring>

// Tracks received MoldUDP64 sequence numbers using a sliding bitmap.
// Maintains a fixed-size window of 65,536 sequence nubmers.
// - base_seq marks the start of the window.
// - Sequences below base_seq are considered old (already seen).
// - Sequences within the window are tracked using 1 bit each.
// - Sequences beyond the window cause it to slide forward.
// Memory usage: 8 KB (65536 bits = 8192 bytes)

static constexpr int WINDOW_SIZE = 65536;
static constexpr int BITMAP_BYTES = WINDOW_SIZE / 8;

class SequenceTracker {
public:
    SequenceTracker() : base_seq(0), initialized(false) {
        std::memset(bitmap, 0, BITMAP_BYTES);
    }

    // Check if a sequence has already been seen.
    // Returns true if duplicate, false if new
    bool is_seen(uint64_t seq) {
        // First packet - set base and mark as new
        if (!initialized) {
            base_seq = seq;
            initialized = true;
            return false;
        }

        // if sequence is bellow
        // window - too old (treat as duplicate)
        if (seq < base_seq) {
            return true;
        }

        // If sequence number is 
        // beyond the window - slide
        // forward to include it
        // e.g., [1000 ........ 66535]
        // and receive seq = 80000 which is 
        // outside of our current window
        // 1. Shift the window forward
        // 2. Drop old data
        // 3. Make space for newer sequence number
        uint64_t offset = seq - base_seq;
        if (offset >= WINDOW_SIZE) {
            move_windo_to(seq);
            offset = seq - base_seq;
        }

        // Check the bit
        int byte_index = offset / 8;
        int bit_index = offset % 8;
        return (bitmap[byte_index] >> bit_index) & 1;
    }

    // Mark Sequence as seen
    void mark_seen(uint64_t seq) {
        if (seq < base_seq) {
            return;
        }

        uint64_t offset = seq - base_seq;
        if (offset >= WINDOW_SIZE) {
            return;
        }

        int byte_index = offset / 8;
        int bit_index  = offset % 8;
        bitmap[byte_index] |= (1 << bit_index);
    }

    // Mark a range of sequence as seen.
    // Used when a packet contains multiple messages
    // (seq ti seq+count-1).
    void mark_range_seen(uint64_t seq, uint64_t count) {
        for (uint16_t i = 0; i < count; i ++) {
            mark_seen(seq + i);
        }
    }

    // Get the base sequence number (start of window)
    uint64_t base() const {
        return base_seq;
    }

private:
    uint64_t base_seq;
    bool initialized;
    uint8_t bitmap[BITMAP_BYTES];

    // Move the window forward to include new_seq.
    // Discards(drop, remove) old data that falls outside the window.
    void move_windo_to(uint64_t new_seq) {
        // Place new_seq near the start of the window
        uint64_t new_base = new_seq - (WINDOW_SIZE / 4);
        if (new_base < base_seq) {
            new_base = base_seq;
        }

        uint64_t shift = new_base - base_seq;

        // Shift is larger than the whole window
        // clean everything
        if (shift >= WINDOW_SIZE) {
            std::memset(bitmap, 0, BITMAP_BYTES);
        }
        else {
            // Shift bitmap forward by 'shift' bits
            int byte_shift = shift / 8;
            int bit_shift  = shift % 8;

            if (bit_shift == 0) {
                // Clean byte boundary shift
                std::memmove(bitmap, bitmap + byte_shift, BITMAP_BYTES - byte_shift);
                std::memset(bitmap + BITMAP_BYTES - byte_shift, 0, byte_shift);
            }
            else {
                // sub-byte shift
                for (int i = 0; i < BITMAP_BYTES - byte_shift - 1; i++) {
                    bitmap[i] = (bitmap[i + byte_shift] >> bit_shift) |
                                (bitmap[i + byte_shift + 1] << (8 - bit_shift));
                }
                bitmap[BITMAP_BYTES - byte_shift - 1] = bitmap[BITMAP_BYTES - 1] >> bit_shift;
                std::memset(bitmap + BITMAP_BYTES - byte_shift, 0, byte_shift);
            }
        }
        base_seq = new_base;
    }
};

#endif
