#pragma once
// module.h v0.4.0
// ============================================================
// Cotucatronics OS — interface abstrata de módulo DSP
//
// Segunda fronteira do projeto, irmã do hal.h:
//   hal.h    isola o FORNECEDOR DE PLACA
//   module.h isola o MOTOR DE DSP
//
// Nada aqui sabe se atrás existe um export do RNBO, DaisySP ou
// código escrito à mão. Regra espelhada: nenhum header do RNBO
// pode aparecer fora de module_rnbo.cpp.
//
// Board-agnostic: inclui só <cstddef>, nunca hal_<board>.
// ============================================================

#include <cstddef>

namespace cotuca
{

#define COTUCA_MODULE_VERSION "0.4.0"

// ------------------------------------------------------------
// SLOTS — posições FIXAS na cadeia do shell.
//
// O shell não carrega "uma lista de módulos": ele tem lugares
// definidos, cada um com uma função no signal path. Isso é
// deliberado. Uma cadeia livre de plugins pediria roteamento
// dinâmico, ordem configurável e a pergunta "o que vem antes do
// quê" aberta em runtime — e num instrumento realimentado a
// ordem É a segurança. O shifter tem que vir antes do governor,
// não depois; o limiter tem que ser último. Slot fixo torna
// isso estrutural em vez de convenção.
//
//   IN_R cru ─────────────────────────┐ (ch2: portadora externa)
//                                     ↓
//   fonte → GATE → [INSTRUMENT] → [FX2] → level → split
//                       ├ OUT_L: SHIFTER → GOVERNOR → LIMITER
//                       └ OUT_R:                      LIMITER
//
// Repare que o ramo do exciter NÃO tem slot de módulo. É
// deliberado: ali só mora política de segurança, toda em C++ do
// shell (dynamics.h). Trocar a escultura não pode trocar o
// anti-feedback — foi por isso que o shifter deixou de ser slot
// e virou bloco permanente.
// ------------------------------------------------------------
enum class ModuleSlot
{
    // O módulo da escultura: delay, reverb, o que a peça for.
    // Caminho principal, antes do split.
    Instrument = 0,

    // Segundo efeito de instrumento, depois do Instrument e antes
    // do level. Vago por ora — é o lugar previsto pro freeze.
    //
    // Este slot já foi "Shifter". O shifter virou bloco permanente
    // do shell (dynamics.h) quando ficou decidido que anti-feedback
    // é política de segurança e não DSP de instrumento: se morasse
    // num slot, trocar o patch da escultura poderia levar a proteção
    // junto. O slot ficou, genérico, pro que vier de verdade.
    Fx2 = 1,

    kCount
};

inline const char* SlotName(ModuleSlot s)
{
    switch(s)
    {
        case ModuleSlot::Instrument: return "instrument";
        case ModuleSlot::Fx2: return "fx2";
        default: return "?";
    }
}

class Module
{
  public:
    virtual ~Module() = default;

    // Uma vez no boot, antes do áudio começar. É AQUI que o módulo
    // pede memória — sempre por hal::AllocBig(), nunca declarando
    // seção de SDRAM (ver BOARDS.md).
    //
    // false = não deu: sem RAM grande, export incompatível, o que for.
    // O boot deve recusar e logar, não seguir com módulo meio vivo.
    virtual bool Init(float sample_rate, size_t block_size) = 0;

    // Pro log de boot: "delay", "shifter"...
    virtual const char* Name() const = 0;

    // ---- parâmetros POR NOME ----
    //
    // O manifest não pode depender de índice do RNBO: reexportar o
    // patch reordena os parâmetros e o mapeamento silenciosamente
    // apodrece. Então o manifest pede "delay_time" e o módulo resolve.
    // FindParam devolve -1 se não existe — quem chama DEVE checar, é
    // assim que um patch trocado vira erro claro no boot.
    virtual int         ParamCount() const                = 0;
    virtual const char* ParamName(int index) const        = 0;
    virtual int         FindParam(const char* name) const = 0;

    // valor normalizado 0..1, na mesma moeda da ParamTable.
    // A conversão pra unidade real é problema do módulo.
    virtual void SetParam(int index, float normalized) = 0;

