#ifndef PAYLOAD_H
#define PAYLOAD_H

#include <vector>
#include <cstdint>
#include <cstring>
#include <type_traits>

constexpr size_t MAX_PAYLOAD_SIZE = 256;

class Payload {
public:
    Payload() : read_offset(0), overflow(false), out_of_bounds(false) {}

    // Write raw bytes
    void write(const void* data, size_t size) {
        if (buffer.size() + size > MAX_PAYLOAD_SIZE) {
            overflow = true;
            return;
        }
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data);
        buffer.insert(buffer.end(), bytes, bytes + size);
    }

    // Read raw bytes
    void read(void* dest, size_t size) {
        if (read_offset + size > buffer.size()) {
            out_of_bounds = true;
            return;
        }
        std::memcpy(dest, buffer.data() + read_offset, size);
        read_offset += size;
    }

    // Write POD types
    template <typename T>
    void write(const T& value) {
        static_assert(std::is_trivially_copyable<T>::value, "Type must be trivially copyable");
        write(&value, sizeof(T));
    }

    // Read POD types
    template <typename T>
    void read(T& value) {
        static_assert(std::is_trivially_copyable<T>::value, "Type must be trivially copyable");
        read(&value, sizeof(T));
    }

    // Reset reading position
    void resetRead() {
        read_offset = 0;
        out_of_bounds = false;
    }

    // Access underlying data
    const std::vector<uint8_t>& data() const { return buffer; }
    std::vector<uint8_t>& data() { return buffer; }

    size_t size() const { return buffer.size(); }
    bool hasOverflow() const { return overflow; }
    bool hasReadError() const { return out_of_bounds; }

    void clear() {
        buffer.clear();
        read_offset = 0;
        overflow = false;
        out_of_bounds = false;
    }

private:
    std::vector<uint8_t> buffer;
    size_t read_offset;
    bool overflow;
    bool out_of_bounds;
};

#endif // PAYLOAD_H