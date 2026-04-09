#include "Mapper_009.h"

Mapper_009::Mapper_009(uint8_t prgBanks, uint8_t chrBanks) : Mapper(prgBanks, chrBanks) { reset(); }
Mapper_009::~Mapper_009() {}

void Mapper_009::reset() {
    nPRGBank = 0;
    nCHRBank0_FD8 = 0;
    nCHRBank0_FE8 = 0;
    nCHRBank1_FD8 = 0;
    nCHRBank1_FE8 = 0;
    nLatch0 = 0;
    nLatch1 = 0;
    // Đã sửa lại thành chữ thay vì số 0
    mirromode = HORIZONTAL;
}

// Đã sửa lại thành MIRROR thay vì int
MIRROR Mapper_009::mirror() {
    return mirromode;
}

bool Mapper_009::cpuMapRead(uint16_t addr, uint32_t& mapped_addr) {
    if (addr >= 0x6000 && addr <= 0x7FFF) {
        mapped_addr = addr & 0x1FFF;
        return true;
    }

    if (addr >= 0x8000 && addr <= 0x9FFF) {
        mapped_addr = (nPRGBank * 0x2000) + (addr & 0x1FFF);
        return true;
    }
    if (addr >= 0xA000 && addr <= 0xBFFF) {
        mapped_addr = ((nPRGBanks * 2 - 3) * 0x2000) + (addr & 0x1FFF);
        return true;
    }
    if (addr >= 0xC000 && addr <= 0xDFFF) {
        mapped_addr = ((nPRGBanks * 2 - 2) * 0x2000) + (addr & 0x1FFF);
        return true;
    }
    if (addr >= 0xE000 && addr <= 0xFFFF) {
        mapped_addr = ((nPRGBanks * 2 - 1) * 0x2000) + (addr & 0x1FFF);
        return true;
    }
    return false;
}

bool Mapper_009::cpuMapWrite(uint16_t addr, uint32_t& mapped_addr, uint8_t data) {
    // 1. Chỉ ĐỘC NHẤT RAM phụ (6000-7FFF) là được trả về TRUE để lưu Save
    if (addr >= 0x6000 && addr <= 0x7FFF) {
        mapped_addr = addr & 0x1FFF;
        return true;
    }

    // 2. TẤT CẢ các lệnh cấu hình chip bên dưới PHẢI TRẢ VỀ FALSE
    // Nếu trả về true, game sẽ tự ghi đè làm nát bét ROM và Crash xanh màn hình!
    if (addr >= 0xA000 && addr <= 0xAFFF) {
        nPRGBank = data & 0x0F;
        return false; // <--- Đổi thành false
    }
    if (addr >= 0xB000 && addr <= 0xBFFF) {
        nCHRBank0_FD8 = data & 0x1F;
        return false; // <--- Đổi thành false
    }
    if (addr >= 0xC000 && addr <= 0xCFFF) {
        nCHRBank0_FE8 = data & 0x1F;
        return false; // <--- Đổi thành false
    }
    if (addr >= 0xD000 && addr <= 0xDFFF) {
        nCHRBank1_FD8 = data & 0x1F;
        return false; // <--- Đổi thành false
    }
    if (addr >= 0xE000 && addr <= 0xEFFF) {
        nCHRBank1_FE8 = data & 0x1F;
        return false; // <--- Đổi thành false
    }
    if (addr >= 0xF000 && addr <= 0xFFFF) {
        if (data & 0x01) mirromode = HORIZONTAL;
        else mirromode = VERTICAL;
        return false; // <--- Đổi thành false
    }
    return false;
}
bool Mapper_009::ppuMapRead(uint16_t addr, uint32_t& mapped_addr) {
    if (addr <= 0x1FFF) {
        if (addr <= 0x0FFF) {
            if (nLatch0 == 0) mapped_addr = (nCHRBank0_FD8 * 0x1000) + (addr & 0x0FFF);
            else              mapped_addr = (nCHRBank0_FE8 * 0x1000) + (addr & 0x0FFF);
        }
        else {
            if (nLatch1 == 0) mapped_addr = (nCHRBank1_FD8 * 0x1000) + (addr & 0x0FFF);
            else              mapped_addr = (nCHRBank1_FE8 * 0x1000) + (addr & 0x0FFF);
        }

        if ((addr & 0x1FF8) == 0x0FD8) nLatch0 = 0;
        else if ((addr & 0x1FF8) == 0x0FE8) nLatch0 = 1;
        else if ((addr & 0x1FF8) == 0x1FD8) nLatch1 = 0;
        else if ((addr & 0x1FF8) == 0x1FE8) nLatch1 = 1;

        return true;
    }
    return false;
}

bool Mapper_009::ppuMapWrite(uint16_t addr, uint32_t& mapped_addr) {
    return false;
}