#include "Mapper_087.h"

Mapper_087::Mapper_087(uint8_t prgBanks, uint8_t chrBanks) : Mapper(prgBanks, chrBanks) {
    reset();
}

Mapper_087::~Mapper_087() {}

void Mapper_087::reset() {
    chrBankSelect = 0;
}

bool Mapper_087::cpuMapRead(uint16_t addr, uint32_t& mapped_addr) {
    // Đọc Code (PRG): Mapper 87 luôn cố định 32KB Code ở địa chỉ $8000 - $FFFF
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        // Nếu game chỉ có 16KB Code thì copy thành 32KB (Mirroring), nếu có 32KB thì đọc thẳng
        mapped_addr = addr & (nPRGBanks > 1 ? 0x7FFF : 0x3FFF);
        return true;
    }
    return false;
}

bool Mapper_087::cpuMapWrite(uint16_t addr, uint32_t& mapped_addr, uint8_t data) {
    // Chìa khóa của Mapper 87: Game gửi lệnh đổi hình vào dải địa chỉ $6000 - $7FFF
    if (addr >= 0x6000 && addr <= 0x7FFF) {

        // GIẢI MÃ ĐIỆN TỬ: Đảo ngược bit 0 và bit 1 của dữ liệu
        // Ví dụ: game gửi 01 (bằng 1) -> Đổi thành 10 (bằng 2)
        //        game gửi 10 (bằng 2) -> Đổi thành 01 (bằng 1)
        uint8_t bit0 = (data & 0x01) << 1;
        uint8_t bit1 = (data & 0x02) >> 1;

        // Gộp lại để ra số thứ tự cuộn hình chính xác
        chrBankSelect = bit0 | bit1;
        return true;
    }

    // Ngăn chặn việc CPU ghi đè bậy bạ vào ROM (tránh lỗi ngất xỉu màn hình đen)
    return false;
}

bool Mapper_087::ppuMapRead(uint16_t addr, uint32_t& mapped_addr) {
    // PPU hỏi xin dữ liệu hình ảnh (Kích thước 8KB)
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        // Dịch con trỏ đến đúng vị trí cuộn hình đang được chọn
        mapped_addr = (chrBankSelect * 0x2000) + addr;
        return true;
    }
    return false;
}

bool Mapper_087::ppuMapWrite(uint16_t addr, uint32_t& mapped_addr) {
    // Băng TwinBee dùng ROM (Read-Only Memory) cho đồ họa, tuyệt đối không cho ghi đè
    return false;
}