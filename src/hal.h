#pragma once
// ============================================================
// Cotucatronics OS — HAL (Hardware Abstraction Layer)
// A fronteira de independência de fornecedor.
//
// Regra de ouro: NADA fora de hal_<board>.cpp pode incluir
// headers de fabricante (daisy_seed.h, Arduino.h, etc).
// main.cpp, params, macros, manifests e módulos RNBO só
// conhecem este arquivo.
//
// Trocar de placa = escrever um novo hal_<board>.cpp
// e compilar com: make BOARD=<nome>
// ============================================================

#include <cstdint>
#include <cstddef>

namespace cotuca::hal
{

// ---- capacidades da placa (preenchido pela implementação) ----
struct BoardInfo
{
    const char* name;         // "Daisy Seed", "Teensy 4.1"...
    int         num_knobs;    // ADCs disponíveis no painel
    bool        has_audio_in; // false = só síntese/playback
    float       sample_rate;
    size_t      block_size;
    size_t      big_ram_bytes; // RAM "grande" p/ delays longos, freeze etc.
                               // Daisy: 64MB SDRAM. Placas sem: 0.
};

// ---- áudio ----
// Callback genérico: buffers de float não-intercalados.
// in/out[canal][amostra]. Canais fixos em 2 por simplicidade.
using AudioCallback = void (*)(const float* const* in,
                               float**             out,
                               size_t              size);

// ---- serial (painel falso / debug) ----
using SerialLineCallback = void (*)(const char* line);

// ---- contrato que toda placa implementa ----
bool             Init();                       // clocks, codec, power
const BoardInfo& Info();
void             StartAudio(AudioCallback cb);
float            ReadKnob(int index);          // 0.0–1.0 (stub até ter pots)
bool             ReadButton(int index);        // true = pressionado
void             SetLed(bool on);
void             Log(const char* fmt, ...);    // printf via USB/serial
void             SetSerialLineCallback(SerialLineCallback cb);
void             PollSerial();                 // chamar no loop principal
void             DelayMs(uint32_t ms);

// Aloca da RAM "grande" (SDRAM na Daisy). Chame só no boot,
// nunca no callback de áudio. Retorna nullptr se a placa não
// tem essa capacidade ou se acabou — quem chama DEVE checar.
void*            AllocBig(size_t bytes);

} // namespace cotuca::hal
