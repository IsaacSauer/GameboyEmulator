//Copyright (c) 2015-2021 Jonathan Gilchrist
//All rights reserved.
//Source: https://github.com/jgilchrist/gbemu/tree/master/src/cartridge

#pragma once

#include "Register.h"
#include "CartridgeInfo.h"

#include <string>
#include <vector>
#include <memory>

class Cartridge
{
public:
    Cartridge(std::vector<uint8_t> rom_data, const std::vector<uint8_t>& ram_data,
        std::unique_ptr<CartridgeInfo> cartridge_info);
    virtual ~Cartridge() = default;

    virtual uint8_t read(const Address& address) const = 0;
    virtual void write(const Address& address, uint8_t value) = 0;

    const std::vector<uint8_t>& get_cartridge_ram() const;

protected:
    std::vector<uint8_t> rom;
    std::vector<uint8_t> ram;

    std::unique_ptr<CartridgeInfo> cartridge_info;
};

std::shared_ptr<Cartridge> get_cartridge(const std::vector<uint8_t>& rom_data, const std::vector<uint8_t>& ram_data = {});

class NoMBC : public Cartridge
{
public:
    NoMBC(std::vector<uint8_t> rom_data, const std::vector<uint8_t>& ram_data,
        std::unique_ptr<CartridgeInfo> cartridge_info);

    uint8_t read(const Address& address) const override;
    void write(const Address& address, uint8_t value) override;
};

class MBC1 : public Cartridge 
{
public:
    MBC1(std::vector<uint8_t> rom_data, const std::vector<uint8_t>& ram_data,
        std::unique_ptr<CartridgeInfo> cartridge_info);

    uint8_t read(const Address& address) const override;
    void write(const Address& address, uint8_t value) override;

private:
    WordRegister rom_bank;
    WordRegister ram_bank;
    bool ram_enabled = false;

    // TODO: ROM/RAM Mode Select (6000-7FFF)
    // This 1bit Register selects whether the two bits of the above register should
    // be used as upper two bits of the ROM Bank, or as RAM Bank Number.
    bool rom_banking_mode = true;
};

class MBC3 : public Cartridge 
{
public:
    MBC3(std::vector<uint8_t> rom_data, const std::vector<uint8_t>& ram_data,
        std::unique_ptr<CartridgeInfo> cartridge_info);

    uint8_t read(const Address& address) const override;
    void write(const Address& address, uint8_t value) override;

private:
    WordRegister rom_bank;
    WordRegister ram_bank;
    bool ram_enabled = false;
    bool ram_over_rtc = true;

    // TODO: ROM/RAM Mode Select (6000-7FFF)
    // This 1bit Register selects whether the two bits of the above register should
    // be used as upper two bits of the ROM Bank, or as RAM Bank Number.
    bool rom_banking_mode = true;
};
