#pragma once
#include <cstdint>
#include <memory>
#include "PPU.h"
#include "APU.h"
#include "Cartridge.h"

class Bus {
public:
    Bus() {
        n_apu.bus = this;
        for (int i = 0; i < 2048; i++) {
            ram[i] = 0xFF;
        }
    }

    uint8_t controller_state = 0x00;
    uint8_t controller_strobe = 0x00;
    uint8_t controller_shift = 0x00;

    bool dma_transfer = false;

    PPU* ppu = nullptr;
    APU n_apu;

    // Bộ nhớ RAM nội bộ (2KB) - ĐÃ BỎ = { 0 } ĐỂ TRÁNH LỖI!
    uint8_t ram[2048];

    // === KHE CẮM BĂNG GAME ===
    std::shared_ptr<Cartridge> cart;
    void insertCartridge(const std::shared_ptr<Cartridge>& cartridge) {
        this->cart = cartridge;
        if (ppu != nullptr) {
            ppu->ConnectCartridge(cartridge);
        }
    }

    void cpuWrite(uint16_t addr, uint8_t data) {
        if (cart != nullptr && cart->cpuWrite(addr, data)) {
            return;
        }

        if (addr >= 0x0000 && addr <= 0x1FFF) {
            ram[addr & 0x07FF] = data;
        }
        else if (addr >= 0x2000 && addr <= 0x3FFF) {
            if (ppu != nullptr)
                ppu->cpuWrite(addr & 0x0007, data);
        }
        // =========================================================
        // 2. SỬA DMA CÓ THỜI GIAN THẬT (REAL TIME OAM DMA)
        // =========================================================
        else if (addr == 0x4014) {
            uint16_t dma_page = (uint16_t)data << 8;
            for (int i = 0; i < 256; i++) {
                if (ppu != nullptr) {
                    // Đọc từ Bus tại địa chỉ (Trang DMA + i) rồi ném vào OAM
                    // ppu->oam_addr sẽ tự động tăng và quay vòng sau 256 lần
                    ((uint8_t*)ppu->OAM)[ppu->oam_addr++] = cpuRead(dma_page | i);
                }
            }
            // Bật cờ báo hiệu DMA để CPU nghỉ ngơi (giữ nguyên của anh)
            dma_transfer = true;
        }
        else if (addr == 0x4016) {
            controller_strobe = data & 0x01;
            controller_shift = controller_state;
        }
        else if ((addr >= 0x4000 && addr <= 0x4013) || addr == 0x4015 || addr == 0x4017) {
            n_apu.cpuWrite(addr, data);
        }
    }

    uint8_t cpuRead(uint16_t addr) {
        uint8_t data = 0x00;
        if (cart != nullptr && cart->cpuRead(addr, data)) {
            return data;
        }

        if (addr >= 0x0000 && addr <= 0x1FFF) {
            data = ram[addr & 0x07FF];
        }
        else if (addr == 0x4016) {
            if (controller_strobe) {
                data = (controller_state & 0x80) ? 1 : 0;
            }
            else {
                data = (controller_shift & 0x80) ? 1 : 0;
                controller_shift <<= 1;
            }
            data |= 0x40;
        }
        else if (addr >= 0x2000 && addr <= 0x3FFF) {
            if (ppu != nullptr)
                data = ppu->cpuRead(addr & 0x0007);
        }
        return data;
    }
};