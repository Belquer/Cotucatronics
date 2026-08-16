#pragma once
// ============================================================
// Cotucatronics OS — midi v0.1.0
// Máquina de estados MIDI mínima. Board-agnostic: não inclui
// hal.h nem header de fabricante nenhum, exatamente como
// params.h e dynamics.h.
//
// O HAL entrega BYTES; este arquivo entende o que eles querem
// dizer. A divisão é de propósito: "como os bytes chegam" muda
// de placa pra placa (DMA, interrupção, polling), "o que eles
// significam" não muda nunca.
//
// Sem alocação dinâmica, sem exceções, sem std::.
// ============================================================

#include <cstdint>
#include <cstddef>

namespace cotuca
{

#define COTUCA_MIDI_VERSION "0.1.0"

// canal 0–15 (o do fio) ou omni
constexpr int kMidiOmni    = -1;
constexpr int kMaxMidiMap  = 16; // entradas na tabela CC→param
constexpr int kMidiHistory = 8;  // últimos CCs guardados pro `midi`

// 0–127 → 0.0–1.0. Divide por 127, não por 128: o valor máximo do
// MIDI é 127, e dividir por 128 nunca alcançaria 1.0 — o knob remoto
// não chegaria ao fim do curso.
inline float MidiNorm(uint8_t v) { return (v > 127 ? 127 : v) * (1.f / 127.f); }

// Uma linha da tabela do manifest: "CC tal move o parâmetro tal".
struct MidiCcBind
{
    uint8_t cc;       // número do controlador, 0–119
    int     param_id; // id na ParamTable
};

// Um CC observado, pro comando de debug.
struct MidiCcEvent
{
    uint8_t cc;
    uint8_t value;
    uint8_t channel; // 0–15, como vem no fio
};

// ------------------------------------------------------------
// MidiParser
//
// O QUE ELE FAZ: status byte, running status, Control Change.
// O QUE ELE IGNORA: note on/off, aftertouch, program change,
// pitch bend, sysex, system common, real-time.
//
// "Ignora" aqui quer dizer ENGOLE SEM ENGASGAR — não é o mesmo que
// "não trata". Uma nota mal ignorada desalinha o fluxo, e o próximo
// CC vira lixo: os dois bytes de dados da nota seriam lidos como se
// fossem de um controlador. Por isso o parser conta os bytes de
// dados de TODA mensagem de canal, mesmo das que descarta.
//
// RUNNING STATUS. O MIDI permite omitir o status byte quando ele
// repete: `B0 14 40 15 20` são dois CCs, não um CC e lixo. Controlador
// que varre um knob manda assim o tempo todo, então isto não é
// refinamento — é o caso comum.
//
// SYSEX. Corpo de sysex é tudo < 0x80, indistinguível de dados. Se
// não houver estado, cada byte de um dump de patch vira um dado
// solto, e um deles cedo ou tarde chega com running status ativo e
// move um parâmetro. Daí o `in_sysex_`.
//
// REAL-TIME (0xF8–0xFF). Clock, start, stop. Podem aparecer NO MEIO
// de outra mensagem — inclusive entre os dois bytes de dados de um
// CC. Por isso são tratados antes de tudo e retornam sem tocar em
// running status nem no contador de dados. Tratá-los como status
// comum apagaria o running status a cada pulso de clock, e um
// controlador que mande clock pararia de funcionar.
// ------------------------------------------------------------
class MidiParser
{
  public:
    using CcCallback = void (*)(uint8_t channel, uint8_t cc, uint8_t value);

    // channel: 0–15, ou kMidiOmni pra aceitar todos.
    void Init(int channel, CcCallback cb)
    {
        channel_ = (channel >= 0 && channel <= 15) ? channel : kMidiOmni;
        cb_      = cb;
        Reset();
    }

    void Reset()
    {
        status_ = 0;
        nd_     = 0;
        in_sysex_ = false;
        bytes_ = cc_ = other_ = orphan_ = realtime_ = filtered_ = 0;
        n_hist_ = hist_head_ = 0;
    }