    // Quantos canais de entrada o módulo consome. Default 1.
    //
    // >>> ch2, QUANDO EXISTE, É A PORTADORA EXTERNA. <<<
    //
    // ch1 é sempre o sinal do ponto da cadeia onde o slot está —
    // pós-crossfade de fontes e pós-gate, ou seja o instrumento.
    //
    // ch2 é IN_R CRU: direto do jack de linha, ANTES do crossfade e
    // ANTES do gate. É deliberado que ele não passe por nenhum dos
    // dois. O crossfade existe pra escolher UMA fonte entre piezo e
    // jack; um módulo de duas entradas quer as duas ao mesmo tempo,
    // então passar pelo crossfade seria pedir que ele escolhesse
    // justamente o que ele não quer escolher. E o gate é política
    // sobre o instrumento — silenciar o chão do piezo — que não tem
    // por que se aplicar a uma fonte externa que o operador plugou
    // de propósito.
    //
    // É isso que viabiliza cross-synthesis: a escultura modula, o
    // jack porta. Vale pro vocoder e pra qualquer módulo futuro que
    // precise de uma referência externa.
    //
    // Sem entrada de áudio na placa, ch2 é silêncio, não lixo.
    virtual int InputCount() const { return 1; }

    // Mesma convenção do hal::AudioCallback: não-intercalado,
    // in/out[canal][amostra]. Tempo real: sem alocar, sem logar,
    // sem bloquear.
    //
    // `in` traz SEMPRE dois ponteiros válidos; módulo de 1 input
    // simplesmente ignora in[1]. Passar sempre dois evita que um
    // export de 2 canais leia fora dos limites — sem crash, com
    // lixo no áudio, que é a pior classe de defeito.
    virtual void Process(const float* const* in,
                         float**             out,
                         size_t              size) = 0;

    // Extrato de memória do módulo, pro `mods` e pro log de boot.
    // Genérico de propósito: sem isto o main precisaria chamar uma
    // função concreta do motor (era RnboAllocStatsGet()) e voltaria
    // a incluir o header do RNBO — furando a fronteira pela porta
    // dos fundos. Motor que não sabe se medir devolve zeros.
    struct MemReport
    {
        size_t internal_bytes = 0; // servido de pool próprio (SRAM)
        size_t internal_cap   = 0; // tamanho desse pool
        size_t big_bytes      = 0; // servido por hal::AllocBig (SDRAM)
        int    blocks         = 0;
        int    failures       = 0;
    };
    virtual MemReport Memory() const { return MemReport{}; }
};

// ------------------------------------------------------------
// REGISTRO
//
// Cada implementação se registra sozinha num slot, por objeto
// estático. O main pergunta ao registro e NUNCA inclui header
// concreto de motor — era essa a dívida: `#include
// "module_rnbo.h"` no main.cpp. Com um módulo só dava pra viver
// com ela; com dois, cada motor novo somaria um include e uma
// chamada concreta, e a fronteira viraria decoração.
//
// Os slots vivem em .bss (zerados antes do __libc_init_array),
// então registrar de construtor estático é seguro: nenhum
// registrador roda antes do array existir zerado.
// ------------------------------------------------------------
namespace detail
{
inline Module** RegistrySlots()
{
    static Module* slots[static_cast<int>(ModuleSlot::kCount)] = {};
    return slots;
}
} // namespace detail

inline void RegisterModule(ModuleSlot slot, Module* m)
{
    const int i = static_cast<int>(slot);
    if(i >= 0 && i < static_cast<int>(ModuleSlot::kCount))
        detail::RegistrySlots()[i] = m;
}

// nullptr = slot vazio. O shell trata como passthrough, não como
// erro: um instrumento sem shifter é um instrumento válido.
inline Module* GetModule(ModuleSlot slot)
{
    const int i = static_cast<int>(slot);
    if(i < 0 || i >= static_cast<int>(ModuleSlot::kCount))
        return nullptr;
    return detail::RegistrySlots()[i];
}

// Auto-registro: basta declarar um destes ao lado da instância.
//
//     static RnboModule       g_delay;
//     static ModuleRegistrar  g_reg(ModuleSlot::Instrument, &g_delay);
struct ModuleRegistrar
{
    ModuleRegistrar(ModuleSlot slot, Module* m) { RegisterModule(slot, m); }
};

} // namespace cotuca
