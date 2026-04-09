#pragma once
#include "Mapper.h"

class Mapper_002 : public Mapper {
public:
    Mapper_002(uint8_t prgBanks, uint8_t chrBanks);
    ~Mapper_002();

    // Đã xóa tham số 'data' ở hàm Read để khớp chuẩn với file Mapper.h gốc!
    bool cpuMapRead(uint16_t addr, uint32_t& mapped_addr) override;
    bool cpuMapWrite(uint16_t addr, uint32_t& mapped_addr, uint8_t data) override;

    bool ppuMapRead(uint16_t addr, uint32_t& mapped_addr) override;
    bool ppuMapWrite(uint16_t addr, uint32_t& mapped_addr) override;

    // Đã khai báo reset()
    void reset() override;

private:
    uint8_t nPRGBankSelectLo = 0x00;
};