#pragma once
// module_rnbo_alloc.h v0.1.0
// ============================================================
// Alocador compartilhado dos módulos RNBO.
//
// Vive fora dos namespaces de runtime de propósito: cada patcher
// carrega uma cópia isolada do runtime do RNBO (ver
// module_rnbo_delay.cpp), mas a MEMÓRIA é uma só — o pool de SRAM
// e a SDRAM do hal::AllocBig são recursos da placa, não de um
// motor. Dois alocadores independentes dariam dois extratos que
// não fecham com o do boot.
//
// Estratégia: pool estático em SRAM primeiro, porque sinais de
// bloco são lidos amostra a amostra e SDRAM é mais lenta. O que
// não couber — linhas de delay, buffers de freeze — cai pra
// hal::AllocBig(), respeitando o portão único do BOARDS.md.
//
// free() é no-op: bump não devolve. Como nada é liberado em
// runtime, isso não vaza — só torna a memória dos módulos
// vitalícia, que é o comportamento desejado.
// ============================================================

#include "hal.h"
#include "module.h"
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace cotuca
{
namespace rnbo_alloc
{

// 48 KB: o delay usa ~2 KB, o freeze é bem maior. Sobra é barata
// em SRAM (512 KB na Seed) e evita empurrar sinal de bloco pra
// SDRAM, que custaria CPU no callback.
constexpr size_t kPool   = 48u * 1024u;
constexpr size_t kHeader = 8; // guarda o tamanho, mantém alinhamento 8

inline uint8_t* Pool()
{
    alignas(8) static uint8_t pool[kPool];
    return pool;
}

inline size_t& Used()
{
    static size_t used = 0;
    return used;
}

inline Module::MemReport& Stats()
{
    static Module::MemReport s = {0, kPool, 0, 0, 0};
    return s;
}

inline size_t Align8(size_t n) { return (n + 7u) & ~size_t(7); }

inline void* Alloc(size_t size)
{
    const size_t total = Align8(size + kHeader);
    uint8_t*     base  = nullptr;

    if(Used() + total <= kPool)
    {
        base = Pool() + Used();
        Used() += total;
        Stats().internal_bytes += total;
    }
    else
    {
        base = static_cast<uint8_t*>(hal::AllocBig(total));
        if(!base)
        {
            Stats().failures++;
            return nullptr;
        }
        Stats().big_bytes += total;
    }

    Stats().blocks++;
    *reinterpret_cast<size_t*>(base) = size; // realloc precisa saber
    return base + kHeader;
}

inline size_t BlockSize(void* p)
{
    return *reinterpret_cast<size_t*>(static_cast<uint8_t*>(p) - kHeader);
}

inline void* Calloc(size_t count, size_t size)
{
    const size_t total = count * size;
    void*        p     = Alloc(total);
    if(p)
        memset(p, 0, total);
    return p;
}

inline void* Realloc(void* ptr, size_t size)
{
    if(!ptr)
        return Alloc(size);
    const size_t old = BlockSize(ptr);
    if(size <= old)
        return ptr; // cabe no bloco atual
    void* p = Alloc(size);
    if(p)
        memcpy(p, ptr, old);
    return p;
}

// Delta de consumo entre dois pontos — é assim que cada módulo
// reporta O QUE ELE gastou, e não o total compartilhado.
inline Module::MemReport Delta(const Module::MemReport& before)
{
    Module::MemReport d;
    d.internal_bytes = Stats().internal_bytes - before.internal_bytes;
    d.internal_cap   = kPool;
    d.big_bytes      = Stats().big_bytes - before.big_bytes;
    d.blocks         = Stats().blocks - before.blocks;
    d.failures       = Stats().failures - before.failures;
    return d;
}

} // namespace rnbo_alloc
} // namespace cotuca