    void Byte(uint8_t b)
    {
        bytes_++;

        // 1) REAL-TIME: transparente. Não toca em nada do estado.
        if(b >= 0xF8)
        {
            realtime_++;
            return;
        }

        // 2) STATUS BYTE
        if(b & 0x80)
        {
            if(b == 0xF7) // fim de sysex
            {
                in_sysex_ = false;
                return;
            }
            // Qualquer status não-real-time encerra um sysex em curso,
            // mesmo sem o 0xF7 — é o que a spec manda, e é o que
            // salva o parser de um cabo arrancado no meio do dump.
            in_sysex_ = false;

            if(b == 0xF0) // início de sysex
            {
                in_sysex_ = true;
                status_   = 0; // sysex cancela running status
                return;
            }
            if(b >= 0xF1) // system common: também cancela
            {
                status_ = 0;
                return;
            }
            // mensagem de canal: vira o running status
            status_ = b;
            nd_     = 0;
            return;
        }

        // 3) DATA BYTE
        if(in_sysex_)
            return; // corpo de sysex: engole calado
        if(status_ == 0)
        {
            orphan_++; // dado sem status: fluxo pego no meio
            return;
        }

        data_[nd_++] = b;
        if(nd_ < ExpectedData(status_))
            return;

        nd_ = 0; // running status FICA: a próxima mensagem pode omitir
        Dispatch();
    }

    // ---- estado, pro comando `midi` ----
    int     Channel() const { return channel_; }
    uint8_t RunningStatus() const { return status_; }
    bool    InSysex() const { return in_sysex_; }
    int     PendingData() const { return nd_; }

    uint32_t Bytes() const { return bytes_; }
    uint32_t CcCount() const { return cc_; }
    uint32_t Other() const { return other_; }
    uint32_t Orphan() const { return orphan_; }
    uint32_t RealTime() const { return realtime_; }
    uint32_t Filtered() const { return filtered_; }

    int HistoryCount() const { return n_hist_; }

    // i = 0 é o mais recente.
    const MidiCcEvent& History(int i) const
    {
        static const MidiCcEvent none = {0, 0, 0};
        if(i < 0 || i >= n_hist_)
            return none;
        const int idx = (hist_head_ - 1 - i + kMidiHistory * 2) % kMidiHistory;
        return hist_[idx];
    }

  private:
    // Program change (0xC) e channel pressure (0xD) têm UM byte de
    // dados. Todo o resto das mensagens de canal tem dois.
    static int ExpectedData(uint8_t status)
    {
        const uint8_t hi = status & 0xF0;
        return (hi == 0xC0 || hi == 0xD0) ? 1 : 2;
    }

    void Dispatch()
    {
        if((status_ & 0xF0) != 0xB0)
        {
            other_++; // nota, bend, aftertouch: contados e descartados
            return;
        }
        const uint8_t ch = status_ & 0x0F;
        if(channel_ != kMidiOmni && ch != (uint8_t)channel_)
        {
            filtered_++; // canal alheio: some, mas aparece no `midi`
            return;
        }
        cc_++;
        hist_[hist_head_] = {data_[0], data_[1], ch};
        hist_head_        = (hist_head_ + 1) % kMidiHistory;
        if(n_hist_ < kMidiHistory)
            n_hist_++;
        if(cb_)
            cb_(ch, data_[0], data_[1]);
    }

    int        channel_ = kMidiOmni;
    CcCallback cb_      = nullptr;

    uint8_t status_   = 0; // running status, 0 = nenhum
    uint8_t data_[2]  = {};
    int     nd_       = 0;
    bool    in_sysex_ = false;

    uint32_t bytes_ = 0, cc_ = 0, other_ = 0, orphan_ = 0;
    uint32_t realtime_ = 0, filtered_ = 0;

    MidiCcEvent hist_[kMidiHistory] = {};
    int         hist_head_          = 0;
    int         n_hist_             = 0;
};

} // namespace cotuca
