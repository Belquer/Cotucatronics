#pragma once
// module.h v0.1.0
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
// Board-agnostic: inclui só params.h, nunca hal_<board>.
// ============================================================

#include <cstddef>

namespace cotuca
{

#define COTUCA_MODULE_VERSION "0.1.0"

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

    // Pro log de boot: "delay", "reverb"...
    virtual const char* Name() const = 0;

    // ---- parâmetros POR NOME ----
    //
    // O manifest não pode depender de índice do RNBO: reexportar o
    // patch reordena os parâmetros e o mapeamento silenciosamente
    // apodrece. Então o manifest pede "delay_time" e o módulo resolve.
    // FindParam devolve -1 se não existe — quem chama DEVE checar, é
    // assim que um patch trocado vira erro claro no boot.
    virtual int         ParamCount() const               = 0;
    virtual const char* ParamName(int index) const       = 0;
    virtual int         FindParam(const char* name) const = 0;

    // valor normalizado 0..1, na mesma moeda da ParamTable.
    // A conversão pra unidade real é problema do módulo.
    virtual void SetParam(int index, float normalized) = 0;

    // Mesma convenção do hal::AudioCallback: não-intercalado,
    // in/out[canal][amostra]. Tempo real: sem alocar, sem logar,
    // sem bloquear.
    virtual void Process(const float* const* in,
                         float**             out,
                         size_t              size) = 0;
};

// Toda implementação expõe uma fábrica com esta cara. O main pega o
// módulo sem saber de que motor ele saiu:
//
//     cotuca::Module* m = cotuca::CreateModule();
//
// Definida em module_<engine>.cpp e escolhida no Makefile, igual ao
// hal_<board>.cpp. Devolve nullptr se o motor não estiver disponível.
Module* CreateModule();

} // namespace cotuca
