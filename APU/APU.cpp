#include "APU.h"
#include "Bus.h"

// --- CÁC BẢNG TRA CỨU CHUẨN NES ---
const uint16_t dmc_rate_table[16] = {
    428, 380, 340, 320, 286, 254, 226, 214,
    190, 160, 142, 128, 106, 84, 72, 54
};

const uint8_t length_table[32] = {
    10, 254, 20,  2, 40,  4, 80,  6,
    160,  8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22,
    192, 24, 72, 26, 16, 28, 32, 30
};

const uint16_t noise_period_table[16] = {
    4, 8, 16, 32, 64, 96, 128, 160,
    202, 254, 380, 508, 762, 1016, 2034, 4068
};

const uint8_t duty_table[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 0, 0, 0},
    {1, 0, 0, 1, 1, 1, 1, 1}
};

APU::APU() {
    // Noise LFSR phải khởi tạo = 1, nếu không noise sẽ chết cứng
    noise.shift_register = 1;
}

APU::~APU() {}

// --- NHẬN LỆNH TỪ CPU (0x4000 - 0x4017) ---
void APU::cpuWrite(uint16_t addr, uint8_t data) {
    switch (addr) {

        // =========================
        // Pulse 1
        // =========================
    case 0x4000:
        pulse1.duty = (data & 0xC0) >> 6;
        pulse1.env.loop = (data & 0x20) != 0;
        pulse1.env.constant_volume = (data & 0x10) != 0;
        pulse1.env.volume = data & 0x0F;
        break;

    case 0x4001:
        pulse1.sweep.enabled = (data & 0x80) != 0;
        pulse1.sweep.period = (data >> 4) & 0x07;
        pulse1.sweep.negate = (data & 0x08) != 0;
        pulse1.sweep.shift = data & 0x07;
        pulse1.sweep.reload = true;
        break;

    case 0x4002:
        pulse1.timer_reload = (pulse1.timer_reload & 0xFF00) | data;
        break;

    case 0x4003:
        pulse1.timer_reload = ((uint16_t)(data & 0x07) << 8) | (pulse1.timer_reload & 0x00FF);
        pulse1.timer = pulse1.timer_reload;
        // KHÓA LẠI: Chỉ nạp khi kênh đang mở!
        if (pulse1_enabled) pulse1.length_counter = length_table[(data & 0xF8) >> 3];
        pulse1.pulse_phase = 0;
        pulse1.env.start = true;
        break;

        // =========================
        // Pulse 2
        // =========================
    case 0x4004:
        pulse2.duty = (data & 0xC0) >> 6;
        pulse2.env.loop = (data & 0x20) != 0;
        pulse2.env.constant_volume = (data & 0x10) != 0;
        pulse2.env.volume = data & 0x0F;
        break;

    case 0x4005:
        pulse2.sweep.enabled = (data & 0x80) != 0;
        pulse2.sweep.period = (data >> 4) & 0x07;
        pulse2.sweep.negate = (data & 0x08) != 0;
        pulse2.sweep.shift = data & 0x07;
        pulse2.sweep.reload = true;
        break;

    case 0x4006:
        pulse2.timer_reload = (pulse2.timer_reload & 0xFF00) | data;
        break;

    case 0x4007:
        pulse2.timer_reload = ((uint16_t)(data & 0x07) << 8) | (pulse2.timer_reload & 0x00FF);
        pulse2.timer = pulse2.timer_reload;
        if (pulse2_enabled) pulse2.length_counter = length_table[(data & 0xF8) >> 3];
        pulse2.pulse_phase = 0;
        pulse2.env.start = true;
        break;

        // =========================
        // Triangle
        // =========================
    case 0x4008:
        triangle.control_flag = (data & 0x80) != 0;
        triangle.linear_reload_value = data & 0x7F;
        break;

    case 0x400A:
        triangle.timer_reload = (triangle.timer_reload & 0xFF00) | data;
        break;

    case 0x400B:
        triangle.timer_reload = ((uint16_t)(data & 0x07) << 8) | (triangle.timer_reload & 0x00FF);
        triangle.timer = triangle.timer_reload;
        if (triangle_enabled) triangle.length_counter = length_table[(data & 0xF8) >> 3];
        triangle.linear_reload_flag = true;
        break;

        // =========================
        // Noise
        // =========================
    case 0x400C:
        noise.env.loop = (data & 0x20) != 0;
        noise.env.constant_volume = (data & 0x10) != 0;
        noise.env.volume = data & 0x0F;
        break;

    case 0x400E:
        noise.mode = (data & 0x80) != 0;
        noise.timer_reload = noise_period_table[data & 0x0F];
        break;

    case 0x400F:
        if (noise_enabled) noise.length_counter = length_table[(data & 0xF8) >> 3];
        noise.timer = noise.timer_reload;
        noise.env.start = true;
        break;

        // =========================
        // DMC
        // =========================
    case 0x4010:
        dmc.irq_enable = (data & 0x80) != 0;
        dmc.loop = (data & 0x40) != 0;
        dmc.rate_index = data & 0x0F;
        dmc.timer_reload = dmc_rate_table[dmc.rate_index];
        break;

    case 0x4011:
        dmc.output_level = data & 0x7F;
        break;

    case 0x4012:
        dmc.sample_address = 0xC000 | (data << 6);
        break;

    case 0x4013:
        dmc.sample_length = (data << 4) + 1;
        break;

        // =========================
        // Channel Enable
        // =========================
    case 0x4015:
        pulse1_enabled = (data & 0x01) != 0;
        pulse2_enabled = (data & 0x02) != 0;
        triangle_enabled = (data & 0x04) != 0;
        noise_enabled = (data & 0x08) != 0;
        dmc_enabled = (data & 0x10) != 0;

        if (!pulse1_enabled)   pulse1.length_counter = 0;
        if (!pulse2_enabled)   pulse2.length_counter = 0;
        if (!triangle_enabled) triangle.length_counter = 0;
        if (!noise_enabled)    noise.length_counter = 0;

        if (dmc_enabled) {
            if (dmc.bytes_remaining == 0) {
                dmc.current_address = dmc.sample_address;
                dmc.bytes_remaining = dmc.sample_length;
                dmc.bits_remaining = 0;
                dmc.timer = dmc.timer_reload;
            }
        }
        else {
            dmc.bytes_remaining = 0;
            dmc.bits_remaining = 0;
        }
        break;
    }
}

