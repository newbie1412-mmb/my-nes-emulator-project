#pragma once
#include "Mapper.h"

class Mapper_009 : public Mapper {
public:
    Mapper_009(uint8_t prgBanks, uint8_t chrBanks);
    ~Mapper_009();

    bool cpuMapRead(uint16_t addr, uint32_t& mapped_addr) override;
    bool cpuMapWrite(uint16_t addr, uint32_t& mapped_addr, uint8_t data) override;
    bool ppuMapRead(uint16_t addr, uint32_t& mapped_addr) override;
    bool ppuMapWrite(uint16_t addr, uint32_t& mapped_addr) override;
    void reset() override;
    MIRROR mirror() override; // Kích hoạt chức năng lật màn hình

private:
    uint8_t nPRGBank = 0;
    uint8_t nCHRBank0_FD8 = 0;
    uint8_t nCHRBank0_FE8 = 0;
    uint8_t nCHRBank1_FD8 = 0;
    uint8_t nCHRBank1_FE8 = 0;
    uint8_t nLatch0 = 0;
    uint8_t nLatch1 = 0;
    MIRROR mirromode = MIRROR::HORIZONTAL;
};