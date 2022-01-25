//Copyright (c) 2015-2021 Jonathan Gilchrist
//All rights reserved.
//Source: https://github.com/jgilchrist/gbemu/blob/master/src/util/bitwise.h
#pragma once
#include <cstdint>

namespace bitwise {

    inline auto compose_bits(const uint8_t high, const uint8_t low) -> uint8_t {
        return static_cast<uint8_t>(high << 1 | low);
    }

    inline auto compose_nibbles(const uint8_t high, const uint8_t low) -> uint8_t {
        return static_cast<uint8_t>((high << 4) | low);
    }

    inline auto compose_bytes(const uint8_t high, const uint8_t low) -> uint16_t {
        return static_cast<uint16_t>((high << 8) | low);
    }

    inline auto check_bit(const uint8_t value, const uint8_t bit) -> bool { return (value & (1 << bit)) != 0; }

    inline auto bit_value(const uint8_t value, const uint8_t bit) -> uint8_t { return (value >> bit) & 1; }

    inline auto set_bit(const uint8_t value, const uint8_t bit) -> uint8_t {
        auto value_set = value | (1 << bit);
        return static_cast<uint8_t>(value_set);
    }

    inline auto clear_bit(const uint8_t value, const uint8_t bit) -> uint8_t {
        auto value_cleared = value & ~(1 << bit);
        return static_cast<uint8_t>(value_cleared);
    }

    inline auto set_bit_to(const uint8_t value, const uint8_t bit, bool bit_on) -> uint8_t {
        return bit_on
            ? set_bit(value, bit)
            : clear_bit(value, bit);
    }

    inline auto compose_bits(const uint16_t high, const uint16_t low) -> uint16_t {
        return static_cast<uint16_t>(high << 1 | low);
    }

    inline auto compose_nibbles(const uint16_t high, const uint16_t low) -> uint16_t {
        return static_cast<uint16_t>((high << 4) | low);
    }

    inline auto compose_bytes(const uint16_t high, const uint16_t low) -> uint16_t {
        return static_cast<uint16_t>((high << 8) | low);
    }

    inline auto check_bit(const uint16_t value, const uint16_t bit) -> bool { return (value & (1 << bit)) != 0; }

    inline auto bit_value(const uint16_t value, const uint16_t bit) -> uint16_t { return (value >> bit) & 1; }

    inline auto set_bit(const uint16_t value, const uint16_t bit) -> uint16_t {
        auto value_set = value | (1 << bit);
        return static_cast<uint16_t>(value_set);
    }

    inline auto clear_bit(const uint16_t value, const uint16_t bit) -> uint16_t {
        auto value_cleared = value & ~(1 << bit);
        return static_cast<uint16_t>(value_cleared);
    }

    inline auto set_bit_to(const uint16_t value, const uint16_t bit, bool bit_on) -> uint16_t {
        return bit_on
            ? set_bit(value, bit)
            : clear_bit(value, bit);
    }

} // namespace bitwise