// --- BỘ MÁY CHẠY NHỊP APU ---
void APU::Step() {
    // =========================
    // 1) Pulse chạy mỗi 2 CPU cycles
    // =========================
    static bool apu_half_clock = false;
    apu_half_clock = !apu_half_clock;

    if (apu_half_clock) {
        if (pulse1_enabled) {
            if (pulse1.timer > 0) pulse1.timer--;
            else {
                pulse1.timer = pulse1.timer_reload;
                pulse1.pulse_phase = (pulse1.pulse_phase + 1) % 8;
            }
        }

        if (pulse2_enabled) {
            if (pulse2.timer > 0) pulse2.timer--;
            else {
                pulse2.timer = pulse2.timer_reload;
                pulse2.pulse_phase = (pulse2.pulse_phase + 1) % 8;
            }
        }
    }

    // =========================
    // 2) Triangle chạy full CPU
    // =========================
    if (triangle_enabled && triangle.length_counter > 0 && triangle.linear_counter > 0) {
        if (triangle.timer > 0) triangle.timer--;
        else {
            triangle.timer = triangle.timer_reload;
            triangle.tri_phase = (triangle.tri_phase + 1) % 32;
        }
    }

    // =========================
    // 3) Noise chạy full CPU
    // =========================
    if (noise_enabled) {
        if (noise.timer > 0) noise.timer--;
        else {
            noise.timer = noise.timer_reload;
            int bit_to_mux = noise.mode ? 6 : 1;
            uint16_t feedback = (noise.shift_register & 0x01) ^ ((noise.shift_register >> bit_to_mux) & 0x01);
            noise.shift_register = (noise.shift_register >> 1) | (feedback << 14);
        }
    }

    // =========================
    // 4) DMC chạy full CPU
    // =========================
    if (dmc_enabled) {
        if (dmc.timer > 0) dmc.timer--;
        else {
            dmc.timer = dmc.timer_reload;

            // Xuất từng bit
            if (dmc.bits_remaining > 0) {
                if (dmc.shift_register & 1) {
                    if (dmc.output_level <= 125) dmc.output_level += 2;
                }
                else {
                    if (dmc.output_level >= 2) dmc.output_level -= 2;
                }

                dmc.shift_register >>= 1;
                dmc.bits_remaining--;
            }

            // Nạp byte mới khi hết bit
            if (dmc.bits_remaining == 0 && dmc.bytes_remaining > 0) {
                dmc.shift_register = cpuRead(dmc.current_address);
                dmc.bits_remaining = 8;

                dmc.current_address++;
                if (dmc.current_address == 0xFFFF)
                    dmc.current_address = 0x8000;

                dmc.bytes_remaining--;

                if (dmc.bytes_remaining == 0 && dmc.loop) {
                    dmc.current_address = dmc.sample_address;
                    dmc.bytes_remaining = dmc.sample_length;
                }
            }
        }
    }

    // =========================
    // 5) Frame Sequencer ~240Hz
    // =========================
    static int frame_seq_count = 0;
    static int step_mode = 0;

    frame_seq_count++;
    if (frame_seq_count >= 7457) {
        frame_seq_count = 0;
        step_mode = (step_mode + 1) % 4;

        // ---- Quarter Frame ----
        pulse1.env.Clock();
        pulse2.env.Clock();
        noise.env.Clock();

        if (triangle.linear_reload_flag)
            triangle.linear_counter = triangle.linear_reload_value;
        else if (triangle.linear_counter > 0)
            triangle.linear_counter--;

        if (!triangle.control_flag)
            triangle.linear_reload_flag = false;

        // ---- Half Frame ----
        if (step_mode == 1 || step_mode == 3) {
            // Length counter
            if (!pulse1.env.loop && pulse1.length_counter > 0) pulse1.length_counter--;
            if (!pulse2.env.loop && pulse2.length_counter > 0) pulse2.length_counter--;
            if (!noise.env.loop && noise.length_counter > 0) noise.length_counter--;
            if (!triangle.control_flag && triangle.length_counter > 0) triangle.length_counter--;

            // =========================
            // Sweep Pulse 1
            // =========================
            if (pulse1.sweep.reload) {
                pulse1.sweep.timer = pulse1.sweep.period;
                pulse1.sweep.reload = false;
            }
            else if (pulse1.sweep.timer > 0) {
                pulse1.sweep.timer--;
            }
            else {
                pulse1.sweep.timer = pulse1.sweep.period;

                if (pulse1.sweep.enabled && pulse1.sweep.shift > 0 && pulse1.length_counter > 0) {
                    uint16_t change = pulse1.timer_reload >> pulse1.sweep.shift;
                    uint16_t target = pulse1.timer_reload;

                    if (pulse1.sweep.negate)
                        target = pulse1.timer_reload - change - 1;
                    else
                        target = pulse1.timer_reload + change;

                    if (target <= 0x7FF && target >= 8)
                        pulse1.timer_reload = target;
                }
            }

            // =========================
            // Sweep Pulse 2
            // =========================
            if (pulse2.sweep.reload) {
                pulse2.sweep.timer = pulse2.sweep.period;
                pulse2.sweep.reload = false;
            }
            else if (pulse2.sweep.timer > 0) {
                pulse2.sweep.timer--;
            }
            else {
                pulse2.sweep.timer = pulse2.sweep.period;

                if (pulse2.sweep.enabled && pulse2.sweep.shift > 0 && pulse2.length_counter > 0) {
                    uint16_t change = pulse2.timer_reload >> pulse2.sweep.shift;
                    uint16_t target = pulse2.timer_reload;

                    if (pulse2.sweep.negate)
                        target = pulse2.timer_reload - change;
                    else
                        target = pulse2.timer_reload + change;

                    if (target <= 0x7FF && target >= 8)
                        pulse2.timer_reload = target;
                }
            }
        }
    }
}

