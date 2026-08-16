#pragma once
// hal.h v0.6.0
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
    uint32_t    cpu_hz;        // clock real do núcleo, lido da placa —
                               // não o nominal. Boost muda isto.
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

// Pede um tamanho de bloco. Chamar ANTES de StartAudio; depois
// dele não tem efeito. A placa pode recusar e manter o seu — o
// valor efetivo fica em Info().block_size, que é a verdade.
//
// É o manifest quem pede, não a placa quem impõe: bloco define
// LATÊNCIA, e latência é característica do instrumento.
void             RequestBlockSize(size_t block_size);
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
//
// Este é o ÚNICO portão de acesso à RAM grande no projeto.
// Declarar DSY_SDRAM_BSS (ou equivalente) fora de hal_<board>.cpp
// é proibido — ver BOARDS.md.
void*            AllocBig(size_t bytes);

// Carga do bloco de áudio, 0.0–1.0 (1.0 = estourou o prazo).
// Média e pico desde o boot. Placa sem medidor devolve -1.
float            CpuLoadAvg();
float            CpuLoadMax();

// Zera média e pico. Necessário pra rearmar o watchdog de módulo:
// CpuLoadMax() é monotônico, então sem reset um estouro passado
// travaria o watchdog pra sempre.
void             CpuLoadReset();

// Total já entregue por AllocBig(), em bytes. Serve de extrato:
// o boot loga isto, e módulo que aloque por fora não aparece aqui.
size_t           AllocBigUsed();

} // namespace cotuca::hal
