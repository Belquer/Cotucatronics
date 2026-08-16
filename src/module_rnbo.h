#pragma once
// module_rnbo.h v0.2.0
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

#define COTUCA_MODULE_RNBO_VERSION "0.2.0"

// Este header existe só pra documentar que o módulo RNBO EXISTE.
// Ele NÃO é incluído pelo main: o main pega o módulo pelo registro
// de module.h e lê a memória por Module::Memory().
//
// O extrato do alocador (pool em SRAM + queda pra hal::AllocBig)
// era exposto aqui como RnboAllocStatsGet() e o main o chamava
// direto — o que obrigava o main a incluir este header e furava a
// fronteira pela porta dos fundos. Agora é Module::Memory(),
// genérico, e nada concreto vaza.

} // namespace cotuca
