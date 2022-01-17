//Copyright (c) 2015-2021 Jonathan Gilchrist
//All rights reserved.
//Source: https://github.com/jgilchrist/gbemu/blob/master/src/register.cc
#include "pch.h"
#include "register.h"

#include "bitwise.h"

void ByteRegister::set(const uint8_t new_value) {
    val = new_value;
}

void ByteRegister::reset() {
    val = 0;
}

inline auto ByteRegister::value() const -> uint8_t { return val; }

auto ByteRegister::check_bit(uint8_t bit) const -> bool { return bitwise::check_bit(val, bit); }

void ByteRegister::set_bit_to(uint8_t bit, bool set) {
    val = bitwise::set_bit_to(val, bit, set);
}

void ByteRegister::increment() {
    val += 1;
}
void ByteRegister::decrement() {
    val -= 1;
}

auto ByteRegister::operator==(uint8_t other) const -> bool { return val == other; }

ByteRegister& ByteRegister::operator|=(uint8_t rhs)
{
    val |= rhs;
    return *this;
}


void WordRegister::set(const uint16_t new_value) {
    val = new_value;
}
inline auto WordRegister::value() const -> uint16_t { return val; }

auto WordRegister::low() const -> uint8_t { return static_cast<uint8_t>(val); }

auto WordRegister::high() const -> uint8_t { return static_cast<uint8_t>((val) >> 8); }

void WordRegister::increment() {
    val += 1;
}
void WordRegister::decrement() {
    val -= 1;
}

RegisterPair::RegisterPair(ByteRegister& high, ByteRegister& low) :
    low_byte(low),
    high_byte(high)
{
}

void RegisterPair::set(const uint16_t word) {
    /* Discard the upper byte */
    low_byte.set(static_cast<uint8_t>(word));

    /* Discard the lower byte */
    high_byte.set(static_cast<uint8_t>((word) >> 8));
}

auto RegisterPair::low() const -> uint8_t { return low_byte.value(); }

auto RegisterPair::high() const -> uint8_t { return high_byte.value(); }

inline auto RegisterPair::value() const -> uint16_t {
    return bitwise::compose_bytes(high_byte.value(), low_byte.value());
}

void RegisterPair::increment() {
    set(value() + 1);
}

void RegisterPair::decrement() {
    set(value() - 1);
}


void FlagRegister::set(const uint8_t new_value) {
    val = new_value & 0xF0;
}

void FlagRegister::set_flag_zero(bool set) {
    set_bit_to(7, set);
}

void FlagRegister::set_flag_subtract(bool set) {
    set_bit_to(6, set);
}

void FlagRegister::set_flag_half_carry(bool set) {
    set_bit_to(5, set);
}

void FlagRegister::set_flag_carry(bool set) {
    set_bit_to(4, set);
}

auto FlagRegister::flag_zero() const -> bool { return check_bit(7); }

auto FlagRegister::flag_subtract() const -> bool { return check_bit(6); }

auto FlagRegister::flag_half_carry() const -> bool { return check_bit(5); }

auto FlagRegister::flag_carry() const -> bool { return check_bit(4); }

auto FlagRegister::flag_zero_value() const -> uint8_t { return static_cast<uint8_t>(flag_zero() ? 1 : 0); }

auto FlagRegister::flag_subtract_value() const -> uint8_t {
    return static_cast<uint8_t>(flag_subtract() ? 1 : 0);
}

auto FlagRegister::flag_half_carry_value() const -> uint8_t {
    return static_cast<uint8_t>(flag_half_carry() ? 1 : 0);
}

auto FlagRegister::flag_carry_value() const -> uint8_t { return static_cast<uint8_t>(flag_carry() ? 1 : 0); }

Address::Address(uint16_t location) : addr(location) {
}

Address::Address(const RegisterPair& from) : addr(from.value()) {
}

Address::Address(const WordRegister& from) : addr(from.value()) {
}

inline auto Address::value() const -> uint16_t { return addr; }

auto Address::in_range(Address low, Address high) const -> bool {
    return low.value() <= value() && value() <= high.value();
}

auto Address::operator==(uint16_t other) const -> bool { return addr == other; }

auto Address::operator+(unsigned int other) const -> Address {
    uint16_t new_addr = static_cast<uint16_t>(addr + other);
    return Address{ new_addr };
}

auto Address::operator-(unsigned int other) const -> Address {
    uint16_t new_addr = static_cast<uint16_t>(addr - other);
    return Address{ new_addr };
}