// --- BÀN TRỘN MIXER NES ---
float APU::GetOutputSample() {
    float p1_raw = 0.0f;
    float p2_raw = 0.0f;
    float tri_raw = 0.0f;
    float n_raw = 0.0f;
    float dmc_raw = 0.0f;

    // =======================================================
    // TÍNH TOÁN NGƯỠNG CÂM (SWEEP MUTING) CHO PULSE 1 & 2
    // Đây chính là liều thuốc giải cho tiếng rít "ing ing"!
    // =======================================================

    // Pulse 1 Muted?
    bool p1_muted = false;
    if (pulse1.timer_reload < 8) p1_muted = true;
    else {
        int change = pulse1.timer_reload >> pulse1.sweep.shift;
        int target = pulse1.sweep.negate ? (pulse1.timer_reload - change - 1) : (pulse1.timer_reload + change);
        if (target > 0x7FF) p1_muted = true;
    }

    // Pulse 2 Muted?
    bool p2_muted = false;
    if (pulse2.timer_reload < 8) p2_muted = true;
    else {
        int change = pulse2.timer_reload >> pulse2.sweep.shift;
        int target = pulse2.sweep.negate ? (pulse2.timer_reload - change) : (pulse2.timer_reload + change);
        if (target > 0x7FF) p2_muted = true;
    }

    // =========================
    // Lấy âm thanh Pulse
    // =========================
    if (pulse1_enabled && !p1_muted && pulse1.length_counter > 0 && duty_table[pulse1.duty][pulse1.pulse_phase]) {
        p1_raw = (float)pulse1.env.Output();
    }

    if (pulse2_enabled && !p2_muted && pulse2.length_counter > 0 && duty_table[pulse2.duty][pulse2.pulse_phase]) {
        p2_raw = (float)pulse2.env.Output();
    }

    // =========================
    // Triangle
    // =========================
    if (triangle.timer_reload > 2) {
        tri_raw = (triangle.tri_phase < 16)
            ? (float)triangle.tri_phase
            : (float)(31 - triangle.tri_phase);
    }
    else {
        // Tần số siêu âm (timer_reload cực nhỏ), NES thật sẽ khóa kênh tam giác
        // tri_raw giữ nguyên giá trị pha cuối cùng (thường là 7.5, nhưng cứ để yên là an toàn nhất)
        tri_raw = (triangle.tri_phase < 16)
            ? (float)triangle.tri_phase
            : (float)(31 - triangle.tri_phase);
    }
    // =========================
    // Noise
    // =========================
    if (noise_enabled && noise.length_counter > 0 && !(noise.shift_register & 0x01)) {
        n_raw = (float)noise.env.Output();
    }
    else {
        n_raw = 0.0f;
    }

    // =========================
    // DMC
    // =========================
    dmc_raw = (float)dmc.output_level;

    // =========================
    // NES Nonlinear Mixer
    // =========================
    float pulse_out = 0.0f;
    if ((p1_raw + p2_raw) > 0.0f) {
        pulse_out = 95.88f / ((8128.0f / (p1_raw + p2_raw)) + 100.0f);
    }

    float tnd_sum = (tri_raw / 8227.0f) + (n_raw / 12241.0f) + (dmc_raw / 22638.0f);
    float tnd_out = 0.0f;
    if (tnd_sum > 0.0f) {
        tnd_out = 159.79f / ((1.0f / tnd_sum) + 100.0f);
    }

    float output = pulse_out + tnd_out;

    // =========================
    // High-pass filter nhẹ
    // =========================
    float a = 0.995f;
    filter_y = a * (filter_y + output - filter_x);
    filter_x = output;

    return filter_y * 0.5f;
}

// --- DMC đọc sample từ BUS ---
uint8_t APU::cpuRead(uint16_t addr) {
    if (bus != nullptr) {
        return bus->cpuRead(addr);
    }
    return 0;
}