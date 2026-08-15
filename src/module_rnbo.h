#pragma once
// module_rnbo.h v0.1.0
// ============================================================
// Declaração pública do módulo RNBO.
//
// Este header NÃO inclui nada do RNBO — de propósito. Quem o
// inclui (main.cpp) continua limpo. Todo o contato com os
// headers do RNBO fica confinado em module_rnbo.cpp, espelhando
// a regra do hal_<board>.cpp.
// ============================================================

#include "module.h"
#include <cstddef>

namespace cotuca
{

#define COTUCA_MODULE_RNBO_VERSION "0.1.0"

// Extrato do alocador do RNBO. O RNBO aloca no boot (sinais,
// linha de delay); nós interceptamos com RNBO_USECUSTOMALLOCATOR
// e servimos de um pool interno, caindo pra hal::AllocBig() —
// isto é, SDRAM — quando não couber.
//
// Serve pra ver, no log de boot, o que foi parar em cada lugar.
struct RnboAllocStats
{
    size_t internal_bytes; // servido do pool em SRAM
    size_t big_bytes;      // servido pela SDRAM via AllocBig()
    size_t internal_cap;   // tamanho do pool interno
    int    blocks;         // quantas alocações
    int    failures;       // quantas voltaram nullptr
};

const RnboAllocStats& RnboAllocStatsGet();

} // namespace cotuca
