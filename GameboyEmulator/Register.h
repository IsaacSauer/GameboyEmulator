//Copyright (c) 2015-2021 Jonathan Gilchrist
//All rights reserved.
//Source: https://github.com/jgilchrist/gbemu/blob/master/src/register.h
#pragma once

class ByteRegister
{
public:
    ByteRegister() = default;
    virtual ~ByteRegister() = default;

    virtual void set(uint8_t new_value);
    void reset();
    auto value() const->uint8_t;
    uint8_t& ref() { return val; }

    auto check_bit(uint8_t bit) const -> bool;
    void set_bit_to(uint8_t bit, bool set);

    void increment();
    void decrement();

    auto operator==(uint8_t other) const -> bool;
    ByteRegister& operator|=(uint8_t rhs);

protected:
    uint8_t val = 0x0;
};

class FlagRegister : public ByteRegister {
public:
    FlagRegister() = default;

    /* Specialise behaviour for the flag register 'f'.
     * (its lower nibble is always 0s */
    void set(uint8_t new_value) override;

    void set_flag_zero(bool set);
    void set_flag_subtract(bool set);
    void set_flag_half_carry(bool set);
    void set_flag_carry(bool set);

    auto flag_zero() const -> bool;
    auto flag_subtract() const -> bool;
    auto flag_half_carry() const -> bool;
    auto flag_carry() const -> bool;

    auto flag_zero_value() const->uint8_t;
    auto flag_subtract_value() const->uint8_t;
    auto flag_half_carry_value() const->uint8_t;
    auto flag_carry_value() const->uint8_t;
};

class WordValue
{
public:
    WordValue() = default;
    virtual ~WordValue() = default;

    virtual void set(uint16_t new_value) = 0;

    virtual auto value() const->uint16_t = 0;

    virtual auto low() const->uint8_t = 0;
    virtual auto high() const->uint8_t = 0;
};

class WordRegister : public WordValue {
public:
    WordRegister() = default;

    void set(uint16_t new_value) override;

    auto value() const->uint16_t override;

    auto low() const->uint8_t override;
    auto high() const->uint8_t override;

    void increment();
    void decrement();

private:
    uint16_t val = 0x0;
};

class RegisterPair : public WordValue {
public:
    RegisterPair(ByteRegister& high, ByteRegister& low);

    void set(uint16_t word) override;

    auto value() const->uint16_t override;

    auto low() const->uint8_t override;
    auto high() const->uint8_t override;

    void increment();
    void decrement();

private:
    ByteRegister& low_byte;
    ByteRegister& high_byte;
};

class Address
{
public:
    Address(uint16_t location);
    explicit Address(const RegisterPair& from);
    explicit Address(const WordRegister& from);

    auto value() const->uint16_t;

    auto in_range(Address low, Address high) const -> bool;

    auto operator==(uint16_t other) const -> bool;
    auto operator+(unsigned int other) const->Address;
    auto operator-(unsigned int other) const->Address;

private:
    uint16_t addr = 0x0;
};
