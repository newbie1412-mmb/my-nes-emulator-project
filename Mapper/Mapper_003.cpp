#include "Mapper_003.h"

Mapper_003::Mapper_003(uint8_t prgBanks, uint8_t chrBanks)
    : Mapper(prgBanks, chrBanks)
{
    reset();
}

Mapper_003::~Mapper_003() {}

bool Mapper_003::cpuMapRead(uint16_t addr, uint32_t& mapped_addr) {
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        // Mapper 3 không tráo PRG. 
        // Nếu game nhẹ (1 cuộn PRG - 16KB) thì nó tự soi gương (mirror) lặp lại.
        // Nếu game nặng (2 cuộn PRG - 32KB) thì lấy nguyên dải 32KB.
        if (nPRGBanks == 1) {
            mapped_addr = addr & 0x3FFF;
        }
        else {
            mapped_addr = addr & 0x7FFF;
        }
        return true;
    }
    return false;
}

bool Mapper_003::cpuMapWrite(uint16_t addr, uint32_t& mapped_addr, uint8_t data) {
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        // CHÌA KHÓA CỦA MAPPER 3: Ghi vào CPU để đổi cuộn băng hình (CHR)
        // Thường CNROM chỉ dùng 2 bit cuối của data để chọn 4 bank CHR (0-3)
        nCHRBankSelect = data & 0x03;
        if (nCHRBanks > 0) {
            nCHRBankSelect %= nCHRBanks;
        }

        // Vẫn trả về false để báo là KHÔNG ghi đè lên ROM
        return false;
    }
    return false;
}

bool Mapper_003::ppuMapRead(uint16_t addr, uint32_t& mapped_addr) {
    if (addr <= 0x1FFF) {
        // PPU đòi đọc hình, Mapper sẽ trỏ tới đúng cái Bank CHR đang được chọn
        mapped_addr = (nCHRBankSelect * 0x2000) + addr;
        return true;
    }
    return false;
}

bool Mapper_003::ppuMapWrite(uint16_t addr, uint32_t& mapped_addr) {
    // Mapper 3 thường là băng ROM (không cho ghi đè hình)
    return false;
}

void Mapper_003::reset() {
    nCHRBankSelect = 0;
}