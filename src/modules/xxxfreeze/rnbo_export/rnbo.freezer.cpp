/*******************************************************************************************************************
Copyright (c) 2023 Cycling '74

The code that Max generates automatically and that end users are capable of
exporting and using, and any associated documentation files (the “Software”)
is a work of authorship for which Cycling '74 is the author and owner for
copyright purposes.

This Software is dual-licensed either under the terms of the Cycling '74
License for Max-Generated Code for Export, or alternatively under the terms
of the General Public License (GPL) Version 3. You may use the Software
according to either of these licenses as it is most appropriate for your
project on a case-by-case basis (proprietary or not).

A) Cycling '74 License for Max-Generated Code for Export

A license is hereby granted, free of charge, to any person obtaining a copy
of the Software (“Licensee”) to use, copy, modify, merge, publish, and
distribute copies of the Software, and to permit persons to whom the Software
is furnished to do so, subject to the following conditions:

The Software is licensed to Licensee for all uses that do not include the sale,
sublicensing, or commercial distribution of software that incorporates this
source code. This means that the Licensee is free to use this software for
educational, research, and prototyping purposes, to create musical or other
creative works with software that incorporates this source code, or any other
use that does not constitute selling software that makes use of this source
code. Commercial distribution also includes the packaging of free software with
other paid software, hardware, or software-provided commercial services.

For entities with UNDER $200k in annual revenue or funding, a license is hereby
granted, free of charge, for the sale, sublicensing, or commercial distribution
of software that incorporates this source code, for as long as the entity's
annual revenue remains below $200k annual revenue or funding.

For entities with OVER $200k in annual revenue or funding interested in the
sale, sublicensing, or commercial distribution of software that incorporates
this source code, please send inquiries to licensing@cycling74.com.

The above copyright notice and this license shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Please see
https://support.cycling74.com/hc/en-us/articles/10730637742483-RNBO-Export-Licensing-FAQ
for additional information

B) General Public License Version 3 (GPLv3)
Details of the GPLv3 license can be found at: https://www.gnu.org/licenses/gpl-3.0.html
*******************************************************************************************************************/

#ifdef RNBO_LIB_PREFIX
#define STR_IMPL(A) #A
#define STR(A) STR_IMPL(A)
#define RNBO_LIB_INCLUDE(X) STR(RNBO_LIB_PREFIX/X)
#else
#define RNBO_LIB_INCLUDE(X) #X
#endif // RNBO_LIB_PREFIX
#ifdef RNBO_INJECTPLATFORM
#define RNBO_USECUSTOMPLATFORM
#include RNBO_INJECTPLATFORM
#endif // RNBO_INJECTPLATFORM

#include RNBO_LIB_INCLUDE(RNBO_Common.h)
#include RNBO_LIB_INCLUDE(RNBO_AudioSignal.h)

namespace RNBO {


#define trunc(x) ((Int)(x))
#define autoref auto&

#if defined(__GNUC__) || defined(__clang__)
    #define RNBO_RESTRICT __restrict__
#elif defined(_MSC_VER)
    #define RNBO_RESTRICT __restrict
#endif

#define FIXEDSIZEARRAYINIT(...) { }

template <class ENGINE = INTERNALENGINE> class rnbomatic : public PatcherInterfaceImpl {

friend class EngineCore;
friend class Engine;
friend class MinimalEngine<>;
public:

rnbomatic()
: _internalEngine(this)
{
}

~rnbomatic()
{
    deallocateSignals();
}

Index getNumMidiInputPorts() const {
    return 1;
}

void processMidiEvent(MillisecondTime time, int port, ConstByteArray data, Index length) {
    this->updateTime(time, (ENGINE*)nullptr);
    this->ctlin_01_midihandler(data[0] & 240, (data[0] & 15) + 1, port, data, length);
    this->ctlin_02_midihandler(data[0] & 240, (data[0] & 15) + 1, port, data, length);
    this->ctlin_03_midihandler(data[0] & 240, (data[0] & 15) + 1, port, data, length);
    this->ctlin_04_midihandler(data[0] & 240, (data[0] & 15) + 1, port, data, length);
    this->ctlin_05_midihandler(data[0] & 240, (data[0] & 15) + 1, port, data, length);
    this->ctlin_06_midihandler(data[0] & 240, (data[0] & 15) + 1, port, data, length);
    this->ctlin_07_midihandler(data[0] & 240, (data[0] & 15) + 1, port, data, length);
    this->ctlin_08_midihandler(data[0] & 240, (data[0] & 15) + 1, port, data, length);
    this->ctlin_09_midihandler(data[0] & 240, (data[0] & 15) + 1, port, data, length);
}

Index getNumMidiOutputPorts() const {
    return 0;
}

void process(
    const SampleValue * const* inputs,
    Index numInputs,
    SampleValue * const* outputs,
    Index numOutputs,
    Index n
) {
    this->vs = n;
    this->updateTime(this->getEngine()->getCurrentTime(), (ENGINE*)nullptr, true);
    SampleValue * out1 = (numOutputs >= 1 && outputs[0] ? outputs[0] : this->dummyBuffer);
    SampleValue * out2 = (numOutputs >= 2 && outputs[1] ? outputs[1] : this->dummyBuffer);
    const SampleValue * in1 = (numInputs >= 1 && inputs[0] ? inputs[0] : this->zeroBuffer);
    const SampleValue * in2 = (numInputs >= 2 && inputs[1] ? inputs[1] : this->zeroBuffer);
    this->p_01_perform(in1, in2, n);

    this->recordtilde_01_perform(
        this->recordtilde_01_record,
        this->recordtilde_01_begin,
        this->recordtilde_01_end,
        in1,
        in2,
        this->dummyBuffer,
        n
    );

    this->recordtilde_02_perform(
        this->recordtilde_02_record,
        this->recordtilde_02_begin,
        this->recordtilde_02_end,
        in1,
        in2,
        this->dummyBuffer,
        n
    );

    this->p_06_perform(in1, in2, this->signals[0], this->signals[1], n);
    this->phasor_01_perform(this->phasor_01_freq, this->signals[2], n);
    this->p_02_perform(this->signals[2], this->signals[3], this->signals[4], n);
    this->p_05_perform(this->signals[2], this->signals[5], this->signals[6], n);
    this->p_04_perform(n);
    this->linetilde_05_perform(this->signals[2], n);
    this->hann_tilde_03_perform(this->signals[2], this->signals[7], n);
    this->dspexpr_40_perform(this->signals[3], this->signals[7], this->signals[2], n);
    this->metertilde_01_perform(this->signals[2], n);
    this->dspexpr_42_perform(this->signals[4], this->signals[7], this->signals[3], n);
    this->metertilde_02_perform(this->signals[3], n);
    this->linetilde_06_perform(this->signals[7], n);
    this->hann_tilde_04_perform(this->signals[7], this->signals[4], n);
    this->dspexpr_45_perform(this->signals[6], this->signals[4], this->signals[7], n);
    this->metertilde_04_perform(this->signals[7], n);
    this->dspexpr_43_perform(this->signals[3], this->signals[7], this->signals[6], n);
    this->dspexpr_44_perform(this->signals[5], this->signals[4], this->signals[7], n);
    this->metertilde_03_perform(this->signals[7], n);
    this->dspexpr_41_perform(this->signals[2], this->signals[7], this->signals[4], n);
    this->p_03_perform(this->signals[4], this->signals[6], this->signals[7], this->signals[2], n);
    this->signaladder_01_perform(this->signals[7], this->signals[0], out1, n);
    this->signaladder_02_perform(this->signals[2], this->signals[1], out2, n);
    this->stackprotect_perform(n);
    this->globaltransport_advance();
    this->advanceTime((ENGINE*)nullptr);
    this->audioProcessSampleCount += this->vs;
}

void prepareToProcess(number sampleRate, Index maxBlockSize, bool force) {
    RNBO_ASSERT(this->_isInitialized);

    if (this->maxvs < maxBlockSize || !this->didAllocateSignals) {
        Index i;

        for (i = 0; i < 8; i++) {
            this->signals[i] = resizeSignal(this->signals[i], this->maxvs, maxBlockSize);
        }

        this->phasor_01_sigbuf = resizeSignal(this->phasor_01_sigbuf, this->maxvs, maxBlockSize);
        this->globaltransport_tempo = resizeSignal(this->globaltransport_tempo, this->maxvs, maxBlockSize);
        this->globaltransport_state = resizeSignal(this->globaltransport_state, this->maxvs, maxBlockSize);
        this->zeroBuffer = resizeSignal(this->zeroBuffer, this->maxvs, maxBlockSize);
        this->dummyBuffer = resizeSignal(this->dummyBuffer, this->maxvs, maxBlockSize);
        this->didAllocateSignals = true;
    }

    const bool sampleRateChanged = sampleRate != this->sr;
    const bool maxvsChanged = maxBlockSize != this->maxvs;
    const bool forceDSPSetup = sampleRateChanged || maxvsChanged || force;

    if (sampleRateChanged || maxvsChanged) {
        this->vs = maxBlockSize;
        this->maxvs = maxBlockSize;
        this->sr = sampleRate;
        this->invsr = 1 / sampleRate;
    }

    this->phasor_01_dspsetup(forceDSPSetup);
    this->metertilde_01_dspsetup(forceDSPSetup);
    this->metertilde_02_dspsetup(forceDSPSetup);
    this->metertilde_04_dspsetup(forceDSPSetup);
    this->metertilde_03_dspsetup(forceDSPSetup);
    this->data_01_dspsetup(forceDSPSetup);
    this->data_02_dspsetup(forceDSPSetup);
    this->globaltransport_dspsetup(forceDSPSetup);
    this->p_01->prepareToProcess(sampleRate, maxBlockSize, force);

    for (Index i = 0; i < 8; i++) {
        this->p_02[i]->prepareToProcess(sampleRate, maxBlockSize, force);
    }

    this->p_03->prepareToProcess(sampleRate, maxBlockSize, force);
    this->p_04->prepareToProcess(sampleRate, maxBlockSize, force);

    for (Index i = 0; i < 8; i++) {
        this->p_05[i]->prepareToProcess(sampleRate, maxBlockSize, force);
    }

    this->p_06->prepareToProcess(sampleRate, maxBlockSize, force);

    if (sampleRateChanged)
        this->onSampleRateChanged(sampleRate);
}

number msToSamps(MillisecondTime ms, number sampleRate) {
    return ms * sampleRate * 0.001;
}

MillisecondTime sampsToMs(SampleIndex samps) {
    return samps * (this->invsr * 1000);
}

Index getNumInputChannels() const {
    return 2;
}

Index getNumOutputChannels() const {
    return 2;
}

DataRef* getDataRef(DataRefIndex index)  {
    switch (index) {
    case 0:
        {
        return addressOf(this->FreezeA);
        break;
        }
    case 1:
        {
        return addressOf(this->RNBODefaultSinus);
        break;
        }
    case 2:
        {
        return addressOf(this->FreezeB);
        break;
        }
    default:
        {
        return nullptr;
        }
    }
}

DataRefIndex getNumDataRefs() const {
    return 3;
}

void processDataViewUpdate(DataRefIndex index, MillisecondTime time) {
    this->updateTime(time, (ENGINE*)nullptr);

    if (index == 0) {
        this->recordtilde_01_buffer = reInitDataView(this->recordtilde_01_buffer, this->FreezeA);
        this->data_02_buffer = reInitDataView(this->data_02_buffer, this->FreezeA);
        this->data_02_afterBufferUpdate();
    }

    if (index == 2) {
        this->data_01_buffer = reInitDataView(this->data_01_buffer, this->FreezeB);
        this->recordtilde_02_buffer = reInitDataView(this->recordtilde_02_buffer, this->FreezeB);
        this->data_01_afterBufferUpdate();
    }

    this->p_01->processDataViewUpdate(index, time);

    for (Index i = 0; i < 8; i++) {
        this->p_02[i]->processDataViewUpdate(index, time);
    }

    this->p_03->processDataViewUpdate(index, time);
    this->p_04->processDataViewUpdate(index, time);

    for (Index i = 0; i < 8; i++) {
        this->p_05[i]->processDataViewUpdate(index, time);
    }

    this->p_06->processDataViewUpdate(index, time);
}

void initialize() {
    RNBO_ASSERT(!this->_isInitialized);

    this->FreezeA = initDataRef(
        this->FreezeA,
        this->dataRefStrings->name0,
        false,
        this->dataRefStrings->file0,
        this->dataRefStrings->tag0
    );

    this->RNBODefaultSinus = initDataRef(
        this->RNBODefaultSinus,
        this->dataRefStrings->name1,
        true,
        this->dataRefStrings->file1,
        this->dataRefStrings->tag1
    );

    this->FreezeB = initDataRef(
        this->FreezeB,
        this->dataRefStrings->name2,
        false,
        this->dataRefStrings->file2,
        this->dataRefStrings->tag2
    );

    this->assign_defaults();
    this->applyState();
    this->FreezeA->setIndex(0);
    this->recordtilde_01_buffer = new Float64Buffer(this->FreezeA);
    this->data_02_buffer = new Float64Buffer(this->FreezeA);
    this->RNBODefaultSinus->setIndex(1);
    this->FreezeB->setIndex(2);
    this->data_01_buffer = new Float64Buffer(this->FreezeB);
    this->recordtilde_02_buffer = new Float64Buffer(this->FreezeB);
    this->initializeObjects();
    this->allocateDataRefs();
    this->startup();
    this->_isInitialized = true;
}

void processTempoEvent(MillisecondTime time, Tempo tempo) {
    this->updateTime(time, (ENGINE*)nullptr);

    if (this->globaltransport_setTempo(this->_currentTime, tempo, false)) {
        this->p_01->processTempoEvent(time, tempo);

        for (Index i = 0; i < 8; i++) {
            this->p_02[i]->processTempoEvent(time, tempo);
        }

        this->p_03->processTempoEvent(time, tempo);
        this->p_04->processTempoEvent(time, tempo);

        for (Index i = 0; i < 8; i++) {
            this->p_05[i]->processTempoEvent(time, tempo);
        }

        this->p_06->processTempoEvent(time, tempo);
    }
}

void processTransportEvent(MillisecondTime time, TransportState state) {
    this->updateTime(time, (ENGINE*)nullptr);

    if (this->globaltransport_setState(this->_currentTime, state, false)) {
        this->p_01->processTransportEvent(time, state);

        for (Index i = 0; i < 8; i++) {
            this->p_02[i]->processTransportEvent(time, state);
        }

        this->p_03->processTransportEvent(time, state);
        this->p_04->processTransportEvent(time, state);

        for (Index i = 0; i < 8; i++) {
            this->p_05[i]->processTransportEvent(time, state);
        }

        this->p_06->processTransportEvent(time, state);
    }
}

void processBeatTimeEvent(MillisecondTime time, BeatTime beattime) {
    this->updateTime(time, (ENGINE*)nullptr);

    if (this->globaltransport_setBeatTime(this->_currentTime, beattime, false)) {
        this->p_01->processBeatTimeEvent(time, beattime);

        for (Index i = 0; i < 8; i++) {
            this->p_02[i]->processBeatTimeEvent(time, beattime);
        }

        this->p_03->processBeatTimeEvent(time, beattime);
        this->p_04->processBeatTimeEvent(time, beattime);

        for (Index i = 0; i < 8; i++) {
            this->p_05[i]->processBeatTimeEvent(time, beattime);
        }

        this->p_06->processBeatTimeEvent(time, beattime);
        this->phasor_01_onBeatTimeChanged(beattime);
    }
}

void processTimeSignatureEvent(MillisecondTime time, Int numerator, Int denominator) {
    this->updateTime(time, (ENGINE*)nullptr);

    if (this->globaltransport_setTimeSignature(this->_currentTime, numerator, denominator, false)) {
        this->p_01->processTimeSignatureEvent(time, numerator, denominator);

        for (Index i = 0; i < 8; i++) {
            this->p_02[i]->processTimeSignatureEvent(time, numerator, denominator);
        }

        this->p_03->processTimeSignatureEvent(time, numerator, denominator);
        this->p_04->processTimeSignatureEvent(time, numerator, denominator);

        for (Index i = 0; i < 8; i++) {
            this->p_05[i]->processTimeSignatureEvent(time, numerator, denominator);
        }

        this->p_06->processTimeSignatureEvent(time, numerator, denominator);
    }
}

void processBBUEvent(MillisecondTime time, number bars, number beats, number units) {
    this->updateTime(time, (ENGINE*)nullptr);

    if (this->globaltransport_setBBU(this->_currentTime, bars, beats, units, false)) {
        this->p_01->processBBUEvent(time, bars, beats, units);

        for (Index i = 0; i < 8; i++) {
            this->p_02[i]->processBBUEvent(time, bars, beats, units);
        }

        this->p_03->processBBUEvent(time, bars, beats, units);
        this->p_04->processBBUEvent(time, bars, beats, units);

        for (Index i = 0; i < 8; i++) {
            this->p_05[i]->processBBUEvent(time, bars, beats, units);
        }

        this->p_06->processBBUEvent(time, bars, beats, units);
    }
}

void getPreset(PatcherStateInterface& preset) {
    this->updateTime(this->getEngine()->getCurrentTime(), (ENGINE*)nullptr);
    preset["__presetid"] = "rnbo";
    this->param_01_getPresetValue(getSubState(preset, "auto"));
    this->param_02_getPresetValue(getSubState(preset, "autodel"));
    this->param_03_getPresetValue(getSubState(preset, "freeze"));
    this->param_04_getPresetValue(getSubState(preset, "lowcut"));
    this->param_05_getPresetValue(getSubState(preset, "highcut"));
    this->param_06_getPresetValue(getSubState(preset, "volume"));
    this->param_07_getPresetValue(getSubState(preset, "fade"));
    this->param_08_getPresetValue(getSubState(preset, "pitchjit"));
    this->param_09_getPresetValue(getSubState(preset, "direct"));
    this->p_01->getPreset(getSubState(getSubState(preset, "__sps"), "Attack~"));

    for (Index i = 0; i < 8; i++)
        this->p_02[i]->getPreset(getSubStateAt(getSubState(preset, "__sps"), "Freezer-Voice~", i));

    this->p_03->getPreset(getSubState(getSubState(preset, "__sps"), "EQ&Volume~"));
    this->p_04->getPreset(getSubState(getSubState(preset, "__sps"), "Freeze"));

    for (Index i = 0; i < 8; i++)
        this->p_05[i]->getPreset(getSubStateAt(getSubState(preset, "__sps"), "Freezer-Voice~[1]", i));

    this->p_06->getPreset(getSubState(getSubState(preset, "__sps"), "Direct~"));
}

void setPreset(MillisecondTime time, PatcherStateInterface& preset) {
    this->updateTime(time, (ENGINE*)nullptr);
    this->p_01->setPreset(time, getSubState(getSubState(preset, "__sps"), "Attack~"));

    for (Index i = 0; i < 8; i++)
        this->p_02[i]->setPreset(time, getSubStateAt(getSubState(preset, "__sps"), "Freezer-Voice~", i));

    this->p_03->setPreset(time, getSubState(getSubState(preset, "__sps"), "EQ&Volume~"));
    this->p_04->setPreset(time, getSubState(getSubState(preset, "__sps"), "Freeze"));

    for (Index i = 0; i < 8; i++)
        this->p_05[i]->setPreset(time, getSubStateAt(getSubState(preset, "__sps"), "Freezer-Voice~[1]", i));

    this->p_06->setPreset(time, getSubState(getSubState(preset, "__sps"), "Direct~"));
    this->param_03_setPresetValue(getSubState(preset, "freeze"));
    this->param_01_setPresetValue(getSubState(preset, "auto"));
    this->param_02_setPresetValue(getSubState(preset, "autodel"));
    this->param_07_setPresetValue(getSubState(preset, "fade"));
    this->param_08_setPresetValue(getSubState(preset, "pitchjit"));
    this->param_04_setPresetValue(getSubState(preset, "lowcut"));
    this->param_05_setPresetValue(getSubState(preset, "highcut"));
    this->param_09_setPresetValue(getSubState(preset, "direct"));
    this->param_06_setPresetValue(getSubState(preset, "volume"));
}

void setParameterValue(ParameterIndex index, ParameterValue v, MillisecondTime time) {
    this->updateTime(time, (ENGINE*)nullptr);

    switch (index) {
    case 0:
        {
        this->param_01_value_set(v);
        break;
        }
    case 1:
        {
        this->param_02_value_set(v);
        break;
        }
    case 2:
        {
        this->param_03_value_set(v);
        break;
        }
    case 3:
        {
        this->param_04_value_set(v);
        break;
        }
    case 4:
        {
        this->param_05_value_set(v);
        break;
        }
    case 5:
        {
        this->param_06_value_set(v);
        break;
        }
    case 6:
        {
        this->param_07_value_set(v);
        break;
        }
    case 7:
        {
        this->param_08_value_set(v);
        break;
        }
    case 8:
        {
        this->param_09_value_set(v);
        break;
        }
    default:
        {
        index -= 9;

        if (index < this->p_01->getNumParameters())
            this->p_01->setParameterValue(index, v, time);

        index -= this->p_01->getNumParameters();

        if (index < this->p_02[0]->getNumParameters())
            this->p_02[0]->setPolyParameterValue(this->p_02, index, v, time);

        index -= this->p_02[0]->getNumParameters();

        if (index < this->p_03->getNumParameters())
            this->p_03->setParameterValue(index, v, time);

        index -= this->p_03->getNumParameters();

        if (index < this->p_04->getNumParameters())
            this->p_04->setParameterValue(index, v, time);

        index -= this->p_04->getNumParameters();

        if (index < this->p_05[0]->getNumParameters())
            this->p_05[0]->setPolyParameterValue(this->p_05, index, v, time);

        index -= this->p_05[0]->getNumParameters();

        if (index < this->p_06->getNumParameters())
            this->p_06->setParameterValue(index, v, time);

        break;
        }
    }
}

void processParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
    this->setParameterValue(index, value, time);
}

void processParameterBangEvent(ParameterIndex index, MillisecondTime time) {
    this->setParameterValue(index, this->getParameterValue(index), time);
}

void processNormalizedParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
    this->setParameterValueNormalized(index, value, time);
}

ParameterValue getParameterValue(ParameterIndex index)  {
    switch (index) {
    case 0:
        {
        return this->param_01_value;
        }
    case 1:
        {
        return this->param_02_value;
        }
    case 2:
        {
        return this->param_03_value;
        }
    case 3:
        {
        return this->param_04_value;
        }
    case 4:
        {
        return this->param_05_value;
        }
    case 5:
        {
        return this->param_06_value;
        }
    case 6:
        {
        return this->param_07_value;
        }
    case 7:
        {
        return this->param_08_value;
        }
    case 8:
        {
        return this->param_09_value;
        }
    default:
        {
        index -= 9;

        if (index < this->p_01->getNumParameters())
            return this->p_01->getParameterValue(index);

        index -= this->p_01->getNumParameters();

        if (index < this->p_02[0]->getNumParameters())
            return this->p_02[0]->getPolyParameterValue(this->p_02, index);

        index -= this->p_02[0]->getNumParameters();

        if (index < this->p_03->getNumParameters())
            return this->p_03->getParameterValue(index);

        index -= this->p_03->getNumParameters();

        if (index < this->p_04->getNumParameters())
            return this->p_04->getParameterValue(index);

        index -= this->p_04->getNumParameters();

        if (index < this->p_05[0]->getNumParameters())
            return this->p_05[0]->getPolyParameterValue(this->p_05, index);

        index -= this->p_05[0]->getNumParameters();

        if (index < this->p_06->getNumParameters())
            return this->p_06->getParameterValue(index);

        return 0;
        }
    }
}

ParameterIndex getNumSignalInParameters() const {
    return 0;
}

ParameterIndex getNumSignalOutParameters() const {
    return 0;
}

ParameterIndex getNumParameters() const {
    return 9 + this->p_01->getNumParameters() + this->p_02[0]->getNumParameters() + this->p_03->getNumParameters() + this->p_04->getNumParameters() + this->p_05[0]->getNumParameters() + this->p_06->getNumParameters();
}

ConstCharPointer getParameterName(ParameterIndex index) const {
    switch (index) {
    case 0:
        {
        return "auto";
        }
    case 1:
        {
        return "autodel";
        }
    case 2:
        {
        return "freeze";
        }
    case 3:
        {
        return "lowcut";
        }
    case 4:
        {
        return "highcut";
        }
    case 5:
        {
        return "volume";
        }
    case 6:
        {
        return "fade";
        }
    case 7:
        {
        return "pitchjit";
        }
    case 8:
        {
        return "direct";
        }
    default:
        {
        index -= 9;

        if (index < this->p_01->getNumParameters())
            return this->p_01->getParameterName(index);

        index -= this->p_01->getNumParameters();

        if (index < this->p_02[0]->getNumParameters()) {
            {
                return this->p_02[0]->getParameterName(index);
            }
        }

        index -= this->p_02[0]->getNumParameters();

        if (index < this->p_03->getNumParameters())
            return this->p_03->getParameterName(index);

        index -= this->p_03->getNumParameters();

        if (index < this->p_04->getNumParameters())
            return this->p_04->getParameterName(index);

        index -= this->p_04->getNumParameters();

        if (index < this->p_05[0]->getNumParameters()) {
            {
                return this->p_05[0]->getParameterName(index);
            }
        }

        index -= this->p_05[0]->getNumParameters();

        if (index < this->p_06->getNumParameters())
            return this->p_06->getParameterName(index);

        return "bogus";
        }
    }
}

ConstCharPointer getParameterId(ParameterIndex index) const {
    switch (index) {
    case 0:
        {
        return "auto";
        }
    case 1:
        {
        return "autodel";
        }
    case 2:
        {
        return "freeze";
        }
    case 3:
        {
        return "lowcut";
        }
    case 4:
        {
        return "highcut";
        }
    case 5:
        {
        return "volume";
        }
    case 6:
        {
        return "fade";
        }
    case 7:
        {
        return "pitchjit";
        }
    case 8:
        {
        return "direct";
        }
    default:
        {
        index -= 9;

        if (index < this->p_01->getNumParameters())
            return this->p_01->getParameterId(index);

        index -= this->p_01->getNumParameters();

        if (index < this->p_02[0]->getNumParameters()) {
            {
                return this->p_02[0]->getParameterId(index);
            }
        }

        index -= this->p_02[0]->getNumParameters();

        if (index < this->p_03->getNumParameters())
            return this->p_03->getParameterId(index);

        index -= this->p_03->getNumParameters();

        if (index < this->p_04->getNumParameters())
            return this->p_04->getParameterId(index);

        index -= this->p_04->getNumParameters();

        if (index < this->p_05[0]->getNumParameters()) {
            {
                return this->p_05[0]->getParameterId(index);
            }
        }

        index -= this->p_05[0]->getNumParameters();

        if (index < this->p_06->getNumParameters())
            return this->p_06->getParameterId(index);

        return "bogus";
        }
    }
}

void getParameterInfo(ParameterIndex index, ParameterInfo * info) const {
    {
        switch (index) {
        case 0:
            {
            info->type = ParameterTypeNumber;
            info->initialValue = 0;
            info->min = 0;
            info->max = 100;
            info->exponent = 1;
            info->steps = 0;
            info->debug = false;
            info->saveable = true;
            info->transmittable = true;
            info->initialized = true;
            info->visible = true;
            info->displayName = "Auto";
            info->unit = "%";
            info->ioType = IOTypeUndefined;
            info->signalIndex = INVALID_INDEX;
            break;
            }
        case 1:
            {
            info->type = ParameterTypeNumber;
            info->initialValue = 0;
            info->min = 0;
            info->max = 100;
            info->exponent = 1;
            info->steps = 0;
            info->debug = false;
            info->saveable = true;
            info->transmittable = true;
            info->initialized = true;
            info->visible = true;
            info->displayName = "AutoDel";
            info->unit = "%";
            info->ioType = IOTypeUndefined;
            info->signalIndex = INVALID_INDEX;
            break;
            }
        case 2:
            {
            info->type = ParameterTypeNumber;
            info->initialValue = 0;
            info->min = 0;
            info->max = 1;
            info->exponent = 1;
            info->steps = 2;
            static const char * eVal2[] = {"0", "1"};
            info->enumValues = eVal2;
            info->debug = false;
            info->saveable = true;
            info->transmittable = true;
            info->initialized = true;
            info->visible = true;
            info->displayName = "Frezze";
            info->unit = "On/Off";
            info->ioType = IOTypeUndefined;
            info->signalIndex = INVALID_INDEX;
            break;
            }
        case 3:
            {
            info->type = ParameterTypeNumber;
            info->initialValue = 0;
            info->min = 0;
            info->max = 100;
            info->exponent = 1;
            info->steps = 0;
            info->debug = false;
            info->saveable = true;
            info->transmittable = true;
            info->initialized = true;
            info->visible = true;
            info->displayName = "LowCut";
            info->unit = "%";
            info->ioType = IOTypeUndefined;
            info->signalIndex = INVALID_INDEX;
            break;
            }
        case 4:
            {
            info->type = ParameterTypeNumber;
            info->initialValue = 0;
            info->min = 0;
            info->max = 100;
            info->exponent = 1;
            info->steps = 0;
            info->debug = false;
            info->saveable = true;
            info->transmittable = true;
            info->initialized = true;
            info->visible = true;
            info->displayName = "HighCut";
            info->unit = "%";
            info->ioType = IOTypeUndefined;
            info->signalIndex = INVALID_INDEX;
            break;
            }
        case 5:
            {
            info->type = ParameterTypeNumber;
            info->initialValue = 0;
            info->min = -100;
            info->max = 100;
            info->exponent = 1;
            info->steps = 0;
            info->debug = false;
            info->saveable = true;
            info->transmittable = true;
            info->initialized = true;
            info->visible = true;
            info->displayName = "Volume";
            info->unit = "%";
            info->ioType = IOTypeUndefined;
            info->signalIndex = INVALID_INDEX;
            break;
            }
        case 6:
            {
            info->type = ParameterTypeNumber;
            info->initialValue = 0;
            info->min = 0;
            info->max = 100;
            info->exponent = 1;
            info->steps = 0;
            info->debug = false;
            info->saveable = true;
            info->transmittable = true;
            info->initialized = true;
            info->visible = true;
            info->displayName = "Fade";
            info->unit = "%";
            info->ioType = IOTypeUndefined;
            info->signalIndex = INVALID_INDEX;
            break;
            }
        case 7:
            {
            info->type = ParameterTypeNumber;
            info->initialValue = 0;
            info->min = 0;
            info->max = 100;
            info->exponent = 1;
            info->steps = 0;
            info->debug = false;
            info->saveable = true;
            info->transmittable = true;
            info->initialized = true;
            info->visible = true;
            info->displayName = "PitchJitter";
            info->unit = "%";
            info->ioType = IOTypeUndefined;
            info->signalIndex = INVALID_INDEX;
            break;
            }
        case 8:
            {
            info->type = ParameterTypeNumber;
            info->initialValue = 0;
            info->min = 0;
            info->max = 1;
            info->exponent = 1;
            info->steps = 2;
            static const char * eVal8[] = {"0", "1"};
            info->enumValues = eVal8;
            info->debug = false;
            info->saveable = true;
            info->transmittable = true;
            info->initialized = true;
            info->visible = true;
            info->displayName = "Direct";
            info->unit = "On/Off";
            info->ioType = IOTypeUndefined;
            info->signalIndex = INVALID_INDEX;
            break;
            }
        default:
            {
            index -= 9;

            if (index < this->p_01->getNumParameters())
                this->p_01->getParameterInfo(index, info);

            index -= this->p_01->getNumParameters();

            if (index < this->p_02[0]->getNumParameters()) {
                for (Index i = 0; i < 8; i++) {
                    this->p_02[i]->getParameterInfo(index, info);
                }
            }

            index -= this->p_02[0]->getNumParameters();

            if (index < this->p_03->getNumParameters())
                this->p_03->getParameterInfo(index, info);

            index -= this->p_03->getNumParameters();

            if (index < this->p_04->getNumParameters())
                this->p_04->getParameterInfo(index, info);

            index -= this->p_04->getNumParameters();

            if (index < this->p_05[0]->getNumParameters()) {
                for (Index i = 0; i < 8; i++) {
                    this->p_05[i]->getParameterInfo(index, info);
                }
            }

            index -= this->p_05[0]->getNumParameters();

            if (index < this->p_06->getNumParameters())
                this->p_06->getParameterInfo(index, info);

            break;
            }
        }
    }
}

ParameterValue applyStepsToNormalizedParameterValue(ParameterValue normalizedValue, int steps) const {
    if (steps == 1) {
        if (normalizedValue > 0) {
            normalizedValue = 1.;
        }
    } else {
        ParameterValue oneStep = (number)1. / (steps - 1);
        ParameterValue numberOfSteps = rnbo_fround(normalizedValue / oneStep * 1 / (number)1) * (number)1;
        normalizedValue = numberOfSteps * oneStep;
    }

    return normalizedValue;
}

ParameterValue convertToNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
    switch (index) {
    case 2:
    case 8:
        {
        {
            value = (value < 0 ? 0 : (value > 1 ? 1 : value));
            ParameterValue normalizedValue = (value - 0) / (1 - 0);

            {
                normalizedValue = this->applyStepsToNormalizedParameterValue(normalizedValue, 2);
            }

            return normalizedValue;
        }
        }
    case 0:
    case 1:
    case 3:
    case 4:
    case 6:
    case 7:
        {
        {
            value = (value < 0 ? 0 : (value > 100 ? 100 : value));
            ParameterValue normalizedValue = (value - 0) / (100 - 0);
            return normalizedValue;
        }
        }
    case 5:
        {
        {
            value = (value < -100 ? -100 : (value > 100 ? 100 : value));
            ParameterValue normalizedValue = (value - -100) / (100 - -100);
            return normalizedValue;
        }
        }
    default:
        {
        index -= 9;

        if (index < this->p_01->getNumParameters())
            return this->p_01->convertToNormalizedParameterValue(index, value);

        index -= this->p_01->getNumParameters();

        if (index < this->p_02[0]->getNumParameters()) {
            {
                return this->p_02[0]->convertToNormalizedParameterValue(index, value);
            }
        }

        index -= this->p_02[0]->getNumParameters();

        if (index < this->p_03->getNumParameters())
            return this->p_03->convertToNormalizedParameterValue(index, value);

        index -= this->p_03->getNumParameters();

        if (index < this->p_04->getNumParameters())
            return this->p_04->convertToNormalizedParameterValue(index, value);

        index -= this->p_04->getNumParameters();

        if (index < this->p_05[0]->getNumParameters()) {
            {
                return this->p_05[0]->convertToNormalizedParameterValue(index, value);
            }
        }

        index -= this->p_05[0]->getNumParameters();

        if (index < this->p_06->getNumParameters())
            return this->p_06->convertToNormalizedParameterValue(index, value);

        return value;
        }
    }
}

ParameterValue convertFromNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
    value = (value < 0 ? 0 : (value > 1 ? 1 : value));

    switch (index) {
    case 2:
    case 8:
        {
        {
            {
                value = this->applyStepsToNormalizedParameterValue(value, 2);
            }

            {
                return 0 + value * (1 - 0);
            }
        }
        }
    case 0:
    case 1:
    case 3:
    case 4:
    case 6:
    case 7:
        {
        {
            {
                return 0 + value * (100 - 0);
            }
        }
        }
    case 5:
        {
        {
            {
                return -100 + value * (100 - -100);
            }
        }
        }
    default:
        {
        index -= 9;

        if (index < this->p_01->getNumParameters())
            return this->p_01->convertFromNormalizedParameterValue(index, value);

        index -= this->p_01->getNumParameters();

        if (index < this->p_02[0]->getNumParameters()) {
            {
                return this->p_02[0]->convertFromNormalizedParameterValue(index, value);
            }
        }

        index -= this->p_02[0]->getNumParameters();

        if (index < this->p_03->getNumParameters())
            return this->p_03->convertFromNormalizedParameterValue(index, value);

        index -= this->p_03->getNumParameters();

        if (index < this->p_04->getNumParameters())
            return this->p_04->convertFromNormalizedParameterValue(index, value);

        index -= this->p_04->getNumParameters();

        if (index < this->p_05[0]->getNumParameters()) {
            {
                return this->p_05[0]->convertFromNormalizedParameterValue(index, value);
            }
        }

        index -= this->p_05[0]->getNumParameters();

        if (index < this->p_06->getNumParameters())
            return this->p_06->convertFromNormalizedParameterValue(index, value);

        return value;
        }
    }
}

ParameterValue constrainParameterValue(ParameterIndex index, ParameterValue value) const {
    switch (index) {
    case 0:
        {
        return this->param_01_value_constrain(value);
        }
    case 1:
        {
        return this->param_02_value_constrain(value);
        }
    case 2:
        {
        return this->param_03_value_constrain(value);
        }
    case 3:
        {
        return this->param_04_value_constrain(value);
        }
    case 4:
        {
        return this->param_05_value_constrain(value);
        }
    case 5:
        {
        return this->param_06_value_constrain(value);
        }
    case 6:
        {
        return this->param_07_value_constrain(value);
        }
    case 7:
        {
        return this->param_08_value_constrain(value);
        }
    case 8:
        {
        return this->param_09_value_constrain(value);
        }
    default:
        {
        index -= 9;

        if (index < this->p_01->getNumParameters())
            return this->p_01->constrainParameterValue(index, value);

        index -= this->p_01->getNumParameters();

        if (index < this->p_02[0]->getNumParameters()) {
            {
                return this->p_02[0]->constrainParameterValue(index, value);
            }
        }

        index -= this->p_02[0]->getNumParameters();

        if (index < this->p_03->getNumParameters())
            return this->p_03->constrainParameterValue(index, value);

        index -= this->p_03->getNumParameters();

        if (index < this->p_04->getNumParameters())
            return this->p_04->constrainParameterValue(index, value);

        index -= this->p_04->getNumParameters();

        if (index < this->p_05[0]->getNumParameters()) {
            {
                return this->p_05[0]->constrainParameterValue(index, value);
            }
        }

        index -= this->p_05[0]->getNumParameters();

        if (index < this->p_06->getNumParameters())
            return this->p_06->constrainParameterValue(index, value);

        return value;
        }
    }
}

void processNumMessage(MessageTag tag, MessageTag objectId, MillisecondTime time, number payload) {
    this->updateTime(time, (ENGINE*)nullptr);

    switch (tag) {
    case TAG("listin"):
        {
        if (TAG("message_obj-107") == objectId)
            this->message_10_listin_number_set(payload);

        break;
        }
    case TAG("valin"):
        {
        if (TAG("number_obj-110") == objectId)
            this->numberobj_17_valin_set(payload);

        if (TAG("number_obj-122") == objectId)
            this->numberobj_18_valin_set(payload);

        if (TAG("toggle_obj-11") == objectId)
            this->toggle_07_valin_set(payload);

        if (TAG("number_obj-103") == objectId)
            this->numberobj_19_valin_set(payload);

        if (TAG("toggle_obj-101") == objectId)
            this->toggle_08_valin_set(payload);

        if (TAG("number_obj-50") == objectId)
            this->numberobj_20_valin_set(payload);

        if (TAG("number_obj-39") == objectId)
            this->numberobj_21_valin_set(payload);

        if (TAG("number_obj-105") == objectId)
            this->numberobj_22_valin_set(payload);

        if (TAG("number_obj-57") == objectId)
            this->numberobj_23_valin_set(payload);

        if (TAG("number_obj-153") == objectId)
            this->numberobj_24_valin_set(payload);

        if (TAG("number_obj-123") == objectId)
            this->numberobj_25_valin_set(payload);

        if (TAG("toggle_obj-104") == objectId)
            this->toggle_09_valin_set(payload);

        if (TAG("number_obj-182") == objectId)
            this->numberobj_26_valin_set(payload);

        if (TAG("number_obj-184") == objectId)
            this->numberobj_27_valin_set(payload);

        if (TAG("toggle_obj-79") == objectId)
            this->toggle_10_valin_set(payload);

        break;
        }
    case TAG("format"):
        {
        if (TAG("number_obj-110") == objectId)
            this->numberobj_17_format_set(payload);

        if (TAG("number_obj-122") == objectId)
            this->numberobj_18_format_set(payload);

        if (TAG("number_obj-103") == objectId)
            this->numberobj_19_format_set(payload);

        if (TAG("number_obj-50") == objectId)
            this->numberobj_20_format_set(payload);

        if (TAG("number_obj-39") == objectId)
            this->numberobj_21_format_set(payload);

        if (TAG("number_obj-105") == objectId)
            this->numberobj_22_format_set(payload);

        if (TAG("number_obj-57") == objectId)
            this->numberobj_23_format_set(payload);

        if (TAG("number_obj-153") == objectId)
            this->numberobj_24_format_set(payload);

        if (TAG("number_obj-123") == objectId)
            this->numberobj_25_format_set(payload);

        if (TAG("number_obj-182") == objectId)
            this->numberobj_26_format_set(payload);

        if (TAG("number_obj-184") == objectId)
            this->numberobj_27_format_set(payload);

        break;
        }
    }

    this->p_01->processNumMessage(tag, objectId, time, payload);

    for (Index i = 0; i < 8; i++) {
        this->p_02[i]->processNumMessage(tag, objectId, time, payload);
    }

    this->p_03->processNumMessage(tag, objectId, time, payload);
    this->p_04->processNumMessage(tag, objectId, time, payload);

    for (Index i = 0; i < 8; i++) {
        this->p_05[i]->processNumMessage(tag, objectId, time, payload);
    }

    this->p_06->processNumMessage(tag, objectId, time, payload);
}

void processListMessage(
    MessageTag tag,
    MessageTag objectId,
    MillisecondTime time,
    const list& payload
) {
    this->updateTime(time, (ENGINE*)nullptr);

    switch (tag) {
    case TAG("listin"):
        {
        if (TAG("message_obj-107") == objectId)
            this->message_10_listin_list_set(payload);

        break;
        }
    }

    this->p_01->processListMessage(tag, objectId, time, payload);

    for (Index i = 0; i < 8; i++) {
        this->p_02[i]->processListMessage(tag, objectId, time, payload);
    }

    this->p_03->processListMessage(tag, objectId, time, payload);
    this->p_04->processListMessage(tag, objectId, time, payload);

    for (Index i = 0; i < 8; i++) {
        this->p_05[i]->processListMessage(tag, objectId, time, payload);
    }

    this->p_06->processListMessage(tag, objectId, time, payload);
}

void processBangMessage(MessageTag tag, MessageTag objectId, MillisecondTime time) {
    this->updateTime(time, (ENGINE*)nullptr);

    switch (tag) {
    case TAG("listin"):
        {
        if (TAG("message_obj-107") == objectId)
            this->message_10_listin_bang_bang();

        break;
        }
    case TAG("bangin"):
        {
        if (TAG("button_obj-102") == objectId)
            this->button_09_bangin_bang();

        break;
        }
    }

    this->p_01->processBangMessage(tag, objectId, time);

    for (Index i = 0; i < 8; i++) {
        this->p_02[i]->processBangMessage(tag, objectId, time);
    }

    this->p_03->processBangMessage(tag, objectId, time);
    this->p_04->processBangMessage(tag, objectId, time);

    for (Index i = 0; i < 8; i++) {
        this->p_05[i]->processBangMessage(tag, objectId, time);
    }

    this->p_06->processBangMessage(tag, objectId, time);
}

MessageTagInfo resolveTag(MessageTag tag) const {
    switch (tag) {
    case TAG("listout"):
        {
        return "listout";
        }
    case TAG("message_obj-107"):
        {
        return "message_obj-107";
        }
    case TAG("bangout"):
        {
        return "bangout";
        }
    case TAG("button_obj-102"):
        {
        return "button_obj-102";
        }
    case TAG("valout"):
        {
        return "valout";
        }
    case TAG("number_obj-110"):
        {
        return "number_obj-110";
        }
    case TAG("setup"):
        {
        return "setup";
        }
    case TAG("meter~_obj-96"):
        {
        return "meter~_obj-96";
        }
    case TAG("peak"):
        {
        return "peak";
        }
    case TAG("meter~_obj-97"):
        {
        return "meter~_obj-97";
        }
    case TAG("number_obj-122"):
        {
        return "number_obj-122";
        }
    case TAG("toggle_obj-11"):
        {
        return "toggle_obj-11";
        }
    case TAG("number_obj-103"):
        {
        return "number_obj-103";
        }
    case TAG("toggle_obj-101"):
        {
        return "toggle_obj-101";
        }
    case TAG("number_obj-50"):
        {
        return "number_obj-50";
        }
    case TAG("meter~_obj-99"):
        {
        return "meter~_obj-99";
        }
    case TAG("number_obj-39"):
        {
        return "number_obj-39";
        }
    case TAG("meter~_obj-98"):
        {
        return "meter~_obj-98";
        }
    case TAG("number_obj-105"):
        {
        return "number_obj-105";
        }
    case TAG("number_obj-57"):
        {
        return "number_obj-57";
        }
    case TAG("number_obj-153"):
        {
        return "number_obj-153";
        }
    case TAG("number_obj-123"):
        {
        return "number_obj-123";
        }
    case TAG("toggle_obj-104"):
        {
        return "toggle_obj-104";
        }
    case TAG("number_obj-182"):
        {
        return "number_obj-182";
        }
    case TAG("number_obj-184"):
        {
        return "number_obj-184";
        }
    case TAG("toggle_obj-79"):
        {
        return "toggle_obj-79";
        }
    case TAG("listin"):
        {
        return "listin";
        }
    case TAG("bangin"):
        {
        return "bangin";
        }
    case TAG("valin"):
        {
        return "valin";
        }
    case TAG("format"):
        {
        return "format";
        }
    }

    auto subpatchResult_0 = this->p_01->resolveTag(tag);

    if (subpatchResult_0)
        return subpatchResult_0;

    auto subpatchResult_1 = this->p_02[0]->resolveTag(tag);

    if (subpatchResult_1)
        return subpatchResult_1;

    auto subpatchResult_2 = this->p_03->resolveTag(tag);

    if (subpatchResult_2)
        return subpatchResult_2;

    auto subpatchResult_3 = this->p_04->resolveTag(tag);

    if (subpatchResult_3)
        return subpatchResult_3;

    auto subpatchResult_4 = this->p_05[0]->resolveTag(tag);

    if (subpatchResult_4)
        return subpatchResult_4;

    auto subpatchResult_5 = this->p_06->resolveTag(tag);

    if (subpatchResult_5)
        return subpatchResult_5;

    return "";
}

MessageIndex getNumMessages() const {
    return 1;
}

const MessageInfo& getMessageInfo(MessageIndex index) const {
    switch (index) {
    case 0:
        {
        static const MessageInfo r0 = {
            "attack",
            Outport
        };

        return r0;
        }
    }

    return NullMessageInfo;
}

protected:

class RNBOSubpatcher_244 : public PatcherInterfaceImpl {
    
    friend class rnbomatic;
    
    public:
    
    RNBOSubpatcher_244()
    {}
    
    ~RNBOSubpatcher_244()
    {
        deallocateSignals();
    }
    
    Index getNumMidiInputPorts() const {
        return 0;
    }
    
    void processMidiEvent(MillisecondTime , int , ConstByteArray , Index ) {}
    
    Index getNumMidiOutputPorts() const {
        return 0;
    }
    
    void process(
        const SampleValue * const* inputs,
        Index numInputs,
        SampleValue * const* outputs,
        Index numOutputs,
        Index n
    ) {
        RNBO_UNUSED(numOutputs);
        RNBO_UNUSED(outputs);
        this->vs = n;
        this->updateTime(this->getEngine()->getCurrentTime(), (ENGINE*)nullptr, true);
        const SampleValue * in1 = (numInputs >= 1 && inputs[0] ? inputs[0] : this->zeroBuffer);
        const SampleValue * in2 = (numInputs >= 2 && inputs[1] ? inputs[1] : this->zeroBuffer);
    
        this->rampsmooth_tilde_01_perform(
            this->rampsmooth_tilde_01_x,
            this->rampsmooth_tilde_01_up,
            this->rampsmooth_tilde_01_down,
            this->signals[0],
            n
        );
    
        this->dspexpr_03_perform(this->signals[0], this->dspexpr_03_in2, this->signals[1], n);
    
        this->average_absolute_tilde_01_perform(
            this->signals[1],
            this->average_absolute_tilde_01_windowSize,
            this->average_absolute_tilde_01_reset,
            this->signals[0],
            n
        );
    
        this->peakamp_02_perform(this->signals[0], n);
        this->dspexpr_01_perform(in1, in2, this->signals[0], n);
        this->dspexpr_02_perform(this->signals[0], this->dspexpr_02_in2, this->signals[2], n);
        this->peakamp_01_perform(this->signals[2], n);
        this->dspexpr_05_perform(this->dspexpr_05_in1, this->dspexpr_05_in2, this->signals[2], n);
        this->dspexpr_04_perform(this->signals[2], this->dspexpr_04_in2, this->signals[0], n);
        this->thresh_tilde_01_perform(this->signals[1], this->signals[0], this->signals[2], this->signals[3], n);
        this->edge_01_perform(this->signals[3], n);
        this->stackprotect_perform(n);
        this->audioProcessSampleCount += this->vs;
    }
    
    void prepareToProcess(number sampleRate, Index maxBlockSize, bool force) {
        RNBO_ASSERT(this->_isInitialized);
    
        if (this->maxvs < maxBlockSize || !this->didAllocateSignals) {
            Index i;
    
            for (i = 0; i < 4; i++) {
                this->signals[i] = resizeSignal(this->signals[i], this->maxvs, maxBlockSize);
            }
    
            this->zeroBuffer = resizeSignal(this->zeroBuffer, this->maxvs, maxBlockSize);
            this->dummyBuffer = resizeSignal(this->dummyBuffer, this->maxvs, maxBlockSize);
            this->didAllocateSignals = true;
        }
    
        const bool sampleRateChanged = sampleRate != this->sr;
        const bool maxvsChanged = maxBlockSize != this->maxvs;
        const bool forceDSPSetup = sampleRateChanged || maxvsChanged || force;
    
        if (sampleRateChanged || maxvsChanged) {
            this->vs = maxBlockSize;
            this->maxvs = maxBlockSize;
            this->sr = sampleRate;
            this->invsr = 1 / sampleRate;
        }
    
        this->rampsmooth_tilde_01_dspsetup(forceDSPSetup);
        this->average_absolute_tilde_01_dspsetup(forceDSPSetup);
        this->peakamp_02_dspsetup(forceDSPSetup);
        this->peakamp_01_dspsetup(forceDSPSetup);
        this->edge_01_dspsetup(forceDSPSetup);
    
        if (sampleRateChanged)
            this->onSampleRateChanged(sampleRate);
    }
    
    number msToSamps(MillisecondTime ms, number sampleRate) {
        return ms * sampleRate * 0.001;
    }
    
    MillisecondTime sampsToMs(SampleIndex samps) {
        return samps * (this->invsr * 1000);
    }
    
    Index getNumInputChannels() const {
        return 2;
    }
    
    Index getNumOutputChannels() const {
        return 0;
    }
    
    void getPreset(PatcherStateInterface& ) {}
    
    void setPreset(MillisecondTime , PatcherStateInterface& ) {}
    
    void setParameterValue(ParameterIndex , ParameterValue , MillisecondTime ) {}
    
    void processParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
        this->setParameterValue(index, value, time);
    }
    
    void processParameterBangEvent(ParameterIndex index, MillisecondTime time) {
        this->setParameterValue(index, this->getParameterValue(index), time);
    }
    
    void processNormalizedParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
        this->setParameterValueNormalized(index, value, time);
    }
    
    ParameterValue getParameterValue(ParameterIndex index)  {
        switch (index) {
        default:
            {
            return 0;
            }
        }
    }
    
    ParameterIndex getNumSignalInParameters() const {
        return 0;
    }
    
    ParameterIndex getNumSignalOutParameters() const {
        return 0;
    }
    
    ParameterIndex getNumParameters() const {
        return 0;
    }
    
    ConstCharPointer getParameterName(ParameterIndex index) const {
        switch (index) {
        default:
            {
            return "bogus";
            }
        }
    }
    
    ConstCharPointer getParameterId(ParameterIndex index) const {
        switch (index) {
        default:
            {
            return "bogus";
            }
        }
    }
    
    void getParameterInfo(ParameterIndex , ParameterInfo * ) const {}
    
    ParameterValue applyStepsToNormalizedParameterValue(ParameterValue normalizedValue, int steps) const {
        if (steps == 1) {
            if (normalizedValue > 0) {
                normalizedValue = 1.;
            }
        } else {
            ParameterValue oneStep = (number)1. / (steps - 1);
            ParameterValue numberOfSteps = rnbo_fround(normalizedValue / oneStep * 1 / (number)1) * (number)1;
            normalizedValue = numberOfSteps * oneStep;
        }
    
        return normalizedValue;
    }
    
    ParameterValue convertToNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
        switch (index) {
        default:
            {
            return value;
            }
        }
    }
    
    ParameterValue convertFromNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
        value = (value < 0 ? 0 : (value > 1 ? 1 : value));
    
        switch (index) {
        default:
            {
            return value;
            }
        }
    }
    
    ParameterValue constrainParameterValue(ParameterIndex index, ParameterValue value) const {
        switch (index) {
        default:
            {
            return value;
            }
        }
    }
    
    void processNumMessage(MessageTag tag, MessageTag objectId, MillisecondTime time, number payload) {
        this->updateTime(time, (ENGINE*)nullptr);
    
        switch (tag) {
        case TAG("valin"):
            {
            if (TAG("Attack~/number_obj-74") == objectId)
                this->numberobj_01_valin_set(payload);
    
            if (TAG("Attack~/toggle_obj-64") == objectId)
                this->toggle_01_valin_set(payload);
    
            if (TAG("Attack~/number_obj-29") == objectId)
                this->numberobj_02_valin_set(payload);
    
            if (TAG("Attack~/toggle_obj-16") == objectId)
                this->toggle_02_valin_set(payload);
    
            if (TAG("Attack~/number_obj-61") == objectId)
                this->numberobj_03_valin_set(payload);
    
            if (TAG("Attack~/number_obj-58") == objectId)
                this->numberobj_04_valin_set(payload);
    
            if (TAG("Attack~/number_obj-38") == objectId)
                this->numberobj_05_valin_set(payload);
    
            if (TAG("Attack~/number_obj-124") == objectId)
                this->numberobj_06_valin_set(payload);
    
            break;
            }
        case TAG("format"):
            {
            if (TAG("Attack~/number_obj-74") == objectId)
                this->numberobj_01_format_set(payload);
    
            if (TAG("Attack~/number_obj-29") == objectId)
                this->numberobj_02_format_set(payload);
    
            if (TAG("Attack~/number_obj-61") == objectId)
                this->numberobj_03_format_set(payload);
    
            if (TAG("Attack~/number_obj-58") == objectId)
                this->numberobj_04_format_set(payload);
    
            if (TAG("Attack~/number_obj-38") == objectId)
                this->numberobj_05_format_set(payload);
    
            if (TAG("Attack~/number_obj-124") == objectId)
                this->numberobj_06_format_set(payload);
    
            break;
            }
        case TAG("listin"):
            {
            if (TAG("Attack~/message_obj-44") == objectId)
                this->message_01_listin_number_set(payload);
    
            break;
            }
        }
    }
    
    void processListMessage(
        MessageTag tag,
        MessageTag objectId,
        MillisecondTime time,
        const list& payload
    ) {
        this->updateTime(time, (ENGINE*)nullptr);
    
        switch (tag) {
        case TAG("listin"):
            {
            if (TAG("Attack~/message_obj-44") == objectId)
                this->message_01_listin_list_set(payload);
    
            break;
            }
        }
    }
    
    void processBangMessage(MessageTag tag, MessageTag objectId, MillisecondTime time) {
        this->updateTime(time, (ENGINE*)nullptr);
    
        switch (tag) {
        case TAG("bangin"):
            {
            if (TAG("Attack~/button_obj-78") == objectId)
                this->button_01_bangin_bang();
    
            if (TAG("Attack~/button_obj-80") == objectId)
                this->button_02_bangin_bang();
    
            if (TAG("Attack~/button_obj-53") == objectId)
                this->button_03_bangin_bang();
    
            if (TAG("Attack~/button_obj-43") == objectId)
                this->button_04_bangin_bang();
    
            break;
            }
        case TAG("listin"):
            {
            if (TAG("Attack~/message_obj-44") == objectId)
                this->message_01_listin_bang_bang();
    
            break;
            }
        }
    }
    
    MessageTagInfo resolveTag(MessageTag tag) const {
        switch (tag) {
        case TAG("bangout"):
            {
            return "bangout";
            }
        case TAG("Attack~/button_obj-78"):
            {
            return "Attack~/button_obj-78";
            }
        case TAG("valout"):
            {
            return "valout";
            }
        case TAG("Attack~/number_obj-74"):
            {
            return "Attack~/number_obj-74";
            }
        case TAG("setup"):
            {
            return "setup";
            }
        case TAG("Attack~/button_obj-80"):
            {
            return "Attack~/button_obj-80";
            }
        case TAG("Attack~/toggle_obj-64"):
            {
            return "Attack~/toggle_obj-64";
            }
        case TAG("Attack~/number_obj-29"):
            {
            return "Attack~/number_obj-29";
            }
        case TAG("Attack~/toggle_obj-16"):
            {
            return "Attack~/toggle_obj-16";
            }
        case TAG("Attack~/button_obj-53"):
            {
            return "Attack~/button_obj-53";
            }
        case TAG("listout"):
            {
            return "listout";
            }
        case TAG("Attack~/message_obj-44"):
            {
            return "Attack~/message_obj-44";
            }
        case TAG("attack"):
            {
            return "attack";
            }
        case TAG(""):
            {
            return "";
            }
        case TAG("Attack~/number_obj-61"):
            {
            return "Attack~/number_obj-61";
            }
        case TAG("Attack~/number_obj-58"):
            {
            return "Attack~/number_obj-58";
            }
        case TAG("Attack~/button_obj-43"):
            {
            return "Attack~/button_obj-43";
            }
        case TAG("Attack~/number_obj-38"):
            {
            return "Attack~/number_obj-38";
            }
        case TAG("Attack~/number_obj-124"):
            {
            return "Attack~/number_obj-124";
            }
        case TAG("bangin"):
            {
            return "bangin";
            }
        case TAG("valin"):
            {
            return "valin";
            }
        case TAG("format"):
            {
            return "format";
            }
        case TAG("listin"):
            {
            return "listin";
            }
        }
    
        return nullptr;
    }
    
    DataRef* getDataRef(DataRefIndex index)  {
        switch (index) {
        case 3:
            {
            return addressOf(this->average_absolute_tilde_01_av_bufferobj);
            break;
            }
        default:
            {
            return nullptr;
            }
        }
    }
    
    DataRefIndex getNumDataRefs() const {
        return 1;
    }
    
    void processDataViewUpdate(DataRefIndex index, MillisecondTime time) {
        this->updateTime(time, (ENGINE*)nullptr);
    
        if (index == 3) {
            this->average_absolute_tilde_01_av_buffer = reInitDataView(
                this->average_absolute_tilde_01_av_buffer,
                this->average_absolute_tilde_01_av_bufferobj
            );
        }
    }
    
    void initialize() {
        RNBO_ASSERT(!this->_isInitialized);
    
        this->average_absolute_tilde_01_av_bufferobj = initDataRef(
            this->average_absolute_tilde_01_av_bufferobj,
            this->dataRefStrings->name0,
            true,
            this->dataRefStrings->file0,
            this->dataRefStrings->tag0
        );
    
        this->assign_defaults();
        this->applyState();
        this->average_absolute_tilde_01_av_bufferobj->setIndex(3);
        this->average_absolute_tilde_01_av_buffer = new Float64Buffer(this->average_absolute_tilde_01_av_bufferobj);
        this->_isInitialized = true;
    }
    
    protected:
    
    void updateTime(MillisecondTime time, INTERNALENGINE*, bool inProcess = false) {
    	if (time == TimeNow) time = getTopLevelPatcher()->getPatcherTime();
    	getTopLevelPatcher()->processInternalEvents(time);
    	updateTime(time, (EXTERNALENGINE*)nullptr);
    }
    
    RNBOSubpatcher_244* operator->() {
        return this;
    }
    const RNBOSubpatcher_244* operator->() const {
        return this;
    }
    virtual rnbomatic* getPatcher() const {
        return static_cast<rnbomatic *>(_parentPatcher);
    }
    
    rnbomatic* getTopLevelPatcher() {
        return this->getPatcher()->getTopLevelPatcher();
    }
    
    void cancelClockEvents()
    {
        getEngine()->flushClockEvents(this, -611950441, false);
        getEngine()->flushClockEvents(this, -1584063977, false);
        getEngine()->flushClockEvents(this, -1357044121, false);
        getEngine()->flushClockEvents(this, 1812006465, false);
        getEngine()->flushClockEvents(this, 252884992, false);
    }
    
    number mstosamps(MillisecondTime ms) {
        return ms * this->sr * 0.001;
    }
    
    inline number safediv(number num, number denom) {
        return (denom == 0.0 ? 0.0 : num / denom);
    }
    
    number safepow(number base, number exponent) {
        return fixnan(rnbo_pow(base, exponent));
    }
    
    number scale(
        number x,
        number lowin,
        number hiin,
        number lowout,
        number highout,
        number pow
    ) {
        auto inscale = this->safediv(1., hiin - lowin);
        number outdiff = highout - lowout;
        number value = (x - lowin) * inscale;
    
        if (pow != 1) {
            if (value > 0)
                value = this->safepow(value, pow);
            else
                value = -this->safepow(-value, pow);
        }
    
        value = value * outdiff + lowout;
        return value;
    }
    
    MillisecondTime getPatcherTime() const {
        return this->_currentTime;
    }
    
    void button_01_bangin_bang() {
        this->button_01_bangval_bang();
    }
    
    void numberobj_01_valin_set(number v) {
        this->numberobj_01_value_set(v);
    }
    
    void numberobj_01_format_set(number v) {
        this->numberobj_01_currentFormat = trunc((v > 6 ? 6 : (v < 0 ? 0 : v)));
    }
    
    void edge_01_onout_bang() {
        this->button_01_bangval_bang();
    }
    
    void edge_01_offout_bang() {
        this->button_02_bangval_bang();
    }
    
    void button_02_bangin_bang() {
        this->button_02_bangval_bang();
    }
    
    void toggle_01_valin_set(number v) {
        this->toggle_01_value_number_set(v);
    }
    
    void numberobj_02_valin_set(number v) {
        this->numberobj_02_value_set(v);
    }
    
    void numberobj_02_format_set(number v) {
        this->numberobj_02_currentFormat = trunc((v > 6 ? 6 : (v < 0 ? 0 : v)));
    }
    
    void toggle_02_valin_set(number v) {
        this->toggle_02_value_number_set(v);
    }
    
    void button_03_bangin_bang() {
        this->button_03_bangval_bang();
    }
    
    template<typename LISTTYPE> void message_01_listin_list_set(const LISTTYPE& v) {
        this->message_01_set_set(v);
    }
    
    void message_01_listin_number_set(number v) {
        this->message_01_set_set(v);
    }
    
    void message_01_listin_bang_bang() {
        this->message_01_trigger_bang();
    }
    
    void numberobj_03_valin_set(number v) {
        this->numberobj_03_value_set(v);
    }
    
    void numberobj_03_format_set(number v) {
        this->numberobj_03_currentFormat = trunc((v > 6 ? 6 : (v < 0 ? 0 : v)));
    }
    
    void numberobj_04_valin_set(number v) {
        this->numberobj_04_value_set(v);
    }
    
    void numberobj_04_format_set(number v) {
        this->numberobj_04_currentFormat = trunc((v > 6 ? 6 : (v < 0 ? 0 : v)));
    }
    
    void eventinlet_01_out1_bang_bang() {
        this->numberobj_03_value_bang();
    }
    
    template<typename LISTTYPE> void eventinlet_01_out1_list_set(const LISTTYPE& v) {
        {
            number converted = (v->length > 0 ? v[0] : 0);
            this->numberobj_03_value_set(converted);
        }
    }
    
    void button_04_bangin_bang() {
        this->button_04_bangval_bang();
    }
    
    void delay_01_out_bang() {
        this->button_04_bangval_bang();
    }
    
    void numberobj_05_valin_set(number v) {
        this->numberobj_05_value_set(v);
    }
    
    void numberobj_05_format_set(number v) {
        this->numberobj_05_currentFormat = trunc((v > 6 ? 6 : (v < 0 ? 0 : v)));
    }
    
    void eventinlet_02_out1_bang_bang() {
        this->numberobj_05_value_bang();
    }
    
    template<typename LISTTYPE> void eventinlet_02_out1_list_set(const LISTTYPE& v) {
        {
            number converted = (v->length > 0 ? v[0] : 0);
            this->numberobj_05_value_set(converted);
        }
    }
    
    void numberobj_06_valin_set(number v) {
        this->numberobj_06_value_set(v);
    }
    
    void numberobj_06_format_set(number v) {
        this->numberobj_06_currentFormat = trunc((v > 6 ? 6 : (v < 0 ? 0 : v)));
    }
    
    void peakamp_02_output_set(number v) {
        this->peakamp_02_output = v;
        this->dspexpr_05_in1_set(v);
    }
    
    void peakamp_01_output_set(number v) {
        this->peakamp_01_output = v;
        this->rampsmooth_tilde_01_x_set(v);
    }
    
    void deallocateSignals() {
        Index i;
    
        for (i = 0; i < 4; i++) {
            this->signals[i] = freeSignal(this->signals[i]);
        }
    
        this->zeroBuffer = freeSignal(this->zeroBuffer);
        this->dummyBuffer = freeSignal(this->dummyBuffer);
    }
    
    Index getMaxBlockSize() const {
        return this->maxvs;
    }
    
    number getSampleRate() const {
        return this->sr;
    }
    
    bool hasFixedVectorSize() const {
        return false;
    }
    
    void setProbingTarget(MessageTag ) {}
    
    void initializeObjects() {
        this->numberobj_01_init();
        this->thresh_tilde_01_init();
        this->average_absolute_tilde_01_av_init();
        this->numberobj_02_init();
        this->message_01_init();
        this->numberobj_03_init();
        this->numberobj_04_init();
        this->numberobj_05_init();
        this->numberobj_06_init();
    }
    
    Index getIsMuted()  {
        return this->isMuted;
    }
    
    void setIsMuted(Index v)  {
        this->isMuted = v;
    }
    
    void onSampleRateChanged(double ) {}
    
    void extractState(PatcherStateInterface& ) {}
    
    void applyState() {}
    
    void setParameterOffset(ParameterIndex offset) {
        this->parameterOffset = offset;
    }
    
    void processClockEvent(MillisecondTime time, ClockId index, bool hasValue, ParameterValue value) {
        RNBO_UNUSED(hasValue);
        this->updateTime(time, (ENGINE*)nullptr);
    
        switch (index) {
        case -611950441:
            {
            this->edge_01_onout_bang();
            break;
            }
        case -1584063977:
            {
            this->edge_01_offout_bang();
            break;
            }
        case -1357044121:
            {
            this->delay_01_out_bang();
            break;
            }
        case 1812006465:
            {
            this->peakamp_02_output_set(value);
            break;
            }
        case 252884992:
            {
            this->peakamp_01_output_set(value);
            break;
            }
        }
    }
    
    void processOutletAtCurrentTime(EngineLink* , OutletIndex , ParameterValue ) {}
    
    void processOutletEvent(
        EngineLink* sender,
        OutletIndex index,
        ParameterValue value,
        MillisecondTime time
    ) {
        this->updateTime(time, (ENGINE*)nullptr);
        this->processOutletAtCurrentTime(sender, index, value);
    }
    
    void sendOutlet(OutletIndex index, ParameterValue value) {
        this->getEngine()->sendOutlet(this, index, value);
    }
    
    void startup() {}
    
    void fillDataRef(DataRefIndex , DataRef& ) {}
    
    void zeroDataRef(DataRef& ref) {
        ref->setZero();
    }
    
    void allocateDataRefs() {
        this->average_absolute_tilde_01_av_buffer = this->average_absolute_tilde_01_av_buffer->allocateIfNeeded();
    
        if (this->average_absolute_tilde_01_av_bufferobj->hasRequestedSize()) {
            if (this->average_absolute_tilde_01_av_bufferobj->wantsFill())
                this->zeroDataRef(this->average_absolute_tilde_01_av_bufferobj);
    
            this->getEngine()->sendDataRefUpdated(3);
        }
    }
    
    void expr_02_in2_set(number v) {
        this->expr_02_in2 = v;
    }
    
    void numberobj_04_output_set(number v) {
        this->expr_02_in2_set(v);
    }
    
    void numberobj_04_value_set(number v) {
        this->numberobj_04_value_setter(v);
        v = this->numberobj_04_value;
        number localvalue = v;
    
        if (this->numberobj_04_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_04_output_set(localvalue);
    }
    
    template<typename LISTTYPE> void scale_02_out_set(const LISTTYPE& v) {
        this->scale_02_out = jsCreateListCopy(v);
    
        {
            number converted = (v->length > 0 ? v[0] : 0);
            this->numberobj_04_value_set(converted);
        }
    }
    
    template<typename LISTTYPE> void scale_02_input_set(const LISTTYPE& v) {
        this->scale_02_input = jsCreateListCopy(v);
        list tmp = {};
    
        for (Index i = 0; i < v->length; i++) {
            tmp->push(this->scale(
                v[(Index)i],
                this->scale_02_inlow,
                this->scale_02_inhigh,
                this->scale_02_outlow,
                this->scale_02_outhigh,
                this->scale_02_power
            ));
        }
    
        this->scale_02_out_set(tmp);
    }
    
    void dspexpr_05_in2_set(number v) {
        this->dspexpr_05_in2 = v;
    }
    
    void numberobj_02_output_set(number v) {
        this->dspexpr_05_in2_set(v);
    }
    
    void numberobj_02_value_set(number v) {
        this->numberobj_02_value_setter(v);
        v = this->numberobj_02_value;
        number localvalue = v;
    
        if (this->numberobj_02_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_02_output_set(localvalue);
    }
    
    template<typename LISTTYPE> void scale_01_out_set(const LISTTYPE& v) {
        this->scale_01_out = jsCreateListCopy(v);
    
        {
            number converted = (v->length > 0 ? v[0] : 0);
            this->numberobj_02_value_set(converted);
        }
    }
    
    template<typename LISTTYPE> void scale_01_input_set(const LISTTYPE& v) {
        this->scale_01_input = jsCreateListCopy(v);
        list tmp = {};
    
        for (Index i = 0; i < v->length; i++) {
            tmp->push(this->scale(
                v[(Index)i],
                this->scale_01_inlow,
                this->scale_01_inhigh,
                this->scale_01_outlow,
                this->scale_01_outhigh,
                this->scale_01_power
            ));
        }
    
        this->scale_01_out_set(tmp);
    }
    
    void dspexpr_02_in2_set(number v) {
        this->dspexpr_02_in2 = v;
    }
    
    void toggle_01_output_set(number v) {
        this->dspexpr_02_in2_set(v);
    }
    
    void toggle_01_value_number_set(number v) {
        this->toggle_01_value_number_setter(v);
        v = this->toggle_01_value_number;
        this->toggle_01_output_set(v);
    }
    
    void expr_01_out1_set(number v) {
        this->expr_01_out1 = v;
        this->toggle_01_value_number_set(this->expr_01_out1);
    }
    
    void expr_01_in1_set(number in1) {
        this->expr_01_in1 = in1;
        this->expr_01_out1_set(this->expr_01_in1 > this->expr_01_in2);//#map:Attack~/>_obj-59:1
    }
    
    void numberobj_03_output_set(number v) {
        {
            listbase<number, 1> converted = {v};
            this->scale_02_input_set(converted);
        }
    
        {
            listbase<number, 1> converted = {v};
            this->scale_01_input_set(converted);
        }
    
        this->expr_01_in1_set(v);
    }
    
    void numberobj_03_value_set(number v) {
        this->numberobj_03_value_setter(v);
        v = this->numberobj_03_value;
        number localvalue = v;
    
        if (this->numberobj_03_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_03_output_set(localvalue);
    }
    
    void eventinlet_01_out1_number_set(number v) {
        this->numberobj_03_value_set(v);
    }
    
    number delay_01_time_constrain(number v) const {
        if (v < 0)
            v = 0;
    
        return v;
    }
    
    void delay_01_time_set(number v) {
        v = this->delay_01_time_constrain(v);
        this->delay_01_time = v;
    }
    
    void numberobj_06_output_set(number v) {
        this->delay_01_time_set(v);
    }
    
    void numberobj_06_value_set(number v) {
        this->numberobj_06_value_setter(v);
        v = this->numberobj_06_value;
        number localvalue = v;
    
        if (this->numberobj_06_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_06_output_set(localvalue);
    }
    
    template<typename LISTTYPE> void scale_03_out_set(const LISTTYPE& v) {
        this->scale_03_out = jsCreateListCopy(v);
    
        {
            number converted = (v->length > 0 ? v[0] : 0);
            this->numberobj_06_value_set(converted);
        }
    }
    
    template<typename LISTTYPE> void scale_03_input_set(const LISTTYPE& v) {
        this->scale_03_input = jsCreateListCopy(v);
        list tmp = {};
    
        for (Index i = 0; i < v->length; i++) {
            tmp->push(this->scale(
                v[(Index)i],
                this->scale_03_inlow,
                this->scale_03_inhigh,
                this->scale_03_outlow,
                this->scale_03_outhigh,
                this->scale_03_power
            ));
        }
    
        this->scale_03_out_set(tmp);
    }
    
    void numberobj_05_output_set(number v) {
        {
            listbase<number, 1> converted = {v};
            this->scale_03_input_set(converted);
        }
    }
    
    void numberobj_05_value_set(number v) {
        this->numberobj_05_value_setter(v);
        v = this->numberobj_05_value;
        number localvalue = v;
    
        if (this->numberobj_05_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_05_output_set(localvalue);
    }
    
    void eventinlet_02_out1_number_set(number v) {
        this->numberobj_05_value_set(v);
    }
    
    void delay_01_stop_bang() {
        this->getEngine()->flushClockEvents(this, -1357044121, false);;
    }
    
    void delay_01_input_bang() {
        if ((bool)(!(bool)(this->delay_01_delayall)))
            this->delay_01_stop_bang();
    
        this->getEngine()->scheduleClockEvent(this, -1357044121, this->delay_01_time + this->_currentTime);;
    }
    
    template<typename LISTTYPE> void outport_01_input_list_set(const LISTTYPE& v) {
        this->getEngine()->sendListMessage(TAG("attack"), TAG(""), v, this->_currentTime);
    }
    
    template<typename LISTTYPE> void message_01_out_set(const LISTTYPE& v) {
        this->outport_01_input_list_set(v);
    }
    
    void message_01_trigger_bang() {
        if ((bool)(this->message_01_set->length) || (bool)(false)) {
            this->message_01_out_set(this->message_01_set);
        }
    }
    
    void button_03_output_bang() {
        this->delay_01_input_bang();
        this->message_01_trigger_bang();
    }
    
    void button_03_bangval_bang() {
        ;
        this->button_03_output_bang();
    }
    
    void select_01_match1_bang() {
        this->button_03_bangval_bang();
    }
    
    void select_01_nomatch_number_set(number ) {}
    
    void select_01_input_number_set(number v) {
        if (v == this->select_01_test1)
            this->select_01_match1_bang();
        else
            this->select_01_nomatch_number_set(v);
    }
    
    void toggle_02_output_set(number v) {
        this->select_01_input_number_set(v);
    }
    
    void toggle_02_value_number_set(number v) {
        this->toggle_02_value_number_setter(v);
        v = this->toggle_02_value_number;
        this->toggle_02_output_set(v);
    }
    
    void expr_02_out1_set(number v) {
        this->expr_02_out1 = v;
        this->toggle_02_value_number_set(this->expr_02_out1);
    }
    
    void expr_02_in1_set(number in1) {
        this->expr_02_in1 = in1;
        this->expr_02_out1_set(this->expr_02_in1 >= this->expr_02_in2);//#map:Attack~/>=_obj-26:1
    }
    
    void numberobj_01_output_set(number v) {
        this->expr_02_in1_set(v);
    }
    
    void numberobj_01_value_set(number v) {
        this->numberobj_01_value_setter(v);
        v = this->numberobj_01_value;
        number localvalue = v;
    
        if (this->numberobj_01_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_01_output_set(localvalue);
    }
    
    void timer_01_out_set(number v) {
        this->numberobj_01_value_set(v);
    }
    
    void timer_01_end_bang() {
        this->timer_01_elapsed = this->_currentTime;
        this->timer_01_out_set(this->timer_01_elapsed - this->timer_01_starttime);
        this->timer_01_elapsed = 0;
    }
    
    void button_01_output_bang() {
        this->timer_01_end_bang();
    }
    
    void button_01_bangval_bang() {
        ;
        this->button_01_output_bang();
    }
    
    void timer_01_start_bang() {
        this->timer_01_starttime = this->_currentTime;
    }
    
    void button_02_output_bang() {
        this->timer_01_start_bang();
    }
    
    void button_02_bangval_bang() {
        ;
        this->button_02_output_bang();
    }
    
    template<typename LISTTYPE> void message_01_set_set(const LISTTYPE& v) {
        this->message_01_set = jsCreateListCopy(v);
    }
    
    void numberobj_03_value_bang() {
        number v = this->numberobj_03_value;
        number localvalue = v;
    
        if (this->numberobj_03_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_03_output_set(localvalue);
    }
    
    void eventoutlet_01_in1_bang_bang() {
        ;
        this->getPatcher()->updateTime(this->_currentTime, (ENGINE*)nullptr);
        this->getPatcher()->p_01_out1_bang_bang();
    }
    
    void button_04_output_bang() {
        this->eventoutlet_01_in1_bang_bang();
    }
    
    void button_04_bangval_bang() {
        ;
        this->button_04_output_bang();
    }
    
    void numberobj_05_value_bang() {
        number v = this->numberobj_05_value;
        number localvalue = v;
    
        if (this->numberobj_05_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_05_output_set(localvalue);
    }
    
    void rampsmooth_tilde_01_perform(number x, number up, number down, SampleValue * out1, Index n) {
        RNBO_UNUSED(down);
        RNBO_UNUSED(up);
        auto __rampsmooth_tilde_01_increment = this->rampsmooth_tilde_01_increment;
        auto __rampsmooth_tilde_01_index = this->rampsmooth_tilde_01_index;
        auto __rampsmooth_tilde_01_prev = this->rampsmooth_tilde_01_prev;
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            if (this->rampsmooth_tilde_01_d_next(x) != 0.) {
                if (x > __rampsmooth_tilde_01_prev) {
                    number _up = 480;
    
                    if (_up < 1)
                        _up = 1;
    
                    __rampsmooth_tilde_01_index = _up;
                    __rampsmooth_tilde_01_increment = (x - __rampsmooth_tilde_01_prev) / _up;
                } else if (x < __rampsmooth_tilde_01_prev) {
                    number _down = 480;
    
                    if (_down < 1)
                        _down = 1;
    
                    __rampsmooth_tilde_01_index = _down;
                    __rampsmooth_tilde_01_increment = (x - __rampsmooth_tilde_01_prev) / _down;
                }
            }
    
            if (__rampsmooth_tilde_01_index > 0) {
                __rampsmooth_tilde_01_prev += __rampsmooth_tilde_01_increment;
                __rampsmooth_tilde_01_index -= 1;
            } else {
                __rampsmooth_tilde_01_prev = x;
            }
    
            out1[(Index)i] = __rampsmooth_tilde_01_prev;
        }
    
        this->rampsmooth_tilde_01_prev = __rampsmooth_tilde_01_prev;
        this->rampsmooth_tilde_01_index = __rampsmooth_tilde_01_index;
        this->rampsmooth_tilde_01_increment = __rampsmooth_tilde_01_increment;
    }
    
    void dspexpr_03_perform(const Sample * in1, number in2, SampleValue * out1, Index n) {
        RNBO_UNUSED(in2);
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] * 100;//#map:_###_obj_###_:1
        }
    }
    
    void average_absolute_tilde_01_perform(
        const Sample * x,
        number windowSize,
        number reset,
        SampleValue * out1,
        Index n
    ) {
        RNBO_UNUSED(reset);
        RNBO_UNUSED(windowSize);
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = this->average_absolute_tilde_01_av_next(rnbo_abs(x[(Index)i]), 22000, 0);
        }
    }
    
    void dspexpr_05_in1_set(number v) {
        this->dspexpr_05_in1 = v;
    }
    
    void peakamp_02_perform(const SampleValue * input_signal, Index n) {
        auto __peakamp_02_index = this->peakamp_02_index;
        auto __peakamp_02_lastMaximum = this->peakamp_02_lastMaximum;
        auto __peakamp_02_maxIndex = this->peakamp_02_maxIndex;
        auto __peakamp_02_interval = this->peakamp_02_interval;
    
        for (Index i = 0; i < n; i++) {
            if ((bool)(this->peakamp_02_d_next(__peakamp_02_interval))) {
                __peakamp_02_maxIndex = this->mstosamps(__peakamp_02_interval);
            }
    
            number temp = rnbo_abs(input_signal[(Index)i]);
    
            if (temp > __peakamp_02_lastMaximum) {
                __peakamp_02_lastMaximum = temp;
            }
    
            __peakamp_02_index++;
    
            if (__peakamp_02_maxIndex > 0 && __peakamp_02_index >= __peakamp_02_maxIndex) {
                this->getEngine()->scheduleClockEventWithValue(
                    this,
                    1812006465,
                    this->sampsToMs((SampleIndex)(this->vs)) + this->_currentTime,
                    __peakamp_02_lastMaximum
                );;
    
                __peakamp_02_lastMaximum = 0;
            }
    
            if (__peakamp_02_maxIndex == 0 || __peakamp_02_index >= __peakamp_02_maxIndex) {
                __peakamp_02_index = 0;
            }
        }
    
        this->peakamp_02_maxIndex = __peakamp_02_maxIndex;
        this->peakamp_02_lastMaximum = __peakamp_02_lastMaximum;
        this->peakamp_02_index = __peakamp_02_index;
    }
    
    void dspexpr_01_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] + in2[(Index)i];//#map:_###_obj_###_:1
        }
    }
    
    void dspexpr_02_perform(const Sample * in1, number in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] * in2;//#map:_###_obj_###_:1
        }
    }
    
    void rampsmooth_tilde_01_x_set(number v) {
        this->rampsmooth_tilde_01_x = v;
    }
    
    void peakamp_01_perform(const SampleValue * input_signal, Index n) {
        auto __peakamp_01_index = this->peakamp_01_index;
        auto __peakamp_01_lastMaximum = this->peakamp_01_lastMaximum;
        auto __peakamp_01_maxIndex = this->peakamp_01_maxIndex;
        auto __peakamp_01_interval = this->peakamp_01_interval;
    
        for (Index i = 0; i < n; i++) {
            if ((bool)(this->peakamp_01_d_next(__peakamp_01_interval))) {
                __peakamp_01_maxIndex = this->mstosamps(__peakamp_01_interval);
            }
    
            number temp = rnbo_abs(input_signal[(Index)i]);
    
            if (temp > __peakamp_01_lastMaximum) {
                __peakamp_01_lastMaximum = temp;
            }
    
            __peakamp_01_index++;
    
            if (__peakamp_01_maxIndex > 0 && __peakamp_01_index >= __peakamp_01_maxIndex) {
                this->getEngine()->scheduleClockEventWithValue(
                    this,
                    252884992,
                    this->sampsToMs((SampleIndex)(this->vs)) + this->_currentTime,
                    __peakamp_01_lastMaximum
                );;
    
                __peakamp_01_lastMaximum = 0;
            }
    
            if (__peakamp_01_maxIndex == 0 || __peakamp_01_index >= __peakamp_01_maxIndex) {
                __peakamp_01_index = 0;
            }
        }
    
        this->peakamp_01_maxIndex = __peakamp_01_maxIndex;
        this->peakamp_01_lastMaximum = __peakamp_01_lastMaximum;
        this->peakamp_01_index = __peakamp_01_index;
    }
    
    void dspexpr_05_perform(number in1, number in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1 + in2;//#map:_###_obj_###_:1
        }
    }
    
    void dspexpr_04_perform(const Sample * in1, number in2, SampleValue * out1, Index n) {
        RNBO_UNUSED(in2);
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] + -1;//#map:_###_obj_###_:1
        }
    }
    
    void thresh_tilde_01_perform(
        const Sample * x,
        const Sample * lowThresh,
        const Sample * highThresh,
        SampleValue * out1,
        Index n
    ) {
        auto __thresh_tilde_01_isAbove = this->thresh_tilde_01_isAbove;
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            if ((bool)(__thresh_tilde_01_isAbove)) {
                if (lowThresh[(Index)i] == highThresh[(Index)i]) {
                    if (x[(Index)i] < lowThresh[(Index)i])
                        __thresh_tilde_01_isAbove = false;
                } else if (x[(Index)i] <= lowThresh[(Index)i])
                    __thresh_tilde_01_isAbove = false;
            } else if (x[(Index)i] >= highThresh[(Index)i]) {
                __thresh_tilde_01_isAbove = true;
            }
    
            out1[(Index)i] = ((bool)(__thresh_tilde_01_isAbove) ? 1. : 0.);
        }
    
        this->thresh_tilde_01_isAbove = __thresh_tilde_01_isAbove;
    }
    
    void edge_01_perform(const SampleValue * input, Index n) {
        auto __edge_01_currentState = this->edge_01_currentState;
    
        for (Index i = 0; i < n; i++) {
            if (__edge_01_currentState == 1) {
                if (input[(Index)i] == 0.) {
                    this->getEngine()->scheduleClockEvent(this, -1584063977, this->sampsToMs(i) + this->_currentTime);;
                    __edge_01_currentState = 0;
                }
            } else {
                if (input[(Index)i] != 0.) {
                    this->getEngine()->scheduleClockEvent(this, -611950441, this->sampsToMs(i) + this->_currentTime);;
                    __edge_01_currentState = 1;
                }
            }
        }
    
        this->edge_01_currentState = __edge_01_currentState;
    }
    
    void stackprotect_perform(Index n) {
        RNBO_UNUSED(n);
        auto __stackprotect_count = this->stackprotect_count;
        __stackprotect_count = 0;
        this->stackprotect_count = __stackprotect_count;
    }
    
    void numberobj_01_value_setter(number v) {
        number localvalue = v;
    
        if (this->numberobj_01_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_01_value = localvalue;
    }
    
    void toggle_01_value_number_setter(number v) {
        this->toggle_01_value_number = (v != 0 ? 1 : 0);
    }
    
    void numberobj_02_value_setter(number v) {
        number localvalue = v;
    
        if (this->numberobj_02_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_02_value = localvalue;
    }
    
    void toggle_02_value_number_setter(number v) {
        this->toggle_02_value_number = (v != 0 ? 1 : 0);
    }
    
    void numberobj_03_value_setter(number v) {
        number localvalue = v;
    
        if (this->numberobj_03_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_03_value = localvalue;
    }
    
    void numberobj_04_value_setter(number v) {
        number localvalue = v;
    
        if (this->numberobj_04_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_04_value = localvalue;
    }
    
    void numberobj_05_value_setter(number v) {
        number localvalue = v;
    
        if (this->numberobj_05_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_05_value = localvalue;
    }
    
    void numberobj_06_value_setter(number v) {
        number localvalue = v;
    
        if (this->numberobj_06_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_06_value = localvalue;
    }
    
    void numberobj_01_init() {
        this->numberobj_01_currentFormat = 6;
    }
    
    void numberobj_01_getPresetValue(PatcherStateInterface& preset) {
        preset["value"] = this->numberobj_01_value;
    }
    
    void numberobj_01_setPresetValue(PatcherStateInterface& preset) {
        if ((bool)(stateIsEmpty(preset)))
            return;
    
        this->numberobj_01_value_set(preset["value"]);
    }
    
    number rampsmooth_tilde_01_d_next(number x) {
        number temp = (number)(x - this->rampsmooth_tilde_01_d_prev);
        this->rampsmooth_tilde_01_d_prev = x;
        return temp;
    }
    
    void rampsmooth_tilde_01_d_dspsetup() {
        this->rampsmooth_tilde_01_d_reset();
    }
    
    void rampsmooth_tilde_01_d_reset() {
        this->rampsmooth_tilde_01_d_prev = 0;
    }
    
    void rampsmooth_tilde_01_dspsetup(bool force) {
        if ((bool)(this->rampsmooth_tilde_01_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->rampsmooth_tilde_01_setupDone = true;
        this->rampsmooth_tilde_01_d_dspsetup();
    }
    
    void edge_01_dspsetup(bool force) {
        if ((bool)(this->edge_01_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->edge_01_setupDone = true;
    }
    
    void thresh_tilde_01_init() {
        this->thresh_tilde_01_isAbove = false;
    }
    
    number peakamp_01_d_next(number x) {
        number temp = (number)(x - this->peakamp_01_d_prev);
        this->peakamp_01_d_prev = x;
        return temp;
    }
    
    void peakamp_01_d_dspsetup() {
        this->peakamp_01_d_reset();
    }
    
    void peakamp_01_d_reset() {
        this->peakamp_01_d_prev = 0;
    }
    
    void peakamp_01_dspsetup(bool force) {
        if ((bool)(this->peakamp_01_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->peakamp_01_index = 0;
        this->peakamp_01_lastMaximum = 0;
        this->peakamp_01_lastOutput = 0;
        this->peakamp_01_setupDone = true;
        this->peakamp_01_d_dspsetup();
    }
    
    number peakamp_02_d_next(number x) {
        number temp = (number)(x - this->peakamp_02_d_prev);
        this->peakamp_02_d_prev = x;
        return temp;
    }
    
    void peakamp_02_d_dspsetup() {
        this->peakamp_02_d_reset();
    }
    
    void peakamp_02_d_reset() {
        this->peakamp_02_d_prev = 0;
    }
    
    void peakamp_02_dspsetup(bool force) {
        if ((bool)(this->peakamp_02_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->peakamp_02_index = 0;
        this->peakamp_02_lastMaximum = 0;
        this->peakamp_02_lastOutput = 0;
        this->peakamp_02_setupDone = true;
        this->peakamp_02_d_dspsetup();
    }
    
    number average_absolute_tilde_01_av_next(number x, Int windowSize, bool reset) {
        if (windowSize > 0)
            this->average_absolute_tilde_01_av_setwindowsize(windowSize);
    
        if (reset != 0) {
            if (this->average_absolute_tilde_01_av_resetFlag != 1) {
                this->average_absolute_tilde_01_av_wantsReset = 1;
                this->average_absolute_tilde_01_av_resetFlag = 1;
            }
        } else {
            this->average_absolute_tilde_01_av_resetFlag = 0;
        }
    
        if (this->average_absolute_tilde_01_av_wantsReset == 1) {
            this->average_absolute_tilde_01_av_doReset();
        }
    
        this->average_absolute_tilde_01_av_accum += x;
        this->average_absolute_tilde_01_av_buffer[(Index)this->average_absolute_tilde_01_av_bufferPos] = x;
        number bufferSize = this->average_absolute_tilde_01_av_buffer->getSize();
    
        if (this->average_absolute_tilde_01_av_effectiveWindowSize < this->average_absolute_tilde_01_av_currentWindowSize) {
            this->average_absolute_tilde_01_av_effectiveWindowSize++;
        } else {
            number bufferReadPos = this->average_absolute_tilde_01_av_bufferPos - this->average_absolute_tilde_01_av_effectiveWindowSize;
    
            while (bufferReadPos < 0)
                bufferReadPos += bufferSize;
    
            this->average_absolute_tilde_01_av_accum -= this->average_absolute_tilde_01_av_buffer[(Index)bufferReadPos];
        }
    
        this->average_absolute_tilde_01_av_bufferPos++;
    
        if (this->average_absolute_tilde_01_av_bufferPos >= bufferSize) {
            this->average_absolute_tilde_01_av_bufferPos -= bufferSize;
        }
    
        return this->average_absolute_tilde_01_av_accum / this->average_absolute_tilde_01_av_effectiveWindowSize;
    }
    
    void average_absolute_tilde_01_av_setwindowsize(Int wsize) {
        wsize = trunc(wsize);
    
        if (wsize != this->average_absolute_tilde_01_av_currentWindowSize && wsize > 0 && wsize <= this->sr) {
            this->average_absolute_tilde_01_av_currentWindowSize = wsize;
            this->average_absolute_tilde_01_av_wantsReset = 1;
        }
    }
    
    void average_absolute_tilde_01_av_reset() {
        this->average_absolute_tilde_01_av_wantsReset = 1;
    }
    
    void average_absolute_tilde_01_av_dspsetup() {
        this->average_absolute_tilde_01_av_wantsReset = 1;
    
        if (this->sr > this->average_absolute_tilde_01_av_buffer->getSize()) {
            this->average_absolute_tilde_01_av_buffer->setSize(this->sr + 1);
            updateDataRef(this, this->average_absolute_tilde_01_av_buffer);
        }
    }
    
    void average_absolute_tilde_01_av_doReset() {
        this->average_absolute_tilde_01_av_accum = 0;
        this->average_absolute_tilde_01_av_effectiveWindowSize = 0;
        this->average_absolute_tilde_01_av_bufferPos = 0;
        this->average_absolute_tilde_01_av_wantsReset = 0;
    }
    
    void average_absolute_tilde_01_av_init() {
        this->average_absolute_tilde_01_av_currentWindowSize = this->sr;
        this->average_absolute_tilde_01_av_buffer->requestSize(this->sr + 1, 1);
        this->average_absolute_tilde_01_av_doReset();
    }
    
    void average_absolute_tilde_01_dspsetup(bool force) {
        if ((bool)(this->average_absolute_tilde_01_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->average_absolute_tilde_01_setupDone = true;
        this->average_absolute_tilde_01_av_dspsetup();
    }
    
    void toggle_01_getPresetValue(PatcherStateInterface& preset) {
        preset["value"] = this->toggle_01_value_number;
    }
    
    void toggle_01_setPresetValue(PatcherStateInterface& preset) {
        if ((bool)(stateIsEmpty(preset)))
            return;
    
        this->toggle_01_value_number_set(preset["value"]);
    }
    
    void numberobj_02_init() {
        this->numberobj_02_currentFormat = 6;
    }
    
    void numberobj_02_getPresetValue(PatcherStateInterface& preset) {
        preset["value"] = this->numberobj_02_value;
    }
    
    void numberobj_02_setPresetValue(PatcherStateInterface& preset) {
        if ((bool)(stateIsEmpty(preset)))
            return;
    
        this->numberobj_02_value_set(preset["value"]);
    }
    
    void toggle_02_getPresetValue(PatcherStateInterface& preset) {
        preset["value"] = this->toggle_02_value_number;
    }
    
    void toggle_02_setPresetValue(PatcherStateInterface& preset) {
        if ((bool)(stateIsEmpty(preset)))
            return;
    
        this->toggle_02_value_number_set(preset["value"]);
    }
    
    void message_01_init() {
        this->message_01_set_set(listbase<number, 1>{1});
    }
    
    void numberobj_03_init() {
        this->numberobj_03_currentFormat = 6;
    }
    
    void numberobj_03_getPresetValue(PatcherStateInterface& preset) {
        preset["value"] = this->numberobj_03_value;
    }
    
    void numberobj_03_setPresetValue(PatcherStateInterface& preset) {
        if ((bool)(stateIsEmpty(preset)))
            return;
    
        this->numberobj_03_value_set(preset["value"]);
    }
    
    void numberobj_04_init() {
        this->numberobj_04_currentFormat = 6;
    }
    
    void numberobj_04_getPresetValue(PatcherStateInterface& preset) {
        preset["value"] = this->numberobj_04_value;
    }
    
    void numberobj_04_setPresetValue(PatcherStateInterface& preset) {
        if ((bool)(stateIsEmpty(preset)))
            return;
    
        this->numberobj_04_value_set(preset["value"]);
    }
    
    void numberobj_05_init() {
        this->numberobj_05_currentFormat = 6;
    }
    
    void numberobj_05_getPresetValue(PatcherStateInterface& preset) {
        preset["value"] = this->numberobj_05_value;
    }
    
    void numberobj_05_setPresetValue(PatcherStateInterface& preset) {
        if ((bool)(stateIsEmpty(preset)))
            return;
    
        this->numberobj_05_value_set(preset["value"]);
    }
    
    void numberobj_06_init() {
        this->numberobj_06_currentFormat = 6;
    }
    
    void numberobj_06_getPresetValue(PatcherStateInterface& preset) {
        preset["value"] = this->numberobj_06_value;
    }
    
    void numberobj_06_setPresetValue(PatcherStateInterface& preset) {
        if ((bool)(stateIsEmpty(preset)))
            return;
    
        this->numberobj_06_value_set(preset["value"]);
    }
    
    bool stackprotect_check() {
        this->stackprotect_count++;
    
        if (this->stackprotect_count > 128) {
            console->log("STACK OVERFLOW DETECTED - stopped processing branch !");
            return true;
        }
    
        return false;
    }
    
    Index getPatcherSerial() const {
        return 0;
    }
    
    void sendParameter(ParameterIndex index, bool ignoreValue) {
        this->getPatcher()->sendParameter(index + this->parameterOffset, ignoreValue);
    }
    
    void scheduleParamInit(ParameterIndex index, Index order) {
        this->getPatcher()->scheduleParamInit(index + this->parameterOffset, order);
    }
    
    void updateTime(MillisecondTime time, EXTERNALENGINE* engine, bool inProcess = false) {
        RNBO_UNUSED(inProcess);
        RNBO_UNUSED(engine);
        this->_currentTime = time;
        auto offset = rnbo_fround(this->msToSamps(time - this->getEngine()->getCurrentTime(), this->sr));
    
        if (offset >= (SampleIndex)(this->vs))
            offset = (SampleIndex)(this->vs) - 1;
    
        if (offset < 0)
            offset = 0;
    
        this->sampleOffsetIntoNextAudioBuffer = (Index)(offset);
    }
    
    void assign_defaults()
    {
        dspexpr_01_in1 = 0;
        dspexpr_01_in2 = 0;
        numberobj_01_value = 0;
        numberobj_01_value_setter(numberobj_01_value);
        dspexpr_02_in1 = 0;
        dspexpr_02_in2 = 0;
        rampsmooth_tilde_01_x = 0;
        rampsmooth_tilde_01_up = 480;
        rampsmooth_tilde_01_down = 480;
        thresh_tilde_01_x = 0;
        thresh_tilde_01_lowThresh = 0;
        thresh_tilde_01_highThresh = 1;
        dspexpr_03_in1 = 0;
        dspexpr_03_in2 = 100;
        peakamp_01_interval = 10;
        peakamp_01_output = 0;
        dspexpr_04_in1 = 0;
        dspexpr_04_in2 = -1;
        dspexpr_05_in1 = 0;
        dspexpr_05_in2 = 50;
        peakamp_02_interval = 5;
        peakamp_02_output = 0;
        average_absolute_tilde_01_x = 0;
        average_absolute_tilde_01_windowSize = 22000;
        average_absolute_tilde_01_reset = 0;
        toggle_01_value_number = 0;
        toggle_01_value_number_setter(toggle_01_value_number);
        expr_01_in1 = 0;
        expr_01_in2 = 0;
        expr_01_out1 = 0;
        numberobj_02_value = 0;
        numberobj_02_value_setter(numberobj_02_value);
        scale_01_inlow = 0;
        scale_01_inhigh = 100;
        scale_01_outlow = 100;
        scale_01_outhigh = 0;
        scale_01_power = 0.5;
        toggle_02_value_number = 0;
        toggle_02_value_number_setter(toggle_02_value_number);
        select_01_test1 = 1;
        expr_02_in1 = 0;
        expr_02_in2 = 1000;
        expr_02_out1 = 0;
        numberobj_03_value = 0;
        numberobj_03_value_setter(numberobj_03_value);
        numberobj_04_value = 0;
        numberobj_04_value_setter(numberobj_04_value);
        scale_02_inlow = 0;
        scale_02_inhigh = 100;
        scale_02_outlow = 1000;
        scale_02_outhigh = 0;
        scale_02_power = 0.5;
        delay_01_time = 500;
        delay_01_delayall = 1;
        numberobj_05_value = 0;
        numberobj_05_value_setter(numberobj_05_value);
        scale_03_inlow = 0;
        scale_03_inhigh = 100;
        scale_03_outlow = 0;
        scale_03_outhigh = 200;
        scale_03_power = 1;
        numberobj_06_value = 0;
        numberobj_06_value_setter(numberobj_06_value);
        _currentTime = 0;
        audioProcessSampleCount = 0;
        sampleOffsetIntoNextAudioBuffer = 0;
        zeroBuffer = nullptr;
        dummyBuffer = nullptr;
        signals[0] = nullptr;
        signals[1] = nullptr;
        signals[2] = nullptr;
        signals[3] = nullptr;
        didAllocateSignals = 0;
        vs = 0;
        maxvs = 0;
        sr = 44100;
        invsr = 0.000022675736961451248;
        numberobj_01_currentFormat = 6;
        numberobj_01_lastValue = 0;
        rampsmooth_tilde_01_prev = 0;
        rampsmooth_tilde_01_index = 0;
        rampsmooth_tilde_01_increment = 0;
        rampsmooth_tilde_01_d_prev = 0;
        rampsmooth_tilde_01_setupDone = false;
        timer_01_starttime = 0;
        timer_01_elapsed = 0;
        edge_01_setupDone = false;
        thresh_tilde_01_isAbove = false;
        peakamp_01_index = 0;
        peakamp_01_maxIndex = 0;
        peakamp_01_lastMaximum = 0;
        peakamp_01_lastOutput = 0;
        peakamp_01_d_prev = 0;
        peakamp_01_setupDone = false;
        peakamp_02_index = 0;
        peakamp_02_maxIndex = 0;
        peakamp_02_lastMaximum = 0;
        peakamp_02_lastOutput = 0;
        peakamp_02_d_prev = 0;
        peakamp_02_setupDone = false;
        average_absolute_tilde_01_av_currentWindowSize = 44100;
        average_absolute_tilde_01_av_accum = 0;
        average_absolute_tilde_01_av_effectiveWindowSize = 0;
        average_absolute_tilde_01_av_bufferPos = 0;
        average_absolute_tilde_01_av_wantsReset = false;
        average_absolute_tilde_01_av_resetFlag = false;
        average_absolute_tilde_01_setupDone = false;
        toggle_01_lastValue = 0;
        numberobj_02_currentFormat = 6;
        numberobj_02_lastValue = 0;
        toggle_02_lastValue = 0;
        numberobj_03_currentFormat = 6;
        numberobj_03_lastValue = 0;
        numberobj_04_currentFormat = 6;
        numberobj_04_lastValue = 0;
        numberobj_05_currentFormat = 6;
        numberobj_05_lastValue = 0;
        numberobj_06_currentFormat = 6;
        numberobj_06_lastValue = 0;
        stackprotect_count = 0;
        _voiceIndex = 0;
        _noteNumber = 0;
        isMuted = 1;
        parameterOffset = 0;
    }
    
        // data ref strings
        struct DataRefStrings {
        	static constexpr auto& name0 = "average_absolute_tilde_01_av_bufferobj";
        	static constexpr auto& file0 = "";
        	static constexpr auto& tag0 = "buffer~";
        	DataRefStrings* operator->() { return this; }
        	const DataRefStrings* operator->() const { return this; }
        };
    
        DataRefStrings dataRefStrings;
    
    // member variables
    
        number dspexpr_01_in1;
        number dspexpr_01_in2;
        number numberobj_01_value;
        number dspexpr_02_in1;
        number dspexpr_02_in2;
        number rampsmooth_tilde_01_x;
        number rampsmooth_tilde_01_up;
        number rampsmooth_tilde_01_down;
        number thresh_tilde_01_x;
        number thresh_tilde_01_lowThresh;
        number thresh_tilde_01_highThresh;
        number dspexpr_03_in1;
        number dspexpr_03_in2;
        number peakamp_01_interval;
        number peakamp_01_output;
        number dspexpr_04_in1;
        number dspexpr_04_in2;
        number dspexpr_05_in1;
        number dspexpr_05_in2;
        number peakamp_02_interval;
        number peakamp_02_output;
        number average_absolute_tilde_01_x;
        number average_absolute_tilde_01_windowSize;
        number average_absolute_tilde_01_reset;
        number toggle_01_value_number;
        number expr_01_in1;
        number expr_01_in2;
        number expr_01_out1;
        number numberobj_02_value;
        list scale_01_input;
        number scale_01_inlow;
        number scale_01_inhigh;
        number scale_01_outlow;
        number scale_01_outhigh;
        number scale_01_power;
        list scale_01_out;
        number toggle_02_value_number;
        number select_01_test1;
        number expr_02_in1;
        number expr_02_in2;
        number expr_02_out1;
        list message_01_set;
        number numberobj_03_value;
        number numberobj_04_value;
        list scale_02_input;
        number scale_02_inlow;
        number scale_02_inhigh;
        number scale_02_outlow;
        number scale_02_outhigh;
        number scale_02_power;
        list scale_02_out;
        number delay_01_time;
        number delay_01_delayall;
        number numberobj_05_value;
        list scale_03_input;
        number scale_03_inlow;
        number scale_03_inhigh;
        number scale_03_outlow;
        number scale_03_outhigh;
        number scale_03_power;
        list scale_03_out;
        number numberobj_06_value;
        MillisecondTime _currentTime;
        UInt64 audioProcessSampleCount;
        Index sampleOffsetIntoNextAudioBuffer;
        signal zeroBuffer;
        signal dummyBuffer;
        SampleValue * signals[4];
        bool didAllocateSignals;
        Index vs;
        Index maxvs;
        number sr;
        number invsr;
        Int numberobj_01_currentFormat;
        number numberobj_01_lastValue;
        number rampsmooth_tilde_01_prev;
        number rampsmooth_tilde_01_index;
        number rampsmooth_tilde_01_increment;
        number rampsmooth_tilde_01_d_prev;
        bool rampsmooth_tilde_01_setupDone;
        MillisecondTime timer_01_starttime;
        MillisecondTime timer_01_elapsed;
        number edge_01_currentState;
        bool edge_01_setupDone;
        bool thresh_tilde_01_isAbove;
        Int peakamp_01_index;
        Int peakamp_01_maxIndex;
        number peakamp_01_lastMaximum;
        number peakamp_01_lastOutput;
        number peakamp_01_d_prev;
        bool peakamp_01_setupDone;
        Int peakamp_02_index;
        Int peakamp_02_maxIndex;
        number peakamp_02_lastMaximum;
        number peakamp_02_lastOutput;
        number peakamp_02_d_prev;
        bool peakamp_02_setupDone;
        Int average_absolute_tilde_01_av_currentWindowSize;
        number average_absolute_tilde_01_av_accum;
        Int average_absolute_tilde_01_av_effectiveWindowSize;
        Int average_absolute_tilde_01_av_bufferPos;
        bool average_absolute_tilde_01_av_wantsReset;
        bool average_absolute_tilde_01_av_resetFlag;
        Float64BufferRef average_absolute_tilde_01_av_buffer;
        bool average_absolute_tilde_01_setupDone;
        number toggle_01_lastValue;
        Int numberobj_02_currentFormat;
        number numberobj_02_lastValue;
        number toggle_02_lastValue;
        Int numberobj_03_currentFormat;
        number numberobj_03_lastValue;
        Int numberobj_04_currentFormat;
        number numberobj_04_lastValue;
        Int numberobj_05_currentFormat;
        number numberobj_05_lastValue;
        Int numberobj_06_currentFormat;
        number numberobj_06_lastValue;
        number stackprotect_count;
        DataRef average_absolute_tilde_01_av_bufferobj;
        Index _voiceIndex;
        Int _noteNumber;
        Index isMuted;
        ParameterIndex parameterOffset;
        bool _isInitialized = false;
};

class RNBOSubpatcher_245 : public PatcherInterfaceImpl {
    
    friend class rnbomatic;
    
    public:
    
    RNBOSubpatcher_245()
    {}
    
    ~RNBOSubpatcher_245()
    {
        deallocateSignals();
    }
    
    Index getNumMidiInputPorts() const {
        return 0;
    }
    
    void processMidiEvent(MillisecondTime , int , ConstByteArray , Index ) {}
    
    Index getNumMidiOutputPorts() const {
        return 0;
    }
    
    void process(
        const SampleValue * const* inputs,
        Index numInputs,
        SampleValue * const* outputs,
        Index numOutputs,
        Index n
    ) {
        this->vs = n;
        this->updateTime(this->getEngine()->getCurrentTime(), (ENGINE*)nullptr, true);
        SampleValue * out1 = (numOutputs >= 1 && outputs[0] ? outputs[0] : this->dummyBuffer);
        SampleValue * out2 = (numOutputs >= 2 && outputs[1] ? outputs[1] : this->dummyBuffer);
        const SampleValue * in1 = (numInputs >= 1 && inputs[0] ? inputs[0] : this->zeroBuffer);
    
        if (this->getIsMuted())
            return;
    
        this->dspexpr_09_perform(in1, this->dspexpr_09_in2, this->signals[0], n);
        this->rand_tilde_01_perform(this->rand_tilde_01_frequency, this->signals[1], n);
    
        this->cycle_tilde_01_perform(
            this->signals[1],
            this->cycle_tilde_01_phase_offset,
            this->signals[2],
            this->dummyBuffer,
            n
        );
    
        this->dspexpr_12_perform(this->signals[2], this->dspexpr_12_in2, this->signals[1], n);
    
        this->rampsmooth_tilde_04_perform(
            this->signals[1],
            this->rampsmooth_tilde_04_up,
            this->rampsmooth_tilde_04_down,
            this->signals[2],
            n
        );
    
        this->dspexpr_08_perform(this->signals[0], this->signals[2], this->signals[1], n);
    
        this->wrap_tilde_01_perform(
            this->signals[1],
            this->wrap_tilde_01_low,
            this->wrap_tilde_01_high,
            this->signals[2],
            n
        );
    
        this->sample_01_perform(
            this->signals[2],
            this->sample_01_offset,
            this->signals[1],
            this->signals[0],
            this->dummyBuffer,
            n
        );
    
        this->hann_tilde_01_perform(this->signals[2], this->signals[3], n);
        this->dspexpr_07_perform(this->signals[1], this->signals[3], this->signals[2], n);
        this->dspexpr_11_perform(this->signals[0], this->signals[3], this->signals[1], n);
        this->rand_tilde_02_perform(this->rand_tilde_02_frequency, this->signals[3], n);
    
        this->cycle_tilde_02_perform(
            this->signals[3],
            this->cycle_tilde_02_phase_offset,
            this->signals[0],
            this->dummyBuffer,
            n
        );
    
        this->dspexpr_15_perform(this->signals[0], this->dspexpr_15_in2, this->signals[3], n);
        this->dspexpr_14_perform(this->signals[3], this->dspexpr_14_in2, this->signals[0], n);
        this->dspexpr_13_perform(this->signals[0], this->dspexpr_13_in2, this->signals[3], n);
    
        this->rampsmooth_tilde_02_perform(
            this->signals[3],
            this->rampsmooth_tilde_02_up,
            this->rampsmooth_tilde_02_down,
            this->signals[0],
            n
        );
    
        this->dspexpr_06_perform(this->signals[2], this->signals[0], this->signals[3], n);
        this->dspexpr_10_perform(this->signals[1], this->signals[0], this->signals[2], n);
        this->rand_tilde_03_perform(this->rand_tilde_03_frequency, this->signals[0], n);
    
        this->cycle_tilde_03_perform(
            this->signals[0],
            this->cycle_tilde_03_phase_offset,
            this->signals[1],
            this->dummyBuffer,
            n
        );
    
        this->dspexpr_18_perform(this->signals[1], this->dspexpr_18_in2, this->signals[0], n);
        this->dspexpr_17_perform(this->signals[0], this->dspexpr_17_in2, this->signals[1], n);
        this->dspexpr_16_perform(this->signals[1], this->dspexpr_16_in2, this->signals[0], n);
    
        this->rampsmooth_tilde_03_perform(
            this->signals[0],
            this->rampsmooth_tilde_03_up,
            this->rampsmooth_tilde_03_down,
            this->signals[1],
            n
        );
    
        this->pan_tilde_01_perform(
            this->signals[1],
            this->signals[3],
            this->signals[2],
            this->signals[0],
            this->signals[4],
            n
        );
    
        this->signaladder_03_perform(this->signals[0], out1, out1, n);
        this->signaladder_04_perform(this->signals[4], out2, out2, n);
        this->stackprotect_perform(n);
        this->audioProcessSampleCount += this->vs;
    }
    
    void prepareToProcess(number sampleRate, Index maxBlockSize, bool force) {
        RNBO_ASSERT(this->_isInitialized);
    
        if (this->maxvs < maxBlockSize || !this->didAllocateSignals) {
            Index i;
    
            for (i = 0; i < 5; i++) {
                this->signals[i] = resizeSignal(this->signals[i], this->maxvs, maxBlockSize);
            }
    
            this->zeroBuffer = resizeSignal(this->zeroBuffer, this->maxvs, maxBlockSize);
            this->dummyBuffer = resizeSignal(this->dummyBuffer, this->maxvs, maxBlockSize);
            this->didAllocateSignals = true;
        }
    
        const bool sampleRateChanged = sampleRate != this->sr;
        const bool maxvsChanged = maxBlockSize != this->maxvs;
        const bool forceDSPSetup = sampleRateChanged || maxvsChanged || force;
    
        if (sampleRateChanged || maxvsChanged) {
            this->vs = maxBlockSize;
            this->maxvs = maxBlockSize;
            this->sr = sampleRate;
            this->invsr = 1 / sampleRate;
        }
    
        this->rand_tilde_01_dspsetup(forceDSPSetup);
        this->cycle_tilde_01_dspsetup(forceDSPSetup);
        this->rampsmooth_tilde_04_dspsetup(forceDSPSetup);
        this->rand_tilde_02_dspsetup(forceDSPSetup);
        this->cycle_tilde_02_dspsetup(forceDSPSetup);
        this->rampsmooth_tilde_02_dspsetup(forceDSPSetup);
        this->rand_tilde_03_dspsetup(forceDSPSetup);
        this->cycle_tilde_03_dspsetup(forceDSPSetup);
        this->rampsmooth_tilde_03_dspsetup(forceDSPSetup);
    
        if (sampleRateChanged)
            this->onSampleRateChanged(sampleRate);
    }
    
    number msToSamps(MillisecondTime ms, number sampleRate) {
        return ms * sampleRate * 0.001;
    }
    
    MillisecondTime sampsToMs(SampleIndex samps) {
        return samps * (this->invsr * 1000);
    }
    
    Index getNumInputChannels() const {
        return 1;
    }
    
    Index getNumOutputChannels() const {
        return 2;
    }
    
    void getPreset(PatcherStateInterface& ) {}
    
    void setPreset(MillisecondTime , PatcherStateInterface& ) {}
    
    void setParameterValue(ParameterIndex , ParameterValue , MillisecondTime ) {}
    
    void processParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
        this->setParameterValue(index, value, time);
    }
    
    void processParameterBangEvent(ParameterIndex index, MillisecondTime time) {
        this->setParameterValue(index, this->getParameterValue(index), time);
    }
    
    void processNormalizedParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
        this->setParameterValueNormalized(index, value, time);
    }
    
    ParameterValue getParameterValue(ParameterIndex index)  {
        switch (index) {
        default:
            {
            return 0;
            }
        }
    }
    
    ParameterIndex getNumSignalInParameters() const {
        return 0;
    }
    
    ParameterIndex getNumSignalOutParameters() const {
        return 0;
    }
    
    ParameterIndex getNumParameters() const {
        return 0;
    }
    
    ConstCharPointer getParameterName(ParameterIndex index) const {
        switch (index) {
        default:
            {
            return "bogus";
            }
        }
    }
    
    ConstCharPointer getParameterId(ParameterIndex index) const {
        switch (index) {
        default:
            {
            return "bogus";
            }
        }
    }
    
    void getParameterInfo(ParameterIndex , ParameterInfo * ) const {}
    
    ParameterValue applyStepsToNormalizedParameterValue(ParameterValue normalizedValue, int steps) const {
        if (steps == 1) {
            if (normalizedValue > 0) {
                normalizedValue = 1.;
            }
        } else {
            ParameterValue oneStep = (number)1. / (steps - 1);
            ParameterValue numberOfSteps = rnbo_fround(normalizedValue / oneStep * 1 / (number)1) * (number)1;
            normalizedValue = numberOfSteps * oneStep;
        }
    
        return normalizedValue;
    }
    
    ParameterValue convertToNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
        switch (index) {
        default:
            {
            return value;
            }
        }
    }
    
    ParameterValue convertFromNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
        value = (value < 0 ? 0 : (value > 1 ? 1 : value));
    
        switch (index) {
        default:
            {
            return value;
            }
        }
    }
    
    ParameterValue constrainParameterValue(ParameterIndex index, ParameterValue value) const {
        switch (index) {
        default:
            {
            return value;
            }
        }
    }
    
    void processNumMessage(MessageTag , MessageTag , MillisecondTime , number ) {}
    
    void processListMessage(MessageTag , MessageTag , MillisecondTime , const list& ) {}
    
    void processBangMessage(MessageTag tag, MessageTag objectId, MillisecondTime time) {
        this->updateTime(time, (ENGINE*)nullptr);
    
        switch (tag) {
        case TAG("startupbang"):
            {
            if (TAG("Freezer-Voice~/loadbang_obj-8") == objectId)
                this->loadbang_01_startupbang_bang();
    
            break;
            }
        }
    }
    
    MessageTagInfo resolveTag(MessageTag tag) const {
        switch (tag) {
        case TAG("startupbang"):
            {
            return "startupbang";
            }
        case TAG("Freezer-Voice~/loadbang_obj-8"):
            {
            return "Freezer-Voice~/loadbang_obj-8";
            }
        }
    
        return nullptr;
    }
    
    DataRef* getDataRef(DataRefIndex index)  {
        switch (index) {
        default:
            {
            return nullptr;
            }
        }
    }
    
    DataRefIndex getNumDataRefs() const {
        return 0;
    }
    
    void processDataViewUpdate(DataRefIndex index, MillisecondTime time) {
        this->updateTime(time, (ENGINE*)nullptr);
    
        if (index == 0) {
            this->sample_01_buffer = reInitDataView(this->sample_01_buffer, this->getPatcher()->FreezeA);
        }
    
        if (index == 1) {
            this->cycle_tilde_01_buffer = reInitDataView(this->cycle_tilde_01_buffer, this->getPatcher()->RNBODefaultSinus);
            this->cycle_tilde_02_buffer = reInitDataView(this->cycle_tilde_02_buffer, this->getPatcher()->RNBODefaultSinus);
            this->cycle_tilde_03_buffer = reInitDataView(this->cycle_tilde_03_buffer, this->getPatcher()->RNBODefaultSinus);
            this->cycle_tilde_01_bufferUpdated();
            this->cycle_tilde_02_bufferUpdated();
            this->cycle_tilde_03_bufferUpdated();
        }
    }
    
    void initialize() {
        RNBO_ASSERT(!this->_isInitialized);
        this->assign_defaults();
        this->applyState();
        this->sample_01_buffer = new Float64Buffer(this->getPatcher()->FreezeA);
        this->cycle_tilde_01_buffer = new SampleBuffer(this->getPatcher()->RNBODefaultSinus);
        this->cycle_tilde_02_buffer = new SampleBuffer(this->getPatcher()->RNBODefaultSinus);
        this->cycle_tilde_03_buffer = new SampleBuffer(this->getPatcher()->RNBODefaultSinus);
        this->_isInitialized = true;
    }
    
    protected:
    
    void updateTime(MillisecondTime time, INTERNALENGINE*, bool inProcess = false) {
    	if (time == TimeNow) time = getTopLevelPatcher()->getPatcherTime();
    	getTopLevelPatcher()->processInternalEvents(time);
    	updateTime(time, (EXTERNALENGINE*)nullptr);
    }
    
    RNBOSubpatcher_245* operator->() {
        return this;
    }
    const RNBOSubpatcher_245* operator->() const {
        return this;
    }
    virtual rnbomatic* getPatcher() const {
        return static_cast<rnbomatic *>(_parentPatcher);
    }
    
    rnbomatic* getTopLevelPatcher() {
        return this->getPatcher()->getTopLevelPatcher();
    }
    
    void cancelClockEvents()
    {
        getEngine()->flushClockEvents(this, -871642103, false);
    }
    
    inline number safemod(number f, number m) {
        if (m != 0) {
            Int f_trunc = (Int)(trunc(f));
            Int m_trunc = (Int)(trunc(m));
    
            if (f == f_trunc && m == m_trunc) {
                f = f_trunc % m_trunc;
            } else {
                if (m < 0) {
                    m = -m;
                }
    
                if (f >= m) {
                    if (f >= m * 2.0) {
                        number d = f / m;
                        Int i = (Int)(trunc(d));
                        d = d - i;
                        f = d * m;
                    } else {
                        f -= m;
                    }
                } else if (f <= -m) {
                    if (f <= -m * 2.0) {
                        number d = f / m;
                        Int i = (Int)(trunc(d));
                        d = d - i;
                        f = d * m;
                    } else {
                        f += m;
                    }
                }
            }
        } else {
            f = 0.0;
        }
    
        return f;
    }
    
    inline number safesqrt(number num) {
        return (num > 0.0 ? rnbo_sqrt(num) : 0.0);
    }
    
    number wrap(number x, number low, number high) {
        number lo;
        number hi;
    
        if (low == high)
            return low;
    
        if (low > high) {
            hi = low;
            lo = high;
        } else {
            lo = low;
            hi = high;
        }
    
        number range = hi - lo;
    
        if (x >= lo && x < hi)
            return x;
    
        if (range <= 0.000000001)
            return lo;
    
        Int numWraps = (Int)(trunc((x - lo) / range));
        numWraps = numWraps - ((x < lo ? 1 : 0));
        number result = x - range * numWraps;
    
        if (result >= hi)
            return result - range;
        else
            return result;
    }
    
    inline number linearinterp(number frac, number x, number y) {
        return x + (y - x) * frac;
    }
    
    inline number cubicinterp(number a, number w, number x, number y, number z) {
        number a1 = 1. + a;
        number aa = a * a1;
        number b = 1. - a;
        number b1 = 2. - a;
        number bb = b * b1;
        number fw = -.1666667 * bb * a;
        number fx = .5 * bb * a1;
        number fy = .5 * aa * b1;
        number fz = -.1666667 * aa * b;
        return w * fw + x * fx + y * fy + z * fz;
    }
    
    inline number fastcubicinterp(number a, number w, number x, number y, number z) {
        number a2 = a * a;
        number f0 = z - y - w + x;
        number f1 = w - x - f0;
        number f2 = y - w;
        number f3 = x;
        return f0 * a * a2 + f1 * a2 + f2 * a + f3;
    }
    
    inline number splineinterp(number a, number w, number x, number y, number z) {
        number a2 = a * a;
        number f0 = -0.5 * w + 1.5 * x - 1.5 * y + 0.5 * z;
        number f1 = w - 2.5 * x + 2 * y - 0.5 * z;
        number f2 = -0.5 * w + 0.5 * y;
        return f0 * a * a2 + f1 * a2 + f2 * a + x;
    }
    
    inline number spline6interp(number a, number y0, number y1, number y2, number y3, number y4, number y5) {
        number ym2py2 = y0 + y4;
        number ym1py1 = y1 + y3;
        number y2mym2 = y4 - y0;
        number y1mym1 = y3 - y1;
        number sixthym1py1 = (number)1 / (number)6.0 * ym1py1;
        number c0 = (number)1 / (number)120.0 * ym2py2 + (number)13 / (number)60.0 * ym1py1 + (number)11 / (number)20.0 * y2;
        number c1 = (number)1 / (number)24.0 * y2mym2 + (number)5 / (number)12.0 * y1mym1;
        number c2 = (number)1 / (number)12.0 * ym2py2 + sixthym1py1 - (number)1 / (number)2.0 * y2;
        number c3 = (number)1 / (number)12.0 * y2mym2 - (number)1 / (number)6.0 * y1mym1;
        number c4 = (number)1 / (number)24.0 * ym2py2 - sixthym1py1 + (number)1 / (number)4.0 * y2;
        number c5 = (number)1 / (number)120.0 * (y5 - y0) + (number)1 / (number)24.0 * (y1 - y4) + (number)1 / (number)12.0 * (y3 - y2);
        return ((((c5 * a + c4) * a + c3) * a + c2) * a + c1) * a + c0;
    }
    
    inline number cosT8(number r) {
        number t84 = 56.0;
        number t83 = 1680.0;
        number t82 = 20160.0;
        number t81 = 2.4801587302e-05;
        number t73 = 42.0;
        number t72 = 840.0;
        number t71 = 1.9841269841e-04;
    
        if (r < 0.785398163397448309615660845819875721 && r > -0.785398163397448309615660845819875721) {
            number rr = r * r;
            return 1.0 - rr * t81 * (t82 - rr * (t83 - rr * (t84 - rr)));
        } else if (r > 0.0) {
            r -= 1.57079632679489661923132169163975144;
            number rr = r * r;
            return -r * (1.0 - t71 * rr * (t72 - rr * (t73 - rr)));
        } else {
            r += 1.57079632679489661923132169163975144;
            number rr = r * r;
            return r * (1.0 - t71 * rr * (t72 - rr * (t73 - rr)));
        }
    }
    
    inline number cosineinterp(number frac, number x, number y) {
        number a2 = (1.0 - this->cosT8(frac * 3.14159265358979323846)) / (number)2.0;
        return x * (1.0 - a2) + y * a2;
    }
    
    template<typename BUFFERTYPE> array<SampleValue, 2 + 1> sample_boundmode_ignore_channelmode_ignore_indexmode_phase_interp_linear_channels_2(BUFFERTYPE& buffer, SampleValue sampleIndex, Index channelOffset) {
        number bufferSize = buffer->getSize();
        const Index bufferChannels = (const Index)(buffer->getChannels());
        constexpr int ___N3 = 2 + 1;
        array<SampleValue, ___N3> out = FIXEDSIZEARRAYINIT(2 + 1);
    
        {
            SampleValue bufferphasetoindex_result;
    
            {
                auto __end = bufferSize;
                auto __start = 0;
                auto __phase = sampleIndex;
                number size;
    
                {
                    size = __end - __start - 1;
                }
    
                {
                    bufferphasetoindex_result = __phase * size;
                }
            }
    
            sampleIndex = bufferphasetoindex_result;
        }
    
        if (bufferSize == 0 || (5 == 5 && (sampleIndex < 0 || sampleIndex >= bufferSize))) {
            for (Index i = 0; i < 2 + 1; i++) {
                out[(Index)i] = 0;
            }
        } else {
            for (Index channel = 0; channel < 2; channel++) {
                Index channelIndex = (Index)(channel + channelOffset);
    
                {
                    if (channelIndex >= bufferChannels || channelIndex < 0) {
                        out[(Index)channel] = 0;
                        continue;
                    }
                }
    
                SampleValue bufferreadsample_result;
    
                {
                    auto& __buffer = buffer;
    
                    if (sampleIndex < 0.0) {
                        bufferreadsample_result = 0.0;
                    }
    
                    SampleIndex truncIndex = (SampleIndex)(trunc(sampleIndex));
    
                    {
                        number frac = sampleIndex - truncIndex;
                        number wrap = bufferSize - 1;
    
                        {
                            SampleIndex index1 = (SampleIndex)(truncIndex);
    
                            if (index1 < 0)
                                index1 = wrap;
    
                            SampleIndex index2 = (SampleIndex)(index1 + 1);
    
                            if (index2 > wrap)
                                index2 = 0;
    
                            bufferreadsample_result = this->linearinterp(
                                frac,
                                __buffer->getSample(channelIndex, index1),
                                __buffer->getSample(channelIndex, index2)
                            );
                        }
                    }
                }
    
                out[(Index)channel] = bufferreadsample_result;
            }
    
            out[2] = sampleIndex;
        }
    
        return out;
    }
    
    Int notenumber() {
        return this->_noteNumber;
    }
    
    Index voice() {
        return this->_voiceIndex;
    }
    
    number random(number low, number high) {
        number range = high - low;
        return globalrandom() * range + low;
    }
    
    MillisecondTime getPatcherTime() const {
        return this->_currentTime;
    }
    
    void loadbang_01_startupbang_bang() {
        this->loadbang_01_output_bang();
    }
    
    void deallocateSignals() {
        Index i;
    
        for (i = 0; i < 5; i++) {
            this->signals[i] = freeSignal(this->signals[i]);
        }
    
        this->zeroBuffer = freeSignal(this->zeroBuffer);
        this->dummyBuffer = freeSignal(this->dummyBuffer);
    }
    
    Index getMaxBlockSize() const {
        return this->maxvs;
    }
    
    number getSampleRate() const {
        return this->sr;
    }
    
    bool hasFixedVectorSize() const {
        return false;
    }
    
    void setProbingTarget(MessageTag ) {}
    
    void initializeObjects() {
        this->rand_tilde_01_n_init();
        this->rand_tilde_01_init();
        this->rand_tilde_02_n_init();
        this->rand_tilde_02_init();
        this->rand_tilde_03_n_init();
        this->rand_tilde_03_init();
    }
    
    void setVoiceIndex(Index index)  {
        this->_voiceIndex = index;
    }
    
    void setNoteNumber(Int noteNumber)  {
        this->_noteNumber = noteNumber;
    }
    
    Index getIsMuted()  {
        return this->isMuted;
    }
    
    void setIsMuted(Index v)  {
        this->isMuted = v;
    }
    
    void onSampleRateChanged(double ) {}
    
    void extractState(PatcherStateInterface& ) {}
    
    void applyState() {}
    
    ParameterValue getPolyParameterValue(RNBOSubpatcher_245* voices, ParameterIndex index)  {
        switch (index) {
        default:
            {
            return voices[0]->getParameterValue(index);
            }
        }
    }
    
    void setPolyParameterValue(
        RNBOSubpatcher_245* voices,
        ParameterIndex index,
        ParameterValue value,
        MillisecondTime time
    ) {
        switch (index) {
        default:
            {
            for (Index i = 0; i < 8; i++)
                voices[i]->setParameterValue(index, value, time);
            }
        }
    }
    
    void sendPolyParameter(ParameterIndex index, Index voiceIndex, bool ignoreValue) {
        this->getPatcher()->sendParameter(index + this->parameterOffset + voiceIndex - 1, ignoreValue);
    }
    
    void setParameterOffset(ParameterIndex offset) {
        this->parameterOffset = offset;
    }
    
    void processClockEvent(MillisecondTime time, ClockId index, bool hasValue, ParameterValue value) {
        RNBO_UNUSED(value);
        RNBO_UNUSED(hasValue);
        this->updateTime(time, (ENGINE*)nullptr);
    
        switch (index) {
        case -871642103:
            {
            this->loadbang_01_startupbang_bang();
            break;
            }
        }
    }
    
    void processOutletAtCurrentTime(EngineLink* , OutletIndex , ParameterValue ) {}
    
    void processOutletEvent(
        EngineLink* sender,
        OutletIndex index,
        ParameterValue value,
        MillisecondTime time
    ) {
        this->updateTime(time, (ENGINE*)nullptr);
        this->processOutletAtCurrentTime(sender, index, value);
    }
    
    void sendOutlet(OutletIndex index, ParameterValue value) {
        this->getEngine()->sendOutlet(this, index, value);
    }
    
    void startup() {
        this->updateTime(this->getEngine()->getCurrentTime(), (ENGINE*)nullptr);
        this->getEngine()->scheduleClockEvent(this, -871642103, 0 + this->_currentTime);;
    }
    
    void fillDataRef(DataRefIndex , DataRef& ) {}
    
    void allocateDataRefs() {
        this->cycle_tilde_01_buffer->requestSize(16384, 1);
        this->cycle_tilde_01_buffer->setSampleRate(this->sr);
        this->cycle_tilde_02_buffer->requestSize(16384, 1);
        this->cycle_tilde_02_buffer->setSampleRate(this->sr);
        this->cycle_tilde_03_buffer->requestSize(16384, 1);
        this->cycle_tilde_03_buffer->setSampleRate(this->sr);
        this->sample_01_buffer = this->sample_01_buffer->allocateIfNeeded();
        this->cycle_tilde_01_buffer = this->cycle_tilde_01_buffer->allocateIfNeeded();
        this->cycle_tilde_02_buffer = this->cycle_tilde_02_buffer->allocateIfNeeded();
        this->cycle_tilde_03_buffer = this->cycle_tilde_03_buffer->allocateIfNeeded();
    }
    
    void dspexpr_12_in2_set(number v) {
        this->dspexpr_12_in2 = v;
    }
    
    void receive_04_output_number_set(number v) {
        this->receive_04_output_number = v;
        this->dspexpr_12_in2_set(v);
    }
    
    void voice_01_noteNumber_set(number ) {}
    
    void dspexpr_09_in2_set(number v) {
        this->dspexpr_09_in2 = v;
    }
    
    void expr_03_out1_set(number v) {
        this->expr_03_out1 = v;
        this->dspexpr_09_in2_set(this->expr_03_out1);
    }
    
    void expr_03_in1_set(number in1) {
        this->expr_03_in1 = in1;
    
        this->expr_03_out1_set(
            (this->expr_03_in2 == 0 ? 0 : (this->expr_03_in2 == 0. ? 0. : this->expr_03_in1 / this->expr_03_in2))
        );//#map:Freezer-Voice~//_obj-7:1
    }
    
    void expr_04_out1_set(number v) {
        this->expr_04_out1 = v;
        this->expr_03_in1_set(this->expr_04_out1);
    }
    
    void expr_04_in1_set(number in1) {
        this->expr_04_in1 = in1;
        this->expr_04_out1_set(this->expr_04_in1 - this->expr_04_in2);//#map:Freezer-Voice~/-_obj-6:1
    }
    
    void voice_01_voicenumber_set(number v) {
        this->expr_04_in1_set(v);
    }
    
    void voice_01_voicebang_bang() {
        this->voice_01_noteNumber_set(this->notenumber());
        this->voice_01_voicenumber_set(this->voice());
    }
    
    void loadbang_01_output_bang() {
        this->voice_01_voicebang_bang();
    }
    
    void midiouthelper_midiout_set(number ) {}
    
    void dspexpr_09_perform(const Sample * in1, number in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] + in2;//#map:_###_obj_###_:1
        }
    }
    
    void rand_tilde_01_perform(number frequency, SampleValue * out1, Index n) {
        auto __rand_tilde_01_lastValue = this->rand_tilde_01_lastValue;
        auto __rand_tilde_01_oldTargetValue = this->rand_tilde_01_oldTargetValue;
        auto __rand_tilde_01_targetValue = this->rand_tilde_01_targetValue;
        number f = (frequency > 0.0001 ? frequency : 0.0001);
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            number phasorValue = this->rand_tilde_01_p_next(f, -1);
    
            if (this->rand_tilde_01_d_next(phasorValue) < 0) {
                __rand_tilde_01_targetValue = this->rand_tilde_01_n_next();
                __rand_tilde_01_oldTargetValue = __rand_tilde_01_lastValue;
            }
    
            __rand_tilde_01_lastValue = __rand_tilde_01_oldTargetValue + phasorValue * (__rand_tilde_01_targetValue - __rand_tilde_01_oldTargetValue);
            out1[(Index)i] = __rand_tilde_01_lastValue;
        }
    
        this->rand_tilde_01_targetValue = __rand_tilde_01_targetValue;
        this->rand_tilde_01_oldTargetValue = __rand_tilde_01_oldTargetValue;
        this->rand_tilde_01_lastValue = __rand_tilde_01_lastValue;
    }
    
    void cycle_tilde_01_perform(
        const Sample * frequency,
        number phase_offset,
        SampleValue * out1,
        SampleValue * out2,
        Index n
    ) {
        RNBO_UNUSED(phase_offset);
        auto __cycle_tilde_01_buffer = this->cycle_tilde_01_buffer;
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            {
                UInt32 uint_phase;
    
                {
                    {
                        uint_phase = uint32_trunc(frequency[(Index)i] * 4294967296.0);
                    }
                }
    
                UInt32 idx = (UInt32)(uint32_rshift(uint_phase, 18));
                number frac = ((BinOpInt)((BinOpInt)uint_phase & (BinOpInt)262143)) * 3.81471181759574e-6;
                number y0 = __cycle_tilde_01_buffer[(Index)idx];
                number y1 = __cycle_tilde_01_buffer[(Index)((BinOpInt)(idx + 1) & (BinOpInt)16383)];
                number y = y0 + frac * (y1 - y0);
                out1[(Index)i] = y;
                out2[(Index)i] = uint_phase * 0.232830643653869629e-9;
                continue;
            }
        }
    }
    
    void dspexpr_12_perform(const Sample * in1, number in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] * in2;//#map:_###_obj_###_:1
        }
    }
    
    void rampsmooth_tilde_04_perform(const Sample * x, number up, number down, SampleValue * out1, Index n) {
        RNBO_UNUSED(down);
        RNBO_UNUSED(up);
        auto __rampsmooth_tilde_04_increment = this->rampsmooth_tilde_04_increment;
        auto __rampsmooth_tilde_04_index = this->rampsmooth_tilde_04_index;
        auto __rampsmooth_tilde_04_prev = this->rampsmooth_tilde_04_prev;
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            if (this->rampsmooth_tilde_04_d_next(x[(Index)i]) != 0.) {
                if (x[(Index)i] > __rampsmooth_tilde_04_prev) {
                    number _up = 480;
    
                    if (_up < 1)
                        _up = 1;
    
                    __rampsmooth_tilde_04_index = _up;
                    __rampsmooth_tilde_04_increment = (x[(Index)i] - __rampsmooth_tilde_04_prev) / _up;
                } else if (x[(Index)i] < __rampsmooth_tilde_04_prev) {
                    number _down = 480;
    
                    if (_down < 1)
                        _down = 1;
    
                    __rampsmooth_tilde_04_index = _down;
                    __rampsmooth_tilde_04_increment = (x[(Index)i] - __rampsmooth_tilde_04_prev) / _down;
                }
            }
    
            if (__rampsmooth_tilde_04_index > 0) {
                __rampsmooth_tilde_04_prev += __rampsmooth_tilde_04_increment;
                __rampsmooth_tilde_04_index -= 1;
            } else {
                __rampsmooth_tilde_04_prev = x[(Index)i];
            }
    
            out1[(Index)i] = __rampsmooth_tilde_04_prev;
        }
    
        this->rampsmooth_tilde_04_prev = __rampsmooth_tilde_04_prev;
        this->rampsmooth_tilde_04_index = __rampsmooth_tilde_04_index;
        this->rampsmooth_tilde_04_increment = __rampsmooth_tilde_04_increment;
    }
    
    void dspexpr_08_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] + in2[(Index)i];//#map:_###_obj_###_:1
        }
    }
    
    void wrap_tilde_01_perform(const Sample * x, number low, number high, SampleValue * out1, Index n) {
        RNBO_UNUSED(high);
        RNBO_UNUSED(low);
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            number lo;
            number hi;
            lo = 0;
            hi = 1;
            number range = hi - lo;
    
            if (x[(Index)i] >= lo && x[(Index)i] < hi) {
                out1[(Index)i] = x[(Index)i];
                continue;
            }
    
            if (range <= 0.000000001) {
                out1[(Index)i] = lo;
                continue;
            }
    
            Int numWraps = (Int)(trunc((x[(Index)i] - lo) / range));
            numWraps = numWraps - ((x[(Index)i] < lo ? 1 : 0));
            number result = x[(Index)i] - range * numWraps;
    
            if (result >= hi) {
                out1[(Index)i] = result - range;
                continue;
            } else {
                out1[(Index)i] = result;
                continue;
            }
        }
    }
    
    void sample_01_perform(
        const Sample * value,
        number offset,
        SampleValue * out1,
        SampleValue * out2,
        SampleValue * outindex,
        Index n
    ) {
        RNBO_UNUSED(out2);
        RNBO_UNUSED(out1);
        RNBO_UNUSED(offset);
        auto __sample_01_buffer = this->sample_01_buffer;
        SampleArray<2> out = {out1, out2};
    
        for (Index i = 0; i < n; i++) {
            auto result = this->sample_boundmode_ignore_channelmode_ignore_indexmode_phase_interp_linear_channels_2(__sample_01_buffer, value[(Index)i], 0);
    
            for (Index j = 0; j < 2; j++) {
                out[(Index)j][(Index)i] = result[(Index)j];
            }
    
            outindex[(Index)i] = result[2];
        }
    }
    
    void hann_tilde_01_perform(const Sample * x, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            number __x = x[(Index)i];
            __x = (__x > 1 ? 1 : (__x < 0 ? 0 : __x));
            out1[(Index)i] = 0.5 * (1 - rnbo_cos(6.28318530717958647692 * __x));
        }
    }
    
    void dspexpr_07_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] * in2[(Index)i];//#map:_###_obj_###_:1
        }
    }
    
    void dspexpr_11_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] * in2[(Index)i];//#map:_###_obj_###_:1
        }
    }
    
    void rand_tilde_02_perform(number frequency, SampleValue * out1, Index n) {
        auto __rand_tilde_02_lastValue = this->rand_tilde_02_lastValue;
        auto __rand_tilde_02_oldTargetValue = this->rand_tilde_02_oldTargetValue;
        auto __rand_tilde_02_targetValue = this->rand_tilde_02_targetValue;
        number f = (frequency > 0.0001 ? frequency : 0.0001);
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            number phasorValue = this->rand_tilde_02_p_next(f, -1);
    
            if (this->rand_tilde_02_d_next(phasorValue) < 0) {
                __rand_tilde_02_targetValue = this->rand_tilde_02_n_next();
                __rand_tilde_02_oldTargetValue = __rand_tilde_02_lastValue;
            }
    
            __rand_tilde_02_lastValue = __rand_tilde_02_oldTargetValue + phasorValue * (__rand_tilde_02_targetValue - __rand_tilde_02_oldTargetValue);
            out1[(Index)i] = __rand_tilde_02_lastValue;
        }
    
        this->rand_tilde_02_targetValue = __rand_tilde_02_targetValue;
        this->rand_tilde_02_oldTargetValue = __rand_tilde_02_oldTargetValue;
        this->rand_tilde_02_lastValue = __rand_tilde_02_lastValue;
    }
    
    void cycle_tilde_02_perform(
        const Sample * frequency,
        number phase_offset,
        SampleValue * out1,
        SampleValue * out2,
        Index n
    ) {
        RNBO_UNUSED(phase_offset);
        auto __cycle_tilde_02_buffer = this->cycle_tilde_02_buffer;
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            {
                UInt32 uint_phase;
    
                {
                    {
                        uint_phase = uint32_trunc(frequency[(Index)i] * 4294967296.0);
                    }
                }
    
                UInt32 idx = (UInt32)(uint32_rshift(uint_phase, 18));
                number frac = ((BinOpInt)((BinOpInt)uint_phase & (BinOpInt)262143)) * 3.81471181759574e-6;
                number y0 = __cycle_tilde_02_buffer[(Index)idx];
                number y1 = __cycle_tilde_02_buffer[(Index)((BinOpInt)(idx + 1) & (BinOpInt)16383)];
                number y = y0 + frac * (y1 - y0);
                out1[(Index)i] = y;
                out2[(Index)i] = uint_phase * 0.232830643653869629e-9;
                continue;
            }
        }
    }
    
    void dspexpr_15_perform(const Sample * in1, number in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] * in2;//#map:_###_obj_###_:1
        }
    }
    
    void dspexpr_14_perform(const Sample * in1, number in2, SampleValue * out1, Index n) {
        RNBO_UNUSED(in2);
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] + 1;//#map:_###_obj_###_:1
        }
    }
    
    void dspexpr_13_perform(const Sample * in1, number in2, SampleValue * out1, Index n) {
        RNBO_UNUSED(in2);
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] * 0.5;//#map:_###_obj_###_:1
        }
    }
    
    void rampsmooth_tilde_02_perform(const Sample * x, number up, number down, SampleValue * out1, Index n) {
        RNBO_UNUSED(down);
        RNBO_UNUSED(up);
        auto __rampsmooth_tilde_02_increment = this->rampsmooth_tilde_02_increment;
        auto __rampsmooth_tilde_02_index = this->rampsmooth_tilde_02_index;
        auto __rampsmooth_tilde_02_prev = this->rampsmooth_tilde_02_prev;
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            if (this->rampsmooth_tilde_02_d_next(x[(Index)i]) != 0.) {
                if (x[(Index)i] > __rampsmooth_tilde_02_prev) {
                    number _up = 480;
    
                    if (_up < 1)
                        _up = 1;
    
                    __rampsmooth_tilde_02_index = _up;
                    __rampsmooth_tilde_02_increment = (x[(Index)i] - __rampsmooth_tilde_02_prev) / _up;
                } else if (x[(Index)i] < __rampsmooth_tilde_02_prev) {
                    number _down = 480;
    
                    if (_down < 1)
                        _down = 1;
    
                    __rampsmooth_tilde_02_index = _down;
                    __rampsmooth_tilde_02_increment = (x[(Index)i] - __rampsmooth_tilde_02_prev) / _down;
                }
            }
    
            if (__rampsmooth_tilde_02_index > 0) {
                __rampsmooth_tilde_02_prev += __rampsmooth_tilde_02_increment;
                __rampsmooth_tilde_02_index -= 1;
            } else {
                __rampsmooth_tilde_02_prev = x[(Index)i];
            }
    
            out1[(Index)i] = __rampsmooth_tilde_02_prev;
        }
    
        this->rampsmooth_tilde_02_prev = __rampsmooth_tilde_02_prev;
        this->rampsmooth_tilde_02_index = __rampsmooth_tilde_02_index;
        this->rampsmooth_tilde_02_increment = __rampsmooth_tilde_02_increment;
    }
    
    void dspexpr_06_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] * in2[(Index)i];//#map:_###_obj_###_:1
        }
    }
    
    void dspexpr_10_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] * in2[(Index)i];//#map:_###_obj_###_:1
        }
    }
    
    void rand_tilde_03_perform(number frequency, SampleValue * out1, Index n) {
        auto __rand_tilde_03_lastValue = this->rand_tilde_03_lastValue;
        auto __rand_tilde_03_oldTargetValue = this->rand_tilde_03_oldTargetValue;
        auto __rand_tilde_03_targetValue = this->rand_tilde_03_targetValue;
        number f = (frequency > 0.0001 ? frequency : 0.0001);
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            number phasorValue = this->rand_tilde_03_p_next(f, -1);
    
            if (this->rand_tilde_03_d_next(phasorValue) < 0) {
                __rand_tilde_03_targetValue = this->rand_tilde_03_n_next();
                __rand_tilde_03_oldTargetValue = __rand_tilde_03_lastValue;
            }
    
            __rand_tilde_03_lastValue = __rand_tilde_03_oldTargetValue + phasorValue * (__rand_tilde_03_targetValue - __rand_tilde_03_oldTargetValue);
            out1[(Index)i] = __rand_tilde_03_lastValue;
        }
    
        this->rand_tilde_03_targetValue = __rand_tilde_03_targetValue;
        this->rand_tilde_03_oldTargetValue = __rand_tilde_03_oldTargetValue;
        this->rand_tilde_03_lastValue = __rand_tilde_03_lastValue;
    }
    
    void cycle_tilde_03_perform(
        const Sample * frequency,
        number phase_offset,
        SampleValue * out1,
        SampleValue * out2,
        Index n
    ) {
        RNBO_UNUSED(phase_offset);
        auto __cycle_tilde_03_buffer = this->cycle_tilde_03_buffer;
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            {
                UInt32 uint_phase;
    
                {
                    {
                        uint_phase = uint32_trunc(frequency[(Index)i] * 4294967296.0);
                    }
                }
    
                UInt32 idx = (UInt32)(uint32_rshift(uint_phase, 18));
                number frac = ((BinOpInt)((BinOpInt)uint_phase & (BinOpInt)262143)) * 3.81471181759574e-6;
                number y0 = __cycle_tilde_03_buffer[(Index)idx];
                number y1 = __cycle_tilde_03_buffer[(Index)((BinOpInt)(idx + 1) & (BinOpInt)16383)];
                number y = y0 + frac * (y1 - y0);
                out1[(Index)i] = y;
                out2[(Index)i] = uint_phase * 0.232830643653869629e-9;
                continue;
            }
        }
    }
    
    void dspexpr_18_perform(const Sample * in1, number in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] * in2;//#map:_###_obj_###_:1
        }
    }
    
    void dspexpr_17_perform(const Sample * in1, number in2, SampleValue * out1, Index n) {
        RNBO_UNUSED(in2);
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] + 1;//#map:_###_obj_###_:1
        }
    }
    
    void dspexpr_16_perform(const Sample * in1, number in2, SampleValue * out1, Index n) {
        RNBO_UNUSED(in2);
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] * 0.5;//#map:_###_obj_###_:1
        }
    }
    
    void rampsmooth_tilde_03_perform(const Sample * x, number up, number down, SampleValue * out1, Index n) {
        RNBO_UNUSED(down);
        RNBO_UNUSED(up);
        auto __rampsmooth_tilde_03_increment = this->rampsmooth_tilde_03_increment;
        auto __rampsmooth_tilde_03_index = this->rampsmooth_tilde_03_index;
        auto __rampsmooth_tilde_03_prev = this->rampsmooth_tilde_03_prev;
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            if (this->rampsmooth_tilde_03_d_next(x[(Index)i]) != 0.) {
                if (x[(Index)i] > __rampsmooth_tilde_03_prev) {
                    number _up = 480;
    
                    if (_up < 1)
                        _up = 1;
    
                    __rampsmooth_tilde_03_index = _up;
                    __rampsmooth_tilde_03_increment = (x[(Index)i] - __rampsmooth_tilde_03_prev) / _up;
                } else if (x[(Index)i] < __rampsmooth_tilde_03_prev) {
                    number _down = 480;
    
                    if (_down < 1)
                        _down = 1;
    
                    __rampsmooth_tilde_03_index = _down;
                    __rampsmooth_tilde_03_increment = (x[(Index)i] - __rampsmooth_tilde_03_prev) / _down;
                }
            }
    
            if (__rampsmooth_tilde_03_index > 0) {
                __rampsmooth_tilde_03_prev += __rampsmooth_tilde_03_increment;
                __rampsmooth_tilde_03_index -= 1;
            } else {
                __rampsmooth_tilde_03_prev = x[(Index)i];
            }
    
            out1[(Index)i] = __rampsmooth_tilde_03_prev;
        }
    
        this->rampsmooth_tilde_03_prev = __rampsmooth_tilde_03_prev;
        this->rampsmooth_tilde_03_index = __rampsmooth_tilde_03_index;
        this->rampsmooth_tilde_03_increment = __rampsmooth_tilde_03_increment;
    }
    
    void pan_tilde_01_perform(
        const Sample * pos,
        const SampleValue * in1,
        const SampleValue * in2,
        SampleValue * out1,
        SampleValue * out2,
        Index n
    ) {
        auto __pan_tilde_01_spread = this->pan_tilde_01_spread;
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out2[(Index)i] = in1[(Index)i] * this->pan_tilde_01_func_next(pos[(Index)i] + -0.5 * __pan_tilde_01_spread, 1) + in2[(Index)i] * this->pan_tilde_01_func_next(pos[(Index)i] + 0.5 * __pan_tilde_01_spread, 1);
            out1[(Index)i] = in1[(Index)i] * this->pan_tilde_01_func_next(pos[(Index)i] + -0.5 * __pan_tilde_01_spread, 0) + in2[(Index)i] * this->pan_tilde_01_func_next(pos[(Index)i] + 0.5 * __pan_tilde_01_spread, 0);
        }
    }
    
    void signaladder_03_perform(
        const SampleValue * in1,
        const SampleValue * in2,
        SampleValue * out,
        Index n
    ) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out[(Index)i] = in1[(Index)i] + in2[(Index)i];
        }
    }
    
    void signaladder_04_perform(
        const SampleValue * in1,
        const SampleValue * in2,
        SampleValue * out,
        Index n
    ) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out[(Index)i] = in1[(Index)i] + in2[(Index)i];
        }
    }
    
    void stackprotect_perform(Index n) {
        RNBO_UNUSED(n);
        auto __stackprotect_count = this->stackprotect_count;
        __stackprotect_count = 0;
        this->stackprotect_count = __stackprotect_count;
    }
    
    number pan_tilde_01_func_next(number pos, Int channel) {
        {
            {
                number nchan_1 = 2 - 1;
    
                {
                    pos = pos * nchan_1;
                }
    
                {
                    {
                        if (pos >= 0) {
                            pos = this->safemod(pos, 2);
                        } else {
                            pos = this->safemod(2 + this->safemod(pos, 2), 2);
                        }
    
                        if (channel == 0 && pos >= nchan_1)
                            pos = pos - 2;
                    }
                }
    
                pos = pos - channel;
    
                if (pos > -1 && pos < 1) {
                    {
                        {
                            return this->safesqrt(1.0 - rnbo_abs(pos));
                        }
                    }
                } else {
                    return 0;
                }
            }
        }
    }
    
    void pan_tilde_01_func_reset() {}
    
    number rampsmooth_tilde_02_d_next(number x) {
        number temp = (number)(x - this->rampsmooth_tilde_02_d_prev);
        this->rampsmooth_tilde_02_d_prev = x;
        return temp;
    }
    
    void rampsmooth_tilde_02_d_dspsetup() {
        this->rampsmooth_tilde_02_d_reset();
    }
    
    void rampsmooth_tilde_02_d_reset() {
        this->rampsmooth_tilde_02_d_prev = 0;
    }
    
    void rampsmooth_tilde_02_dspsetup(bool force) {
        if ((bool)(this->rampsmooth_tilde_02_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->rampsmooth_tilde_02_setupDone = true;
        this->rampsmooth_tilde_02_d_dspsetup();
    }
    
    number rampsmooth_tilde_03_d_next(number x) {
        number temp = (number)(x - this->rampsmooth_tilde_03_d_prev);
        this->rampsmooth_tilde_03_d_prev = x;
        return temp;
    }
    
    void rampsmooth_tilde_03_d_dspsetup() {
        this->rampsmooth_tilde_03_d_reset();
    }
    
    void rampsmooth_tilde_03_d_reset() {
        this->rampsmooth_tilde_03_d_prev = 0;
    }
    
    void rampsmooth_tilde_03_dspsetup(bool force) {
        if ((bool)(this->rampsmooth_tilde_03_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->rampsmooth_tilde_03_setupDone = true;
        this->rampsmooth_tilde_03_d_dspsetup();
    }
    
    number rampsmooth_tilde_04_d_next(number x) {
        number temp = (number)(x - this->rampsmooth_tilde_04_d_prev);
        this->rampsmooth_tilde_04_d_prev = x;
        return temp;
    }
    
    void rampsmooth_tilde_04_d_dspsetup() {
        this->rampsmooth_tilde_04_d_reset();
    }
    
    void rampsmooth_tilde_04_d_reset() {
        this->rampsmooth_tilde_04_d_prev = 0;
    }
    
    void rampsmooth_tilde_04_dspsetup(bool force) {
        if ((bool)(this->rampsmooth_tilde_04_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->rampsmooth_tilde_04_setupDone = true;
        this->rampsmooth_tilde_04_d_dspsetup();
    }
    
    number cycle_tilde_01_ph_next(number freq, number reset) {
        {
            {
                if (reset >= 0.)
                    this->cycle_tilde_01_ph_currentPhase = reset;
            }
        }
    
        number pincr = freq * this->cycle_tilde_01_ph_conv;
    
        if (this->cycle_tilde_01_ph_currentPhase < 0.)
            this->cycle_tilde_01_ph_currentPhase = 1. + this->cycle_tilde_01_ph_currentPhase;
    
        if (this->cycle_tilde_01_ph_currentPhase > 1.)
            this->cycle_tilde_01_ph_currentPhase = this->cycle_tilde_01_ph_currentPhase - 1.;
    
        number tmp = this->cycle_tilde_01_ph_currentPhase;
        this->cycle_tilde_01_ph_currentPhase += pincr;
        return tmp;
    }
    
    void cycle_tilde_01_ph_reset() {
        this->cycle_tilde_01_ph_currentPhase = 0;
    }
    
    void cycle_tilde_01_ph_dspsetup() {
        this->cycle_tilde_01_ph_conv = (number)1 / this->sr;
    }
    
    void cycle_tilde_01_dspsetup(bool force) {
        if ((bool)(this->cycle_tilde_01_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->cycle_tilde_01_phasei = 0;
        this->cycle_tilde_01_f2i = (number)4294967296 / this->sr;
        this->cycle_tilde_01_wrap = (Int)(this->cycle_tilde_01_buffer->getSize()) - 1;
        this->cycle_tilde_01_setupDone = true;
        this->cycle_tilde_01_ph_dspsetup();
    }
    
    void cycle_tilde_01_bufferUpdated() {
        this->cycle_tilde_01_wrap = (Int)(this->cycle_tilde_01_buffer->getSize()) - 1;
    }
    
    number rand_tilde_01_p_next(number freq, number reset) {
        {
            {
                if (reset >= 0.)
                    this->rand_tilde_01_p_currentPhase = reset;
            }
        }
    
        number pincr = freq * this->rand_tilde_01_p_conv;
    
        if (this->rand_tilde_01_p_currentPhase < 0.)
            this->rand_tilde_01_p_currentPhase = 1. + this->rand_tilde_01_p_currentPhase;
    
        if (this->rand_tilde_01_p_currentPhase > 1.)
            this->rand_tilde_01_p_currentPhase = this->rand_tilde_01_p_currentPhase - 1.;
    
        number tmp = this->rand_tilde_01_p_currentPhase;
        this->rand_tilde_01_p_currentPhase += pincr;
        return tmp;
    }
    
    void rand_tilde_01_p_reset() {
        this->rand_tilde_01_p_currentPhase = 0;
    }
    
    void rand_tilde_01_p_dspsetup() {
        this->rand_tilde_01_p_conv = (number)1 / this->sr;
    }
    
    void rand_tilde_01_n_reset() {
        xoshiro_reset(
            systemticks() + this->voice() + this->random(0, 10000),
            this->rand_tilde_01_n_state
        );
    }
    
    void rand_tilde_01_n_init() {
        this->rand_tilde_01_n_reset();
    }
    
    void rand_tilde_01_n_seed(number v) {
        xoshiro_reset(v, this->rand_tilde_01_n_state);
    }
    
    number rand_tilde_01_n_next() {
        return xoshiro_next(this->rand_tilde_01_n_state);
    }
    
    number rand_tilde_01_d_next(number x) {
        number temp = (number)(x - this->rand_tilde_01_d_prev);
        this->rand_tilde_01_d_prev = x;
        return temp;
    }
    
    void rand_tilde_01_d_dspsetup() {
        this->rand_tilde_01_d_reset();
    }
    
    void rand_tilde_01_d_reset() {
        this->rand_tilde_01_d_prev = 0;
    }
    
    void rand_tilde_01_init() {
        this->rand_tilde_01_targetValue = this->rand_tilde_01_n_next();
    }
    
    void rand_tilde_01_dspsetup(bool force) {
        if ((bool)(this->rand_tilde_01_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->rand_tilde_01_setupDone = true;
        this->rand_tilde_01_p_dspsetup();
        this->rand_tilde_01_d_dspsetup();
    }
    
    number cycle_tilde_02_ph_next(number freq, number reset) {
        {
            {
                if (reset >= 0.)
                    this->cycle_tilde_02_ph_currentPhase = reset;
            }
        }
    
        number pincr = freq * this->cycle_tilde_02_ph_conv;
    
        if (this->cycle_tilde_02_ph_currentPhase < 0.)
            this->cycle_tilde_02_ph_currentPhase = 1. + this->cycle_tilde_02_ph_currentPhase;
    
        if (this->cycle_tilde_02_ph_currentPhase > 1.)
            this->cycle_tilde_02_ph_currentPhase = this->cycle_tilde_02_ph_currentPhase - 1.;
    
        number tmp = this->cycle_tilde_02_ph_currentPhase;
        this->cycle_tilde_02_ph_currentPhase += pincr;
        return tmp;
    }
    
    void cycle_tilde_02_ph_reset() {
        this->cycle_tilde_02_ph_currentPhase = 0;
    }
    
    void cycle_tilde_02_ph_dspsetup() {
        this->cycle_tilde_02_ph_conv = (number)1 / this->sr;
    }
    
    void cycle_tilde_02_dspsetup(bool force) {
        if ((bool)(this->cycle_tilde_02_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->cycle_tilde_02_phasei = 0;
        this->cycle_tilde_02_f2i = (number)4294967296 / this->sr;
        this->cycle_tilde_02_wrap = (Int)(this->cycle_tilde_02_buffer->getSize()) - 1;
        this->cycle_tilde_02_setupDone = true;
        this->cycle_tilde_02_ph_dspsetup();
    }
    
    void cycle_tilde_02_bufferUpdated() {
        this->cycle_tilde_02_wrap = (Int)(this->cycle_tilde_02_buffer->getSize()) - 1;
    }
    
    number rand_tilde_02_p_next(number freq, number reset) {
        {
            {
                if (reset >= 0.)
                    this->rand_tilde_02_p_currentPhase = reset;
            }
        }
    
        number pincr = freq * this->rand_tilde_02_p_conv;
    
        if (this->rand_tilde_02_p_currentPhase < 0.)
            this->rand_tilde_02_p_currentPhase = 1. + this->rand_tilde_02_p_currentPhase;
    
        if (this->rand_tilde_02_p_currentPhase > 1.)
            this->rand_tilde_02_p_currentPhase = this->rand_tilde_02_p_currentPhase - 1.;
    
        number tmp = this->rand_tilde_02_p_currentPhase;
        this->rand_tilde_02_p_currentPhase += pincr;
        return tmp;
    }
    
    void rand_tilde_02_p_reset() {
        this->rand_tilde_02_p_currentPhase = 0;
    }
    
    void rand_tilde_02_p_dspsetup() {
        this->rand_tilde_02_p_conv = (number)1 / this->sr;
    }
    
    void rand_tilde_02_n_reset() {
        xoshiro_reset(
            systemticks() + this->voice() + this->random(0, 10000),
            this->rand_tilde_02_n_state
        );
    }
    
    void rand_tilde_02_n_init() {
        this->rand_tilde_02_n_reset();
    }
    
    void rand_tilde_02_n_seed(number v) {
        xoshiro_reset(v, this->rand_tilde_02_n_state);
    }
    
    number rand_tilde_02_n_next() {
        return xoshiro_next(this->rand_tilde_02_n_state);
    }
    
    number rand_tilde_02_d_next(number x) {
        number temp = (number)(x - this->rand_tilde_02_d_prev);
        this->rand_tilde_02_d_prev = x;
        return temp;
    }
    
    void rand_tilde_02_d_dspsetup() {
        this->rand_tilde_02_d_reset();
    }
    
    void rand_tilde_02_d_reset() {
        this->rand_tilde_02_d_prev = 0;
    }
    
    void rand_tilde_02_init() {
        this->rand_tilde_02_targetValue = this->rand_tilde_02_n_next();
    }
    
    void rand_tilde_02_dspsetup(bool force) {
        if ((bool)(this->rand_tilde_02_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->rand_tilde_02_setupDone = true;
        this->rand_tilde_02_p_dspsetup();
        this->rand_tilde_02_d_dspsetup();
    }
    
    number cycle_tilde_03_ph_next(number freq, number reset) {
        {
            {
                if (reset >= 0.)
                    this->cycle_tilde_03_ph_currentPhase = reset;
            }
        }
    
        number pincr = freq * this->cycle_tilde_03_ph_conv;
    
        if (this->cycle_tilde_03_ph_currentPhase < 0.)
            this->cycle_tilde_03_ph_currentPhase = 1. + this->cycle_tilde_03_ph_currentPhase;
    
        if (this->cycle_tilde_03_ph_currentPhase > 1.)
            this->cycle_tilde_03_ph_currentPhase = this->cycle_tilde_03_ph_currentPhase - 1.;
    
        number tmp = this->cycle_tilde_03_ph_currentPhase;
        this->cycle_tilde_03_ph_currentPhase += pincr;
        return tmp;
    }
    
    void cycle_tilde_03_ph_reset() {
        this->cycle_tilde_03_ph_currentPhase = 0;
    }
    
    void cycle_tilde_03_ph_dspsetup() {
        this->cycle_tilde_03_ph_conv = (number)1 / this->sr;
    }
    
    void cycle_tilde_03_dspsetup(bool force) {
        if ((bool)(this->cycle_tilde_03_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->cycle_tilde_03_phasei = 0;
        this->cycle_tilde_03_f2i = (number)4294967296 / this->sr;
        this->cycle_tilde_03_wrap = (Int)(this->cycle_tilde_03_buffer->getSize()) - 1;
        this->cycle_tilde_03_setupDone = true;
        this->cycle_tilde_03_ph_dspsetup();
    }
    
    void cycle_tilde_03_bufferUpdated() {
        this->cycle_tilde_03_wrap = (Int)(this->cycle_tilde_03_buffer->getSize()) - 1;
    }
    
    number rand_tilde_03_p_next(number freq, number reset) {
        {
            {
                if (reset >= 0.)
                    this->rand_tilde_03_p_currentPhase = reset;
            }
        }
    
        number pincr = freq * this->rand_tilde_03_p_conv;
    
        if (this->rand_tilde_03_p_currentPhase < 0.)
            this->rand_tilde_03_p_currentPhase = 1. + this->rand_tilde_03_p_currentPhase;
    
        if (this->rand_tilde_03_p_currentPhase > 1.)
            this->rand_tilde_03_p_currentPhase = this->rand_tilde_03_p_currentPhase - 1.;
    
        number tmp = this->rand_tilde_03_p_currentPhase;
        this->rand_tilde_03_p_currentPhase += pincr;
        return tmp;
    }
    
    void rand_tilde_03_p_reset() {
        this->rand_tilde_03_p_currentPhase = 0;
    }
    
    void rand_tilde_03_p_dspsetup() {
        this->rand_tilde_03_p_conv = (number)1 / this->sr;
    }
    
    void rand_tilde_03_n_reset() {
        xoshiro_reset(
            systemticks() + this->voice() + this->random(0, 10000),
            this->rand_tilde_03_n_state
        );
    }
    
    void rand_tilde_03_n_init() {
        this->rand_tilde_03_n_reset();
    }
    
    void rand_tilde_03_n_seed(number v) {
        xoshiro_reset(v, this->rand_tilde_03_n_state);
    }
    
    number rand_tilde_03_n_next() {
        return xoshiro_next(this->rand_tilde_03_n_state);
    }
    
    number rand_tilde_03_d_next(number x) {
        number temp = (number)(x - this->rand_tilde_03_d_prev);
        this->rand_tilde_03_d_prev = x;
        return temp;
    }
    
    void rand_tilde_03_d_dspsetup() {
        this->rand_tilde_03_d_reset();
    }
    
    void rand_tilde_03_d_reset() {
        this->rand_tilde_03_d_prev = 0;
    }
    
    void rand_tilde_03_init() {
        this->rand_tilde_03_targetValue = this->rand_tilde_03_n_next();
    }
    
    void rand_tilde_03_dspsetup(bool force) {
        if ((bool)(this->rand_tilde_03_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->rand_tilde_03_setupDone = true;
        this->rand_tilde_03_p_dspsetup();
        this->rand_tilde_03_d_dspsetup();
    }
    
    void midiouthelper_sendMidi(number v) {
        this->midiouthelper_midiout_set(v);
    }
    
    bool stackprotect_check() {
        this->stackprotect_count++;
    
        if (this->stackprotect_count > 128) {
            console->log("STACK OVERFLOW DETECTED - stopped processing branch !");
            return true;
        }
    
        return false;
    }
    
    Index getPatcherSerial() const {
        return 0;
    }
    
    void sendParameter(ParameterIndex index, bool ignoreValue) {
        if (this->_voiceIndex == 1)
            this->getPatcher()->sendParameter(index + this->parameterOffset, ignoreValue);
    }
    
    void scheduleParamInit(ParameterIndex index, Index order) {
        this->getPatcher()->scheduleParamInit(index + this->parameterOffset, order);
    }
    
    void updateTime(MillisecondTime time, EXTERNALENGINE* engine, bool inProcess = false) {
        RNBO_UNUSED(inProcess);
        RNBO_UNUSED(engine);
        this->_currentTime = time;
        auto offset = rnbo_fround(this->msToSamps(time - this->getEngine()->getCurrentTime(), this->sr));
    
        if (offset >= (SampleIndex)(this->vs))
            offset = (SampleIndex)(this->vs) - 1;
    
        if (offset < 0)
            offset = 0;
    
        this->sampleOffsetIntoNextAudioBuffer = (Index)(offset);
    }
    
    void assign_defaults()
    {
        dspexpr_06_in1 = 0;
        dspexpr_06_in2 = 0;
        pan_tilde_01_spread = 0.5;
        pan_tilde_01_pos = 0;
        dspexpr_07_in1 = 0;
        dspexpr_07_in2 = 0;
        sample_01_value = 0;
        sample_01_offset = 0;
        wrap_tilde_01_x = 0;
        wrap_tilde_01_low = 0;
        wrap_tilde_01_high = 1;
        dspexpr_08_in1 = 0;
        dspexpr_08_in2 = 0;
        dspexpr_09_in1 = 0;
        dspexpr_09_in2 = 0;
        dspexpr_10_in1 = 0;
        dspexpr_10_in2 = 0;
        dspexpr_11_in1 = 0;
        dspexpr_11_in2 = 0;
        expr_03_in1 = 0;
        expr_03_in2 = 8;
        expr_03_out1 = 0;
        expr_04_in1 = 0;
        expr_04_in2 = 1;
        expr_04_out1 = 0;
        voice_01_mute_number = 0;
        hann_tilde_01_x = 0;
        rampsmooth_tilde_02_x = 0;
        rampsmooth_tilde_02_up = 480;
        rampsmooth_tilde_02_down = 480;
        rampsmooth_tilde_03_x = 0;
        rampsmooth_tilde_03_up = 480;
        rampsmooth_tilde_03_down = 480;
        rampsmooth_tilde_04_x = 0;
        rampsmooth_tilde_04_up = 480;
        rampsmooth_tilde_04_down = 480;
        cycle_tilde_01_frequency = 0;
        cycle_tilde_01_phase_offset = 0;
        dspexpr_12_in1 = 0;
        dspexpr_12_in2 = 0.005;
        rand_tilde_01_frequency = 1;
        receive_01_output_number = 0;
        dspexpr_13_in1 = 0;
        dspexpr_13_in2 = 0.5;
        dspexpr_14_in1 = 0;
        dspexpr_14_in2 = 1;
        cycle_tilde_02_frequency = 0;
        cycle_tilde_02_phase_offset = 0;
        dspexpr_15_in1 = 0;
        dspexpr_15_in2 = 0.1;
        rand_tilde_02_frequency = 1;
        receive_02_output_number = 0;
        dspexpr_16_in1 = 0;
        dspexpr_16_in2 = 0.5;
        dspexpr_17_in1 = 0;
        dspexpr_17_in2 = 1;
        cycle_tilde_03_frequency = 0;
        cycle_tilde_03_phase_offset = 0;
        dspexpr_18_in1 = 0;
        dspexpr_18_in2 = 0.1;
        rand_tilde_03_frequency = 1;
        receive_03_output_number = 0;
        receive_04_output_number = 0;
        receive_05_output_number = 0;
        receive_06_output_number = 0;
        _currentTime = 0;
        audioProcessSampleCount = 0;
        sampleOffsetIntoNextAudioBuffer = 0;
        zeroBuffer = nullptr;
        dummyBuffer = nullptr;
        signals[0] = nullptr;
        signals[1] = nullptr;
        signals[2] = nullptr;
        signals[3] = nullptr;
        signals[4] = nullptr;
        didAllocateSignals = 0;
        vs = 0;
        maxvs = 0;
        sr = 44100;
        invsr = 0.000022675736961451248;
        rampsmooth_tilde_02_prev = 0;
        rampsmooth_tilde_02_index = 0;
        rampsmooth_tilde_02_increment = 0;
        rampsmooth_tilde_02_d_prev = 0;
        rampsmooth_tilde_02_setupDone = false;
        rampsmooth_tilde_03_prev = 0;
        rampsmooth_tilde_03_index = 0;
        rampsmooth_tilde_03_increment = 0;
        rampsmooth_tilde_03_d_prev = 0;
        rampsmooth_tilde_03_setupDone = false;
        rampsmooth_tilde_04_prev = 0;
        rampsmooth_tilde_04_index = 0;
        rampsmooth_tilde_04_increment = 0;
        rampsmooth_tilde_04_d_prev = 0;
        rampsmooth_tilde_04_setupDone = false;
        cycle_tilde_01_wrap = 0;
        cycle_tilde_01_ph_currentPhase = 0;
        cycle_tilde_01_ph_conv = 0;
        cycle_tilde_01_setupDone = false;
        rand_tilde_01_lastValue = 0;
        rand_tilde_01_targetValue = 0;
        rand_tilde_01_oldTargetValue = 0;
        rand_tilde_01_p_currentPhase = 0;
        rand_tilde_01_p_conv = 0;
        rand_tilde_01_d_prev = 0;
        rand_tilde_01_setupDone = false;
        cycle_tilde_02_wrap = 0;
        cycle_tilde_02_ph_currentPhase = 0;
        cycle_tilde_02_ph_conv = 0;
        cycle_tilde_02_setupDone = false;
        rand_tilde_02_lastValue = 0;
        rand_tilde_02_targetValue = 0;
        rand_tilde_02_oldTargetValue = 0;
        rand_tilde_02_p_currentPhase = 0;
        rand_tilde_02_p_conv = 0;
        rand_tilde_02_d_prev = 0;
        rand_tilde_02_setupDone = false;
        cycle_tilde_03_wrap = 0;
        cycle_tilde_03_ph_currentPhase = 0;
        cycle_tilde_03_ph_conv = 0;
        cycle_tilde_03_setupDone = false;
        rand_tilde_03_lastValue = 0;
        rand_tilde_03_targetValue = 0;
        rand_tilde_03_oldTargetValue = 0;
        rand_tilde_03_p_currentPhase = 0;
        rand_tilde_03_p_conv = 0;
        rand_tilde_03_d_prev = 0;
        rand_tilde_03_setupDone = false;
        stackprotect_count = 0;
        _voiceIndex = 0;
        _noteNumber = 0;
        isMuted = 0;
        parameterOffset = 0;
    }
    
    // member variables
    
        number dspexpr_06_in1;
        number dspexpr_06_in2;
        number pan_tilde_01_spread;
        number pan_tilde_01_pos;
        number dspexpr_07_in1;
        number dspexpr_07_in2;
        number sample_01_value;
        number sample_01_offset;
        number wrap_tilde_01_x;
        number wrap_tilde_01_low;
        number wrap_tilde_01_high;
        number dspexpr_08_in1;
        number dspexpr_08_in2;
        number dspexpr_09_in1;
        number dspexpr_09_in2;
        number dspexpr_10_in1;
        number dspexpr_10_in2;
        number dspexpr_11_in1;
        number dspexpr_11_in2;
        number expr_03_in1;
        number expr_03_in2;
        number expr_03_out1;
        number expr_04_in1;
        number expr_04_in2;
        number expr_04_out1;
        number voice_01_mute_number;
        number hann_tilde_01_x;
        number rampsmooth_tilde_02_x;
        number rampsmooth_tilde_02_up;
        number rampsmooth_tilde_02_down;
        number rampsmooth_tilde_03_x;
        number rampsmooth_tilde_03_up;
        number rampsmooth_tilde_03_down;
        number rampsmooth_tilde_04_x;
        number rampsmooth_tilde_04_up;
        number rampsmooth_tilde_04_down;
        number cycle_tilde_01_frequency;
        number cycle_tilde_01_phase_offset;
        number dspexpr_12_in1;
        number dspexpr_12_in2;
        number rand_tilde_01_frequency;
        number receive_01_output_number;
        list receive_01_output_list;
        number dspexpr_13_in1;
        number dspexpr_13_in2;
        number dspexpr_14_in1;
        number dspexpr_14_in2;
        number cycle_tilde_02_frequency;
        number cycle_tilde_02_phase_offset;
        number dspexpr_15_in1;
        number dspexpr_15_in2;
        number rand_tilde_02_frequency;
        number receive_02_output_number;
        list receive_02_output_list;
        number dspexpr_16_in1;
        number dspexpr_16_in2;
        number dspexpr_17_in1;
        number dspexpr_17_in2;
        number cycle_tilde_03_frequency;
        number cycle_tilde_03_phase_offset;
        number dspexpr_18_in1;
        number dspexpr_18_in2;
        number rand_tilde_03_frequency;
        number receive_03_output_number;
        list receive_03_output_list;
        number receive_04_output_number;
        list receive_04_output_list;
        number receive_05_output_number;
        list receive_05_output_list;
        number receive_06_output_number;
        list receive_06_output_list;
        MillisecondTime _currentTime;
        UInt64 audioProcessSampleCount;
        Index sampleOffsetIntoNextAudioBuffer;
        signal zeroBuffer;
        signal dummyBuffer;
        SampleValue * signals[5];
        bool didAllocateSignals;
        Index vs;
        Index maxvs;
        number sr;
        number invsr;
        Float64BufferRef sample_01_buffer;
        number rampsmooth_tilde_02_prev;
        number rampsmooth_tilde_02_index;
        number rampsmooth_tilde_02_increment;
        number rampsmooth_tilde_02_d_prev;
        bool rampsmooth_tilde_02_setupDone;
        number rampsmooth_tilde_03_prev;
        number rampsmooth_tilde_03_index;
        number rampsmooth_tilde_03_increment;
        number rampsmooth_tilde_03_d_prev;
        bool rampsmooth_tilde_03_setupDone;
        number rampsmooth_tilde_04_prev;
        number rampsmooth_tilde_04_index;
        number rampsmooth_tilde_04_increment;
        number rampsmooth_tilde_04_d_prev;
        bool rampsmooth_tilde_04_setupDone;
        SampleBufferRef cycle_tilde_01_buffer;
        Int cycle_tilde_01_wrap;
        UInt32 cycle_tilde_01_phasei;
        SampleValue cycle_tilde_01_f2i;
        number cycle_tilde_01_ph_currentPhase;
        number cycle_tilde_01_ph_conv;
        bool cycle_tilde_01_setupDone;
        number rand_tilde_01_lastValue;
        number rand_tilde_01_targetValue;
        number rand_tilde_01_oldTargetValue;
        number rand_tilde_01_p_currentPhase;
        number rand_tilde_01_p_conv;
        UInt rand_tilde_01_n_state[4] = { };
        number rand_tilde_01_d_prev;
        bool rand_tilde_01_setupDone;
        SampleBufferRef cycle_tilde_02_buffer;
        Int cycle_tilde_02_wrap;
        UInt32 cycle_tilde_02_phasei;
        SampleValue cycle_tilde_02_f2i;
        number cycle_tilde_02_ph_currentPhase;
        number cycle_tilde_02_ph_conv;
        bool cycle_tilde_02_setupDone;
        number rand_tilde_02_lastValue;
        number rand_tilde_02_targetValue;
        number rand_tilde_02_oldTargetValue;
        number rand_tilde_02_p_currentPhase;
        number rand_tilde_02_p_conv;
        UInt rand_tilde_02_n_state[4] = { };
        number rand_tilde_02_d_prev;
        bool rand_tilde_02_setupDone;
        SampleBufferRef cycle_tilde_03_buffer;
        Int cycle_tilde_03_wrap;
        UInt32 cycle_tilde_03_phasei;
        SampleValue cycle_tilde_03_f2i;
        number cycle_tilde_03_ph_currentPhase;
        number cycle_tilde_03_ph_conv;
        bool cycle_tilde_03_setupDone;
        number rand_tilde_03_lastValue;
        number rand_tilde_03_targetValue;
        number rand_tilde_03_oldTargetValue;
        number rand_tilde_03_p_currentPhase;
        number rand_tilde_03_p_conv;
        UInt rand_tilde_03_n_state[4] = { };
        number rand_tilde_03_d_prev;
        bool rand_tilde_03_setupDone;
        number stackprotect_count;
        Index _voiceIndex;
        Int _noteNumber;
        Index isMuted;
        ParameterIndex parameterOffset;
        bool _isInitialized = false;
};

class RNBOSubpatcher_246 : public PatcherInterfaceImpl {
    
    friend class rnbomatic;
    
    public:
    
    RNBOSubpatcher_246()
    {}
    
    ~RNBOSubpatcher_246()
    {
        deallocateSignals();
    }
    
    Index getNumMidiInputPorts() const {
        return 0;
    }
    
    void processMidiEvent(MillisecondTime , int , ConstByteArray , Index ) {}
    
    Index getNumMidiOutputPorts() const {
        return 0;
    }
    
    void process(
        const SampleValue * const* inputs,
        Index numInputs,
        SampleValue * const* outputs,
        Index numOutputs,
        Index n
    ) {
        this->vs = n;
        this->updateTime(this->getEngine()->getCurrentTime(), (ENGINE*)nullptr, true);
        SampleValue * out1 = (numOutputs >= 1 && outputs[0] ? outputs[0] : this->dummyBuffer);
        SampleValue * out2 = (numOutputs >= 2 && outputs[1] ? outputs[1] : this->dummyBuffer);
        const SampleValue * in1 = (numInputs >= 1 && inputs[0] ? inputs[0] : this->zeroBuffer);
        const SampleValue * in2 = (numInputs >= 2 && inputs[1] ? inputs[1] : this->zeroBuffer);
        this->linetilde_01_perform(this->signals[0], n);
        this->onepole_tilde_04_perform(in2, this->signals[0], this->signals[1], n);
        this->dspexpr_24_perform(in2, this->signals[1], this->signals[2], n);
        this->onepole_tilde_02_perform(in1, this->signals[0], this->signals[1], n);
        this->dspexpr_21_perform(in1, this->signals[1], this->signals[0], n);
        this->linetilde_02_perform(this->signals[1], n);
        this->linetilde_03_perform(this->signals[3], n);
        this->onepole_tilde_01_perform(this->signals[0], this->signals[3], this->signals[4], n);
        this->dspexpr_20_perform(this->signals[4], this->signals[1], this->signals[0], n);
        this->dspexpr_19_perform(this->signals[0], out1, n);
        this->onepole_tilde_03_perform(this->signals[2], this->signals[3], this->signals[0], n);
        this->dspexpr_23_perform(this->signals[0], this->signals[1], this->signals[3], n);
        this->dspexpr_22_perform(this->signals[3], out2, n);
        this->stackprotect_perform(n);
        this->audioProcessSampleCount += this->vs;
    }
    
    void prepareToProcess(number sampleRate, Index maxBlockSize, bool force) {
        RNBO_ASSERT(this->_isInitialized);
    
        if (this->maxvs < maxBlockSize || !this->didAllocateSignals) {
            Index i;
    
            for (i = 0; i < 5; i++) {
                this->signals[i] = resizeSignal(this->signals[i], this->maxvs, maxBlockSize);
            }
    
            this->zeroBuffer = resizeSignal(this->zeroBuffer, this->maxvs, maxBlockSize);
            this->dummyBuffer = resizeSignal(this->dummyBuffer, this->maxvs, maxBlockSize);
            this->didAllocateSignals = true;
        }
    
        const bool sampleRateChanged = sampleRate != this->sr;
        const bool maxvsChanged = maxBlockSize != this->maxvs;
        const bool forceDSPSetup = sampleRateChanged || maxvsChanged || force;
    
        if (sampleRateChanged || maxvsChanged) {
            this->vs = maxBlockSize;
            this->maxvs = maxBlockSize;
            this->sr = sampleRate;
            this->invsr = 1 / sampleRate;
        }
    
        this->onepole_tilde_04_dspsetup(forceDSPSetup);
        this->onepole_tilde_02_dspsetup(forceDSPSetup);
        this->onepole_tilde_01_dspsetup(forceDSPSetup);
        this->onepole_tilde_03_dspsetup(forceDSPSetup);
    
        if (sampleRateChanged)
            this->onSampleRateChanged(sampleRate);
    }
    
    number msToSamps(MillisecondTime ms, number sampleRate) {
        return ms * sampleRate * 0.001;
    }
    
    MillisecondTime sampsToMs(SampleIndex samps) {
        return samps * (this->invsr * 1000);
    }
    
    Index getNumInputChannels() const {
        return 2;
    }
    
    Index getNumOutputChannels() const {
        return 2;
    }
    
    void getPreset(PatcherStateInterface& ) {}
    
    void setPreset(MillisecondTime , PatcherStateInterface& ) {}
    
    void setParameterValue(ParameterIndex , ParameterValue , MillisecondTime ) {}
    
    void processParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
        this->setParameterValue(index, value, time);
    }
    
    void processParameterBangEvent(ParameterIndex index, MillisecondTime time) {
        this->setParameterValue(index, this->getParameterValue(index), time);
    }
    
    void processNormalizedParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
        this->setParameterValueNormalized(index, value, time);
    }
    
    ParameterValue getParameterValue(ParameterIndex index)  {
        switch (index) {
        default:
            {
            return 0;
            }
        }
    }
    
    ParameterIndex getNumSignalInParameters() const {
        return 0;
    }
    
    ParameterIndex getNumSignalOutParameters() const {
        return 0;
    }
    
    ParameterIndex getNumParameters() const {
        return 0;
    }
    
    ConstCharPointer getParameterName(ParameterIndex index) const {
        switch (index) {
        default:
            {
            return "bogus";
            }
        }
    }
    
    ConstCharPointer getParameterId(ParameterIndex index) const {
        switch (index) {
        default:
            {
            return "bogus";
            }
        }
    }
    
    void getParameterInfo(ParameterIndex , ParameterInfo * ) const {}
    
    ParameterValue applyStepsToNormalizedParameterValue(ParameterValue normalizedValue, int steps) const {
        if (steps == 1) {
            if (normalizedValue > 0) {
                normalizedValue = 1.;
            }
        } else {
            ParameterValue oneStep = (number)1. / (steps - 1);
            ParameterValue numberOfSteps = rnbo_fround(normalizedValue / oneStep * 1 / (number)1) * (number)1;
            normalizedValue = numberOfSteps * oneStep;
        }
    
        return normalizedValue;
    }
    
    ParameterValue convertToNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
        switch (index) {
        default:
            {
            return value;
            }
        }
    }
    
    ParameterValue convertFromNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
        value = (value < 0 ? 0 : (value > 1 ? 1 : value));
    
        switch (index) {
        default:
            {
            return value;
            }
        }
    }
    
    ParameterValue constrainParameterValue(ParameterIndex index, ParameterValue value) const {
        switch (index) {
        default:
            {
            return value;
            }
        }
    }
    
    void processNumMessage(MessageTag tag, MessageTag objectId, MillisecondTime time, number payload) {
        this->updateTime(time, (ENGINE*)nullptr);
    
        switch (tag) {
        case TAG("valin"):
            {
            if (TAG("EQ&Volume~/number_obj-21") == objectId)
                this->numberobj_07_valin_set(payload);
    
            if (TAG("EQ&Volume~/number_obj-12") == objectId)
                this->numberobj_08_valin_set(payload);
    
            if (TAG("EQ&Volume~/number_obj-45") == objectId)
                this->numberobj_09_valin_set(payload);
    
            if (TAG("EQ&Volume~/number_obj-37") == objectId)
                this->numberobj_10_valin_set(payload);
    
            if (TAG("EQ&Volume~/number_obj-40") == objectId)
                this->numberobj_11_valin_set(payload);
    
            if (TAG("EQ&Volume~/number_obj-27") == objectId)
                this->numberobj_12_valin_set(payload);
    
            if (TAG("EQ&Volume~/number_obj-32") == objectId)
                this->numberobj_13_valin_set(payload);
    
            break;
            }
        case TAG("format"):
            {
            if (TAG("EQ&Volume~/number_obj-21") == objectId)
                this->numberobj_07_format_set(payload);
    
            if (TAG("EQ&Volume~/number_obj-12") == objectId)
                this->numberobj_08_format_set(payload);
    
            if (TAG("EQ&Volume~/number_obj-45") == objectId)
                this->numberobj_09_format_set(payload);
    
            if (TAG("EQ&Volume~/number_obj-37") == objectId)
                this->numberobj_10_format_set(payload);
    
            if (TAG("EQ&Volume~/number_obj-40") == objectId)
                this->numberobj_11_format_set(payload);
    
            if (TAG("EQ&Volume~/number_obj-27") == objectId)
                this->numberobj_12_format_set(payload);
    
            if (TAG("EQ&Volume~/number_obj-32") == objectId)
                this->numberobj_13_format_set(payload);
    
            break;
            }
        }
    }
    
    void processListMessage(MessageTag , MessageTag , MillisecondTime , const list& ) {}
    
    void processBangMessage(MessageTag , MessageTag , MillisecondTime ) {}
    
    MessageTagInfo resolveTag(MessageTag tag) const {
        switch (tag) {
        case TAG("valout"):
            {
            return "valout";
            }
        case TAG("EQ&Volume~/number_obj-21"):
            {
            return "EQ&Volume~/number_obj-21";
            }
        case TAG("setup"):
            {
            return "setup";
            }
        case TAG("EQ&Volume~/number_obj-12"):
            {
            return "EQ&Volume~/number_obj-12";
            }
        case TAG("EQ&Volume~/number_obj-45"):
            {
            return "EQ&Volume~/number_obj-45";
            }
        case TAG("EQ&Volume~/number_obj-37"):
            {
            return "EQ&Volume~/number_obj-37";
            }
        case TAG("EQ&Volume~/number_obj-40"):
            {
            return "EQ&Volume~/number_obj-40";
            }
        case TAG("EQ&Volume~/number_obj-27"):
            {
            return "EQ&Volume~/number_obj-27";
            }
        case TAG("EQ&Volume~/number_obj-32"):
            {
            return "EQ&Volume~/number_obj-32";
            }
        case TAG("valin"):
            {
            return "valin";
            }
        case TAG("format"):
            {
            return "format";
            }
        }
    
        return nullptr;
    }
    
    DataRef* getDataRef(DataRefIndex index)  {
        switch (index) {
        default:
            {
            return nullptr;
            }
        }
    }
    
    DataRefIndex getNumDataRefs() const {
        return 0;
    }
    
    void processDataViewUpdate(DataRefIndex , MillisecondTime ) {}
    
    void initialize() {
        RNBO_ASSERT(!this->_isInitialized);
        this->assign_defaults();
        this->applyState();
        this->_isInitialized = true;
    }
    
    protected:
    
    void updateTime(MillisecondTime time, INTERNALENGINE*, bool inProcess = false) {
    	if (time == TimeNow) time = getTopLevelPatcher()->getPatcherTime();
    	getTopLevelPatcher()->processInternalEvents(time);
    	updateTime(time, (EXTERNALENGINE*)nullptr);
    }
    
    RNBOSubpatcher_246* operator->() {
        return this;
    }
    const RNBOSubpatcher_246* operator->() const {
        return this;
    }
    virtual rnbomatic* getPatcher() const {
        return static_cast<rnbomatic *>(_parentPatcher);
    }
    
    rnbomatic* getTopLevelPatcher() {
        return this->getPatcher()->getTopLevelPatcher();
    }
    
    void cancelClockEvents()
    {
        getEngine()->flushClockEvents(this, -1621164530, false);
        getEngine()->flushClockEvents(this, -62043057, false);
        getEngine()->flushClockEvents(this, 1114681293, false);
    }
    
    inline number safediv(number num, number denom) {
        return (denom == 0.0 ? 0.0 : num / denom);
    }
    
    number safepow(number base, number exponent) {
        return fixnan(rnbo_pow(base, exponent));
    }
    
    number scale(
        number x,
        number lowin,
        number hiin,
        number lowout,
        number highout,
        number pow
    ) {
        auto inscale = this->safediv(1., hiin - lowin);
        number outdiff = highout - lowout;
        number value = (x - lowin) * inscale;
    
        if (pow != 1) {
            if (value > 0)
                value = this->safepow(value, pow);
            else
                value = -this->safepow(-value, pow);
        }
    
        value = value * outdiff + lowout;
        return value;
    }
    
    UInt64 currentsampletime() {
        return this->audioProcessSampleCount + this->sampleOffsetIntoNextAudioBuffer;
    }
    
    number mstosamps(MillisecondTime ms) {
        return ms * this->sr * 0.001;
    }
    
    MillisecondTime getPatcherTime() const {
        return this->_currentTime;
    }
    
    void numberobj_07_valin_set(number v) {
        this->numberobj_07_value_set(v);
    }
    
    void numberobj_07_format_set(number v) {
        this->numberobj_07_currentFormat = trunc((v > 6 ? 6 : (v < 0 ? 0 : v)));
    }
    
    void eventinlet_03_out1_bang_bang() {
        this->numberobj_08_value_bang();
    }
    
    template<typename LISTTYPE> void eventinlet_03_out1_list_set(const LISTTYPE& v) {
        {
            number converted = (v->length > 0 ? v[0] : 0);
            this->numberobj_08_value_set(converted);
        }
    }
    
    void numberobj_08_valin_set(number v) {
        this->numberobj_08_value_set(v);
    }
    
    void numberobj_08_format_set(number v) {
        this->numberobj_08_currentFormat = trunc((v > 6 ? 6 : (v < 0 ? 0 : v)));
    }
    
    void numberobj_09_valin_set(number v) {
        this->numberobj_09_value_set(v);
    }
    
    void numberobj_09_format_set(number v) {
        this->numberobj_09_currentFormat = trunc((v > 6 ? 6 : (v < 0 ? 0 : v)));
    }
    
    void numberobj_10_valin_set(number v) {
        this->numberobj_10_value_set(v);
    }
    
    void numberobj_10_format_set(number v) {
        this->numberobj_10_currentFormat = trunc((v > 6 ? 6 : (v < 0 ? 0 : v)));
    }
    
    void eventinlet_04_out1_bang_bang() {
        this->numberobj_11_value_bang();
    }
    
    template<typename LISTTYPE> void eventinlet_04_out1_list_set(const LISTTYPE& v) {
        {
            number converted = (v->length > 0 ? v[0] : 0);
            this->numberobj_11_value_set(converted);
        }
    }
    
    void numberobj_11_valin_set(number v) {
        this->numberobj_11_value_set(v);
    }
    
    void numberobj_11_format_set(number v) {
        this->numberobj_11_currentFormat = trunc((v > 6 ? 6 : (v < 0 ? 0 : v)));
    }
    
    void numberobj_12_valin_set(number v) {
        this->numberobj_12_value_set(v);
    }
    
    void numberobj_12_format_set(number v) {
        this->numberobj_12_currentFormat = trunc((v > 6 ? 6 : (v < 0 ? 0 : v)));
    }
    
    void eventinlet_05_out1_bang_bang() {
        this->numberobj_13_value_bang();
    }
    
    template<typename LISTTYPE> void eventinlet_05_out1_list_set(const LISTTYPE& v) {
        {
            number converted = (v->length > 0 ? v[0] : 0);
            this->numberobj_13_value_set(converted);
        }
    }
    
    void numberobj_13_valin_set(number v) {
        this->numberobj_13_value_set(v);
    }
    
    void numberobj_13_format_set(number v) {
        this->numberobj_13_currentFormat = trunc((v > 6 ? 6 : (v < 0 ? 0 : v)));
    }
    
    void linetilde_01_target_bang() {}
    
    void linetilde_02_target_bang() {}
    
    void linetilde_03_target_bang() {}
    
    void deallocateSignals() {
        Index i;
    
        for (i = 0; i < 5; i++) {
            this->signals[i] = freeSignal(this->signals[i]);
        }
    
        this->zeroBuffer = freeSignal(this->zeroBuffer);
        this->dummyBuffer = freeSignal(this->dummyBuffer);
    }
    
    Index getMaxBlockSize() const {
        return this->maxvs;
    }
    
    number getSampleRate() const {
        return this->sr;
    }
    
    bool hasFixedVectorSize() const {
        return false;
    }
    
    void setProbingTarget(MessageTag ) {}
    
    void initializeObjects() {
        this->numberobj_07_init();
        this->numberobj_08_init();
        this->numberobj_09_init();
        this->numberobj_10_init();
        this->numberobj_11_init();
        this->numberobj_12_init();
        this->numberobj_13_init();
    }
    
    Index getIsMuted()  {
        return this->isMuted;
    }
    
    void setIsMuted(Index v)  {
        this->isMuted = v;
    }
    
    void onSampleRateChanged(double ) {}
    
    void extractState(PatcherStateInterface& ) {}
    
    void applyState() {}
    
    void setParameterOffset(ParameterIndex offset) {
        this->parameterOffset = offset;
    }
    
    void processClockEvent(MillisecondTime time, ClockId index, bool hasValue, ParameterValue value) {
        RNBO_UNUSED(value);
        RNBO_UNUSED(hasValue);
        this->updateTime(time, (ENGINE*)nullptr);
    
        switch (index) {
        case -1621164530:
            {
            this->linetilde_01_target_bang();
            break;
            }
        case -62043057:
            {
            this->linetilde_02_target_bang();
            break;
            }
        case 1114681293:
            {
            this->linetilde_03_target_bang();
            break;
            }
        }
    }
    
    void processOutletAtCurrentTime(EngineLink* , OutletIndex , ParameterValue ) {}
    
    void processOutletEvent(
        EngineLink* sender,
        OutletIndex index,
        ParameterValue value,
        MillisecondTime time
    ) {
        this->updateTime(time, (ENGINE*)nullptr);
        this->processOutletAtCurrentTime(sender, index, value);
    }
    
    void sendOutlet(OutletIndex index, ParameterValue value) {
        this->getEngine()->sendOutlet(this, index, value);
    }
    
    void startup() {}
    
    void fillDataRef(DataRefIndex , DataRef& ) {}
    
    void allocateDataRefs() {}
    
    void linetilde_01_time_set(number v) {
        this->linetilde_01_time = v;
    }
    
    template<typename LISTTYPE> void linetilde_01_segments_set(const LISTTYPE& v) {
        this->linetilde_01_segments = jsCreateListCopy(v);
    
        if ((bool)(v->length)) {
            if (v->length == 1 && this->linetilde_01_time == 0) {
                this->linetilde_01_activeRamps->length = 0;
                this->linetilde_01_currentValue = v[0];
            } else {
                auto currentTime = this->currentsampletime();
                number lastRampValue = this->linetilde_01_currentValue;
                number rampEnd = currentTime - this->sampleOffsetIntoNextAudioBuffer;
    
                for (Index i = 0; i < this->linetilde_01_activeRamps->length; i += 3) {
                    rampEnd = this->linetilde_01_activeRamps[(Index)(i + 2)];
    
                    if (rampEnd > currentTime) {
                        this->linetilde_01_activeRamps[(Index)(i + 2)] = currentTime;
                        number diff = rampEnd - currentTime;
                        number valueDiff = diff * this->linetilde_01_activeRamps[(Index)(i + 1)];
                        lastRampValue = this->linetilde_01_activeRamps[(Index)i] - valueDiff;
                        this->linetilde_01_activeRamps[(Index)i] = lastRampValue;
                        this->linetilde_01_activeRamps->length = i + 3;
                        rampEnd = currentTime;
                    } else {
                        lastRampValue = this->linetilde_01_activeRamps[(Index)i];
                    }
                }
    
                if (rampEnd < currentTime) {
                    this->linetilde_01_activeRamps->push(lastRampValue);
                    this->linetilde_01_activeRamps->push(0);
                    this->linetilde_01_activeRamps->push(currentTime);
                }
    
                number lastRampEnd = currentTime;
    
                for (Index i = 0; i < v->length; i += 2) {
                    number destinationValue = v[(Index)i];
                    number inc = 0;
                    number rampTimeInSamples;
    
                    if (v->length > i + 1) {
                        rampTimeInSamples = this->mstosamps(v[(Index)(i + 1)]);
    
                        if ((bool)(this->linetilde_01_keepramp)) {
                            this->linetilde_01_time_set(v[(Index)(i + 1)]);
                        }
                    } else {
                        rampTimeInSamples = this->mstosamps(this->linetilde_01_time);
                    }
    
                    if (rampTimeInSamples <= 0) {
                        rampTimeInSamples = 1;
                    }
    
                    inc = (destinationValue - lastRampValue) / rampTimeInSamples;
                    lastRampEnd += rampTimeInSamples;
                    this->linetilde_01_activeRamps->push(destinationValue);
                    this->linetilde_01_activeRamps->push(inc);
                    this->linetilde_01_activeRamps->push(lastRampEnd);
                    lastRampValue = destinationValue;
                }
            }
        }
    }
    
    void numberobj_07_output_set(number v) {
        {
            listbase<number, 1> converted = {v};
            this->linetilde_01_segments_set(converted);
        }
    }
    
    void numberobj_07_value_set(number v) {
        this->numberobj_07_value_setter(v);
        v = this->numberobj_07_value;
        number localvalue = v;
    
        if (this->numberobj_07_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_07_output_set(localvalue);
    }
    
    template<typename LISTTYPE> void scale_04_out_set(const LISTTYPE& v) {
        this->scale_04_out = jsCreateListCopy(v);
    
        {
            number converted = (v->length > 0 ? v[0] : 0);
            this->numberobj_07_value_set(converted);
        }
    }
    
    template<typename LISTTYPE> void scale_04_input_set(const LISTTYPE& v) {
        this->scale_04_input = jsCreateListCopy(v);
        list tmp = {};
    
        for (Index i = 0; i < v->length; i++) {
            tmp->push(this->scale(
                v[(Index)i],
                this->scale_04_inlow,
                this->scale_04_inhigh,
                this->scale_04_outlow,
                this->scale_04_outhigh,
                this->scale_04_power
            ));
        }
    
        this->scale_04_out_set(tmp);
    }
    
    void numberobj_08_output_set(number v) {
        {
            listbase<number, 1> converted = {v};
            this->scale_04_input_set(converted);
        }
    }
    
    void numberobj_08_value_set(number v) {
        this->numberobj_08_value_setter(v);
        v = this->numberobj_08_value;
        number localvalue = v;
    
        if (this->numberobj_08_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_08_output_set(localvalue);
    }
    
    void eventinlet_03_out1_number_set(number v) {
        this->numberobj_08_value_set(v);
    }
    
    void linetilde_03_time_set(number v) {
        this->linetilde_03_time = v;
    }
    
    template<typename LISTTYPE> void linetilde_03_segments_set(const LISTTYPE& v) {
        this->linetilde_03_segments = jsCreateListCopy(v);
    
        if ((bool)(v->length)) {
            if (v->length == 1 && this->linetilde_03_time == 0) {
                this->linetilde_03_activeRamps->length = 0;
                this->linetilde_03_currentValue = v[0];
            } else {
                auto currentTime = this->currentsampletime();
                number lastRampValue = this->linetilde_03_currentValue;
                number rampEnd = currentTime - this->sampleOffsetIntoNextAudioBuffer;
    
                for (Index i = 0; i < this->linetilde_03_activeRamps->length; i += 3) {
                    rampEnd = this->linetilde_03_activeRamps[(Index)(i + 2)];
    
                    if (rampEnd > currentTime) {
                        this->linetilde_03_activeRamps[(Index)(i + 2)] = currentTime;
                        number diff = rampEnd - currentTime;
                        number valueDiff = diff * this->linetilde_03_activeRamps[(Index)(i + 1)];
                        lastRampValue = this->linetilde_03_activeRamps[(Index)i] - valueDiff;
                        this->linetilde_03_activeRamps[(Index)i] = lastRampValue;
                        this->linetilde_03_activeRamps->length = i + 3;
                        rampEnd = currentTime;
                    } else {
                        lastRampValue = this->linetilde_03_activeRamps[(Index)i];
                    }
                }
    
                if (rampEnd < currentTime) {
                    this->linetilde_03_activeRamps->push(lastRampValue);
                    this->linetilde_03_activeRamps->push(0);
                    this->linetilde_03_activeRamps->push(currentTime);
                }
    
                number lastRampEnd = currentTime;
    
                for (Index i = 0; i < v->length; i += 2) {
                    number destinationValue = v[(Index)i];
                    number inc = 0;
                    number rampTimeInSamples;
    
                    if (v->length > i + 1) {
                        rampTimeInSamples = this->mstosamps(v[(Index)(i + 1)]);
    
                        if ((bool)(this->linetilde_03_keepramp)) {
                            this->linetilde_03_time_set(v[(Index)(i + 1)]);
                        }
                    } else {
                        rampTimeInSamples = this->mstosamps(this->linetilde_03_time);
                    }
    
                    if (rampTimeInSamples <= 0) {
                        rampTimeInSamples = 1;
                    }
    
                    inc = (destinationValue - lastRampValue) / rampTimeInSamples;
                    lastRampEnd += rampTimeInSamples;
                    this->linetilde_03_activeRamps->push(destinationValue);
                    this->linetilde_03_activeRamps->push(inc);
                    this->linetilde_03_activeRamps->push(lastRampEnd);
                    lastRampValue = destinationValue;
                }
            }
        }
    }
    
    void numberobj_12_output_set(number v) {
        {
            listbase<number, 1> converted = {v};
            this->linetilde_03_segments_set(converted);
        }
    }
    
    void numberobj_12_value_set(number v) {
        this->numberobj_12_value_setter(v);
        v = this->numberobj_12_value;
        number localvalue = v;
    
        if (this->numberobj_12_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_12_output_set(localvalue);
    }
    
    template<typename LISTTYPE> void scale_06_out_set(const LISTTYPE& v) {
        this->scale_06_out = jsCreateListCopy(v);
    
        {
            number converted = (v->length > 0 ? v[0] : 0);
            this->numberobj_12_value_set(converted);
        }
    }
    
    template<typename LISTTYPE> void scale_06_input_set(const LISTTYPE& v) {
        this->scale_06_input = jsCreateListCopy(v);
        list tmp = {};
    
        for (Index i = 0; i < v->length; i++) {
            tmp->push(this->scale(
                v[(Index)i],
                this->scale_06_inlow,
                this->scale_06_inhigh,
                this->scale_06_outlow,
                this->scale_06_outhigh,
                this->scale_06_power
            ));
        }
    
        this->scale_06_out_set(tmp);
    }
    
    void numberobj_13_output_set(number v) {
        {
            listbase<number, 1> converted = {v};
            this->scale_06_input_set(converted);
        }
    }
    
    void numberobj_13_value_set(number v) {
        this->numberobj_13_value_setter(v);
        v = this->numberobj_13_value;
        number localvalue = v;
    
        if (this->numberobj_13_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_13_output_set(localvalue);
    }
    
    void eventinlet_05_out1_number_set(number v) {
        this->numberobj_13_value_set(v);
    }
    
    void linetilde_02_time_set(number v) {
        this->linetilde_02_time = v;
    }
    
    template<typename LISTTYPE> void linetilde_02_segments_set(const LISTTYPE& v) {
        this->linetilde_02_segments = jsCreateListCopy(v);
    
        if ((bool)(v->length)) {
            if (v->length == 1 && this->linetilde_02_time == 0) {
                this->linetilde_02_activeRamps->length = 0;
                this->linetilde_02_currentValue = v[0];
            } else {
                auto currentTime = this->currentsampletime();
                number lastRampValue = this->linetilde_02_currentValue;
                number rampEnd = currentTime - this->sampleOffsetIntoNextAudioBuffer;
    
                for (Index i = 0; i < this->linetilde_02_activeRamps->length; i += 3) {
                    rampEnd = this->linetilde_02_activeRamps[(Index)(i + 2)];
    
                    if (rampEnd > currentTime) {
                        this->linetilde_02_activeRamps[(Index)(i + 2)] = currentTime;
                        number diff = rampEnd - currentTime;
                        number valueDiff = diff * this->linetilde_02_activeRamps[(Index)(i + 1)];
                        lastRampValue = this->linetilde_02_activeRamps[(Index)i] - valueDiff;
                        this->linetilde_02_activeRamps[(Index)i] = lastRampValue;
                        this->linetilde_02_activeRamps->length = i + 3;
                        rampEnd = currentTime;
                    } else {
                        lastRampValue = this->linetilde_02_activeRamps[(Index)i];
                    }
                }
    
                if (rampEnd < currentTime) {
                    this->linetilde_02_activeRamps->push(lastRampValue);
                    this->linetilde_02_activeRamps->push(0);
                    this->linetilde_02_activeRamps->push(currentTime);
                }
    
                number lastRampEnd = currentTime;
    
                for (Index i = 0; i < v->length; i += 2) {
                    number destinationValue = v[(Index)i];
                    number inc = 0;
                    number rampTimeInSamples;
    
                    if (v->length > i + 1) {
                        rampTimeInSamples = this->mstosamps(v[(Index)(i + 1)]);
    
                        if ((bool)(this->linetilde_02_keepramp)) {
                            this->linetilde_02_time_set(v[(Index)(i + 1)]);
                        }
                    } else {
                        rampTimeInSamples = this->mstosamps(this->linetilde_02_time);
                    }
    
                    if (rampTimeInSamples <= 0) {
                        rampTimeInSamples = 1;
                    }
    
                    inc = (destinationValue - lastRampValue) / rampTimeInSamples;
                    lastRampEnd += rampTimeInSamples;
                    this->linetilde_02_activeRamps->push(destinationValue);
                    this->linetilde_02_activeRamps->push(inc);
                    this->linetilde_02_activeRamps->push(lastRampEnd);
                    lastRampValue = destinationValue;
                }
            }
        }
    }
    
    void numberobj_09_output_set(number v) {
        {
            listbase<number, 1> converted = {v};
            this->linetilde_02_segments_set(converted);
        }
    }
    
    void numberobj_09_value_set(number v) {
        this->numberobj_09_value_setter(v);
        v = this->numberobj_09_value;
        number localvalue = v;
    
        if (this->numberobj_09_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_09_output_set(localvalue);
    }
    
    void expr_05_out1_set(number v) {
        this->expr_05_out1 = v;
        this->numberobj_09_value_set(this->expr_05_out1);
    }
    
    void expr_05_in1_set(number in1) {
        this->expr_05_in1 = in1;
        this->expr_05_out1_set(rnbo_pow(10, this->expr_05_in1 * 0.05));//#map:EQ&Volume~/dbtoa_obj-44:1
    }
    
    void numberobj_10_output_set(number v) {
        this->expr_05_in1_set(v);
    }
    
    void numberobj_10_value_set(number v) {
        this->numberobj_10_value_setter(v);
        v = this->numberobj_10_value;
        number localvalue = v;
    
        if (this->numberobj_10_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_10_output_set(localvalue);
    }
    
    template<typename LISTTYPE> void scale_05_out_set(const LISTTYPE& v) {
        this->scale_05_out = jsCreateListCopy(v);
    
        {
            number converted = (v->length > 0 ? v[0] : 0);
            this->numberobj_10_value_set(converted);
        }
    }
    
    template<typename LISTTYPE> void scale_05_input_set(const LISTTYPE& v) {
        this->scale_05_input = jsCreateListCopy(v);
        list tmp = {};
    
        for (Index i = 0; i < v->length; i++) {
            tmp->push(this->scale(
                v[(Index)i],
                this->scale_05_inlow,
                this->scale_05_inhigh,
                this->scale_05_outlow,
                this->scale_05_outhigh,
                this->scale_05_power
            ));
        }
    
        this->scale_05_out_set(tmp);
    }
    
    void numberobj_11_output_set(number v) {
        {
            listbase<number, 1> converted = {v};
            this->scale_05_input_set(converted);
        }
    }
    
    void numberobj_11_value_set(number v) {
        this->numberobj_11_value_setter(v);
        v = this->numberobj_11_value;
        number localvalue = v;
    
        if (this->numberobj_11_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_11_output_set(localvalue);
    }
    
    void eventinlet_04_out1_number_set(number v) {
        this->numberobj_11_value_set(v);
    }
    
    void numberobj_08_value_bang() {
        number v = this->numberobj_08_value;
        number localvalue = v;
    
        if (this->numberobj_08_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_08_output_set(localvalue);
    }
    
    void numberobj_11_value_bang() {
        number v = this->numberobj_11_value;
        number localvalue = v;
    
        if (this->numberobj_11_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_11_output_set(localvalue);
    }
    
    void numberobj_13_value_bang() {
        number v = this->numberobj_13_value;
        number localvalue = v;
    
        if (this->numberobj_13_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_13_output_set(localvalue);
    }
    
    void linetilde_01_perform(SampleValue * out, Index n) {
        auto __linetilde_01_time = this->linetilde_01_time;
        auto __linetilde_01_keepramp = this->linetilde_01_keepramp;
        auto __linetilde_01_currentValue = this->linetilde_01_currentValue;
        Index i = 0;
    
        if ((bool)(this->linetilde_01_activeRamps->length)) {
            while ((bool)(this->linetilde_01_activeRamps->length) && i < n) {
                number destinationValue = this->linetilde_01_activeRamps[0];
                number inc = this->linetilde_01_activeRamps[1];
                number rampTimeInSamples = this->linetilde_01_activeRamps[2] - this->audioProcessSampleCount - i;
                number val = __linetilde_01_currentValue;
    
                while (rampTimeInSamples > 0 && i < n) {
                    out[(Index)i] = val;
                    val += inc;
                    i++;
                    rampTimeInSamples--;
                }
    
                if (rampTimeInSamples <= 0) {
                    val = destinationValue;
                    this->linetilde_01_activeRamps->splice(0, 3);
    
                    if ((bool)(!(bool)(this->linetilde_01_activeRamps->length))) {
                        this->getEngine()->scheduleClockEventWithValue(
                            this,
                            -1621164530,
                            this->sampsToMs((SampleIndex)(this->vs)) + this->_currentTime,
                            0
                        );;
    
                        if ((bool)(!(bool)(__linetilde_01_keepramp))) {
                            __linetilde_01_time = 0;
                        }
                    }
                }
    
                __linetilde_01_currentValue = val;
            }
        }
    
        while (i < n) {
            out[(Index)i] = __linetilde_01_currentValue;
            i++;
        }
    
        this->linetilde_01_currentValue = __linetilde_01_currentValue;
        this->linetilde_01_time = __linetilde_01_time;
    }
    
    void onepole_tilde_04_perform(const Sample * x, const Sample * freqInHz, SampleValue * out1, Index n) {
        auto __onepole_tilde_04_lastY = this->onepole_tilde_04_lastY;
        auto __onepole_tilde_04_b1 = this->onepole_tilde_04_b1;
        auto __onepole_tilde_04_a0 = this->onepole_tilde_04_a0;
        auto __onepole_tilde_04_needsUpdate = this->onepole_tilde_04_needsUpdate;
        auto __onepole_tilde_04_freq = this->onepole_tilde_04_freq;
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            if (__onepole_tilde_04_freq != freqInHz[(Index)i] || (bool)(__onepole_tilde_04_needsUpdate)) {
                __onepole_tilde_04_freq = freqInHz[(Index)i];
                __onepole_tilde_04_a0 = 1 - rnbo_exp(-6.28318530717958647692 * freqInHz[(Index)i] / this->sr);
                __onepole_tilde_04_a0 = (__onepole_tilde_04_a0 > 0.99999 ? 0.99999 : (__onepole_tilde_04_a0 < 0.00001 ? 0.00001 : __onepole_tilde_04_a0));
                __onepole_tilde_04_b1 = 1 - __onepole_tilde_04_a0;
                __onepole_tilde_04_needsUpdate = false;
            }
    
            __onepole_tilde_04_lastY = __onepole_tilde_04_a0 * x[(Index)i] + __onepole_tilde_04_b1 * __onepole_tilde_04_lastY;
            out1[(Index)i] = __onepole_tilde_04_lastY;
        }
    
        this->onepole_tilde_04_freq = __onepole_tilde_04_freq;
        this->onepole_tilde_04_needsUpdate = __onepole_tilde_04_needsUpdate;
        this->onepole_tilde_04_a0 = __onepole_tilde_04_a0;
        this->onepole_tilde_04_b1 = __onepole_tilde_04_b1;
        this->onepole_tilde_04_lastY = __onepole_tilde_04_lastY;
    }
    
    void dspexpr_24_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] - in2[(Index)i];//#map:_###_obj_###_:1
        }
    }
    
    void onepole_tilde_02_perform(const Sample * x, const Sample * freqInHz, SampleValue * out1, Index n) {
        auto __onepole_tilde_02_lastY = this->onepole_tilde_02_lastY;
        auto __onepole_tilde_02_b1 = this->onepole_tilde_02_b1;
        auto __onepole_tilde_02_a0 = this->onepole_tilde_02_a0;
        auto __onepole_tilde_02_needsUpdate = this->onepole_tilde_02_needsUpdate;
        auto __onepole_tilde_02_freq = this->onepole_tilde_02_freq;
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            if (__onepole_tilde_02_freq != freqInHz[(Index)i] || (bool)(__onepole_tilde_02_needsUpdate)) {
                __onepole_tilde_02_freq = freqInHz[(Index)i];
                __onepole_tilde_02_a0 = 1 - rnbo_exp(-6.28318530717958647692 * freqInHz[(Index)i] / this->sr);
                __onepole_tilde_02_a0 = (__onepole_tilde_02_a0 > 0.99999 ? 0.99999 : (__onepole_tilde_02_a0 < 0.00001 ? 0.00001 : __onepole_tilde_02_a0));
                __onepole_tilde_02_b1 = 1 - __onepole_tilde_02_a0;
                __onepole_tilde_02_needsUpdate = false;
            }
    
            __onepole_tilde_02_lastY = __onepole_tilde_02_a0 * x[(Index)i] + __onepole_tilde_02_b1 * __onepole_tilde_02_lastY;
            out1[(Index)i] = __onepole_tilde_02_lastY;
        }
    
        this->onepole_tilde_02_freq = __onepole_tilde_02_freq;
        this->onepole_tilde_02_needsUpdate = __onepole_tilde_02_needsUpdate;
        this->onepole_tilde_02_a0 = __onepole_tilde_02_a0;
        this->onepole_tilde_02_b1 = __onepole_tilde_02_b1;
        this->onepole_tilde_02_lastY = __onepole_tilde_02_lastY;
    }
    
    void dspexpr_21_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] - in2[(Index)i];//#map:_###_obj_###_:1
        }
    }
    
    void linetilde_02_perform(SampleValue * out, Index n) {
        auto __linetilde_02_time = this->linetilde_02_time;
        auto __linetilde_02_keepramp = this->linetilde_02_keepramp;
        auto __linetilde_02_currentValue = this->linetilde_02_currentValue;
        Index i = 0;
    
        if ((bool)(this->linetilde_02_activeRamps->length)) {
            while ((bool)(this->linetilde_02_activeRamps->length) && i < n) {
                number destinationValue = this->linetilde_02_activeRamps[0];
                number inc = this->linetilde_02_activeRamps[1];
                number rampTimeInSamples = this->linetilde_02_activeRamps[2] - this->audioProcessSampleCount - i;
                number val = __linetilde_02_currentValue;
    
                while (rampTimeInSamples > 0 && i < n) {
                    out[(Index)i] = val;
                    val += inc;
                    i++;
                    rampTimeInSamples--;
                }
    
                if (rampTimeInSamples <= 0) {
                    val = destinationValue;
                    this->linetilde_02_activeRamps->splice(0, 3);
    
                    if ((bool)(!(bool)(this->linetilde_02_activeRamps->length))) {
                        this->getEngine()->scheduleClockEventWithValue(
                            this,
                            -62043057,
                            this->sampsToMs((SampleIndex)(this->vs)) + this->_currentTime,
                            0
                        );;
    
                        if ((bool)(!(bool)(__linetilde_02_keepramp))) {
                            __linetilde_02_time = 0;
                        }
                    }
                }
    
                __linetilde_02_currentValue = val;
            }
        }
    
        while (i < n) {
            out[(Index)i] = __linetilde_02_currentValue;
            i++;
        }
    
        this->linetilde_02_currentValue = __linetilde_02_currentValue;
        this->linetilde_02_time = __linetilde_02_time;
    }
    
    void linetilde_03_perform(SampleValue * out, Index n) {
        auto __linetilde_03_time = this->linetilde_03_time;
        auto __linetilde_03_keepramp = this->linetilde_03_keepramp;
        auto __linetilde_03_currentValue = this->linetilde_03_currentValue;
        Index i = 0;
    
        if ((bool)(this->linetilde_03_activeRamps->length)) {
            while ((bool)(this->linetilde_03_activeRamps->length) && i < n) {
                number destinationValue = this->linetilde_03_activeRamps[0];
                number inc = this->linetilde_03_activeRamps[1];
                number rampTimeInSamples = this->linetilde_03_activeRamps[2] - this->audioProcessSampleCount - i;
                number val = __linetilde_03_currentValue;
    
                while (rampTimeInSamples > 0 && i < n) {
                    out[(Index)i] = val;
                    val += inc;
                    i++;
                    rampTimeInSamples--;
                }
    
                if (rampTimeInSamples <= 0) {
                    val = destinationValue;
                    this->linetilde_03_activeRamps->splice(0, 3);
    
                    if ((bool)(!(bool)(this->linetilde_03_activeRamps->length))) {
                        this->getEngine()->scheduleClockEventWithValue(
                            this,
                            1114681293,
                            this->sampsToMs((SampleIndex)(this->vs)) + this->_currentTime,
                            0
                        );;
    
                        if ((bool)(!(bool)(__linetilde_03_keepramp))) {
                            __linetilde_03_time = 0;
                        }
                    }
                }
    
                __linetilde_03_currentValue = val;
            }
        }
    
        while (i < n) {
            out[(Index)i] = __linetilde_03_currentValue;
            i++;
        }
    
        this->linetilde_03_currentValue = __linetilde_03_currentValue;
        this->linetilde_03_time = __linetilde_03_time;
    }
    
    void onepole_tilde_01_perform(const Sample * x, const Sample * freqInHz, SampleValue * out1, Index n) {
        auto __onepole_tilde_01_lastY = this->onepole_tilde_01_lastY;
        auto __onepole_tilde_01_b1 = this->onepole_tilde_01_b1;
        auto __onepole_tilde_01_a0 = this->onepole_tilde_01_a0;
        auto __onepole_tilde_01_needsUpdate = this->onepole_tilde_01_needsUpdate;
        auto __onepole_tilde_01_freq = this->onepole_tilde_01_freq;
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            if (__onepole_tilde_01_freq != freqInHz[(Index)i] || (bool)(__onepole_tilde_01_needsUpdate)) {
                __onepole_tilde_01_freq = freqInHz[(Index)i];
                __onepole_tilde_01_a0 = 1 - rnbo_exp(-6.28318530717958647692 * freqInHz[(Index)i] / this->sr);
                __onepole_tilde_01_a0 = (__onepole_tilde_01_a0 > 0.99999 ? 0.99999 : (__onepole_tilde_01_a0 < 0.00001 ? 0.00001 : __onepole_tilde_01_a0));
                __onepole_tilde_01_b1 = 1 - __onepole_tilde_01_a0;
                __onepole_tilde_01_needsUpdate = false;
            }
    
            __onepole_tilde_01_lastY = __onepole_tilde_01_a0 * x[(Index)i] + __onepole_tilde_01_b1 * __onepole_tilde_01_lastY;
            out1[(Index)i] = __onepole_tilde_01_lastY;
        }
    
        this->onepole_tilde_01_freq = __onepole_tilde_01_freq;
        this->onepole_tilde_01_needsUpdate = __onepole_tilde_01_needsUpdate;
        this->onepole_tilde_01_a0 = __onepole_tilde_01_a0;
        this->onepole_tilde_01_b1 = __onepole_tilde_01_b1;
        this->onepole_tilde_01_lastY = __onepole_tilde_01_lastY;
    }
    
    void dspexpr_20_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] * in2[(Index)i];//#map:_###_obj_###_:1
        }
    }
    
    void dspexpr_19_perform(const Sample * in1, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = rnbo_atan(in1[(Index)i]);//#map:_###_obj_###_:1
        }
    }
    
    void onepole_tilde_03_perform(const Sample * x, const Sample * freqInHz, SampleValue * out1, Index n) {
        auto __onepole_tilde_03_lastY = this->onepole_tilde_03_lastY;
        auto __onepole_tilde_03_b1 = this->onepole_tilde_03_b1;
        auto __onepole_tilde_03_a0 = this->onepole_tilde_03_a0;
        auto __onepole_tilde_03_needsUpdate = this->onepole_tilde_03_needsUpdate;
        auto __onepole_tilde_03_freq = this->onepole_tilde_03_freq;
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            if (__onepole_tilde_03_freq != freqInHz[(Index)i] || (bool)(__onepole_tilde_03_needsUpdate)) {
                __onepole_tilde_03_freq = freqInHz[(Index)i];
                __onepole_tilde_03_a0 = 1 - rnbo_exp(-6.28318530717958647692 * freqInHz[(Index)i] / this->sr);
                __onepole_tilde_03_a0 = (__onepole_tilde_03_a0 > 0.99999 ? 0.99999 : (__onepole_tilde_03_a0 < 0.00001 ? 0.00001 : __onepole_tilde_03_a0));
                __onepole_tilde_03_b1 = 1 - __onepole_tilde_03_a0;
                __onepole_tilde_03_needsUpdate = false;
            }
    
            __onepole_tilde_03_lastY = __onepole_tilde_03_a0 * x[(Index)i] + __onepole_tilde_03_b1 * __onepole_tilde_03_lastY;
            out1[(Index)i] = __onepole_tilde_03_lastY;
        }
    
        this->onepole_tilde_03_freq = __onepole_tilde_03_freq;
        this->onepole_tilde_03_needsUpdate = __onepole_tilde_03_needsUpdate;
        this->onepole_tilde_03_a0 = __onepole_tilde_03_a0;
        this->onepole_tilde_03_b1 = __onepole_tilde_03_b1;
        this->onepole_tilde_03_lastY = __onepole_tilde_03_lastY;
    }
    
    void dspexpr_23_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] * in2[(Index)i];//#map:_###_obj_###_:1
        }
    }
    
    void dspexpr_22_perform(const Sample * in1, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = rnbo_atan(in1[(Index)i]);//#map:_###_obj_###_:1
        }
    }
    
    void stackprotect_perform(Index n) {
        RNBO_UNUSED(n);
        auto __stackprotect_count = this->stackprotect_count;
        __stackprotect_count = 0;
        this->stackprotect_count = __stackprotect_count;
    }
    
    void numberobj_07_value_setter(number v) {
        number localvalue = v;
    
        if (this->numberobj_07_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_07_value = localvalue;
    }
    
    void numberobj_08_value_setter(number v) {
        number localvalue = v;
    
        if (this->numberobj_08_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_08_value = localvalue;
    }
    
    void numberobj_09_value_setter(number v) {
        number localvalue = v;
    
        if (this->numberobj_09_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_09_value = localvalue;
    }
    
    void numberobj_10_value_setter(number v) {
        number localvalue = v;
    
        if (this->numberobj_10_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_10_value = localvalue;
    }
    
    void numberobj_11_value_setter(number v) {
        number localvalue = v;
    
        if (this->numberobj_11_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_11_value = localvalue;
    }
    
    void numberobj_12_value_setter(number v) {
        number localvalue = v;
    
        if (this->numberobj_12_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_12_value = localvalue;
    }
    
    void numberobj_13_value_setter(number v) {
        number localvalue = v;
    
        if (this->numberobj_13_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_13_value = localvalue;
    }
    
    void onepole_tilde_01_reset() {
        this->onepole_tilde_01_lastY = 0;
        this->onepole_tilde_01_a0 = 0;
        this->onepole_tilde_01_b1 = 0;
    }
    
    void onepole_tilde_01_dspsetup(bool force) {
        if ((bool)(this->onepole_tilde_01_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->onepole_tilde_01_needsUpdate = true;
        this->onepole_tilde_01_reset();
        this->onepole_tilde_01_setupDone = true;
    }
    
    void onepole_tilde_02_reset() {
        this->onepole_tilde_02_lastY = 0;
        this->onepole_tilde_02_a0 = 0;
        this->onepole_tilde_02_b1 = 0;
    }
    
    void onepole_tilde_02_dspsetup(bool force) {
        if ((bool)(this->onepole_tilde_02_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->onepole_tilde_02_needsUpdate = true;
        this->onepole_tilde_02_reset();
        this->onepole_tilde_02_setupDone = true;
    }
    
    void onepole_tilde_03_reset() {
        this->onepole_tilde_03_lastY = 0;
        this->onepole_tilde_03_a0 = 0;
        this->onepole_tilde_03_b1 = 0;
    }
    
    void onepole_tilde_03_dspsetup(bool force) {
        if ((bool)(this->onepole_tilde_03_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->onepole_tilde_03_needsUpdate = true;
        this->onepole_tilde_03_reset();
        this->onepole_tilde_03_setupDone = true;
    }
    
    void onepole_tilde_04_reset() {
        this->onepole_tilde_04_lastY = 0;
        this->onepole_tilde_04_a0 = 0;
        this->onepole_tilde_04_b1 = 0;
    }
    
    void onepole_tilde_04_dspsetup(bool force) {
        if ((bool)(this->onepole_tilde_04_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->onepole_tilde_04_needsUpdate = true;
        this->onepole_tilde_04_reset();
        this->onepole_tilde_04_setupDone = true;
    }
    
    void numberobj_07_init() {
        this->numberobj_07_currentFormat = 6;
    }
    
    void numberobj_07_getPresetValue(PatcherStateInterface& preset) {
        preset["value"] = this->numberobj_07_value;
    }
    
    void numberobj_07_setPresetValue(PatcherStateInterface& preset) {
        if ((bool)(stateIsEmpty(preset)))
            return;
    
        this->numberobj_07_value_set(preset["value"]);
    }
    
    void numberobj_08_init() {
        this->numberobj_08_currentFormat = 6;
    }
    
    void numberobj_08_getPresetValue(PatcherStateInterface& preset) {
        preset["value"] = this->numberobj_08_value;
    }
    
    void numberobj_08_setPresetValue(PatcherStateInterface& preset) {
        if ((bool)(stateIsEmpty(preset)))
            return;
    
        this->numberobj_08_value_set(preset["value"]);
    }
    
    void numberobj_09_init() {
        this->numberobj_09_currentFormat = 6;
    }
    
    void numberobj_09_getPresetValue(PatcherStateInterface& preset) {
        preset["value"] = this->numberobj_09_value;
    }
    
    void numberobj_09_setPresetValue(PatcherStateInterface& preset) {
        if ((bool)(stateIsEmpty(preset)))
            return;
    
        this->numberobj_09_value_set(preset["value"]);
    }
    
    void numberobj_10_init() {
        this->numberobj_10_currentFormat = 6;
    }
    
    void numberobj_10_getPresetValue(PatcherStateInterface& preset) {
        preset["value"] = this->numberobj_10_value;
    }
    
    void numberobj_10_setPresetValue(PatcherStateInterface& preset) {
        if ((bool)(stateIsEmpty(preset)))
            return;
    
        this->numberobj_10_value_set(preset["value"]);
    }
    
    void numberobj_11_init() {
        this->numberobj_11_currentFormat = 6;
    }
    
    void numberobj_11_getPresetValue(PatcherStateInterface& preset) {
        preset["value"] = this->numberobj_11_value;
    }
    
    void numberobj_11_setPresetValue(PatcherStateInterface& preset) {
        if ((bool)(stateIsEmpty(preset)))
            return;
    
        this->numberobj_11_value_set(preset["value"]);
    }
    
    void numberobj_12_init() {
        this->numberobj_12_currentFormat = 6;
    }
    
    void numberobj_12_getPresetValue(PatcherStateInterface& preset) {
        preset["value"] = this->numberobj_12_value;
    }
    
    void numberobj_12_setPresetValue(PatcherStateInterface& preset) {
        if ((bool)(stateIsEmpty(preset)))
            return;
    
        this->numberobj_12_value_set(preset["value"]);
    }
    
    void numberobj_13_init() {
        this->numberobj_13_currentFormat = 6;
    }
    
    void numberobj_13_getPresetValue(PatcherStateInterface& preset) {
        preset["value"] = this->numberobj_13_value;
    }
    
    void numberobj_13_setPresetValue(PatcherStateInterface& preset) {
        if ((bool)(stateIsEmpty(preset)))
            return;
    
        this->numberobj_13_value_set(preset["value"]);
    }
    
    bool stackprotect_check() {
        this->stackprotect_count++;
    
        if (this->stackprotect_count > 128) {
            console->log("STACK OVERFLOW DETECTED - stopped processing branch !");
            return true;
        }
    
        return false;
    }
    
    Index getPatcherSerial() const {
        return 0;
    }
    
    void sendParameter(ParameterIndex index, bool ignoreValue) {
        this->getPatcher()->sendParameter(index + this->parameterOffset, ignoreValue);
    }
    
    void scheduleParamInit(ParameterIndex index, Index order) {
        this->getPatcher()->scheduleParamInit(index + this->parameterOffset, order);
    }
    
    void updateTime(MillisecondTime time, EXTERNALENGINE* engine, bool inProcess = false) {
        RNBO_UNUSED(inProcess);
        RNBO_UNUSED(engine);
        this->_currentTime = time;
        auto offset = rnbo_fround(this->msToSamps(time - this->getEngine()->getCurrentTime(), this->sr));
    
        if (offset >= (SampleIndex)(this->vs))
            offset = (SampleIndex)(this->vs) - 1;
    
        if (offset < 0)
            offset = 0;
    
        this->sampleOffsetIntoNextAudioBuffer = (Index)(offset);
    }
    
    void assign_defaults()
    {
        dspexpr_19_in1 = 0;
        dspexpr_20_in1 = 0;
        dspexpr_20_in2 = 0;
        onepole_tilde_01_x = 0;
        onepole_tilde_01_freqInHz = 1;
        dspexpr_21_in1 = 0;
        dspexpr_21_in2 = 0;
        onepole_tilde_02_x = 0;
        onepole_tilde_02_freqInHz = 1;
        dspexpr_22_in1 = 0;
        dspexpr_23_in1 = 0;
        dspexpr_23_in2 = 0;
        onepole_tilde_03_x = 0;
        onepole_tilde_03_freqInHz = 1;
        dspexpr_24_in1 = 0;
        dspexpr_24_in2 = 0;
        onepole_tilde_04_x = 0;
        onepole_tilde_04_freqInHz = 1;
        numberobj_07_value = 0;
        numberobj_07_value_setter(numberobj_07_value);
        scale_04_inlow = 0;
        scale_04_inhigh = 100;
        scale_04_outlow = 20;
        scale_04_outhigh = 1000;
        scale_04_power = 1;
        numberobj_08_value = 0;
        numberobj_08_value_setter(numberobj_08_value);
        linetilde_01_time = 10;
        linetilde_01_keepramp = true;
        numberobj_09_value = 0;
        numberobj_09_value_setter(numberobj_09_value);
        expr_05_in1 = 0;
        expr_05_out1 = 0;
        numberobj_10_value = 0;
        numberobj_10_value_setter(numberobj_10_value);
        scale_05_inlow = -100;
        scale_05_inhigh = 100;
        scale_05_outlow = -18;
        scale_05_outhigh = 18;
        scale_05_power = 1;
        numberobj_11_value = 0;
        numberobj_11_value_setter(numberobj_11_value);
        numberobj_12_value = 0;
        numberobj_12_value_setter(numberobj_12_value);
        scale_06_inlow = 0;
        scale_06_inhigh = 100;
        scale_06_outlow = 10000;
        scale_06_outhigh = 200;
        scale_06_power = 0.1;
        numberobj_13_value = 0;
        numberobj_13_value_setter(numberobj_13_value);
        linetilde_02_time = 10;
        linetilde_02_keepramp = true;
        linetilde_03_time = 10;
        linetilde_03_keepramp = true;
        _currentTime = 0;
        audioProcessSampleCount = 0;
        sampleOffsetIntoNextAudioBuffer = 0;
        zeroBuffer = nullptr;
        dummyBuffer = nullptr;
        signals[0] = nullptr;
        signals[1] = nullptr;
        signals[2] = nullptr;
        signals[3] = nullptr;
        signals[4] = nullptr;
        didAllocateSignals = 0;
        vs = 0;
        maxvs = 0;
        sr = 44100;
        invsr = 0.000022675736961451248;
        onepole_tilde_01_freq = 0;
        onepole_tilde_01_needsUpdate = false;
        onepole_tilde_01_lastY = 0;
        onepole_tilde_01_a0 = 0;
        onepole_tilde_01_b1 = 0;
        onepole_tilde_01_setupDone = false;
        onepole_tilde_02_freq = 0;
        onepole_tilde_02_needsUpdate = false;
        onepole_tilde_02_lastY = 0;
        onepole_tilde_02_a0 = 0;
        onepole_tilde_02_b1 = 0;
        onepole_tilde_02_setupDone = false;
        onepole_tilde_03_freq = 0;
        onepole_tilde_03_needsUpdate = false;
        onepole_tilde_03_lastY = 0;
        onepole_tilde_03_a0 = 0;
        onepole_tilde_03_b1 = 0;
        onepole_tilde_03_setupDone = false;
        onepole_tilde_04_freq = 0;
        onepole_tilde_04_needsUpdate = false;
        onepole_tilde_04_lastY = 0;
        onepole_tilde_04_a0 = 0;
        onepole_tilde_04_b1 = 0;
        onepole_tilde_04_setupDone = false;
        numberobj_07_currentFormat = 6;
        numberobj_07_lastValue = 0;
        numberobj_08_currentFormat = 6;
        numberobj_08_lastValue = 0;
        linetilde_01_currentValue = 20;
        numberobj_09_currentFormat = 6;
        numberobj_09_lastValue = 0;
        numberobj_10_currentFormat = 6;
        numberobj_10_lastValue = 0;
        numberobj_11_currentFormat = 6;
        numberobj_11_lastValue = 0;
        numberobj_12_currentFormat = 6;
        numberobj_12_lastValue = 0;
        numberobj_13_currentFormat = 6;
        numberobj_13_lastValue = 0;
        linetilde_02_currentValue = 1;
        linetilde_03_currentValue = 10000;
        stackprotect_count = 0;
        _voiceIndex = 0;
        _noteNumber = 0;
        isMuted = 1;
        parameterOffset = 0;
    }
    
    // member variables
    
        number dspexpr_19_in1;
        number dspexpr_20_in1;
        number dspexpr_20_in2;
        number onepole_tilde_01_x;
        number onepole_tilde_01_freqInHz;
        number dspexpr_21_in1;
        number dspexpr_21_in2;
        number onepole_tilde_02_x;
        number onepole_tilde_02_freqInHz;
        number dspexpr_22_in1;
        number dspexpr_23_in1;
        number dspexpr_23_in2;
        number onepole_tilde_03_x;
        number onepole_tilde_03_freqInHz;
        number dspexpr_24_in1;
        number dspexpr_24_in2;
        number onepole_tilde_04_x;
        number onepole_tilde_04_freqInHz;
        number numberobj_07_value;
        list scale_04_input;
        number scale_04_inlow;
        number scale_04_inhigh;
        number scale_04_outlow;
        number scale_04_outhigh;
        number scale_04_power;
        list scale_04_out;
        number numberobj_08_value;
        list linetilde_01_segments;
        number linetilde_01_time;
        number linetilde_01_keepramp;
        number numberobj_09_value;
        number expr_05_in1;
        number expr_05_out1;
        number numberobj_10_value;
        list scale_05_input;
        number scale_05_inlow;
        number scale_05_inhigh;
        number scale_05_outlow;
        number scale_05_outhigh;
        number scale_05_power;
        list scale_05_out;
        number numberobj_11_value;
        number numberobj_12_value;
        list scale_06_input;
        number scale_06_inlow;
        number scale_06_inhigh;
        number scale_06_outlow;
        number scale_06_outhigh;
        number scale_06_power;
        list scale_06_out;
        number numberobj_13_value;
        list linetilde_02_segments;
        number linetilde_02_time;
        number linetilde_02_keepramp;
        list linetilde_03_segments;
        number linetilde_03_time;
        number linetilde_03_keepramp;
        MillisecondTime _currentTime;
        UInt64 audioProcessSampleCount;
        Index sampleOffsetIntoNextAudioBuffer;
        signal zeroBuffer;
        signal dummyBuffer;
        SampleValue * signals[5];
        bool didAllocateSignals;
        Index vs;
        Index maxvs;
        number sr;
        number invsr;
        number onepole_tilde_01_freq;
        bool onepole_tilde_01_needsUpdate;
        number onepole_tilde_01_lastY;
        number onepole_tilde_01_a0;
        number onepole_tilde_01_b1;
        bool onepole_tilde_01_setupDone;
        number onepole_tilde_02_freq;
        bool onepole_tilde_02_needsUpdate;
        number onepole_tilde_02_lastY;
        number onepole_tilde_02_a0;
        number onepole_tilde_02_b1;
        bool onepole_tilde_02_setupDone;
        number onepole_tilde_03_freq;
        bool onepole_tilde_03_needsUpdate;
        number onepole_tilde_03_lastY;
        number onepole_tilde_03_a0;
        number onepole_tilde_03_b1;
        bool onepole_tilde_03_setupDone;
        number onepole_tilde_04_freq;
        bool onepole_tilde_04_needsUpdate;
        number onepole_tilde_04_lastY;
        number onepole_tilde_04_a0;
        number onepole_tilde_04_b1;
        bool onepole_tilde_04_setupDone;
        Int numberobj_07_currentFormat;
        number numberobj_07_lastValue;
        Int numberobj_08_currentFormat;
        number numberobj_08_lastValue;
        list linetilde_01_activeRamps;
        number linetilde_01_currentValue;
        Int numberobj_09_currentFormat;
        number numberobj_09_lastValue;
        Int numberobj_10_currentFormat;
        number numberobj_10_lastValue;
        Int numberobj_11_currentFormat;
        number numberobj_11_lastValue;
        Int numberobj_12_currentFormat;
        number numberobj_12_lastValue;
        Int numberobj_13_currentFormat;
        number numberobj_13_lastValue;
        list linetilde_02_activeRamps;
        number linetilde_02_currentValue;
        list linetilde_03_activeRamps;
        number linetilde_03_currentValue;
        number stackprotect_count;
        Index _voiceIndex;
        Int _noteNumber;
        Index isMuted;
        ParameterIndex parameterOffset;
        bool _isInitialized = false;
};

class RNBOSubpatcher_247 : public PatcherInterfaceImpl {
    
    friend class rnbomatic;
    
    public:
    
    RNBOSubpatcher_247()
    {}
    
    ~RNBOSubpatcher_247()
    {
        deallocateSignals();
    }
    
    Index getNumMidiInputPorts() const {
        return 0;
    }
    
    void processMidiEvent(MillisecondTime , int , ConstByteArray , Index ) {}
    
    Index getNumMidiOutputPorts() const {
        return 0;
    }
    
    void process(
        const SampleValue * const* inputs,
        Index numInputs,
        SampleValue * const* outputs,
        Index numOutputs,
        Index n
    ) {
        RNBO_UNUSED(numOutputs);
        RNBO_UNUSED(outputs);
        RNBO_UNUSED(numInputs);
        RNBO_UNUSED(inputs);
        this->vs = n;
        this->updateTime(this->getEngine()->getCurrentTime(), (ENGINE*)nullptr, true);
        this->stackprotect_perform(n);
        this->audioProcessSampleCount += this->vs;
    }
    
    void prepareToProcess(number sampleRate, Index maxBlockSize, bool force) {
        RNBO_ASSERT(this->_isInitialized);
    
        if (this->maxvs < maxBlockSize || !this->didAllocateSignals) {
            this->zeroBuffer = resizeSignal(this->zeroBuffer, this->maxvs, maxBlockSize);
            this->dummyBuffer = resizeSignal(this->dummyBuffer, this->maxvs, maxBlockSize);
            this->didAllocateSignals = true;
        }
    
        const bool sampleRateChanged = sampleRate != this->sr;
        const bool maxvsChanged = maxBlockSize != this->maxvs;
        const bool forceDSPSetup = sampleRateChanged || maxvsChanged || force;
    
        if (sampleRateChanged || maxvsChanged) {
            this->vs = maxBlockSize;
            this->maxvs = maxBlockSize;
            this->sr = sampleRate;
            this->invsr = 1 / sampleRate;
        }
    
        RNBO_UNUSED(forceDSPSetup);
    
        if (sampleRateChanged)
            this->onSampleRateChanged(sampleRate);
    }
    
    number msToSamps(MillisecondTime ms, number sampleRate) {
        return ms * sampleRate * 0.001;
    }
    
    MillisecondTime sampsToMs(SampleIndex samps) {
        return samps * (this->invsr * 1000);
    }
    
    Index getNumInputChannels() const {
        return 0;
    }
    
    Index getNumOutputChannels() const {
        return 0;
    }
    
    void getPreset(PatcherStateInterface& ) {}
    
    void setPreset(MillisecondTime , PatcherStateInterface& ) {}
    
    void setParameterValue(ParameterIndex , ParameterValue , MillisecondTime ) {}
    
    void processParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
        this->setParameterValue(index, value, time);
    }
    
    void processParameterBangEvent(ParameterIndex index, MillisecondTime time) {
        this->setParameterValue(index, this->getParameterValue(index), time);
    }
    
    void processNormalizedParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
        this->setParameterValueNormalized(index, value, time);
    }
    
    ParameterValue getParameterValue(ParameterIndex index)  {
        switch (index) {
        default:
            {
            return 0;
            }
        }
    }
    
    ParameterIndex getNumSignalInParameters() const {
        return 0;
    }
    
    ParameterIndex getNumSignalOutParameters() const {
        return 0;
    }
    
    ParameterIndex getNumParameters() const {
        return 0;
    }
    
    ConstCharPointer getParameterName(ParameterIndex index) const {
        switch (index) {
        default:
            {
            return "bogus";
            }
        }
    }
    
    ConstCharPointer getParameterId(ParameterIndex index) const {
        switch (index) {
        default:
            {
            return "bogus";
            }
        }
    }
    
    void getParameterInfo(ParameterIndex , ParameterInfo * ) const {}
    
    ParameterValue applyStepsToNormalizedParameterValue(ParameterValue normalizedValue, int steps) const {
        if (steps == 1) {
            if (normalizedValue > 0) {
                normalizedValue = 1.;
            }
        } else {
            ParameterValue oneStep = (number)1. / (steps - 1);
            ParameterValue numberOfSteps = rnbo_fround(normalizedValue / oneStep * 1 / (number)1) * (number)1;
            normalizedValue = numberOfSteps * oneStep;
        }
    
        return normalizedValue;
    }
    
    ParameterValue convertToNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
        switch (index) {
        default:
            {
            return value;
            }
        }
    }
    
    ParameterValue convertFromNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
        value = (value < 0 ? 0 : (value > 1 ? 1 : value));
    
        switch (index) {
        default:
            {
            return value;
            }
        }
    }
    
    ParameterValue constrainParameterValue(ParameterIndex index, ParameterValue value) const {
        switch (index) {
        default:
            {
            return value;
            }
        }
    }
    
    void processNumMessage(MessageTag tag, MessageTag objectId, MillisecondTime time, number payload) {
        this->updateTime(time, (ENGINE*)nullptr);
    
        switch (tag) {
        case TAG("valin"):
            {
            if (TAG("Freeze/number_obj-2") == objectId)
                this->numberobj_14_valin_set(payload);
    
            if (TAG("Freeze/number_obj-81") == objectId)
                this->numberobj_15_valin_set(payload);
    
            if (TAG("Freeze/toggle_obj-4") == objectId)
                this->toggle_03_valin_set(payload);
    
            if (TAG("Freeze/toggle_obj-5") == objectId)
                this->toggle_04_valin_set(payload);
    
            if (TAG("Freeze/number_obj-8") == objectId)
                this->numberobj_16_valin_set(payload);
    
            if (TAG("Freeze/toggle_obj-11") == objectId)
                this->toggle_05_valin_set(payload);
    
            break;
            }
        case TAG("format"):
            {
            if (TAG("Freeze/number_obj-2") == objectId)
                this->numberobj_14_format_set(payload);
    
            if (TAG("Freeze/number_obj-81") == objectId)
                this->numberobj_15_format_set(payload);
    
            if (TAG("Freeze/number_obj-8") == objectId)
                this->numberobj_16_format_set(payload);
    
            break;
            }
        case TAG("listin"):
            {
            if (TAG("Freeze/message_obj-56") == objectId)
                this->message_02_listin_number_set(payload);
    
            if (TAG("Freeze/message_obj-64") == objectId)
                this->message_03_listin_number_set(payload);
    
            if (TAG("Freeze/message_obj-91") == objectId)
                this->message_04_listin_number_set(payload);
    
            if (TAG("Freeze/message_obj-93") == objectId)
                this->message_05_listin_number_set(payload);
    
            if (TAG("Freeze/message_obj-62") == objectId)
                this->message_06_listin_number_set(payload);
    
            if (TAG("Freeze/message_obj-69") == objectId)
                this->message_07_listin_number_set(payload);
    
            if (TAG("Freeze/message_obj-92") == objectId)
                this->message_08_listin_number_set(payload);
    
            if (TAG("Freeze/message_obj-94") == objectId)
                this->message_09_listin_number_set(payload);
    
            break;
            }
        }
    }
    
    void processListMessage(
        MessageTag tag,
        MessageTag objectId,
        MillisecondTime time,
        const list& payload
    ) {
        this->updateTime(time, (ENGINE*)nullptr);
    
        switch (tag) {
        case TAG("listin"):
            {
            if (TAG("Freeze/message_obj-56") == objectId)
                this->message_02_listin_list_set(payload);
    
            if (TAG("Freeze/message_obj-64") == objectId)
                this->message_03_listin_list_set(payload);
    
            if (TAG("Freeze/message_obj-91") == objectId)
                this->message_04_listin_list_set(payload);
    
            if (TAG("Freeze/message_obj-93") == objectId)
                this->message_05_listin_list_set(payload);
    
            if (TAG("Freeze/message_obj-62") == objectId)
                this->message_06_listin_list_set(payload);
    
            if (TAG("Freeze/message_obj-69") == objectId)
                this->message_07_listin_list_set(payload);
    
            if (TAG("Freeze/message_obj-92") == objectId)
                this->message_08_listin_list_set(payload);
    
            if (TAG("Freeze/message_obj-94") == objectId)
                this->message_09_listin_list_set(payload);
    
            break;
            }
        }
    }
    
    void processBangMessage(MessageTag tag, MessageTag objectId, MillisecondTime time) {
        this->updateTime(time, (ENGINE*)nullptr);
    
        switch (tag) {
        case TAG("bangin"):
            {
            if (TAG("Freeze/button_obj-6") == objectId)
                this->button_05_bangin_bang();
    
            if (TAG("Freeze/button_obj-88") == objectId)
                this->button_06_bangin_bang();
    
            if (TAG("Freeze/button_obj-220") == objectId)
                this->button_07_bangin_bang();
    
            if (TAG("Freeze/button_obj-89") == objectId)
                this->button_08_bangin_bang();
    
            break;
            }
        case TAG("listin"):
            {
            if (TAG("Freeze/message_obj-56") == objectId)
                this->message_02_listin_bang_bang();
    
            if (TAG("Freeze/message_obj-64") == objectId)
                this->message_03_listin_bang_bang();
    
            if (TAG("Freeze/message_obj-91") == objectId)
                this->message_04_listin_bang_bang();
    
            if (TAG("Freeze/message_obj-93") == objectId)
                this->message_05_listin_bang_bang();
    
            if (TAG("Freeze/message_obj-62") == objectId)
                this->message_06_listin_bang_bang();
    
            if (TAG("Freeze/message_obj-69") == objectId)
                this->message_07_listin_bang_bang();
    
            if (TAG("Freeze/message_obj-92") == objectId)
                this->message_08_listin_bang_bang();
    
            if (TAG("Freeze/message_obj-94") == objectId)
                this->message_09_listin_bang_bang();
    
            break;
            }
        }
    }
    
    MessageTagInfo resolveTag(MessageTag tag) const {
        switch (tag) {
        case TAG("valout"):
            {
            return "valout";
            }
        case TAG("Freeze/number_obj-2"):
            {
            return "Freeze/number_obj-2";
            }
        case TAG("setup"):
            {
            return "setup";
            }
        case TAG("bangout"):
            {
            return "bangout";
            }
        case TAG("Freeze/button_obj-6"):
            {
            return "Freeze/button_obj-6";
            }
        case TAG("Freeze/number_obj-81"):
            {
            return "Freeze/number_obj-81";
            }
        case TAG("Freeze/button_obj-88"):
            {
            return "Freeze/button_obj-88";
            }
        case TAG("listout"):
            {
            return "listout";
            }
        case TAG("Freeze/message_obj-56"):
            {
            return "Freeze/message_obj-56";
            }
        case TAG("Freeze/message_obj-64"):
            {
            return "Freeze/message_obj-64";
            }
        case TAG("Freeze/toggle_obj-4"):
            {
            return "Freeze/toggle_obj-4";
            }
        case TAG("Freeze/message_obj-91"):
            {
            return "Freeze/message_obj-91";
            }
        case TAG("Freeze/toggle_obj-5"):
            {
            return "Freeze/toggle_obj-5";
            }
        case TAG("Freeze/button_obj-220"):
            {
            return "Freeze/button_obj-220";
            }
        case TAG("Freeze/message_obj-93"):
            {
            return "Freeze/message_obj-93";
            }
        case TAG("Freeze/number_obj-8"):
            {
            return "Freeze/number_obj-8";
            }
        case TAG("Freeze/message_obj-62"):
            {
            return "Freeze/message_obj-62";
            }
        case TAG("Freeze/message_obj-69"):
            {
            return "Freeze/message_obj-69";
            }
        case TAG("Freeze/toggle_obj-11"):
            {
            return "Freeze/toggle_obj-11";
            }
        case TAG("Freeze/message_obj-92"):
            {
            return "Freeze/message_obj-92";
            }
        case TAG("Freeze/button_obj-89"):
            {
            return "Freeze/button_obj-89";
            }
        case TAG("Freeze/message_obj-94"):
            {
            return "Freeze/message_obj-94";
            }
        case TAG("valin"):
            {
            return "valin";
            }
        case TAG("format"):
            {
            return "format";
            }
        case TAG("bangin"):
            {
            return "bangin";
            }
        case TAG("listin"):
            {
            return "listin";
            }
        }
    
        return nullptr;
    }
    
    DataRef* getDataRef(DataRefIndex index)  {
        switch (index) {
        default:
            {
            return nullptr;
            }
        }
    }
    
    DataRefIndex getNumDataRefs() const {
        return 0;
    }
    
    void processDataViewUpdate(DataRefIndex , MillisecondTime ) {}
    
    void initialize() {
        RNBO_ASSERT(!this->_isInitialized);
        this->assign_defaults();
        this->applyState();
        this->_isInitialized = true;
    }
    
    protected:
    
    void updateTime(MillisecondTime time, INTERNALENGINE*, bool inProcess = false) {
    	if (time == TimeNow) time = getTopLevelPatcher()->getPatcherTime();
    	getTopLevelPatcher()->processInternalEvents(time);
    	updateTime(time, (EXTERNALENGINE*)nullptr);
    }
    
    RNBOSubpatcher_247* operator->() {
        return this;
    }
    const RNBOSubpatcher_247* operator->() const {
        return this;
    }
    virtual rnbomatic* getPatcher() const {
        return static_cast<rnbomatic *>(_parentPatcher);
    }
    
    rnbomatic* getTopLevelPatcher() {
        return this->getPatcher()->getTopLevelPatcher();
    }
    
    void cancelClockEvents()
    {
        getEngine()->flushClockEvents(this, -1357044121, false);
    }
    
    MillisecondTime getPatcherTime() const {
        return this->_currentTime;
    }
    
    void numberobj_14_valin_set(number v) {
        this->numberobj_14_value_set(v);
    }
    
    void numberobj_14_format_set(number v) {
        this->numberobj_14_currentFormat = trunc((v > 6 ? 6 : (v < 0 ? 0 : v)));
    }
    
    void button_05_bangin_bang() {
        this->button_05_bangval_bang();
    }
    
    void numberobj_15_valin_set(number v) {
        this->numberobj_15_value_set(v);
    }
    
    void numberobj_15_format_set(number v) {
        this->numberobj_15_currentFormat = trunc((v > 6 ? 6 : (v < 0 ? 0 : v)));
    }
    
    void button_06_bangin_bang() {
        this->button_06_bangval_bang();
    }
    
    template<typename LISTTYPE> void message_02_listin_list_set(const LISTTYPE& v) {
        this->message_02_set_set(v);
    }
    
    void message_02_listin_number_set(number v) {
        this->message_02_set_set(v);
    }
    
    void message_02_listin_bang_bang() {
        this->message_02_trigger_bang();
    }
    
    template<typename LISTTYPE> void message_03_listin_list_set(const LISTTYPE& v) {
        this->message_03_set_set(v);
    }
    
    void message_03_listin_number_set(number v) {
        this->message_03_set_set(v);
    }
    
    void message_03_listin_bang_bang() {
        this->message_03_trigger_bang();
    }
    
    void delay_02_out_bang() {
        this->onebang_01_reset_bang();
    }
    
    void toggle_03_valin_set(number v) {
        this->toggle_03_value_number_set(v);
    }
    
    template<typename LISTTYPE> void message_04_listin_list_set(const LISTTYPE& v) {
        this->message_04_set_set(v);
    }
    
    void message_04_listin_number_set(number v) {
        this->message_04_set_set(v);
    }
    
    void message_04_listin_bang_bang() {
        this->message_04_trigger_bang();
    }
    
    void toggle_04_valin_set(number v) {
        this->toggle_04_value_number_set(v);
    }
    
    void eventinlet_06_out1_bang_bang() {
        this->toggle_04_value_bang_bang();
    }
    
    template<typename LISTTYPE> void eventinlet_06_out1_list_set(const LISTTYPE& v) {
        {
            number converted = (v->length > 0 ? v[0] : 0);
            this->toggle_04_value_number_set(converted);
        }
    }
    
    void button_07_bangin_bang() {
        this->button_07_bangval_bang();
    }
    
    template<typename LISTTYPE> void message_05_listin_list_set(const LISTTYPE& v) {
        this->message_05_set_set(v);
    }
    
    void message_05_listin_number_set(number v) {
        this->message_05_set_set(v);
    }
    
    void message_05_listin_bang_bang() {
        this->message_05_trigger_bang();
    }
    
    void numberobj_16_valin_set(number v) {
        this->numberobj_16_value_set(v);
    }
    
    void numberobj_16_format_set(number v) {
        this->numberobj_16_currentFormat = trunc((v > 6 ? 6 : (v < 0 ? 0 : v)));
    }
    
    template<typename LISTTYPE> void message_06_listin_list_set(const LISTTYPE& v) {
        this->message_06_set_set(v);
    }
    
    void message_06_listin_number_set(number v) {
        this->message_06_set_set(v);
    }
    
    void message_06_listin_bang_bang() {
        this->message_06_trigger_bang();
    }
    
    template<typename LISTTYPE> void message_07_listin_list_set(const LISTTYPE& v) {
        this->message_07_set_set(v);
    }
    
    void message_07_listin_number_set(number v) {
        this->message_07_set_set(v);
    }
    
    void message_07_listin_bang_bang() {
        this->message_07_trigger_bang();
    }
    
    void toggle_05_valin_set(number v) {
        this->toggle_05_value_number_set(v);
    }
    
    template<typename LISTTYPE> void message_08_listin_list_set(const LISTTYPE& v) {
        this->message_08_set_set(v);
    }
    
    void message_08_listin_number_set(number v) {
        this->message_08_set_set(v);
    }
    
    void message_08_listin_bang_bang() {
        this->message_08_trigger_bang();
    }
    
    void button_08_bangin_bang() {
        this->button_08_bangval_bang();
    }
    
    template<typename LISTTYPE> void message_09_listin_list_set(const LISTTYPE& v) {
        this->message_09_set_set(v);
    }
    
    void message_09_listin_number_set(number v) {
        this->message_09_set_set(v);
    }
    
    void message_09_listin_bang_bang() {
        this->message_09_trigger_bang();
    }
    
    void deallocateSignals() {
        Index i;
        this->zeroBuffer = freeSignal(this->zeroBuffer);
        this->dummyBuffer = freeSignal(this->dummyBuffer);
    }
    
    Index getMaxBlockSize() const {
        return this->maxvs;
    }
    
    number getSampleRate() const {
        return this->sr;
    }
    
    bool hasFixedVectorSize() const {
        return false;
    }
    
    void setProbingTarget(MessageTag ) {}
    
    void initializeObjects() {
        this->numberobj_14_init();
        this->numberobj_15_init();
        this->message_02_init();
        this->message_03_init();
        this->message_04_init();
        this->message_05_init();
        this->numberobj_16_init();
        this->message_06_init();
        this->message_07_init();
        this->message_08_init();
        this->message_09_init();
    }
    
    Index getIsMuted()  {
        return this->isMuted;
    }
    
    void setIsMuted(Index v)  {
        this->isMuted = v;
    }
    
    void onSampleRateChanged(double ) {}
    
    void extractState(PatcherStateInterface& ) {}
    
    void applyState() {}
    
    void setParameterOffset(ParameterIndex offset) {
        this->parameterOffset = offset;
    }
    
    void processClockEvent(MillisecondTime time, ClockId index, bool hasValue, ParameterValue value) {
        RNBO_UNUSED(value);
        RNBO_UNUSED(hasValue);
        this->updateTime(time, (ENGINE*)nullptr);
    
        switch (index) {
        case -1357044121:
            {
            this->delay_02_out_bang();
            break;
            }
        }
    }
    
    void processOutletAtCurrentTime(EngineLink* , OutletIndex , ParameterValue ) {}
    
    void processOutletEvent(
        EngineLink* sender,
        OutletIndex index,
        ParameterValue value,
        MillisecondTime time
    ) {
        this->updateTime(time, (ENGINE*)nullptr);
        this->processOutletAtCurrentTime(sender, index, value);
    }
    
    void sendOutlet(OutletIndex index, ParameterValue value) {
        this->getEngine()->sendOutlet(this, index, value);
    }
    
    void startup() {}
    
    void fillDataRef(DataRefIndex , DataRef& ) {}
    
    void allocateDataRefs() {}
    
    void delay_02_stop_bang() {
        this->getEngine()->flushClockEvents(this, -1357044121, false);;
    }
    
    void delay_02_input_bang() {
        if ((bool)(!(bool)(this->delay_02_delayall)))
            this->delay_02_stop_bang();
    
        this->getEngine()->scheduleClockEvent(this, -1357044121, this->delay_02_time + this->_currentTime);;
    }
    
    void eventoutlet_05_in1_number_set(number v) {
        this->getPatcher()->updateTime(this->_currentTime, (ENGINE*)nullptr);
        this->getPatcher()->p_04_out4_number_set(v);
    }
    
    void toggle_05_output_set(number v) {
        this->eventoutlet_05_in1_number_set(v);
    }
    
    void toggle_05_value_number_set(number v) {
        this->toggle_05_value_number_setter(v);
        v = this->toggle_05_value_number;
        this->toggle_05_output_set(v);
    }
    
    template<typename LISTTYPE> void message_09_out_set(const LISTTYPE& v) {
        {
            number converted = (v->length > 0 ? v[0] : 0);
            this->toggle_05_value_number_set(converted);
        }
    }
    
    void message_09_trigger_bang() {
        if ((bool)(this->message_09_set->length) || (bool)(false)) {
            this->message_09_out_set(this->message_09_set);
        }
    }
    
    void eventoutlet_04_in1_number_set(number v) {
        this->getPatcher()->updateTime(this->_currentTime, (ENGINE*)nullptr);
        this->getPatcher()->p_04_out3_number_set(v);
    }
    
    void numberobj_16_output_set(number v) {
        this->eventoutlet_04_in1_number_set(v);
    }
    
    void numberobj_16_value_set(number v) {
        this->numberobj_16_value_setter(v);
        v = this->numberobj_16_value;
        number localvalue = v;
    
        if (this->numberobj_16_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_16_output_set(localvalue);
    }
    
    template<typename LISTTYPE> void message_07_out_set(const LISTTYPE& v) {
        {
            number converted = (v->length > 0 ? v[0] : 0);
            this->numberobj_16_value_set(converted);
        }
    }
    
    void message_07_trigger_bang() {
        if ((bool)(this->message_07_set->length) || (bool)(false)) {
            this->message_07_out_set(this->message_07_set);
        }
    }
    
    void eventoutlet_03_in1_number_set(number v) {
        this->getPatcher()->updateTime(this->_currentTime, (ENGINE*)nullptr);
        this->getPatcher()->p_04_out2_number_set(v);
    }
    
    void toggle_03_output_set(number v) {
        this->eventoutlet_03_in1_number_set(v);
    }
    
    void toggle_03_value_number_set(number v) {
        this->toggle_03_value_number_setter(v);
        v = this->toggle_03_value_number;
        this->toggle_03_output_set(v);
    }
    
    template<typename LISTTYPE> void message_04_out_set(const LISTTYPE& v) {
        {
            number converted = (v->length > 0 ? v[0] : 0);
            this->toggle_03_value_number_set(converted);
        }
    }
    
    void message_04_trigger_bang() {
        if ((bool)(this->message_04_set->length) || (bool)(false)) {
            this->message_04_out_set(this->message_04_set);
        }
    }
    
    void eventoutlet_02_in1_number_set(number v) {
        this->getPatcher()->updateTime(this->_currentTime, (ENGINE*)nullptr);
        this->getPatcher()->p_04_out1_number_set(v);
    }
    
    void numberobj_14_output_set(number v) {
        this->eventoutlet_02_in1_number_set(v);
    }
    
    void numberobj_14_value_set(number v) {
        this->numberobj_14_value_setter(v);
        v = this->numberobj_14_value;
        number localvalue = v;
    
        if (this->numberobj_14_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_14_output_set(localvalue);
    }
    
    template<typename LISTTYPE> void message_02_out_set(const LISTTYPE& v) {
        {
            number converted = (v->length > 0 ? v[0] : 0);
            this->numberobj_14_value_set(converted);
        }
    }
    
    void message_02_trigger_bang() {
        if ((bool)(this->message_02_set->length) || (bool)(false)) {
            this->message_02_out_set(this->message_02_set);
        }
    }
    
    void button_06_output_bang() {
        this->message_09_trigger_bang();
        this->message_07_trigger_bang();
        this->message_04_trigger_bang();
        this->message_02_trigger_bang();
    }
    
    void button_06_bangval_bang() {
        ;
        this->button_06_output_bang();
    }
    
    void select_02_match1_bang() {
        this->button_06_bangval_bang();
    }
    
    template<typename LISTTYPE> void message_08_out_set(const LISTTYPE& v) {
        {
            number converted = (v->length > 0 ? v[0] : 0);
            this->toggle_05_value_number_set(converted);
        }
    }
    
    void message_08_trigger_bang() {
        if ((bool)(this->message_08_set->length) || (bool)(false)) {
            this->message_08_out_set(this->message_08_set);
        }
    }
    
    template<typename LISTTYPE> void message_06_out_set(const LISTTYPE& v) {
        {
            number converted = (v->length > 0 ? v[0] : 0);
            this->numberobj_16_value_set(converted);
        }
    }
    
    void message_06_trigger_bang() {
        if ((bool)(this->message_06_set->length) || (bool)(false)) {
            this->message_06_out_set(this->message_06_set);
        }
    }
    
    template<typename LISTTYPE> void message_05_out_set(const LISTTYPE& v) {
        {
            number converted = (v->length > 0 ? v[0] : 0);
            this->toggle_03_value_number_set(converted);
        }
    }
    
    void message_05_trigger_bang() {
        if ((bool)(this->message_05_set->length) || (bool)(false)) {
            this->message_05_out_set(this->message_05_set);
        }
    }
    
    template<typename LISTTYPE> void message_03_out_set(const LISTTYPE& v) {
        {
            number converted = (v->length > 0 ? v[0] : 0);
            this->numberobj_14_value_set(converted);
        }
    }
    
    void message_03_trigger_bang() {
        if ((bool)(this->message_03_set->length) || (bool)(false)) {
            this->message_03_out_set(this->message_03_set);
        }
    }
    
    void button_08_output_bang() {
        this->message_08_trigger_bang();
        this->message_06_trigger_bang();
        this->message_05_trigger_bang();
        this->message_03_trigger_bang();
    }
    
    void button_08_bangval_bang() {
        ;
        this->button_08_output_bang();
    }
    
    void select_02_match2_bang() {
        this->button_08_bangval_bang();
    }
    
    void select_02_nomatch_number_set(number ) {}
    
    void select_02_input_number_set(number v) {
        if (v == this->select_02_test1)
            this->select_02_match1_bang();
        else if (v == this->select_02_test2)
            this->select_02_match2_bang();
        else
            this->select_02_nomatch_number_set(v);
    }
    
    void numberobj_15_output_set(number v) {
        this->select_02_input_number_set(v);
    }
    
    void numberobj_15_value_set(number v) {
        this->numberobj_15_value_setter(v);
        v = this->numberobj_15_value;
        number localvalue = v;
    
        if (this->numberobj_15_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_15_output_set(localvalue);
    }
    
    void counter_01_output_set(number v) {
        this->counter_01_output = v;
        this->numberobj_15_value_set(v);
    }
    
    void counter_01_overflow_bang_bang() {}
    
    number counter_01_overflow_number_constrain(number v) const {
        if (v < 0)
            v = 0;
    
        if (v > 1)
            v = 1;
    
        return v;
    }
    
    void counter_01_overflow_number_set(number v) {
        v = this->counter_01_overflow_number_constrain(v);
        this->counter_01_overflow_number = v;
    }
    
    void counter_01_carry_set(number v) {
        this->counter_01_carry = v;
    }
    
    void counter_01_underflow_bang_bang() {}
    
    number counter_01_underflow_number_constrain(number v) const {
        if (v < 0)
            v = 0;
    
        if (v > 1)
            v = 1;
    
        return v;
    }
    
    void counter_01_underflow_number_set(number v) {
        v = this->counter_01_underflow_number_constrain(v);
        this->counter_01_underflow_number = v;
    }
    
    void counter_01_input_bang() {
        this->counter_01_output_set(this->counter_01_count);
        this->counter_01_inc();
    
        if (this->counter_01_count > this->counter_01_maximum) {
            if (this->counter_01_direction == 2) {
                this->counter_01_phase = !(bool)(this->counter_01_phase);
                this->counter_01_count = this->counter_01_maximum;
                this->counter_01_inc();
            } else
                this->counter_01_count = this->counter_01_minimum;
    
            if ((bool)(!(bool)(this->counter_01_overflow_number))) {
                if (this->counter_01_carryflag == 1)
                    this->counter_01_overflow_bang_bang();
                else
                    this->counter_01_overflow_number_set(1);
            }
    
            this->counter_01_carry_set(this->counter_01_carry + 1);
        } else if ((bool)(this->counter_01_overflow_number) && this->counter_01_carryflag == 0)
            this->counter_01_overflow_number_set(0);
    
        if (this->counter_01_count < this->counter_01_minimum) {
            if (this->counter_01_direction == 2) {
                this->counter_01_phase = !(bool)(this->counter_01_phase);
                this->counter_01_count = this->counter_01_minimum;
                this->counter_01_inc();
            } else
                this->counter_01_count = this->counter_01_maximum;
    
            if ((bool)(!(bool)(this->counter_01_underflow_number))) {
                if (this->counter_01_carryflag == 1)
                    this->counter_01_underflow_bang_bang();
                else
                    this->counter_01_underflow_number_set(1);
            }
    
            this->counter_01_carry_set(this->counter_01_carry + 1);
        } else if ((bool)(this->counter_01_underflow_number) && this->counter_01_carryflag == 0)
            this->counter_01_underflow_number_set(0);
    }
    
    void onebang_01_out1_bang() {
        this->delay_02_input_bang();
        this->counter_01_input_bang();
    }
    
    void onebang_01_out2_bang() {}
    
    void onebang_01_input_bang() {
        if ((bool)(this->onebang_01_allow)) {
            this->onebang_01_out1_bang();
            this->onebang_01_allow = false;
        } else
            this->onebang_01_out2_bang();
    }
    
    void button_05_output_bang() {
        this->onebang_01_input_bang();
    }
    
    void button_05_bangval_bang() {
        ;
        this->button_05_output_bang();
    }
    
    void select_03_match1_bang() {
        this->button_05_bangval_bang();
    }
    
    void button_07_output_bang() {
        this->message_09_trigger_bang();
        this->message_07_trigger_bang();
        this->message_05_trigger_bang();
        this->message_03_trigger_bang();
    }
    
    void button_07_bangval_bang() {
        ;
        this->button_07_output_bang();
    }
    
    void select_03_match2_bang() {
        this->button_07_bangval_bang();
    }
    
    void select_03_nomatch_number_set(number ) {}
    
    void select_03_input_number_set(number v) {
        if (v == this->select_03_test1)
            this->select_03_match1_bang();
        else if (v == this->select_03_test2)
            this->select_03_match2_bang();
        else
            this->select_03_nomatch_number_set(v);
    }
    
    void toggle_04_output_set(number v) {
        this->select_03_input_number_set(v);
    }
    
    void toggle_04_value_number_set(number v) {
        this->toggle_04_value_number_setter(v);
        v = this->toggle_04_value_number;
        this->toggle_04_output_set(v);
    }
    
    void eventinlet_06_out1_number_set(number v) {
        this->toggle_04_value_number_set(v);
    }
    
    template<typename LISTTYPE> void message_02_set_set(const LISTTYPE& v) {
        this->message_02_set = jsCreateListCopy(v);
    }
    
    template<typename LISTTYPE> void message_03_set_set(const LISTTYPE& v) {
        this->message_03_set = jsCreateListCopy(v);
    }
    
    void onebang_01_reset_bang() {
        this->onebang_01_allow = true;
    }
    
    template<typename LISTTYPE> void message_04_set_set(const LISTTYPE& v) {
        this->message_04_set = jsCreateListCopy(v);
    }
    
    void toggle_04_value_bang_bang() {
        number val = (this->toggle_04_value_number == 1 ? 0 : 1);
        this->toggle_04_value_number_set(val);
    }
    
    template<typename LISTTYPE> void message_05_set_set(const LISTTYPE& v) {
        this->message_05_set = jsCreateListCopy(v);
    }
    
    template<typename LISTTYPE> void message_06_set_set(const LISTTYPE& v) {
        this->message_06_set = jsCreateListCopy(v);
    }
    
    template<typename LISTTYPE> void message_07_set_set(const LISTTYPE& v) {
        this->message_07_set = jsCreateListCopy(v);
    }
    
    template<typename LISTTYPE> void message_08_set_set(const LISTTYPE& v) {
        this->message_08_set = jsCreateListCopy(v);
    }
    
    template<typename LISTTYPE> void message_09_set_set(const LISTTYPE& v) {
        this->message_09_set = jsCreateListCopy(v);
    }
    
    void stackprotect_perform(Index n) {
        RNBO_UNUSED(n);
        auto __stackprotect_count = this->stackprotect_count;
        __stackprotect_count = 0;
        this->stackprotect_count = __stackprotect_count;
    }
    
    void numberobj_14_value_setter(number v) {
        number localvalue = v;
    
        if (this->numberobj_14_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_14_value = localvalue;
    }
    
    void onebang_01_initial_setter(number v) {
        if (v > 0)
            this->onebang_01_allow = true;
        else
            this->onebang_01_allow = false;
    }
    
    void numberobj_15_value_setter(number v) {
        number localvalue = v;
    
        if (this->numberobj_15_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_15_value = localvalue;
    }
    
    void toggle_03_value_number_setter(number v) {
        this->toggle_03_value_number = (v != 0 ? 1 : 0);
    }
    
    void toggle_04_value_number_setter(number v) {
        this->toggle_04_value_number = (v != 0 ? 1 : 0);
    }
    
    void numberobj_16_value_setter(number v) {
        number localvalue = v;
    
        if (this->numberobj_16_currentFormat != 6) {
            localvalue = trunc(localvalue);
        }
    
        this->numberobj_16_value = localvalue;
    }
    
    void toggle_05_value_number_setter(number v) {
        this->toggle_05_value_number = (v != 0 ? 1 : 0);
    }
    
    void numberobj_14_init() {
        this->numberobj_14_currentFormat = 6;
    }
    
    void numberobj_14_getPresetValue(PatcherStateInterface& preset) {
        preset["value"] = this->numberobj_14_value;
    }
    
    void numberobj_14_setPresetValue(PatcherStateInterface& preset) {
        if ((bool)(stateIsEmpty(preset)))
            return;
    
        this->numberobj_14_value_set(preset["value"]);
    }
    
    void numberobj_15_init() {
        this->numberobj_15_currentFormat = 6;
    }
    
    void numberobj_15_getPresetValue(PatcherStateInterface& preset) {
        preset["value"] = this->numberobj_15_value;
    }
    
    void numberobj_15_setPresetValue(PatcherStateInterface& preset) {
        if ((bool)(stateIsEmpty(preset)))
            return;
    
        this->numberobj_15_value_set(preset["value"]);
    }
    
    void counter_01_inc() {
        this->counter_01_count = ((bool)(this->counter_01_phase) ? this->counter_01_count + 1 : this->counter_01_count - 1);
    }
    
    void message_02_init() {
        this->message_02_set_set(listbase<number, 1>{0.5});
    }
    
    void message_03_init() {
        this->message_03_set_set(listbase<number, 1>{0});
    }
    
    void toggle_03_getPresetValue(PatcherStateInterface& preset) {
        preset["value"] = this->toggle_03_value_number;
    }
    
    void toggle_03_setPresetValue(PatcherStateInterface& preset) {
        if ((bool)(stateIsEmpty(preset)))
            return;
    
        this->toggle_03_value_number_set(preset["value"]);
    }
    
    void message_04_init() {
        this->message_04_set_set(listbase<number, 1>{1});
    }
    
    void toggle_04_getPresetValue(PatcherStateInterface& preset) {
        preset["value"] = this->toggle_04_value_number;
    }
    
    void toggle_04_setPresetValue(PatcherStateInterface& preset) {
        if ((bool)(stateIsEmpty(preset)))
            return;
    
        this->toggle_04_value_number_set(preset["value"]);
    }
    
    void message_05_init() {
        this->message_05_set_set(listbase<number, 1>{0});
    }
    
    void numberobj_16_init() {
        this->numberobj_16_currentFormat = 6;
    }
    
    void numberobj_16_getPresetValue(PatcherStateInterface& preset) {
        preset["value"] = this->numberobj_16_value;
    }
    
    void numberobj_16_setPresetValue(PatcherStateInterface& preset) {
        if ((bool)(stateIsEmpty(preset)))
            return;
    
        this->numberobj_16_value_set(preset["value"]);
    }
    
    void message_06_init() {
        this->message_06_set_set(listbase<number, 1>{0.5});
    }
    
    void message_07_init() {
        this->message_07_set_set(listbase<number, 1>{0});
    }
    
    void toggle_05_getPresetValue(PatcherStateInterface& preset) {
        preset["value"] = this->toggle_05_value_number;
    }
    
    void toggle_05_setPresetValue(PatcherStateInterface& preset) {
        if ((bool)(stateIsEmpty(preset)))
            return;
    
        this->toggle_05_value_number_set(preset["value"]);
    }
    
    void message_08_init() {
        this->message_08_set_set(listbase<number, 1>{1});
    }
    
    void message_09_init() {
        this->message_09_set_set(listbase<number, 1>{0});
    }
    
    bool stackprotect_check() {
        this->stackprotect_count++;
    
        if (this->stackprotect_count > 128) {
            console->log("STACK OVERFLOW DETECTED - stopped processing branch !");
            return true;
        }
    
        return false;
    }
    
    Index getPatcherSerial() const {
        return 0;
    }
    
    void sendParameter(ParameterIndex index, bool ignoreValue) {
        this->getPatcher()->sendParameter(index + this->parameterOffset, ignoreValue);
    }
    
    void scheduleParamInit(ParameterIndex index, Index order) {
        this->getPatcher()->scheduleParamInit(index + this->parameterOffset, order);
    }
    
    void updateTime(MillisecondTime time, EXTERNALENGINE* engine, bool inProcess = false) {
        RNBO_UNUSED(inProcess);
        RNBO_UNUSED(engine);
        this->_currentTime = time;
        auto offset = rnbo_fround(this->msToSamps(time - this->getEngine()->getCurrentTime(), this->sr));
    
        if (offset >= (SampleIndex)(this->vs))
            offset = (SampleIndex)(this->vs) - 1;
    
        if (offset < 0)
            offset = 0;
    
        this->sampleOffsetIntoNextAudioBuffer = (Index)(offset);
    }
    
    void assign_defaults()
    {
        numberobj_14_value = 0;
        numberobj_14_value_setter(numberobj_14_value);
        onebang_01_initial = 1;
        onebang_01_initial_setter(onebang_01_initial);
        select_02_test1 = 1;
        select_02_test2 = 2;
        numberobj_15_value = 0;
        numberobj_15_value_setter(numberobj_15_value);
        counter_01_carryflag = 0;
        counter_01_direction = 0;
        counter_01_resetnext_number = 0;
        counter_01_resetnow_number = 0;
        counter_01_maximum = 2;
        counter_01_output = 0;
        counter_01_underflow_number = 0;
        counter_01_overflow_number = 0;
        counter_01_carry = 0;
        counter_01_minimum = 1;
        delay_02_time = 500;
        delay_02_delayall = 1;
        toggle_03_value_number = 0;
        toggle_03_value_number_setter(toggle_03_value_number);
        select_03_test1 = 1;
        select_03_test2 = 0;
        toggle_04_value_number = 0;
        toggle_04_value_number_setter(toggle_04_value_number);
        numberobj_16_value = 0;
        numberobj_16_value_setter(numberobj_16_value);
        toggle_05_value_number = 0;
        toggle_05_value_number_setter(toggle_05_value_number);
        _currentTime = 0;
        audioProcessSampleCount = 0;
        sampleOffsetIntoNextAudioBuffer = 0;
        zeroBuffer = nullptr;
        dummyBuffer = nullptr;
        didAllocateSignals = 0;
        vs = 0;
        maxvs = 0;
        sr = 44100;
        invsr = 0.000022675736961451248;
        numberobj_14_currentFormat = 6;
        numberobj_14_lastValue = 0;
        numberobj_15_currentFormat = 6;
        numberobj_15_lastValue = 0;
        counter_01_count = 1;
        counter_01_phase = true;
        toggle_03_lastValue = 0;
        toggle_04_lastValue = 0;
        numberobj_16_currentFormat = 6;
        numberobj_16_lastValue = 0;
        toggle_05_lastValue = 0;
        stackprotect_count = 0;
        _voiceIndex = 0;
        _noteNumber = 0;
        isMuted = 1;
        parameterOffset = 0;
    }
    
    // member variables
    
        number numberobj_14_value;
        number onebang_01_initial;
        number select_02_test1;
        number select_02_test2;
        number numberobj_15_value;
        Int counter_01_carryflag;
        number counter_01_direction;
        number counter_01_resetnext_number;
        number counter_01_resetnow_number;
        number counter_01_maximum;
        number counter_01_output;
        number counter_01_underflow_number;
        number counter_01_overflow_number;
        number counter_01_carry;
        number counter_01_minimum;
        list message_02_set;
        list message_03_set;
        number delay_02_time;
        number delay_02_delayall;
        number toggle_03_value_number;
        list message_04_set;
        number select_03_test1;
        number select_03_test2;
        number toggle_04_value_number;
        list message_05_set;
        number numberobj_16_value;
        list message_06_set;
        list message_07_set;
        number toggle_05_value_number;
        list message_08_set;
        list message_09_set;
        MillisecondTime _currentTime;
        UInt64 audioProcessSampleCount;
        Index sampleOffsetIntoNextAudioBuffer;
        signal zeroBuffer;
        signal dummyBuffer;
        bool didAllocateSignals;
        Index vs;
        Index maxvs;
        number sr;
        number invsr;
        Int numberobj_14_currentFormat;
        number numberobj_14_lastValue;
        bool onebang_01_allow;
        Int numberobj_15_currentFormat;
        number numberobj_15_lastValue;
        number counter_01_count;
        bool counter_01_phase;
        number toggle_03_lastValue;
        number toggle_04_lastValue;
        Int numberobj_16_currentFormat;
        number numberobj_16_lastValue;
        number toggle_05_lastValue;
        number stackprotect_count;
        Index _voiceIndex;
        Int _noteNumber;
        Index isMuted;
        ParameterIndex parameterOffset;
        bool _isInitialized = false;
};

class RNBOSubpatcher_248 : public PatcherInterfaceImpl {
    
    friend class rnbomatic;
    
    public:
    
    RNBOSubpatcher_248()
    {}
    
    ~RNBOSubpatcher_248()
    {
        deallocateSignals();
    }
    
    Index getNumMidiInputPorts() const {
        return 0;
    }
    
    void processMidiEvent(MillisecondTime , int , ConstByteArray , Index ) {}
    
    Index getNumMidiOutputPorts() const {
        return 0;
    }
    
    void process(
        const SampleValue * const* inputs,
        Index numInputs,
        SampleValue * const* outputs,
        Index numOutputs,
        Index n
    ) {
        this->vs = n;
        this->updateTime(this->getEngine()->getCurrentTime(), (ENGINE*)nullptr, true);
        SampleValue * out1 = (numOutputs >= 1 && outputs[0] ? outputs[0] : this->dummyBuffer);
        SampleValue * out2 = (numOutputs >= 2 && outputs[1] ? outputs[1] : this->dummyBuffer);
        const SampleValue * in1 = (numInputs >= 1 && inputs[0] ? inputs[0] : this->zeroBuffer);
    
        if (this->getIsMuted())
            return;
    
        this->dspexpr_28_perform(in1, this->dspexpr_28_in2, this->signals[0], n);
        this->rand_tilde_04_perform(this->rand_tilde_04_frequency, this->signals[1], n);
    
        this->cycle_tilde_04_perform(
            this->signals[1],
            this->cycle_tilde_04_phase_offset,
            this->signals[2],
            this->dummyBuffer,
            n
        );
    
        this->dspexpr_31_perform(this->signals[2], this->dspexpr_31_in2, this->signals[1], n);
    
        this->rampsmooth_tilde_07_perform(
            this->signals[1],
            this->rampsmooth_tilde_07_up,
            this->rampsmooth_tilde_07_down,
            this->signals[2],
            n
        );
    
        this->dspexpr_27_perform(this->signals[0], this->signals[2], this->signals[1], n);
    
        this->wrap_tilde_02_perform(
            this->signals[1],
            this->wrap_tilde_02_low,
            this->wrap_tilde_02_high,
            this->signals[2],
            n
        );
    
        this->sample_02_perform(
            this->signals[2],
            this->sample_02_offset,
            this->signals[1],
            this->signals[0],
            this->dummyBuffer,
            n
        );
    
        this->hann_tilde_02_perform(this->signals[2], this->signals[3], n);
        this->dspexpr_26_perform(this->signals[1], this->signals[3], this->signals[2], n);
        this->dspexpr_30_perform(this->signals[0], this->signals[3], this->signals[1], n);
        this->rand_tilde_05_perform(this->rand_tilde_05_frequency, this->signals[3], n);
    
        this->cycle_tilde_05_perform(
            this->signals[3],
            this->cycle_tilde_05_phase_offset,
            this->signals[0],
            this->dummyBuffer,
            n
        );
    
        this->dspexpr_34_perform(this->signals[0], this->dspexpr_34_in2, this->signals[3], n);
        this->dspexpr_33_perform(this->signals[3], this->dspexpr_33_in2, this->signals[0], n);
        this->dspexpr_32_perform(this->signals[0], this->dspexpr_32_in2, this->signals[3], n);
    
        this->rampsmooth_tilde_05_perform(
            this->signals[3],
            this->rampsmooth_tilde_05_up,
            this->rampsmooth_tilde_05_down,
            this->signals[0],
            n
        );
    
        this->dspexpr_25_perform(this->signals[2], this->signals[0], this->signals[3], n);
        this->dspexpr_29_perform(this->signals[1], this->signals[0], this->signals[2], n);
        this->rand_tilde_06_perform(this->rand_tilde_06_frequency, this->signals[0], n);
    
        this->cycle_tilde_06_perform(
            this->signals[0],
            this->cycle_tilde_06_phase_offset,
            this->signals[1],
            this->dummyBuffer,
            n
        );
    
        this->dspexpr_37_perform(this->signals[1], this->dspexpr_37_in2, this->signals[0], n);
        this->dspexpr_36_perform(this->signals[0], this->dspexpr_36_in2, this->signals[1], n);
        this->dspexpr_35_perform(this->signals[1], this->dspexpr_35_in2, this->signals[0], n);
    
        this->rampsmooth_tilde_06_perform(
            this->signals[0],
            this->rampsmooth_tilde_06_up,
            this->rampsmooth_tilde_06_down,
            this->signals[1],
            n
        );
    
        this->pan_tilde_02_perform(
            this->signals[1],
            this->signals[3],
            this->signals[2],
            this->signals[0],
            this->signals[4],
            n
        );
    
        this->signaladder_05_perform(this->signals[0], out1, out1, n);
        this->signaladder_06_perform(this->signals[4], out2, out2, n);
        this->stackprotect_perform(n);
        this->audioProcessSampleCount += this->vs;
    }
    
    void prepareToProcess(number sampleRate, Index maxBlockSize, bool force) {
        RNBO_ASSERT(this->_isInitialized);
    
        if (this->maxvs < maxBlockSize || !this->didAllocateSignals) {
            Index i;
    
            for (i = 0; i < 5; i++) {
                this->signals[i] = resizeSignal(this->signals[i], this->maxvs, maxBlockSize);
            }
    
            this->zeroBuffer = resizeSignal(this->zeroBuffer, this->maxvs, maxBlockSize);
            this->dummyBuffer = resizeSignal(this->dummyBuffer, this->maxvs, maxBlockSize);
            this->didAllocateSignals = true;
        }
    
        const bool sampleRateChanged = sampleRate != this->sr;
        const bool maxvsChanged = maxBlockSize != this->maxvs;
        const bool forceDSPSetup = sampleRateChanged || maxvsChanged || force;
    
        if (sampleRateChanged || maxvsChanged) {
            this->vs = maxBlockSize;
            this->maxvs = maxBlockSize;
            this->sr = sampleRate;
            this->invsr = 1 / sampleRate;
        }
    
        this->rand_tilde_04_dspsetup(forceDSPSetup);
        this->cycle_tilde_04_dspsetup(forceDSPSetup);
        this->rampsmooth_tilde_07_dspsetup(forceDSPSetup);
        this->rand_tilde_05_dspsetup(forceDSPSetup);
        this->cycle_tilde_05_dspsetup(forceDSPSetup);
        this->rampsmooth_tilde_05_dspsetup(forceDSPSetup);
        this->rand_tilde_06_dspsetup(forceDSPSetup);
        this->cycle_tilde_06_dspsetup(forceDSPSetup);
        this->rampsmooth_tilde_06_dspsetup(forceDSPSetup);
    
        if (sampleRateChanged)
            this->onSampleRateChanged(sampleRate);
    }
    
    number msToSamps(MillisecondTime ms, number sampleRate) {
        return ms * sampleRate * 0.001;
    }
    
    MillisecondTime sampsToMs(SampleIndex samps) {
        return samps * (this->invsr * 1000);
    }
    
    Index getNumInputChannels() const {
        return 1;
    }
    
    Index getNumOutputChannels() const {
        return 2;
    }
    
    void getPreset(PatcherStateInterface& ) {}
    
    void setPreset(MillisecondTime , PatcherStateInterface& ) {}
    
    void setParameterValue(ParameterIndex , ParameterValue , MillisecondTime ) {}
    
    void processParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
        this->setParameterValue(index, value, time);
    }
    
    void processParameterBangEvent(ParameterIndex index, MillisecondTime time) {
        this->setParameterValue(index, this->getParameterValue(index), time);
    }
    
    void processNormalizedParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
        this->setParameterValueNormalized(index, value, time);
    }
    
    ParameterValue getParameterValue(ParameterIndex index)  {
        switch (index) {
        default:
            {
            return 0;
            }
        }
    }
    
    ParameterIndex getNumSignalInParameters() const {
        return 0;
    }
    
    ParameterIndex getNumSignalOutParameters() const {
        return 0;
    }
    
    ParameterIndex getNumParameters() const {
        return 0;
    }
    
    ConstCharPointer getParameterName(ParameterIndex index) const {
        switch (index) {
        default:
            {
            return "bogus";
            }
        }
    }
    
    ConstCharPointer getParameterId(ParameterIndex index) const {
        switch (index) {
        default:
            {
            return "bogus";
            }
        }
    }
    
    void getParameterInfo(ParameterIndex , ParameterInfo * ) const {}
    
    ParameterValue applyStepsToNormalizedParameterValue(ParameterValue normalizedValue, int steps) const {
        if (steps == 1) {
            if (normalizedValue > 0) {
                normalizedValue = 1.;
            }
        } else {
            ParameterValue oneStep = (number)1. / (steps - 1);
            ParameterValue numberOfSteps = rnbo_fround(normalizedValue / oneStep * 1 / (number)1) * (number)1;
            normalizedValue = numberOfSteps * oneStep;
        }
    
        return normalizedValue;
    }
    
    ParameterValue convertToNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
        switch (index) {
        default:
            {
            return value;
            }
        }
    }
    
    ParameterValue convertFromNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
        value = (value < 0 ? 0 : (value > 1 ? 1 : value));
    
        switch (index) {
        default:
            {
            return value;
            }
        }
    }
    
    ParameterValue constrainParameterValue(ParameterIndex index, ParameterValue value) const {
        switch (index) {
        default:
            {
            return value;
            }
        }
    }
    
    void processNumMessage(MessageTag , MessageTag , MillisecondTime , number ) {}
    
    void processListMessage(MessageTag , MessageTag , MillisecondTime , const list& ) {}
    
    void processBangMessage(MessageTag tag, MessageTag objectId, MillisecondTime time) {
        this->updateTime(time, (ENGINE*)nullptr);
    
        switch (tag) {
        case TAG("startupbang"):
            {
            if (TAG("Freezer-Voice~[1]/loadbang_obj-8") == objectId)
                this->loadbang_02_startupbang_bang();
    
            break;
            }
        }
    }
    
    MessageTagInfo resolveTag(MessageTag tag) const {
        switch (tag) {
        case TAG("startupbang"):
            {
            return "startupbang";
            }
        case TAG("Freezer-Voice~[1]/loadbang_obj-8"):
            {
            return "Freezer-Voice~[1]/loadbang_obj-8";
            }
        }
    
        return nullptr;
    }
    
    DataRef* getDataRef(DataRefIndex index)  {
        switch (index) {
        default:
            {
            return nullptr;
            }
        }
    }
    
    DataRefIndex getNumDataRefs() const {
        return 0;
    }
    
    void processDataViewUpdate(DataRefIndex index, MillisecondTime time) {
        this->updateTime(time, (ENGINE*)nullptr);
    
        if (index == 2) {
            this->sample_02_buffer = reInitDataView(this->sample_02_buffer, this->getPatcher()->FreezeB);
        }
    
        if (index == 1) {
            this->cycle_tilde_04_buffer = reInitDataView(this->cycle_tilde_04_buffer, this->getPatcher()->RNBODefaultSinus);
            this->cycle_tilde_05_buffer = reInitDataView(this->cycle_tilde_05_buffer, this->getPatcher()->RNBODefaultSinus);
            this->cycle_tilde_06_buffer = reInitDataView(this->cycle_tilde_06_buffer, this->getPatcher()->RNBODefaultSinus);
            this->cycle_tilde_04_bufferUpdated();
            this->cycle_tilde_05_bufferUpdated();
            this->cycle_tilde_06_bufferUpdated();
        }
    }
    
    void initialize() {
        RNBO_ASSERT(!this->_isInitialized);
        this->assign_defaults();
        this->applyState();
        this->sample_02_buffer = new Float64Buffer(this->getPatcher()->FreezeB);
        this->cycle_tilde_04_buffer = new SampleBuffer(this->getPatcher()->RNBODefaultSinus);
        this->cycle_tilde_05_buffer = new SampleBuffer(this->getPatcher()->RNBODefaultSinus);
        this->cycle_tilde_06_buffer = new SampleBuffer(this->getPatcher()->RNBODefaultSinus);
        this->_isInitialized = true;
    }
    
    protected:
    
    void updateTime(MillisecondTime time, INTERNALENGINE*, bool inProcess = false) {
    	if (time == TimeNow) time = getTopLevelPatcher()->getPatcherTime();
    	getTopLevelPatcher()->processInternalEvents(time);
    	updateTime(time, (EXTERNALENGINE*)nullptr);
    }
    
    RNBOSubpatcher_248* operator->() {
        return this;
    }
    const RNBOSubpatcher_248* operator->() const {
        return this;
    }
    virtual rnbomatic* getPatcher() const {
        return static_cast<rnbomatic *>(_parentPatcher);
    }
    
    rnbomatic* getTopLevelPatcher() {
        return this->getPatcher()->getTopLevelPatcher();
    }
    
    void cancelClockEvents()
    {
        getEngine()->flushClockEvents(this, -871642103, false);
    }
    
    inline number safemod(number f, number m) {
        if (m != 0) {
            Int f_trunc = (Int)(trunc(f));
            Int m_trunc = (Int)(trunc(m));
    
            if (f == f_trunc && m == m_trunc) {
                f = f_trunc % m_trunc;
            } else {
                if (m < 0) {
                    m = -m;
                }
    
                if (f >= m) {
                    if (f >= m * 2.0) {
                        number d = f / m;
                        Int i = (Int)(trunc(d));
                        d = d - i;
                        f = d * m;
                    } else {
                        f -= m;
                    }
                } else if (f <= -m) {
                    if (f <= -m * 2.0) {
                        number d = f / m;
                        Int i = (Int)(trunc(d));
                        d = d - i;
                        f = d * m;
                    } else {
                        f += m;
                    }
                }
            }
        } else {
            f = 0.0;
        }
    
        return f;
    }
    
    inline number safesqrt(number num) {
        return (num > 0.0 ? rnbo_sqrt(num) : 0.0);
    }
    
    number wrap(number x, number low, number high) {
        number lo;
        number hi;
    
        if (low == high)
            return low;
    
        if (low > high) {
            hi = low;
            lo = high;
        } else {
            lo = low;
            hi = high;
        }
    
        number range = hi - lo;
    
        if (x >= lo && x < hi)
            return x;
    
        if (range <= 0.000000001)
            return lo;
    
        Int numWraps = (Int)(trunc((x - lo) / range));
        numWraps = numWraps - ((x < lo ? 1 : 0));
        number result = x - range * numWraps;
    
        if (result >= hi)
            return result - range;
        else
            return result;
    }
    
    inline number linearinterp(number frac, number x, number y) {
        return x + (y - x) * frac;
    }
    
    inline number cubicinterp(number a, number w, number x, number y, number z) {
        number a1 = 1. + a;
        number aa = a * a1;
        number b = 1. - a;
        number b1 = 2. - a;
        number bb = b * b1;
        number fw = -.1666667 * bb * a;
        number fx = .5 * bb * a1;
        number fy = .5 * aa * b1;
        number fz = -.1666667 * aa * b;
        return w * fw + x * fx + y * fy + z * fz;
    }
    
    inline number fastcubicinterp(number a, number w, number x, number y, number z) {
        number a2 = a * a;
        number f0 = z - y - w + x;
        number f1 = w - x - f0;
        number f2 = y - w;
        number f3 = x;
        return f0 * a * a2 + f1 * a2 + f2 * a + f3;
    }
    
    inline number splineinterp(number a, number w, number x, number y, number z) {
        number a2 = a * a;
        number f0 = -0.5 * w + 1.5 * x - 1.5 * y + 0.5 * z;
        number f1 = w - 2.5 * x + 2 * y - 0.5 * z;
        number f2 = -0.5 * w + 0.5 * y;
        return f0 * a * a2 + f1 * a2 + f2 * a + x;
    }
    
    inline number spline6interp(number a, number y0, number y1, number y2, number y3, number y4, number y5) {
        number ym2py2 = y0 + y4;
        number ym1py1 = y1 + y3;
        number y2mym2 = y4 - y0;
        number y1mym1 = y3 - y1;
        number sixthym1py1 = (number)1 / (number)6.0 * ym1py1;
        number c0 = (number)1 / (number)120.0 * ym2py2 + (number)13 / (number)60.0 * ym1py1 + (number)11 / (number)20.0 * y2;
        number c1 = (number)1 / (number)24.0 * y2mym2 + (number)5 / (number)12.0 * y1mym1;
        number c2 = (number)1 / (number)12.0 * ym2py2 + sixthym1py1 - (number)1 / (number)2.0 * y2;
        number c3 = (number)1 / (number)12.0 * y2mym2 - (number)1 / (number)6.0 * y1mym1;
        number c4 = (number)1 / (number)24.0 * ym2py2 - sixthym1py1 + (number)1 / (number)4.0 * y2;
        number c5 = (number)1 / (number)120.0 * (y5 - y0) + (number)1 / (number)24.0 * (y1 - y4) + (number)1 / (number)12.0 * (y3 - y2);
        return ((((c5 * a + c4) * a + c3) * a + c2) * a + c1) * a + c0;
    }
    
    inline number cosT8(number r) {
        number t84 = 56.0;
        number t83 = 1680.0;
        number t82 = 20160.0;
        number t81 = 2.4801587302e-05;
        number t73 = 42.0;
        number t72 = 840.0;
        number t71 = 1.9841269841e-04;
    
        if (r < 0.785398163397448309615660845819875721 && r > -0.785398163397448309615660845819875721) {
            number rr = r * r;
            return 1.0 - rr * t81 * (t82 - rr * (t83 - rr * (t84 - rr)));
        } else if (r > 0.0) {
            r -= 1.57079632679489661923132169163975144;
            number rr = r * r;
            return -r * (1.0 - t71 * rr * (t72 - rr * (t73 - rr)));
        } else {
            r += 1.57079632679489661923132169163975144;
            number rr = r * r;
            return r * (1.0 - t71 * rr * (t72 - rr * (t73 - rr)));
        }
    }
    
    inline number cosineinterp(number frac, number x, number y) {
        number a2 = (1.0 - this->cosT8(frac * 3.14159265358979323846)) / (number)2.0;
        return x * (1.0 - a2) + y * a2;
    }
    
    template<typename BUFFERTYPE> array<SampleValue, 2 + 1> sample_boundmode_ignore_channelmode_ignore_indexmode_phase_interp_linear_channels_2(BUFFERTYPE& buffer, SampleValue sampleIndex, Index channelOffset) {
        number bufferSize = buffer->getSize();
        const Index bufferChannels = (const Index)(buffer->getChannels());
        constexpr int ___N3 = 2 + 1;
        array<SampleValue, ___N3> out = FIXEDSIZEARRAYINIT(2 + 1);
    
        {
            SampleValue bufferphasetoindex_result;
    
            {
                auto __end = bufferSize;
                auto __start = 0;
                auto __phase = sampleIndex;
                number size;
    
                {
                    size = __end - __start - 1;
                }
    
                {
                    bufferphasetoindex_result = __phase * size;
                }
            }
    
            sampleIndex = bufferphasetoindex_result;
        }
    
        if (bufferSize == 0 || (5 == 5 && (sampleIndex < 0 || sampleIndex >= bufferSize))) {
            for (Index i = 0; i < 2 + 1; i++) {
                out[(Index)i] = 0;
            }
        } else {
            for (Index channel = 0; channel < 2; channel++) {
                Index channelIndex = (Index)(channel + channelOffset);
    
                {
                    if (channelIndex >= bufferChannels || channelIndex < 0) {
                        out[(Index)channel] = 0;
                        continue;
                    }
                }
    
                SampleValue bufferreadsample_result;
    
                {
                    auto& __buffer = buffer;
    
                    if (sampleIndex < 0.0) {
                        bufferreadsample_result = 0.0;
                    }
    
                    SampleIndex truncIndex = (SampleIndex)(trunc(sampleIndex));
    
                    {
                        number frac = sampleIndex - truncIndex;
                        number wrap = bufferSize - 1;
    
                        {
                            SampleIndex index1 = (SampleIndex)(truncIndex);
    
                            if (index1 < 0)
                                index1 = wrap;
    
                            SampleIndex index2 = (SampleIndex)(index1 + 1);
    
                            if (index2 > wrap)
                                index2 = 0;
    
                            bufferreadsample_result = this->linearinterp(
                                frac,
                                __buffer->getSample(channelIndex, index1),
                                __buffer->getSample(channelIndex, index2)
                            );
                        }
                    }
                }
    
                out[(Index)channel] = bufferreadsample_result;
            }
    
            out[2] = sampleIndex;
        }
    
        return out;
    }
    
    Int notenumber() {
        return this->_noteNumber;
    }
    
    Index voice() {
        return this->_voiceIndex;
    }
    
    number random(number low, number high) {
        number range = high - low;
        return globalrandom() * range + low;
    }
    
    MillisecondTime getPatcherTime() const {
        return this->_currentTime;
    }
    
    void loadbang_02_startupbang_bang() {
        this->loadbang_02_output_bang();
    }
    
    void deallocateSignals() {
        Index i;
    
        for (i = 0; i < 5; i++) {
            this->signals[i] = freeSignal(this->signals[i]);
        }
    
        this->zeroBuffer = freeSignal(this->zeroBuffer);
        this->dummyBuffer = freeSignal(this->dummyBuffer);
    }
    
    Index getMaxBlockSize() const {
        return this->maxvs;
    }
    
    number getSampleRate() const {
        return this->sr;
    }
    
    bool hasFixedVectorSize() const {
        return false;
    }
    
    void setProbingTarget(MessageTag ) {}
    
    void initializeObjects() {
        this->rand_tilde_04_n_init();
        this->rand_tilde_04_init();
        this->rand_tilde_05_n_init();
        this->rand_tilde_05_init();
        this->rand_tilde_06_n_init();
        this->rand_tilde_06_init();
    }
    
    void setVoiceIndex(Index index)  {
        this->_voiceIndex = index;
    }
    
    void setNoteNumber(Int noteNumber)  {
        this->_noteNumber = noteNumber;
    }
    
    Index getIsMuted()  {
        return this->isMuted;
    }
    
    void setIsMuted(Index v)  {
        this->isMuted = v;
    }
    
    void onSampleRateChanged(double ) {}
    
    void extractState(PatcherStateInterface& ) {}
    
    void applyState() {}
    
    ParameterValue getPolyParameterValue(RNBOSubpatcher_248* voices, ParameterIndex index)  {
        switch (index) {
        default:
            {
            return voices[0]->getParameterValue(index);
            }
        }
    }
    
    void setPolyParameterValue(
        RNBOSubpatcher_248* voices,
        ParameterIndex index,
        ParameterValue value,
        MillisecondTime time
    ) {
        switch (index) {
        default:
            {
            for (Index i = 0; i < 8; i++)
                voices[i]->setParameterValue(index, value, time);
            }
        }
    }
    
    void sendPolyParameter(ParameterIndex index, Index voiceIndex, bool ignoreValue) {
        this->getPatcher()->sendParameter(index + this->parameterOffset + voiceIndex - 1, ignoreValue);
    }
    
    void setParameterOffset(ParameterIndex offset) {
        this->parameterOffset = offset;
    }
    
    void processClockEvent(MillisecondTime time, ClockId index, bool hasValue, ParameterValue value) {
        RNBO_UNUSED(value);
        RNBO_UNUSED(hasValue);
        this->updateTime(time, (ENGINE*)nullptr);
    
        switch (index) {
        case -871642103:
            {
            this->loadbang_02_startupbang_bang();
            break;
            }
        }
    }
    
    void processOutletAtCurrentTime(EngineLink* , OutletIndex , ParameterValue ) {}
    
    void processOutletEvent(
        EngineLink* sender,
        OutletIndex index,
        ParameterValue value,
        MillisecondTime time
    ) {
        this->updateTime(time, (ENGINE*)nullptr);
        this->processOutletAtCurrentTime(sender, index, value);
    }
    
    void sendOutlet(OutletIndex index, ParameterValue value) {
        this->getEngine()->sendOutlet(this, index, value);
    }
    
    void startup() {
        this->updateTime(this->getEngine()->getCurrentTime(), (ENGINE*)nullptr);
        this->getEngine()->scheduleClockEvent(this, -871642103, 0 + this->_currentTime);;
    }
    
    void fillDataRef(DataRefIndex , DataRef& ) {}
    
    void allocateDataRefs() {
        this->cycle_tilde_04_buffer->requestSize(16384, 1);
        this->cycle_tilde_04_buffer->setSampleRate(this->sr);
        this->cycle_tilde_05_buffer->requestSize(16384, 1);
        this->cycle_tilde_05_buffer->setSampleRate(this->sr);
        this->cycle_tilde_06_buffer->requestSize(16384, 1);
        this->cycle_tilde_06_buffer->setSampleRate(this->sr);
        this->sample_02_buffer = this->sample_02_buffer->allocateIfNeeded();
        this->cycle_tilde_04_buffer = this->cycle_tilde_04_buffer->allocateIfNeeded();
        this->cycle_tilde_05_buffer = this->cycle_tilde_05_buffer->allocateIfNeeded();
        this->cycle_tilde_06_buffer = this->cycle_tilde_06_buffer->allocateIfNeeded();
    }
    
    void dspexpr_31_in2_set(number v) {
        this->dspexpr_31_in2 = v;
    }
    
    void receive_10_output_number_set(number v) {
        this->receive_10_output_number = v;
        this->dspexpr_31_in2_set(v);
    }
    
    void voice_02_noteNumber_set(number ) {}
    
    void dspexpr_28_in2_set(number v) {
        this->dspexpr_28_in2 = v;
    }
    
    void expr_06_out1_set(number v) {
        this->expr_06_out1 = v;
        this->dspexpr_28_in2_set(this->expr_06_out1);
    }
    
    void expr_06_in1_set(number in1) {
        this->expr_06_in1 = in1;
    
        this->expr_06_out1_set(
            (this->expr_06_in2 == 0 ? 0 : (this->expr_06_in2 == 0. ? 0. : this->expr_06_in1 / this->expr_06_in2))
        );//#map:Freezer-Voice~[1]//_obj-7:1
    }
    
    void expr_07_out1_set(number v) {
        this->expr_07_out1 = v;
        this->expr_06_in1_set(this->expr_07_out1);
    }
    
    void expr_07_in1_set(number in1) {
        this->expr_07_in1 = in1;
        this->expr_07_out1_set(this->expr_07_in1 - this->expr_07_in2);//#map:Freezer-Voice~[1]/-_obj-6:1
    }
    
    void voice_02_voicenumber_set(number v) {
        this->expr_07_in1_set(v);
    }
    
    void voice_02_voicebang_bang() {
        this->voice_02_noteNumber_set(this->notenumber());
        this->voice_02_voicenumber_set(this->voice());
    }
    
    void loadbang_02_output_bang() {
        this->voice_02_voicebang_bang();
    }
    
    void midiouthelper_midiout_set(number ) {}
    
    void dspexpr_28_perform(const Sample * in1, number in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] + in2;//#map:_###_obj_###_:1
        }
    }
    
    void rand_tilde_04_perform(number frequency, SampleValue * out1, Index n) {
        auto __rand_tilde_04_lastValue = this->rand_tilde_04_lastValue;
        auto __rand_tilde_04_oldTargetValue = this->rand_tilde_04_oldTargetValue;
        auto __rand_tilde_04_targetValue = this->rand_tilde_04_targetValue;
        number f = (frequency > 0.0001 ? frequency : 0.0001);
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            number phasorValue = this->rand_tilde_04_p_next(f, -1);
    
            if (this->rand_tilde_04_d_next(phasorValue) < 0) {
                __rand_tilde_04_targetValue = this->rand_tilde_04_n_next();
                __rand_tilde_04_oldTargetValue = __rand_tilde_04_lastValue;
            }
    
            __rand_tilde_04_lastValue = __rand_tilde_04_oldTargetValue + phasorValue * (__rand_tilde_04_targetValue - __rand_tilde_04_oldTargetValue);
            out1[(Index)i] = __rand_tilde_04_lastValue;
        }
    
        this->rand_tilde_04_targetValue = __rand_tilde_04_targetValue;
        this->rand_tilde_04_oldTargetValue = __rand_tilde_04_oldTargetValue;
        this->rand_tilde_04_lastValue = __rand_tilde_04_lastValue;
    }
    
    void cycle_tilde_04_perform(
        const Sample * frequency,
        number phase_offset,
        SampleValue * out1,
        SampleValue * out2,
        Index n
    ) {
        RNBO_UNUSED(phase_offset);
        auto __cycle_tilde_04_buffer = this->cycle_tilde_04_buffer;
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            {
                UInt32 uint_phase;
    
                {
                    {
                        uint_phase = uint32_trunc(frequency[(Index)i] * 4294967296.0);
                    }
                }
    
                UInt32 idx = (UInt32)(uint32_rshift(uint_phase, 18));
                number frac = ((BinOpInt)((BinOpInt)uint_phase & (BinOpInt)262143)) * 3.81471181759574e-6;
                number y0 = __cycle_tilde_04_buffer[(Index)idx];
                number y1 = __cycle_tilde_04_buffer[(Index)((BinOpInt)(idx + 1) & (BinOpInt)16383)];
                number y = y0 + frac * (y1 - y0);
                out1[(Index)i] = y;
                out2[(Index)i] = uint_phase * 0.232830643653869629e-9;
                continue;
            }
        }
    }
    
    void dspexpr_31_perform(const Sample * in1, number in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] * in2;//#map:_###_obj_###_:1
        }
    }
    
    void rampsmooth_tilde_07_perform(const Sample * x, number up, number down, SampleValue * out1, Index n) {
        RNBO_UNUSED(down);
        RNBO_UNUSED(up);
        auto __rampsmooth_tilde_07_increment = this->rampsmooth_tilde_07_increment;
        auto __rampsmooth_tilde_07_index = this->rampsmooth_tilde_07_index;
        auto __rampsmooth_tilde_07_prev = this->rampsmooth_tilde_07_prev;
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            if (this->rampsmooth_tilde_07_d_next(x[(Index)i]) != 0.) {
                if (x[(Index)i] > __rampsmooth_tilde_07_prev) {
                    number _up = 480;
    
                    if (_up < 1)
                        _up = 1;
    
                    __rampsmooth_tilde_07_index = _up;
                    __rampsmooth_tilde_07_increment = (x[(Index)i] - __rampsmooth_tilde_07_prev) / _up;
                } else if (x[(Index)i] < __rampsmooth_tilde_07_prev) {
                    number _down = 480;
    
                    if (_down < 1)
                        _down = 1;
    
                    __rampsmooth_tilde_07_index = _down;
                    __rampsmooth_tilde_07_increment = (x[(Index)i] - __rampsmooth_tilde_07_prev) / _down;
                }
            }
    
            if (__rampsmooth_tilde_07_index > 0) {
                __rampsmooth_tilde_07_prev += __rampsmooth_tilde_07_increment;
                __rampsmooth_tilde_07_index -= 1;
            } else {
                __rampsmooth_tilde_07_prev = x[(Index)i];
            }
    
            out1[(Index)i] = __rampsmooth_tilde_07_prev;
        }
    
        this->rampsmooth_tilde_07_prev = __rampsmooth_tilde_07_prev;
        this->rampsmooth_tilde_07_index = __rampsmooth_tilde_07_index;
        this->rampsmooth_tilde_07_increment = __rampsmooth_tilde_07_increment;
    }
    
    void dspexpr_27_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] + in2[(Index)i];//#map:_###_obj_###_:1
        }
    }
    
    void wrap_tilde_02_perform(const Sample * x, number low, number high, SampleValue * out1, Index n) {
        RNBO_UNUSED(high);
        RNBO_UNUSED(low);
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            number lo;
            number hi;
            lo = 0;
            hi = 1;
            number range = hi - lo;
    
            if (x[(Index)i] >= lo && x[(Index)i] < hi) {
                out1[(Index)i] = x[(Index)i];
                continue;
            }
    
            if (range <= 0.000000001) {
                out1[(Index)i] = lo;
                continue;
            }
    
            Int numWraps = (Int)(trunc((x[(Index)i] - lo) / range));
            numWraps = numWraps - ((x[(Index)i] < lo ? 1 : 0));
            number result = x[(Index)i] - range * numWraps;
    
            if (result >= hi) {
                out1[(Index)i] = result - range;
                continue;
            } else {
                out1[(Index)i] = result;
                continue;
            }
        }
    }
    
    void sample_02_perform(
        const Sample * value,
        number offset,
        SampleValue * out1,
        SampleValue * out2,
        SampleValue * outindex,
        Index n
    ) {
        RNBO_UNUSED(out2);
        RNBO_UNUSED(out1);
        RNBO_UNUSED(offset);
        auto __sample_02_buffer = this->sample_02_buffer;
        SampleArray<2> out = {out1, out2};
    
        for (Index i = 0; i < n; i++) {
            auto result = this->sample_boundmode_ignore_channelmode_ignore_indexmode_phase_interp_linear_channels_2(__sample_02_buffer, value[(Index)i], 0);
    
            for (Index j = 0; j < 2; j++) {
                out[(Index)j][(Index)i] = result[(Index)j];
            }
    
            outindex[(Index)i] = result[2];
        }
    }
    
    void hann_tilde_02_perform(const Sample * x, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            number __x = x[(Index)i];
            __x = (__x > 1 ? 1 : (__x < 0 ? 0 : __x));
            out1[(Index)i] = 0.5 * (1 - rnbo_cos(6.28318530717958647692 * __x));
        }
    }
    
    void dspexpr_26_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] * in2[(Index)i];//#map:_###_obj_###_:1
        }
    }
    
    void dspexpr_30_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] * in2[(Index)i];//#map:_###_obj_###_:1
        }
    }
    
    void rand_tilde_05_perform(number frequency, SampleValue * out1, Index n) {
        auto __rand_tilde_05_lastValue = this->rand_tilde_05_lastValue;
        auto __rand_tilde_05_oldTargetValue = this->rand_tilde_05_oldTargetValue;
        auto __rand_tilde_05_targetValue = this->rand_tilde_05_targetValue;
        number f = (frequency > 0.0001 ? frequency : 0.0001);
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            number phasorValue = this->rand_tilde_05_p_next(f, -1);
    
            if (this->rand_tilde_05_d_next(phasorValue) < 0) {
                __rand_tilde_05_targetValue = this->rand_tilde_05_n_next();
                __rand_tilde_05_oldTargetValue = __rand_tilde_05_lastValue;
            }
    
            __rand_tilde_05_lastValue = __rand_tilde_05_oldTargetValue + phasorValue * (__rand_tilde_05_targetValue - __rand_tilde_05_oldTargetValue);
            out1[(Index)i] = __rand_tilde_05_lastValue;
        }
    
        this->rand_tilde_05_targetValue = __rand_tilde_05_targetValue;
        this->rand_tilde_05_oldTargetValue = __rand_tilde_05_oldTargetValue;
        this->rand_tilde_05_lastValue = __rand_tilde_05_lastValue;
    }
    
    void cycle_tilde_05_perform(
        const Sample * frequency,
        number phase_offset,
        SampleValue * out1,
        SampleValue * out2,
        Index n
    ) {
        RNBO_UNUSED(phase_offset);
        auto __cycle_tilde_05_buffer = this->cycle_tilde_05_buffer;
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            {
                UInt32 uint_phase;
    
                {
                    {
                        uint_phase = uint32_trunc(frequency[(Index)i] * 4294967296.0);
                    }
                }
    
                UInt32 idx = (UInt32)(uint32_rshift(uint_phase, 18));
                number frac = ((BinOpInt)((BinOpInt)uint_phase & (BinOpInt)262143)) * 3.81471181759574e-6;
                number y0 = __cycle_tilde_05_buffer[(Index)idx];
                number y1 = __cycle_tilde_05_buffer[(Index)((BinOpInt)(idx + 1) & (BinOpInt)16383)];
                number y = y0 + frac * (y1 - y0);
                out1[(Index)i] = y;
                out2[(Index)i] = uint_phase * 0.232830643653869629e-9;
                continue;
            }
        }
    }
    
    void dspexpr_34_perform(const Sample * in1, number in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] * in2;//#map:_###_obj_###_:1
        }
    }
    
    void dspexpr_33_perform(const Sample * in1, number in2, SampleValue * out1, Index n) {
        RNBO_UNUSED(in2);
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] + 1;//#map:_###_obj_###_:1
        }
    }
    
    void dspexpr_32_perform(const Sample * in1, number in2, SampleValue * out1, Index n) {
        RNBO_UNUSED(in2);
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] * 0.5;//#map:_###_obj_###_:1
        }
    }
    
    void rampsmooth_tilde_05_perform(const Sample * x, number up, number down, SampleValue * out1, Index n) {
        RNBO_UNUSED(down);
        RNBO_UNUSED(up);
        auto __rampsmooth_tilde_05_increment = this->rampsmooth_tilde_05_increment;
        auto __rampsmooth_tilde_05_index = this->rampsmooth_tilde_05_index;
        auto __rampsmooth_tilde_05_prev = this->rampsmooth_tilde_05_prev;
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            if (this->rampsmooth_tilde_05_d_next(x[(Index)i]) != 0.) {
                if (x[(Index)i] > __rampsmooth_tilde_05_prev) {
                    number _up = 480;
    
                    if (_up < 1)
                        _up = 1;
    
                    __rampsmooth_tilde_05_index = _up;
                    __rampsmooth_tilde_05_increment = (x[(Index)i] - __rampsmooth_tilde_05_prev) / _up;
                } else if (x[(Index)i] < __rampsmooth_tilde_05_prev) {
                    number _down = 480;
    
                    if (_down < 1)
                        _down = 1;
    
                    __rampsmooth_tilde_05_index = _down;
                    __rampsmooth_tilde_05_increment = (x[(Index)i] - __rampsmooth_tilde_05_prev) / _down;
                }
            }
    
            if (__rampsmooth_tilde_05_index > 0) {
                __rampsmooth_tilde_05_prev += __rampsmooth_tilde_05_increment;
                __rampsmooth_tilde_05_index -= 1;
            } else {
                __rampsmooth_tilde_05_prev = x[(Index)i];
            }
    
            out1[(Index)i] = __rampsmooth_tilde_05_prev;
        }
    
        this->rampsmooth_tilde_05_prev = __rampsmooth_tilde_05_prev;
        this->rampsmooth_tilde_05_index = __rampsmooth_tilde_05_index;
        this->rampsmooth_tilde_05_increment = __rampsmooth_tilde_05_increment;
    }
    
    void dspexpr_25_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] * in2[(Index)i];//#map:_###_obj_###_:1
        }
    }
    
    void dspexpr_29_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] * in2[(Index)i];//#map:_###_obj_###_:1
        }
    }
    
    void rand_tilde_06_perform(number frequency, SampleValue * out1, Index n) {
        auto __rand_tilde_06_lastValue = this->rand_tilde_06_lastValue;
        auto __rand_tilde_06_oldTargetValue = this->rand_tilde_06_oldTargetValue;
        auto __rand_tilde_06_targetValue = this->rand_tilde_06_targetValue;
        number f = (frequency > 0.0001 ? frequency : 0.0001);
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            number phasorValue = this->rand_tilde_06_p_next(f, -1);
    
            if (this->rand_tilde_06_d_next(phasorValue) < 0) {
                __rand_tilde_06_targetValue = this->rand_tilde_06_n_next();
                __rand_tilde_06_oldTargetValue = __rand_tilde_06_lastValue;
            }
    
            __rand_tilde_06_lastValue = __rand_tilde_06_oldTargetValue + phasorValue * (__rand_tilde_06_targetValue - __rand_tilde_06_oldTargetValue);
            out1[(Index)i] = __rand_tilde_06_lastValue;
        }
    
        this->rand_tilde_06_targetValue = __rand_tilde_06_targetValue;
        this->rand_tilde_06_oldTargetValue = __rand_tilde_06_oldTargetValue;
        this->rand_tilde_06_lastValue = __rand_tilde_06_lastValue;
    }
    
    void cycle_tilde_06_perform(
        const Sample * frequency,
        number phase_offset,
        SampleValue * out1,
        SampleValue * out2,
        Index n
    ) {
        RNBO_UNUSED(phase_offset);
        auto __cycle_tilde_06_buffer = this->cycle_tilde_06_buffer;
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            {
                UInt32 uint_phase;
    
                {
                    {
                        uint_phase = uint32_trunc(frequency[(Index)i] * 4294967296.0);
                    }
                }
    
                UInt32 idx = (UInt32)(uint32_rshift(uint_phase, 18));
                number frac = ((BinOpInt)((BinOpInt)uint_phase & (BinOpInt)262143)) * 3.81471181759574e-6;
                number y0 = __cycle_tilde_06_buffer[(Index)idx];
                number y1 = __cycle_tilde_06_buffer[(Index)((BinOpInt)(idx + 1) & (BinOpInt)16383)];
                number y = y0 + frac * (y1 - y0);
                out1[(Index)i] = y;
                out2[(Index)i] = uint_phase * 0.232830643653869629e-9;
                continue;
            }
        }
    }
    
    void dspexpr_37_perform(const Sample * in1, number in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] * in2;//#map:_###_obj_###_:1
        }
    }
    
    void dspexpr_36_perform(const Sample * in1, number in2, SampleValue * out1, Index n) {
        RNBO_UNUSED(in2);
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] + 1;//#map:_###_obj_###_:1
        }
    }
    
    void dspexpr_35_perform(const Sample * in1, number in2, SampleValue * out1, Index n) {
        RNBO_UNUSED(in2);
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] * 0.5;//#map:_###_obj_###_:1
        }
    }
    
    void rampsmooth_tilde_06_perform(const Sample * x, number up, number down, SampleValue * out1, Index n) {
        RNBO_UNUSED(down);
        RNBO_UNUSED(up);
        auto __rampsmooth_tilde_06_increment = this->rampsmooth_tilde_06_increment;
        auto __rampsmooth_tilde_06_index = this->rampsmooth_tilde_06_index;
        auto __rampsmooth_tilde_06_prev = this->rampsmooth_tilde_06_prev;
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            if (this->rampsmooth_tilde_06_d_next(x[(Index)i]) != 0.) {
                if (x[(Index)i] > __rampsmooth_tilde_06_prev) {
                    number _up = 480;
    
                    if (_up < 1)
                        _up = 1;
    
                    __rampsmooth_tilde_06_index = _up;
                    __rampsmooth_tilde_06_increment = (x[(Index)i] - __rampsmooth_tilde_06_prev) / _up;
                } else if (x[(Index)i] < __rampsmooth_tilde_06_prev) {
                    number _down = 480;
    
                    if (_down < 1)
                        _down = 1;
    
                    __rampsmooth_tilde_06_index = _down;
                    __rampsmooth_tilde_06_increment = (x[(Index)i] - __rampsmooth_tilde_06_prev) / _down;
                }
            }
    
            if (__rampsmooth_tilde_06_index > 0) {
                __rampsmooth_tilde_06_prev += __rampsmooth_tilde_06_increment;
                __rampsmooth_tilde_06_index -= 1;
            } else {
                __rampsmooth_tilde_06_prev = x[(Index)i];
            }
    
            out1[(Index)i] = __rampsmooth_tilde_06_prev;
        }
    
        this->rampsmooth_tilde_06_prev = __rampsmooth_tilde_06_prev;
        this->rampsmooth_tilde_06_index = __rampsmooth_tilde_06_index;
        this->rampsmooth_tilde_06_increment = __rampsmooth_tilde_06_increment;
    }
    
    void pan_tilde_02_perform(
        const Sample * pos,
        const SampleValue * in1,
        const SampleValue * in2,
        SampleValue * out1,
        SampleValue * out2,
        Index n
    ) {
        auto __pan_tilde_02_spread = this->pan_tilde_02_spread;
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out2[(Index)i] = in1[(Index)i] * this->pan_tilde_02_func_next(pos[(Index)i] + -0.5 * __pan_tilde_02_spread, 1) + in2[(Index)i] * this->pan_tilde_02_func_next(pos[(Index)i] + 0.5 * __pan_tilde_02_spread, 1);
            out1[(Index)i] = in1[(Index)i] * this->pan_tilde_02_func_next(pos[(Index)i] + -0.5 * __pan_tilde_02_spread, 0) + in2[(Index)i] * this->pan_tilde_02_func_next(pos[(Index)i] + 0.5 * __pan_tilde_02_spread, 0);
        }
    }
    
    void signaladder_05_perform(
        const SampleValue * in1,
        const SampleValue * in2,
        SampleValue * out,
        Index n
    ) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out[(Index)i] = in1[(Index)i] + in2[(Index)i];
        }
    }
    
    void signaladder_06_perform(
        const SampleValue * in1,
        const SampleValue * in2,
        SampleValue * out,
        Index n
    ) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out[(Index)i] = in1[(Index)i] + in2[(Index)i];
        }
    }
    
    void stackprotect_perform(Index n) {
        RNBO_UNUSED(n);
        auto __stackprotect_count = this->stackprotect_count;
        __stackprotect_count = 0;
        this->stackprotect_count = __stackprotect_count;
    }
    
    number pan_tilde_02_func_next(number pos, Int channel) {
        {
            {
                number nchan_1 = 2 - 1;
    
                {
                    pos = pos * nchan_1;
                }
    
                {
                    {
                        if (pos >= 0) {
                            pos = this->safemod(pos, 2);
                        } else {
                            pos = this->safemod(2 + this->safemod(pos, 2), 2);
                        }
    
                        if (channel == 0 && pos >= nchan_1)
                            pos = pos - 2;
                    }
                }
    
                pos = pos - channel;
    
                if (pos > -1 && pos < 1) {
                    {
                        {
                            return this->safesqrt(1.0 - rnbo_abs(pos));
                        }
                    }
                } else {
                    return 0;
                }
            }
        }
    }
    
    void pan_tilde_02_func_reset() {}
    
    number rampsmooth_tilde_05_d_next(number x) {
        number temp = (number)(x - this->rampsmooth_tilde_05_d_prev);
        this->rampsmooth_tilde_05_d_prev = x;
        return temp;
    }
    
    void rampsmooth_tilde_05_d_dspsetup() {
        this->rampsmooth_tilde_05_d_reset();
    }
    
    void rampsmooth_tilde_05_d_reset() {
        this->rampsmooth_tilde_05_d_prev = 0;
    }
    
    void rampsmooth_tilde_05_dspsetup(bool force) {
        if ((bool)(this->rampsmooth_tilde_05_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->rampsmooth_tilde_05_setupDone = true;
        this->rampsmooth_tilde_05_d_dspsetup();
    }
    
    number rampsmooth_tilde_06_d_next(number x) {
        number temp = (number)(x - this->rampsmooth_tilde_06_d_prev);
        this->rampsmooth_tilde_06_d_prev = x;
        return temp;
    }
    
    void rampsmooth_tilde_06_d_dspsetup() {
        this->rampsmooth_tilde_06_d_reset();
    }
    
    void rampsmooth_tilde_06_d_reset() {
        this->rampsmooth_tilde_06_d_prev = 0;
    }
    
    void rampsmooth_tilde_06_dspsetup(bool force) {
        if ((bool)(this->rampsmooth_tilde_06_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->rampsmooth_tilde_06_setupDone = true;
        this->rampsmooth_tilde_06_d_dspsetup();
    }
    
    number rampsmooth_tilde_07_d_next(number x) {
        number temp = (number)(x - this->rampsmooth_tilde_07_d_prev);
        this->rampsmooth_tilde_07_d_prev = x;
        return temp;
    }
    
    void rampsmooth_tilde_07_d_dspsetup() {
        this->rampsmooth_tilde_07_d_reset();
    }
    
    void rampsmooth_tilde_07_d_reset() {
        this->rampsmooth_tilde_07_d_prev = 0;
    }
    
    void rampsmooth_tilde_07_dspsetup(bool force) {
        if ((bool)(this->rampsmooth_tilde_07_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->rampsmooth_tilde_07_setupDone = true;
        this->rampsmooth_tilde_07_d_dspsetup();
    }
    
    number cycle_tilde_04_ph_next(number freq, number reset) {
        {
            {
                if (reset >= 0.)
                    this->cycle_tilde_04_ph_currentPhase = reset;
            }
        }
    
        number pincr = freq * this->cycle_tilde_04_ph_conv;
    
        if (this->cycle_tilde_04_ph_currentPhase < 0.)
            this->cycle_tilde_04_ph_currentPhase = 1. + this->cycle_tilde_04_ph_currentPhase;
    
        if (this->cycle_tilde_04_ph_currentPhase > 1.)
            this->cycle_tilde_04_ph_currentPhase = this->cycle_tilde_04_ph_currentPhase - 1.;
    
        number tmp = this->cycle_tilde_04_ph_currentPhase;
        this->cycle_tilde_04_ph_currentPhase += pincr;
        return tmp;
    }
    
    void cycle_tilde_04_ph_reset() {
        this->cycle_tilde_04_ph_currentPhase = 0;
    }
    
    void cycle_tilde_04_ph_dspsetup() {
        this->cycle_tilde_04_ph_conv = (number)1 / this->sr;
    }
    
    void cycle_tilde_04_dspsetup(bool force) {
        if ((bool)(this->cycle_tilde_04_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->cycle_tilde_04_phasei = 0;
        this->cycle_tilde_04_f2i = (number)4294967296 / this->sr;
        this->cycle_tilde_04_wrap = (Int)(this->cycle_tilde_04_buffer->getSize()) - 1;
        this->cycle_tilde_04_setupDone = true;
        this->cycle_tilde_04_ph_dspsetup();
    }
    
    void cycle_tilde_04_bufferUpdated() {
        this->cycle_tilde_04_wrap = (Int)(this->cycle_tilde_04_buffer->getSize()) - 1;
    }
    
    number rand_tilde_04_p_next(number freq, number reset) {
        {
            {
                if (reset >= 0.)
                    this->rand_tilde_04_p_currentPhase = reset;
            }
        }
    
        number pincr = freq * this->rand_tilde_04_p_conv;
    
        if (this->rand_tilde_04_p_currentPhase < 0.)
            this->rand_tilde_04_p_currentPhase = 1. + this->rand_tilde_04_p_currentPhase;
    
        if (this->rand_tilde_04_p_currentPhase > 1.)
            this->rand_tilde_04_p_currentPhase = this->rand_tilde_04_p_currentPhase - 1.;
    
        number tmp = this->rand_tilde_04_p_currentPhase;
        this->rand_tilde_04_p_currentPhase += pincr;
        return tmp;
    }
    
    void rand_tilde_04_p_reset() {
        this->rand_tilde_04_p_currentPhase = 0;
    }
    
    void rand_tilde_04_p_dspsetup() {
        this->rand_tilde_04_p_conv = (number)1 / this->sr;
    }
    
    void rand_tilde_04_n_reset() {
        xoshiro_reset(
            systemticks() + this->voice() + this->random(0, 10000),
            this->rand_tilde_04_n_state
        );
    }
    
    void rand_tilde_04_n_init() {
        this->rand_tilde_04_n_reset();
    }
    
    void rand_tilde_04_n_seed(number v) {
        xoshiro_reset(v, this->rand_tilde_04_n_state);
    }
    
    number rand_tilde_04_n_next() {
        return xoshiro_next(this->rand_tilde_04_n_state);
    }
    
    number rand_tilde_04_d_next(number x) {
        number temp = (number)(x - this->rand_tilde_04_d_prev);
        this->rand_tilde_04_d_prev = x;
        return temp;
    }
    
    void rand_tilde_04_d_dspsetup() {
        this->rand_tilde_04_d_reset();
    }
    
    void rand_tilde_04_d_reset() {
        this->rand_tilde_04_d_prev = 0;
    }
    
    void rand_tilde_04_init() {
        this->rand_tilde_04_targetValue = this->rand_tilde_04_n_next();
    }
    
    void rand_tilde_04_dspsetup(bool force) {
        if ((bool)(this->rand_tilde_04_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->rand_tilde_04_setupDone = true;
        this->rand_tilde_04_p_dspsetup();
        this->rand_tilde_04_d_dspsetup();
    }
    
    number cycle_tilde_05_ph_next(number freq, number reset) {
        {
            {
                if (reset >= 0.)
                    this->cycle_tilde_05_ph_currentPhase = reset;
            }
        }
    
        number pincr = freq * this->cycle_tilde_05_ph_conv;
    
        if (this->cycle_tilde_05_ph_currentPhase < 0.)
            this->cycle_tilde_05_ph_currentPhase = 1. + this->cycle_tilde_05_ph_currentPhase;
    
        if (this->cycle_tilde_05_ph_currentPhase > 1.)
            this->cycle_tilde_05_ph_currentPhase = this->cycle_tilde_05_ph_currentPhase - 1.;
    
        number tmp = this->cycle_tilde_05_ph_currentPhase;
        this->cycle_tilde_05_ph_currentPhase += pincr;
        return tmp;
    }
    
    void cycle_tilde_05_ph_reset() {
        this->cycle_tilde_05_ph_currentPhase = 0;
    }
    
    void cycle_tilde_05_ph_dspsetup() {
        this->cycle_tilde_05_ph_conv = (number)1 / this->sr;
    }
    
    void cycle_tilde_05_dspsetup(bool force) {
        if ((bool)(this->cycle_tilde_05_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->cycle_tilde_05_phasei = 0;
        this->cycle_tilde_05_f2i = (number)4294967296 / this->sr;
        this->cycle_tilde_05_wrap = (Int)(this->cycle_tilde_05_buffer->getSize()) - 1;
        this->cycle_tilde_05_setupDone = true;
        this->cycle_tilde_05_ph_dspsetup();
    }
    
    void cycle_tilde_05_bufferUpdated() {
        this->cycle_tilde_05_wrap = (Int)(this->cycle_tilde_05_buffer->getSize()) - 1;
    }
    
    number rand_tilde_05_p_next(number freq, number reset) {
        {
            {
                if (reset >= 0.)
                    this->rand_tilde_05_p_currentPhase = reset;
            }
        }
    
        number pincr = freq * this->rand_tilde_05_p_conv;
    
        if (this->rand_tilde_05_p_currentPhase < 0.)
            this->rand_tilde_05_p_currentPhase = 1. + this->rand_tilde_05_p_currentPhase;
    
        if (this->rand_tilde_05_p_currentPhase > 1.)
            this->rand_tilde_05_p_currentPhase = this->rand_tilde_05_p_currentPhase - 1.;
    
        number tmp = this->rand_tilde_05_p_currentPhase;
        this->rand_tilde_05_p_currentPhase += pincr;
        return tmp;
    }
    
    void rand_tilde_05_p_reset() {
        this->rand_tilde_05_p_currentPhase = 0;
    }
    
    void rand_tilde_05_p_dspsetup() {
        this->rand_tilde_05_p_conv = (number)1 / this->sr;
    }
    
    void rand_tilde_05_n_reset() {
        xoshiro_reset(
            systemticks() + this->voice() + this->random(0, 10000),
            this->rand_tilde_05_n_state
        );
    }
    
    void rand_tilde_05_n_init() {
        this->rand_tilde_05_n_reset();
    }
    
    void rand_tilde_05_n_seed(number v) {
        xoshiro_reset(v, this->rand_tilde_05_n_state);
    }
    
    number rand_tilde_05_n_next() {
        return xoshiro_next(this->rand_tilde_05_n_state);
    }
    
    number rand_tilde_05_d_next(number x) {
        number temp = (number)(x - this->rand_tilde_05_d_prev);
        this->rand_tilde_05_d_prev = x;
        return temp;
    }
    
    void rand_tilde_05_d_dspsetup() {
        this->rand_tilde_05_d_reset();
    }
    
    void rand_tilde_05_d_reset() {
        this->rand_tilde_05_d_prev = 0;
    }
    
    void rand_tilde_05_init() {
        this->rand_tilde_05_targetValue = this->rand_tilde_05_n_next();
    }
    
    void rand_tilde_05_dspsetup(bool force) {
        if ((bool)(this->rand_tilde_05_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->rand_tilde_05_setupDone = true;
        this->rand_tilde_05_p_dspsetup();
        this->rand_tilde_05_d_dspsetup();
    }
    
    number cycle_tilde_06_ph_next(number freq, number reset) {
        {
            {
                if (reset >= 0.)
                    this->cycle_tilde_06_ph_currentPhase = reset;
            }
        }
    
        number pincr = freq * this->cycle_tilde_06_ph_conv;
    
        if (this->cycle_tilde_06_ph_currentPhase < 0.)
            this->cycle_tilde_06_ph_currentPhase = 1. + this->cycle_tilde_06_ph_currentPhase;
    
        if (this->cycle_tilde_06_ph_currentPhase > 1.)
            this->cycle_tilde_06_ph_currentPhase = this->cycle_tilde_06_ph_currentPhase - 1.;
    
        number tmp = this->cycle_tilde_06_ph_currentPhase;
        this->cycle_tilde_06_ph_currentPhase += pincr;
        return tmp;
    }
    
    void cycle_tilde_06_ph_reset() {
        this->cycle_tilde_06_ph_currentPhase = 0;
    }
    
    void cycle_tilde_06_ph_dspsetup() {
        this->cycle_tilde_06_ph_conv = (number)1 / this->sr;
    }
    
    void cycle_tilde_06_dspsetup(bool force) {
        if ((bool)(this->cycle_tilde_06_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->cycle_tilde_06_phasei = 0;
        this->cycle_tilde_06_f2i = (number)4294967296 / this->sr;
        this->cycle_tilde_06_wrap = (Int)(this->cycle_tilde_06_buffer->getSize()) - 1;
        this->cycle_tilde_06_setupDone = true;
        this->cycle_tilde_06_ph_dspsetup();
    }
    
    void cycle_tilde_06_bufferUpdated() {
        this->cycle_tilde_06_wrap = (Int)(this->cycle_tilde_06_buffer->getSize()) - 1;
    }
    
    number rand_tilde_06_p_next(number freq, number reset) {
        {
            {
                if (reset >= 0.)
                    this->rand_tilde_06_p_currentPhase = reset;
            }
        }
    
        number pincr = freq * this->rand_tilde_06_p_conv;
    
        if (this->rand_tilde_06_p_currentPhase < 0.)
            this->rand_tilde_06_p_currentPhase = 1. + this->rand_tilde_06_p_currentPhase;
    
        if (this->rand_tilde_06_p_currentPhase > 1.)
            this->rand_tilde_06_p_currentPhase = this->rand_tilde_06_p_currentPhase - 1.;
    
        number tmp = this->rand_tilde_06_p_currentPhase;
        this->rand_tilde_06_p_currentPhase += pincr;
        return tmp;
    }
    
    void rand_tilde_06_p_reset() {
        this->rand_tilde_06_p_currentPhase = 0;
    }
    
    void rand_tilde_06_p_dspsetup() {
        this->rand_tilde_06_p_conv = (number)1 / this->sr;
    }
    
    void rand_tilde_06_n_reset() {
        xoshiro_reset(
            systemticks() + this->voice() + this->random(0, 10000),
            this->rand_tilde_06_n_state
        );
    }
    
    void rand_tilde_06_n_init() {
        this->rand_tilde_06_n_reset();
    }
    
    void rand_tilde_06_n_seed(number v) {
        xoshiro_reset(v, this->rand_tilde_06_n_state);
    }
    
    number rand_tilde_06_n_next() {
        return xoshiro_next(this->rand_tilde_06_n_state);
    }
    
    number rand_tilde_06_d_next(number x) {
        number temp = (number)(x - this->rand_tilde_06_d_prev);
        this->rand_tilde_06_d_prev = x;
        return temp;
    }
    
    void rand_tilde_06_d_dspsetup() {
        this->rand_tilde_06_d_reset();
    }
    
    void rand_tilde_06_d_reset() {
        this->rand_tilde_06_d_prev = 0;
    }
    
    void rand_tilde_06_init() {
        this->rand_tilde_06_targetValue = this->rand_tilde_06_n_next();
    }
    
    void rand_tilde_06_dspsetup(bool force) {
        if ((bool)(this->rand_tilde_06_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->rand_tilde_06_setupDone = true;
        this->rand_tilde_06_p_dspsetup();
        this->rand_tilde_06_d_dspsetup();
    }
    
    void midiouthelper_sendMidi(number v) {
        this->midiouthelper_midiout_set(v);
    }
    
    bool stackprotect_check() {
        this->stackprotect_count++;
    
        if (this->stackprotect_count > 128) {
            console->log("STACK OVERFLOW DETECTED - stopped processing branch !");
            return true;
        }
    
        return false;
    }
    
    Index getPatcherSerial() const {
        return 0;
    }
    
    void sendParameter(ParameterIndex index, bool ignoreValue) {
        if (this->_voiceIndex == 1)
            this->getPatcher()->sendParameter(index + this->parameterOffset, ignoreValue);
    }
    
    void scheduleParamInit(ParameterIndex index, Index order) {
        this->getPatcher()->scheduleParamInit(index + this->parameterOffset, order);
    }
    
    void updateTime(MillisecondTime time, EXTERNALENGINE* engine, bool inProcess = false) {
        RNBO_UNUSED(inProcess);
        RNBO_UNUSED(engine);
        this->_currentTime = time;
        auto offset = rnbo_fround(this->msToSamps(time - this->getEngine()->getCurrentTime(), this->sr));
    
        if (offset >= (SampleIndex)(this->vs))
            offset = (SampleIndex)(this->vs) - 1;
    
        if (offset < 0)
            offset = 0;
    
        this->sampleOffsetIntoNextAudioBuffer = (Index)(offset);
    }
    
    void assign_defaults()
    {
        dspexpr_25_in1 = 0;
        dspexpr_25_in2 = 0;
        pan_tilde_02_spread = 0.5;
        pan_tilde_02_pos = 0;
        dspexpr_26_in1 = 0;
        dspexpr_26_in2 = 0;
        sample_02_value = 0;
        sample_02_offset = 0;
        wrap_tilde_02_x = 0;
        wrap_tilde_02_low = 0;
        wrap_tilde_02_high = 1;
        dspexpr_27_in1 = 0;
        dspexpr_27_in2 = 0;
        dspexpr_28_in1 = 0;
        dspexpr_28_in2 = 0;
        dspexpr_29_in1 = 0;
        dspexpr_29_in2 = 0;
        dspexpr_30_in1 = 0;
        dspexpr_30_in2 = 0;
        expr_06_in1 = 0;
        expr_06_in2 = 8;
        expr_06_out1 = 0;
        expr_07_in1 = 0;
        expr_07_in2 = 1;
        expr_07_out1 = 0;
        voice_02_mute_number = 0;
        hann_tilde_02_x = 0;
        rampsmooth_tilde_05_x = 0;
        rampsmooth_tilde_05_up = 480;
        rampsmooth_tilde_05_down = 480;
        rampsmooth_tilde_06_x = 0;
        rampsmooth_tilde_06_up = 480;
        rampsmooth_tilde_06_down = 480;
        rampsmooth_tilde_07_x = 0;
        rampsmooth_tilde_07_up = 480;
        rampsmooth_tilde_07_down = 480;
        cycle_tilde_04_frequency = 0;
        cycle_tilde_04_phase_offset = 0;
        dspexpr_31_in1 = 0;
        dspexpr_31_in2 = 0.005;
        rand_tilde_04_frequency = 1;
        receive_07_output_number = 0;
        dspexpr_32_in1 = 0;
        dspexpr_32_in2 = 0.5;
        dspexpr_33_in1 = 0;
        dspexpr_33_in2 = 1;
        cycle_tilde_05_frequency = 0;
        cycle_tilde_05_phase_offset = 0;
        dspexpr_34_in1 = 0;
        dspexpr_34_in2 = 0.1;
        rand_tilde_05_frequency = 1;
        receive_08_output_number = 0;
        dspexpr_35_in1 = 0;
        dspexpr_35_in2 = 0.5;
        dspexpr_36_in1 = 0;
        dspexpr_36_in2 = 1;
        cycle_tilde_06_frequency = 0;
        cycle_tilde_06_phase_offset = 0;
        dspexpr_37_in1 = 0;
        dspexpr_37_in2 = 0.1;
        rand_tilde_06_frequency = 1;
        receive_09_output_number = 0;
        receive_10_output_number = 0;
        receive_11_output_number = 0;
        receive_12_output_number = 0;
        _currentTime = 0;
        audioProcessSampleCount = 0;
        sampleOffsetIntoNextAudioBuffer = 0;
        zeroBuffer = nullptr;
        dummyBuffer = nullptr;
        signals[0] = nullptr;
        signals[1] = nullptr;
        signals[2] = nullptr;
        signals[3] = nullptr;
        signals[4] = nullptr;
        didAllocateSignals = 0;
        vs = 0;
        maxvs = 0;
        sr = 44100;
        invsr = 0.000022675736961451248;
        rampsmooth_tilde_05_prev = 0;
        rampsmooth_tilde_05_index = 0;
        rampsmooth_tilde_05_increment = 0;
        rampsmooth_tilde_05_d_prev = 0;
        rampsmooth_tilde_05_setupDone = false;
        rampsmooth_tilde_06_prev = 0;
        rampsmooth_tilde_06_index = 0;
        rampsmooth_tilde_06_increment = 0;
        rampsmooth_tilde_06_d_prev = 0;
        rampsmooth_tilde_06_setupDone = false;
        rampsmooth_tilde_07_prev = 0;
        rampsmooth_tilde_07_index = 0;
        rampsmooth_tilde_07_increment = 0;
        rampsmooth_tilde_07_d_prev = 0;
        rampsmooth_tilde_07_setupDone = false;
        cycle_tilde_04_wrap = 0;
        cycle_tilde_04_ph_currentPhase = 0;
        cycle_tilde_04_ph_conv = 0;
        cycle_tilde_04_setupDone = false;
        rand_tilde_04_lastValue = 0;
        rand_tilde_04_targetValue = 0;
        rand_tilde_04_oldTargetValue = 0;
        rand_tilde_04_p_currentPhase = 0;
        rand_tilde_04_p_conv = 0;
        rand_tilde_04_d_prev = 0;
        rand_tilde_04_setupDone = false;
        cycle_tilde_05_wrap = 0;
        cycle_tilde_05_ph_currentPhase = 0;
        cycle_tilde_05_ph_conv = 0;
        cycle_tilde_05_setupDone = false;
        rand_tilde_05_lastValue = 0;
        rand_tilde_05_targetValue = 0;
        rand_tilde_05_oldTargetValue = 0;
        rand_tilde_05_p_currentPhase = 0;
        rand_tilde_05_p_conv = 0;
        rand_tilde_05_d_prev = 0;
        rand_tilde_05_setupDone = false;
        cycle_tilde_06_wrap = 0;
        cycle_tilde_06_ph_currentPhase = 0;
        cycle_tilde_06_ph_conv = 0;
        cycle_tilde_06_setupDone = false;
        rand_tilde_06_lastValue = 0;
        rand_tilde_06_targetValue = 0;
        rand_tilde_06_oldTargetValue = 0;
        rand_tilde_06_p_currentPhase = 0;
        rand_tilde_06_p_conv = 0;
        rand_tilde_06_d_prev = 0;
        rand_tilde_06_setupDone = false;
        stackprotect_count = 0;
        _voiceIndex = 0;
        _noteNumber = 0;
        isMuted = 0;
        parameterOffset = 0;
    }
    
    // member variables
    
        number dspexpr_25_in1;
        number dspexpr_25_in2;
        number pan_tilde_02_spread;
        number pan_tilde_02_pos;
        number dspexpr_26_in1;
        number dspexpr_26_in2;
        number sample_02_value;
        number sample_02_offset;
        number wrap_tilde_02_x;
        number wrap_tilde_02_low;
        number wrap_tilde_02_high;
        number dspexpr_27_in1;
        number dspexpr_27_in2;
        number dspexpr_28_in1;
        number dspexpr_28_in2;
        number dspexpr_29_in1;
        number dspexpr_29_in2;
        number dspexpr_30_in1;
        number dspexpr_30_in2;
        number expr_06_in1;
        number expr_06_in2;
        number expr_06_out1;
        number expr_07_in1;
        number expr_07_in2;
        number expr_07_out1;
        number voice_02_mute_number;
        number hann_tilde_02_x;
        number rampsmooth_tilde_05_x;
        number rampsmooth_tilde_05_up;
        number rampsmooth_tilde_05_down;
        number rampsmooth_tilde_06_x;
        number rampsmooth_tilde_06_up;
        number rampsmooth_tilde_06_down;
        number rampsmooth_tilde_07_x;
        number rampsmooth_tilde_07_up;
        number rampsmooth_tilde_07_down;
        number cycle_tilde_04_frequency;
        number cycle_tilde_04_phase_offset;
        number dspexpr_31_in1;
        number dspexpr_31_in2;
        number rand_tilde_04_frequency;
        number receive_07_output_number;
        list receive_07_output_list;
        number dspexpr_32_in1;
        number dspexpr_32_in2;
        number dspexpr_33_in1;
        number dspexpr_33_in2;
        number cycle_tilde_05_frequency;
        number cycle_tilde_05_phase_offset;
        number dspexpr_34_in1;
        number dspexpr_34_in2;
        number rand_tilde_05_frequency;
        number receive_08_output_number;
        list receive_08_output_list;
        number dspexpr_35_in1;
        number dspexpr_35_in2;
        number dspexpr_36_in1;
        number dspexpr_36_in2;
        number cycle_tilde_06_frequency;
        number cycle_tilde_06_phase_offset;
        number dspexpr_37_in1;
        number dspexpr_37_in2;
        number rand_tilde_06_frequency;
        number receive_09_output_number;
        list receive_09_output_list;
        number receive_10_output_number;
        list receive_10_output_list;
        number receive_11_output_number;
        list receive_11_output_list;
        number receive_12_output_number;
        list receive_12_output_list;
        MillisecondTime _currentTime;
        UInt64 audioProcessSampleCount;
        Index sampleOffsetIntoNextAudioBuffer;
        signal zeroBuffer;
        signal dummyBuffer;
        SampleValue * signals[5];
        bool didAllocateSignals;
        Index vs;
        Index maxvs;
        number sr;
        number invsr;
        Float64BufferRef sample_02_buffer;
        number rampsmooth_tilde_05_prev;
        number rampsmooth_tilde_05_index;
        number rampsmooth_tilde_05_increment;
        number rampsmooth_tilde_05_d_prev;
        bool rampsmooth_tilde_05_setupDone;
        number rampsmooth_tilde_06_prev;
        number rampsmooth_tilde_06_index;
        number rampsmooth_tilde_06_increment;
        number rampsmooth_tilde_06_d_prev;
        bool rampsmooth_tilde_06_setupDone;
        number rampsmooth_tilde_07_prev;
        number rampsmooth_tilde_07_index;
        number rampsmooth_tilde_07_increment;
        number rampsmooth_tilde_07_d_prev;
        bool rampsmooth_tilde_07_setupDone;
        SampleBufferRef cycle_tilde_04_buffer;
        Int cycle_tilde_04_wrap;
        UInt32 cycle_tilde_04_phasei;
        SampleValue cycle_tilde_04_f2i;
        number cycle_tilde_04_ph_currentPhase;
        number cycle_tilde_04_ph_conv;
        bool cycle_tilde_04_setupDone;
        number rand_tilde_04_lastValue;
        number rand_tilde_04_targetValue;
        number rand_tilde_04_oldTargetValue;
        number rand_tilde_04_p_currentPhase;
        number rand_tilde_04_p_conv;
        UInt rand_tilde_04_n_state[4] = { };
        number rand_tilde_04_d_prev;
        bool rand_tilde_04_setupDone;
        SampleBufferRef cycle_tilde_05_buffer;
        Int cycle_tilde_05_wrap;
        UInt32 cycle_tilde_05_phasei;
        SampleValue cycle_tilde_05_f2i;
        number cycle_tilde_05_ph_currentPhase;
        number cycle_tilde_05_ph_conv;
        bool cycle_tilde_05_setupDone;
        number rand_tilde_05_lastValue;
        number rand_tilde_05_targetValue;
        number rand_tilde_05_oldTargetValue;
        number rand_tilde_05_p_currentPhase;
        number rand_tilde_05_p_conv;
        UInt rand_tilde_05_n_state[4] = { };
        number rand_tilde_05_d_prev;
        bool rand_tilde_05_setupDone;
        SampleBufferRef cycle_tilde_06_buffer;
        Int cycle_tilde_06_wrap;
        UInt32 cycle_tilde_06_phasei;
        SampleValue cycle_tilde_06_f2i;
        number cycle_tilde_06_ph_currentPhase;
        number cycle_tilde_06_ph_conv;
        bool cycle_tilde_06_setupDone;
        number rand_tilde_06_lastValue;
        number rand_tilde_06_targetValue;
        number rand_tilde_06_oldTargetValue;
        number rand_tilde_06_p_currentPhase;
        number rand_tilde_06_p_conv;
        UInt rand_tilde_06_n_state[4] = { };
        number rand_tilde_06_d_prev;
        bool rand_tilde_06_setupDone;
        number stackprotect_count;
        Index _voiceIndex;
        Int _noteNumber;
        Index isMuted;
        ParameterIndex parameterOffset;
        bool _isInitialized = false;
};

class RNBOSubpatcher_249 : public PatcherInterfaceImpl {
    
    friend class rnbomatic;
    
    public:
    
    RNBOSubpatcher_249()
    {}
    
    ~RNBOSubpatcher_249()
    {
        deallocateSignals();
    }
    
    Index getNumMidiInputPorts() const {
        return 0;
    }
    
    void processMidiEvent(MillisecondTime , int , ConstByteArray , Index ) {}
    
    Index getNumMidiOutputPorts() const {
        return 0;
    }
    
    void process(
        const SampleValue * const* inputs,
        Index numInputs,
        SampleValue * const* outputs,
        Index numOutputs,
        Index n
    ) {
        this->vs = n;
        this->updateTime(this->getEngine()->getCurrentTime(), (ENGINE*)nullptr, true);
        SampleValue * out1 = (numOutputs >= 1 && outputs[0] ? outputs[0] : this->dummyBuffer);
        SampleValue * out2 = (numOutputs >= 2 && outputs[1] ? outputs[1] : this->dummyBuffer);
        const SampleValue * in1 = (numInputs >= 1 && inputs[0] ? inputs[0] : this->zeroBuffer);
        const SampleValue * in2 = (numInputs >= 2 && inputs[1] ? inputs[1] : this->zeroBuffer);
        this->linetilde_04_perform(this->signals[0], n);
        this->dspexpr_39_perform(in2, this->signals[0], out2, n);
        this->dspexpr_38_perform(in1, this->signals[0], out1, n);
        this->stackprotect_perform(n);
        this->audioProcessSampleCount += this->vs;
    }
    
    void prepareToProcess(number sampleRate, Index maxBlockSize, bool force) {
        RNBO_ASSERT(this->_isInitialized);
    
        if (this->maxvs < maxBlockSize || !this->didAllocateSignals) {
            Index i;
    
            for (i = 0; i < 1; i++) {
                this->signals[i] = resizeSignal(this->signals[i], this->maxvs, maxBlockSize);
            }
    
            this->zeroBuffer = resizeSignal(this->zeroBuffer, this->maxvs, maxBlockSize);
            this->dummyBuffer = resizeSignal(this->dummyBuffer, this->maxvs, maxBlockSize);
            this->didAllocateSignals = true;
        }
    
        const bool sampleRateChanged = sampleRate != this->sr;
        const bool maxvsChanged = maxBlockSize != this->maxvs;
        const bool forceDSPSetup = sampleRateChanged || maxvsChanged || force;
    
        if (sampleRateChanged || maxvsChanged) {
            this->vs = maxBlockSize;
            this->maxvs = maxBlockSize;
            this->sr = sampleRate;
            this->invsr = 1 / sampleRate;
        }
    
        RNBO_UNUSED(forceDSPSetup);
    
        if (sampleRateChanged)
            this->onSampleRateChanged(sampleRate);
    }
    
    number msToSamps(MillisecondTime ms, number sampleRate) {
        return ms * sampleRate * 0.001;
    }
    
    MillisecondTime sampsToMs(SampleIndex samps) {
        return samps * (this->invsr * 1000);
    }
    
    Index getNumInputChannels() const {
        return 2;
    }
    
    Index getNumOutputChannels() const {
        return 2;
    }
    
    void getPreset(PatcherStateInterface& ) {}
    
    void setPreset(MillisecondTime , PatcherStateInterface& ) {}
    
    void setParameterValue(ParameterIndex , ParameterValue , MillisecondTime ) {}
    
    void processParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
        this->setParameterValue(index, value, time);
    }
    
    void processParameterBangEvent(ParameterIndex index, MillisecondTime time) {
        this->setParameterValue(index, this->getParameterValue(index), time);
    }
    
    void processNormalizedParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
        this->setParameterValueNormalized(index, value, time);
    }
    
    ParameterValue getParameterValue(ParameterIndex index)  {
        switch (index) {
        default:
            {
            return 0;
            }
        }
    }
    
    ParameterIndex getNumSignalInParameters() const {
        return 0;
    }
    
    ParameterIndex getNumSignalOutParameters() const {
        return 0;
    }
    
    ParameterIndex getNumParameters() const {
        return 0;
    }
    
    ConstCharPointer getParameterName(ParameterIndex index) const {
        switch (index) {
        default:
            {
            return "bogus";
            }
        }
    }
    
    ConstCharPointer getParameterId(ParameterIndex index) const {
        switch (index) {
        default:
            {
            return "bogus";
            }
        }
    }
    
    void getParameterInfo(ParameterIndex , ParameterInfo * ) const {}
    
    ParameterValue applyStepsToNormalizedParameterValue(ParameterValue normalizedValue, int steps) const {
        if (steps == 1) {
            if (normalizedValue > 0) {
                normalizedValue = 1.;
            }
        } else {
            ParameterValue oneStep = (number)1. / (steps - 1);
            ParameterValue numberOfSteps = rnbo_fround(normalizedValue / oneStep * 1 / (number)1) * (number)1;
            normalizedValue = numberOfSteps * oneStep;
        }
    
        return normalizedValue;
    }
    
    ParameterValue convertToNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
        switch (index) {
        default:
            {
            return value;
            }
        }
    }
    
    ParameterValue convertFromNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
        value = (value < 0 ? 0 : (value > 1 ? 1 : value));
    
        switch (index) {
        default:
            {
            return value;
            }
        }
    }
    
    ParameterValue constrainParameterValue(ParameterIndex index, ParameterValue value) const {
        switch (index) {
        default:
            {
            return value;
            }
        }
    }
    
    void processNumMessage(MessageTag tag, MessageTag objectId, MillisecondTime time, number payload) {
        this->updateTime(time, (ENGINE*)nullptr);
    
        switch (tag) {
        case TAG("valin"):
            {
            if (TAG("Direct~/toggle_obj-40") == objectId)
                this->toggle_06_valin_set(payload);
    
            break;
            }
        }
    }
    
    void processListMessage(MessageTag , MessageTag , MillisecondTime , const list& ) {}
    
    void processBangMessage(MessageTag , MessageTag , MillisecondTime ) {}
    
    MessageTagInfo resolveTag(MessageTag tag) const {
        switch (tag) {
        case TAG("valout"):
            {
            return "valout";
            }
        case TAG("Direct~/toggle_obj-40"):
            {
            return "Direct~/toggle_obj-40";
            }
        case TAG("valin"):
            {
            return "valin";
            }
        }
    
        return nullptr;
    }
    
    DataRef* getDataRef(DataRefIndex index)  {
        switch (index) {
        default:
            {
            return nullptr;
            }
        }
    }
    
    DataRefIndex getNumDataRefs() const {
        return 0;
    }
    
    void processDataViewUpdate(DataRefIndex , MillisecondTime ) {}
    
    void initialize() {
        RNBO_ASSERT(!this->_isInitialized);
        this->assign_defaults();
        this->applyState();
        this->_isInitialized = true;
    }
    
    protected:
    
    void updateTime(MillisecondTime time, INTERNALENGINE*, bool inProcess = false) {
    	if (time == TimeNow) time = getTopLevelPatcher()->getPatcherTime();
    	getTopLevelPatcher()->processInternalEvents(time);
    	updateTime(time, (EXTERNALENGINE*)nullptr);
    }
    
    RNBOSubpatcher_249* operator->() {
        return this;
    }
    const RNBOSubpatcher_249* operator->() const {
        return this;
    }
    virtual rnbomatic* getPatcher() const {
        return static_cast<rnbomatic *>(_parentPatcher);
    }
    
    rnbomatic* getTopLevelPatcher() {
        return this->getPatcher()->getTopLevelPatcher();
    }
    
    void cancelClockEvents()
    {
        getEngine()->flushClockEvents(this, -62043057, false);
    }
    
    UInt64 currentsampletime() {
        return this->audioProcessSampleCount + this->sampleOffsetIntoNextAudioBuffer;
    }
    
    number mstosamps(MillisecondTime ms) {
        return ms * this->sr * 0.001;
    }
    
    MillisecondTime getPatcherTime() const {
        return this->_currentTime;
    }
    
    void eventinlet_07_out1_bang_bang() {
        this->toggle_06_value_bang_bang();
    }
    
    template<typename LISTTYPE> void eventinlet_07_out1_list_set(const LISTTYPE& v) {
        {
            number converted = (v->length > 0 ? v[0] : 0);
            this->toggle_06_value_number_set(converted);
        }
    }
    
    void toggle_06_valin_set(number v) {
        this->toggle_06_value_number_set(v);
    }
    
    void linetilde_04_target_bang() {}
    
    void deallocateSignals() {
        Index i;
    
        for (i = 0; i < 1; i++) {
            this->signals[i] = freeSignal(this->signals[i]);
        }
    
        this->zeroBuffer = freeSignal(this->zeroBuffer);
        this->dummyBuffer = freeSignal(this->dummyBuffer);
    }
    
    Index getMaxBlockSize() const {
        return this->maxvs;
    }
    
    number getSampleRate() const {
        return this->sr;
    }
    
    bool hasFixedVectorSize() const {
        return false;
    }
    
    void setProbingTarget(MessageTag ) {}
    
    void initializeObjects() {}
    
    Index getIsMuted()  {
        return this->isMuted;
    }
    
    void setIsMuted(Index v)  {
        this->isMuted = v;
    }
    
    void onSampleRateChanged(double ) {}
    
    void extractState(PatcherStateInterface& ) {}
    
    void applyState() {}
    
    void setParameterOffset(ParameterIndex offset) {
        this->parameterOffset = offset;
    }
    
    void processClockEvent(MillisecondTime time, ClockId index, bool hasValue, ParameterValue value) {
        RNBO_UNUSED(value);
        RNBO_UNUSED(hasValue);
        this->updateTime(time, (ENGINE*)nullptr);
    
        switch (index) {
        case -62043057:
            {
            this->linetilde_04_target_bang();
            break;
            }
        }
    }
    
    void processOutletAtCurrentTime(EngineLink* , OutletIndex , ParameterValue ) {}
    
    void processOutletEvent(
        EngineLink* sender,
        OutletIndex index,
        ParameterValue value,
        MillisecondTime time
    ) {
        this->updateTime(time, (ENGINE*)nullptr);
        this->processOutletAtCurrentTime(sender, index, value);
    }
    
    void sendOutlet(OutletIndex index, ParameterValue value) {
        this->getEngine()->sendOutlet(this, index, value);
    }
    
    void startup() {}
    
    void fillDataRef(DataRefIndex , DataRef& ) {}
    
    void allocateDataRefs() {}
    
    void linetilde_04_time_set(number v) {
        this->linetilde_04_time = v;
    }
    
    template<typename LISTTYPE> void linetilde_04_segments_set(const LISTTYPE& v) {
        this->linetilde_04_segments = jsCreateListCopy(v);
    
        if ((bool)(v->length)) {
            if (v->length == 1 && this->linetilde_04_time == 0) {
                this->linetilde_04_activeRamps->length = 0;
                this->linetilde_04_currentValue = v[0];
            } else {
                auto currentTime = this->currentsampletime();
                number lastRampValue = this->linetilde_04_currentValue;
                number rampEnd = currentTime - this->sampleOffsetIntoNextAudioBuffer;
    
                for (Index i = 0; i < this->linetilde_04_activeRamps->length; i += 3) {
                    rampEnd = this->linetilde_04_activeRamps[(Index)(i + 2)];
    
                    if (rampEnd > currentTime) {
                        this->linetilde_04_activeRamps[(Index)(i + 2)] = currentTime;
                        number diff = rampEnd - currentTime;
                        number valueDiff = diff * this->linetilde_04_activeRamps[(Index)(i + 1)];
                        lastRampValue = this->linetilde_04_activeRamps[(Index)i] - valueDiff;
                        this->linetilde_04_activeRamps[(Index)i] = lastRampValue;
                        this->linetilde_04_activeRamps->length = i + 3;
                        rampEnd = currentTime;
                    } else {
                        lastRampValue = this->linetilde_04_activeRamps[(Index)i];
                    }
                }
    
                if (rampEnd < currentTime) {
                    this->linetilde_04_activeRamps->push(lastRampValue);
                    this->linetilde_04_activeRamps->push(0);
                    this->linetilde_04_activeRamps->push(currentTime);
                }
    
                number lastRampEnd = currentTime;
    
                for (Index i = 0; i < v->length; i += 2) {
                    number destinationValue = v[(Index)i];
                    number inc = 0;
                    number rampTimeInSamples;
    
                    if (v->length > i + 1) {
                        rampTimeInSamples = this->mstosamps(v[(Index)(i + 1)]);
    
                        if ((bool)(this->linetilde_04_keepramp)) {
                            this->linetilde_04_time_set(v[(Index)(i + 1)]);
                        }
                    } else {
                        rampTimeInSamples = this->mstosamps(this->linetilde_04_time);
                    }
    
                    if (rampTimeInSamples <= 0) {
                        rampTimeInSamples = 1;
                    }
    
                    inc = (destinationValue - lastRampValue) / rampTimeInSamples;
                    lastRampEnd += rampTimeInSamples;
                    this->linetilde_04_activeRamps->push(destinationValue);
                    this->linetilde_04_activeRamps->push(inc);
                    this->linetilde_04_activeRamps->push(lastRampEnd);
                    lastRampValue = destinationValue;
                }
            }
        }
    }
    
    void toggle_06_output_set(number v) {
        {
            listbase<number, 1> converted = {v};
            this->linetilde_04_segments_set(converted);
        }
    }
    
    void toggle_06_value_number_set(number v) {
        this->toggle_06_value_number_setter(v);
        v = this->toggle_06_value_number;
        this->toggle_06_output_set(v);
    }
    
    void eventinlet_07_out1_number_set(number v) {
        this->toggle_06_value_number_set(v);
    }
    
    void toggle_06_value_bang_bang() {
        number val = (this->toggle_06_value_number == 1 ? 0 : 1);
        this->toggle_06_value_number_set(val);
    }
    
    void linetilde_04_perform(SampleValue * out, Index n) {
        auto __linetilde_04_time = this->linetilde_04_time;
        auto __linetilde_04_keepramp = this->linetilde_04_keepramp;
        auto __linetilde_04_currentValue = this->linetilde_04_currentValue;
        Index i = 0;
    
        if ((bool)(this->linetilde_04_activeRamps->length)) {
            while ((bool)(this->linetilde_04_activeRamps->length) && i < n) {
                number destinationValue = this->linetilde_04_activeRamps[0];
                number inc = this->linetilde_04_activeRamps[1];
                number rampTimeInSamples = this->linetilde_04_activeRamps[2] - this->audioProcessSampleCount - i;
                number val = __linetilde_04_currentValue;
    
                while (rampTimeInSamples > 0 && i < n) {
                    out[(Index)i] = val;
                    val += inc;
                    i++;
                    rampTimeInSamples--;
                }
    
                if (rampTimeInSamples <= 0) {
                    val = destinationValue;
                    this->linetilde_04_activeRamps->splice(0, 3);
    
                    if ((bool)(!(bool)(this->linetilde_04_activeRamps->length))) {
                        this->getEngine()->scheduleClockEventWithValue(
                            this,
                            -62043057,
                            this->sampsToMs((SampleIndex)(this->vs)) + this->_currentTime,
                            0
                        );;
    
                        if ((bool)(!(bool)(__linetilde_04_keepramp))) {
                            __linetilde_04_time = 0;
                        }
                    }
                }
    
                __linetilde_04_currentValue = val;
            }
        }
    
        while (i < n) {
            out[(Index)i] = __linetilde_04_currentValue;
            i++;
        }
    
        this->linetilde_04_currentValue = __linetilde_04_currentValue;
        this->linetilde_04_time = __linetilde_04_time;
    }
    
    void dspexpr_39_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] * in2[(Index)i];//#map:_###_obj_###_:1
        }
    }
    
    void dspexpr_38_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] * in2[(Index)i];//#map:_###_obj_###_:1
        }
    }
    
    void stackprotect_perform(Index n) {
        RNBO_UNUSED(n);
        auto __stackprotect_count = this->stackprotect_count;
        __stackprotect_count = 0;
        this->stackprotect_count = __stackprotect_count;
    }
    
    void toggle_06_value_number_setter(number v) {
        this->toggle_06_value_number = (v != 0 ? 1 : 0);
    }
    
    void toggle_06_getPresetValue(PatcherStateInterface& preset) {
        preset["value"] = this->toggle_06_value_number;
    }
    
    void toggle_06_setPresetValue(PatcherStateInterface& preset) {
        if ((bool)(stateIsEmpty(preset)))
            return;
    
        this->toggle_06_value_number_set(preset["value"]);
    }
    
    bool stackprotect_check() {
        this->stackprotect_count++;
    
        if (this->stackprotect_count > 128) {
            console->log("STACK OVERFLOW DETECTED - stopped processing branch !");
            return true;
        }
    
        return false;
    }
    
    Index getPatcherSerial() const {
        return 0;
    }
    
    void sendParameter(ParameterIndex index, bool ignoreValue) {
        this->getPatcher()->sendParameter(index + this->parameterOffset, ignoreValue);
    }
    
    void scheduleParamInit(ParameterIndex index, Index order) {
        this->getPatcher()->scheduleParamInit(index + this->parameterOffset, order);
    }
    
    void updateTime(MillisecondTime time, EXTERNALENGINE* engine, bool inProcess = false) {
        RNBO_UNUSED(inProcess);
        RNBO_UNUSED(engine);
        this->_currentTime = time;
        auto offset = rnbo_fround(this->msToSamps(time - this->getEngine()->getCurrentTime(), this->sr));
    
        if (offset >= (SampleIndex)(this->vs))
            offset = (SampleIndex)(this->vs) - 1;
    
        if (offset < 0)
            offset = 0;
    
        this->sampleOffsetIntoNextAudioBuffer = (Index)(offset);
    }
    
    void assign_defaults()
    {
        dspexpr_38_in1 = 0;
        dspexpr_38_in2 = 0;
        dspexpr_39_in1 = 0;
        dspexpr_39_in2 = 0;
        toggle_06_value_number = 0;
        toggle_06_value_number_setter(toggle_06_value_number);
        linetilde_04_time = 10;
        linetilde_04_keepramp = true;
        _currentTime = 0;
        audioProcessSampleCount = 0;
        sampleOffsetIntoNextAudioBuffer = 0;
        zeroBuffer = nullptr;
        dummyBuffer = nullptr;
        signals[0] = nullptr;
        didAllocateSignals = 0;
        vs = 0;
        maxvs = 0;
        sr = 44100;
        invsr = 0.000022675736961451248;
        toggle_06_lastValue = 0;
        linetilde_04_currentValue = 1;
        stackprotect_count = 0;
        _voiceIndex = 0;
        _noteNumber = 0;
        isMuted = 1;
        parameterOffset = 0;
    }
    
    // member variables
    
        number dspexpr_38_in1;
        number dspexpr_38_in2;
        number dspexpr_39_in1;
        number dspexpr_39_in2;
        number toggle_06_value_number;
        list linetilde_04_segments;
        number linetilde_04_time;
        number linetilde_04_keepramp;
        MillisecondTime _currentTime;
        UInt64 audioProcessSampleCount;
        Index sampleOffsetIntoNextAudioBuffer;
        signal zeroBuffer;
        signal dummyBuffer;
        SampleValue * signals[1];
        bool didAllocateSignals;
        Index vs;
        Index maxvs;
        number sr;
        number invsr;
        number toggle_06_lastValue;
        list linetilde_04_activeRamps;
        number linetilde_04_currentValue;
        number stackprotect_count;
        Index _voiceIndex;
        Int _noteNumber;
        Index isMuted;
        ParameterIndex parameterOffset;
        bool _isInitialized = false;
};

		
void advanceTime(EXTERNALENGINE*) {}
void advanceTime(INTERNALENGINE*) {
	_internalEngine.advanceTime(sampstoms(this->vs));
}

void processInternalEvents(MillisecondTime time) {
	_internalEngine.processEventsUntil(time);
}

void updateTime(MillisecondTime time, INTERNALENGINE*, bool inProcess = false) {
	if (time == TimeNow) time = getPatcherTime();
	processInternalEvents(inProcess ? time + sampsToMs(this->vs) : time);
	updateTime(time, (EXTERNALENGINE*)nullptr);
}

rnbomatic* operator->() {
    return this;
}
const rnbomatic* operator->() const {
    return this;
}
rnbomatic* getTopLevelPatcher() {
    return this;
}

void cancelClockEvents()
{
    getEngine()->flushClockEvents(this, -1621164530, false);
    getEngine()->flushClockEvents(this, 1159774702, false);
    getEngine()->flushClockEvents(this, -1029704085, false);
    getEngine()->flushClockEvents(this, -62043057, false);
    getEngine()->flushClockEvents(this, 1861604205, false);
    getEngine()->flushClockEvents(this, 1075784424, false);
}

template<typename LISTTYPE = list> void listquicksort(LISTTYPE& arr, LISTTYPE& sortindices, Int l, Int h, bool ascending) {
    if (l < h) {
        Int p = (Int)(this->listpartition(arr, sortindices, l, h, ascending));
        this->listquicksort(arr, sortindices, l, p - 1, ascending);
        this->listquicksort(arr, sortindices, p + 1, h, ascending);
    }
}

template<typename LISTTYPE = list> Int listpartition(LISTTYPE& arr, LISTTYPE& sortindices, Int l, Int h, bool ascending) {
    number x = arr[(Index)h];
    Int i = (Int)(l - 1);

    for (Int j = (Int)(l); j <= h - 1; j++) {
        bool asc = (bool)((bool)(ascending) && arr[(Index)j] <= x);
        bool desc = (bool)((bool)(!(bool)(ascending)) && arr[(Index)j] >= x);

        if ((bool)(asc) || (bool)(desc)) {
            i++;
            this->listswapelements(arr, i, j);
            this->listswapelements(sortindices, i, j);
        }
    }

    i++;
    this->listswapelements(arr, i, h);
    this->listswapelements(sortindices, i, h);
    return i;
}

template<typename LISTTYPE = list> void listswapelements(LISTTYPE& arr, Int a, Int b) {
    auto tmp = arr[(Index)a];
    arr[(Index)a] = arr[(Index)b];
    arr[(Index)b] = tmp;
}

number mstosamps(MillisecondTime ms) {
    return ms * this->sr * 0.001;
}

number fromnormalized(Index index, number normalizedValue) {
    return this->convertFromNormalizedParameterValue(index, normalizedValue);
}

UInt64 currentsampletime() {
    return this->audioProcessSampleCount + this->sampleOffsetIntoNextAudioBuffer;
}

inline number safediv(number num, number denom) {
    return (denom == 0.0 ? 0.0 : num / denom);
}

number safepow(number base, number exponent) {
    return fixnan(rnbo_pow(base, exponent));
}

number scale(
    number x,
    number lowin,
    number hiin,
    number lowout,
    number highout,
    number pow
) {
    auto inscale = this->safediv(1., hiin - lowin);
    number outdiff = highout - lowout;
    number value = (x - lowin) * inscale;

    if (pow != 1) {
        if (value > 0)
            value = this->safepow(value, pow);
        else
            value = -this->safepow(-value, pow);
    }

    value = value * outdiff + lowout;
    return value;
}

number maximum(number x, number y) {
    return (x < y ? y : x);
}

MillisecondTime sampstoms(number samps) {
    return samps * 1000 / this->sr;
}

void param_01_value_set(number v) {
    v = this->param_01_value_constrain(v);
    this->param_01_value = v;
    this->sendParameter(0, false);

    if (this->param_01_value != this->param_01_lastValue) {
        {
            this->getEngine()->presetTouched();
        }

        this->param_01_lastValue = this->param_01_value;
    }

    this->numberobj_17_value_set(v);
}

void param_02_value_set(number v) {
    v = this->param_02_value_constrain(v);
    this->param_02_value = v;
    this->sendParameter(1, false);

    if (this->param_02_value != this->param_02_lastValue) {
        {
            this->getEngine()->presetTouched();
        }

        this->param_02_lastValue = this->param_02_value;
    }

    this->numberobj_18_value_set(v);
}

void param_03_value_set(number v) {
    v = this->param_03_value_constrain(v);
    this->param_03_value = v;
    this->sendParameter(2, false);

    if (this->param_03_value != this->param_03_lastValue) {
        {
            this->getEngine()->presetTouched();
        }

        this->param_03_lastValue = this->param_03_value;
    }

    this->toggle_07_value_number_set(v);
}

void param_04_value_set(number v) {
    v = this->param_04_value_constrain(v);
    this->param_04_value = v;
    this->sendParameter(3, false);

    if (this->param_04_value != this->param_04_lastValue) {
        {
            this->getEngine()->presetTouched();
        }

        this->param_04_lastValue = this->param_04_value;
    }

    this->numberobj_20_value_set(v);
}

void param_05_value_set(number v) {
    v = this->param_05_value_constrain(v);
    this->param_05_value = v;
    this->sendParameter(4, false);

    if (this->param_05_value != this->param_05_lastValue) {
        {
            this->getEngine()->presetTouched();
        }

        this->param_05_lastValue = this->param_05_value;
    }

    this->numberobj_21_value_set(v);
}

void param_06_value_set(number v) {
    v = this->param_06_value_constrain(v);
    this->param_06_value = v;
    this->sendParameter(5, false);

    if (this->param_06_value != this->param_06_lastValue) {
        {
            this->getEngine()->presetTouched();
        }

        this->param_06_lastValue = this->param_06_value;
    }

    this->numberobj_23_value_set(v);
}

void param_07_value_set(number v) {
    v = this->param_07_value_constrain(v);
    this->param_07_value = v;
    this->sendParameter(6, false);

    if (this->param_07_value != this->param_07_lastValue) {
        {
            this->getEngine()->presetTouched();
        }

        this->param_07_lastValue = this->param_07_value;
    }

    this->numberobj_24_value_set(v);
}

void param_08_value_set(number v) {
    v = this->param_08_value_constrain(v);
    this->param_08_value = v;
    this->sendParameter(7, false);

    if (this->param_08_value != this->param_08_lastValue) {
        {
            this->getEngine()->presetTouched();
        }

        this->param_08_lastValue = this->param_08_value;
    }

    this->numberobj_26_value_set(v);
}

void param_09_value_set(number v) {
    v = this->param_09_value_constrain(v);
    this->param_09_value = v;
    this->sendParameter(8, false);

    if (this->param_09_value != this->param_09_lastValue) {
        {
            this->getEngine()->presetTouched();
        }

        this->param_09_lastValue = this->param_09_value;
    }

    this->toggle_10_value_number_set(v);
}

MillisecondTime getPatcherTime() const {
    return this->_currentTime;
}

template<typename LISTTYPE> void message_10_listin_list_set(const LISTTYPE& v) {
    this->message_10_set_set(v);
}

void message_10_listin_number_set(number v) {
    this->message_10_set_set(v);
}

void message_10_listin_bang_bang() {
    this->message_10_trigger_bang();
}

void button_09_bangin_bang() {
    this->button_09_bangval_bang();
}

void numberobj_17_valin_set(number v) {
    this->numberobj_17_value_set(v);
}

void numberobj_17_format_set(number v) {
    this->numberobj_17_currentFormat = trunc((v > 6 ? 6 : (v < 0 ? 0 : v)));
}

void numberobj_18_valin_set(number v) {
    this->numberobj_18_value_set(v);
}

void numberobj_18_format_set(number v) {
    this->numberobj_18_currentFormat = trunc((v > 6 ? 6 : (v < 0 ? 0 : v)));
}

void toggle_07_valin_set(number v) {
    this->toggle_07_value_number_set(v);
}

void numberobj_19_valin_set(number v) {
    this->numberobj_19_value_set(v);
}

void numberobj_19_format_set(number v) {
    this->numberobj_19_currentFormat = trunc((v > 6 ? 6 : (v < 0 ? 0 : v)));
}

void toggle_08_valin_set(number v) {
    this->toggle_08_value_number_set(v);
}

void numberobj_20_valin_set(number v) {
    this->numberobj_20_value_set(v);
}

void numberobj_20_format_set(number v) {
    this->numberobj_20_currentFormat = trunc((v > 6 ? 6 : (v < 0 ? 0 : v)));
}

void numberobj_21_valin_set(number v) {
    this->numberobj_21_value_set(v);
}

void numberobj_21_format_set(number v) {
    this->numberobj_21_currentFormat = trunc((v > 6 ? 6 : (v < 0 ? 0 : v)));
}

void numberobj_22_valin_set(number v) {
    this->numberobj_22_value_set(v);
}

void numberobj_22_format_set(number v) {
    this->numberobj_22_currentFormat = trunc((v > 6 ? 6 : (v < 0 ? 0 : v)));
}

void numberobj_23_valin_set(number v) {
    this->numberobj_23_value_set(v);
}

void numberobj_23_format_set(number v) {
    this->numberobj_23_currentFormat = trunc((v > 6 ? 6 : (v < 0 ? 0 : v)));
}

void numberobj_24_valin_set(number v) {
    this->numberobj_24_value_set(v);
}

void numberobj_24_format_set(number v) {
    this->numberobj_24_currentFormat = trunc((v > 6 ? 6 : (v < 0 ? 0 : v)));
}

void numberobj_25_valin_set(number v) {
    this->numberobj_25_value_set(v);
}

void numberobj_25_format_set(number v) {
    this->numberobj_25_currentFormat = trunc((v > 6 ? 6 : (v < 0 ? 0 : v)));
}

void toggle_09_valin_set(number v) {
    this->toggle_09_value_number_set(v);
}

void numberobj_26_valin_set(number v) {
    this->numberobj_26_value_set(v);
}

void numberobj_26_format_set(number v) {
    this->numberobj_26_currentFormat = trunc((v > 6 ? 6 : (v < 0 ? 0 : v)));
}

void numberobj_27_valin_set(number v) {
    this->numberobj_27_value_set(v);
}

void numberobj_27_format_set(number v) {
    this->numberobj_27_currentFormat = trunc((v > 6 ? 6 : (v < 0 ? 0 : v)));
}

void toggle_10_valin_set(number v) {
    this->toggle_10_value_number_set(v);
}

void linetilde_05_target_bang() {}

void metertilde_01_value_set(number ) {}

void metertilde_02_value_set(number ) {}

void linetilde_06_target_bang() {}

void metertilde_04_value_set(number ) {}

void metertilde_03_value_set(number ) {}

void deallocateSignals() {
    Index i;

    for (i = 0; i < 8; i++) {
        this->signals[i] = freeSignal(this->signals[i]);
    }

    this->phasor_01_sigbuf = freeSignal(this->phasor_01_sigbuf);
    this->globaltransport_tempo = freeSignal(this->globaltransport_tempo);
    this->globaltransport_state = freeSignal(this->globaltransport_state);
    this->zeroBuffer = freeSignal(this->zeroBuffer);
    this->dummyBuffer = freeSignal(this->dummyBuffer);
}

Index getMaxBlockSize() const {
    return this->maxvs;
}

number getSampleRate() const {
    return this->sr;
}

bool hasFixedVectorSize() const {
    return false;
}

void setProbingTarget(MessageTag ) {}

void fillRNBODefaultSinus(DataRef& ref) {
    SampleBuffer buffer(ref);
    number bufsize = buffer->getSize();

    for (Index i = 0; i < bufsize; i++) {
        buffer[i] = rnbo_cos(i * 3.14159265358979323846 * 2. / bufsize);
    }
}

void fillDataRef(DataRefIndex index, DataRef& ref) {
    switch (index) {
    case 1:
        {
        this->fillRNBODefaultSinus(ref);
        break;
        }
    }
}

void zeroDataRef(DataRef& ref) {
    ref->setZero();
}

void allocateDataRefs() {
    this->p_01->allocateDataRefs();

    for (Index i = 0; i < 8; i++) {
        this->p_02[i]->allocateDataRefs();
    }

    this->p_03->allocateDataRefs();
    this->p_04->allocateDataRefs();

    for (Index i = 0; i < 8; i++) {
        this->p_05[i]->allocateDataRefs();
    }

    this->p_06->allocateDataRefs();
    this->data_02_buffer->requestSize(this->mstosamps(200), 2);
    this->data_02_buffer->setSampleRate(this->sr);
    this->data_01_buffer->requestSize(this->mstosamps(200), 2);
    this->data_01_buffer->setSampleRate(this->sr);
    this->recordtilde_01_buffer = this->recordtilde_01_buffer->allocateIfNeeded();
    this->data_02_buffer = this->data_02_buffer->allocateIfNeeded();

    if (this->FreezeA->hasRequestedSize()) {
        if (this->FreezeA->wantsFill())
            this->zeroDataRef(this->FreezeA);

        this->getEngine()->sendDataRefUpdated(0);
    }

    if (this->RNBODefaultSinus->hasRequestedSize()) {
        if (this->RNBODefaultSinus->wantsFill())
            this->fillRNBODefaultSinus(this->RNBODefaultSinus);

        this->getEngine()->sendDataRefUpdated(1);
    }

    this->data_01_buffer = this->data_01_buffer->allocateIfNeeded();
    this->recordtilde_02_buffer = this->recordtilde_02_buffer->allocateIfNeeded();

    if (this->FreezeB->hasRequestedSize()) {
        if (this->FreezeB->wantsFill())
            this->zeroDataRef(this->FreezeB);

        this->getEngine()->sendDataRefUpdated(2);
    }
}

void initializeObjects() {
    this->message_10_init();
    this->numberobj_17_init();
    this->metertilde_01_init();
    this->metertilde_02_init();
    this->numberobj_18_init();
    this->numberobj_19_init();
    this->numberobj_20_init();
    this->metertilde_03_init();
    this->numberobj_21_init();
    this->metertilde_04_init();
    this->numberobj_22_init();
    this->numberobj_23_init();
    this->numberobj_24_init();
    this->numberobj_25_init();
    this->data_01_init();
    this->data_02_init();
    this->numberobj_26_init();
    this->numberobj_27_init();
    this->p_01->initializeObjects();

    for (Index i = 0; i < 8; i++) {
        this->p_02[i]->initializeObjects();
    }

    this->p_03->initializeObjects();
    this->p_04->initializeObjects();

    for (Index i = 0; i < 8; i++) {
        this->p_05[i]->initializeObjects();
    }

    this->p_06->initializeObjects();
}

Index getIsMuted()  {
    return this->isMuted;
}

void setIsMuted(Index v)  {
    this->isMuted = v;
}

void onSampleRateChanged(double ) {}

void extractState(PatcherStateInterface& ) {}

void applyState() {

    this->p_01->setEngineAndPatcher(this->getEngine(), this);
    this->p_01->initialize();
    this->p_01->setParameterOffset(this->getParameterOffset(this->p_01));

    for (Index i = 0; i < 8; i++) {

        this->p_02[(Index)i]->setEngineAndPatcher(this->getEngine(), this);
        this->p_02[(Index)i]->initialize();
        this->p_02[(Index)i]->setParameterOffset(this->getParameterOffset(this->p_02[0]));
        this->p_02[(Index)i]->setVoiceIndex(i + 1);
    }


    this->p_03->setEngineAndPatcher(this->getEngine(), this);
    this->p_03->initialize();
    this->p_03->setParameterOffset(this->getParameterOffset(this->p_03));

    this->p_04->setEngineAndPatcher(this->getEngine(), this);
    this->p_04->initialize();
    this->p_04->setParameterOffset(this->getParameterOffset(this->p_04));

    for (Index i = 0; i < 8; i++) {

        this->p_05[(Index)i]->setEngineAndPatcher(this->getEngine(), this);
        this->p_05[(Index)i]->initialize();
        this->p_05[(Index)i]->setParameterOffset(this->getParameterOffset(this->p_05[0]));
        this->p_05[(Index)i]->setVoiceIndex(i + 1);
    }


    this->p_06->setEngineAndPatcher(this->getEngine(), this);
    this->p_06->initialize();
    this->p_06->setParameterOffset(this->getParameterOffset(this->p_06));
}

ParameterIndex getParameterOffset(BaseInterface& subpatcher) const {
    if (addressOf(subpatcher) == addressOf(this->p_01))
        return 9;

    if (addressOf(subpatcher) == addressOf(this->p_02[0]))
        return 9 + this->p_01->getNumParameters();

    if (addressOf(subpatcher) == addressOf(this->p_03))
        return 9 + this->p_01->getNumParameters() + this->p_02[0]->getNumParameters();

    if (addressOf(subpatcher) == addressOf(this->p_04))
        return 9 + this->p_01->getNumParameters() + this->p_02[0]->getNumParameters() + this->p_03->getNumParameters();

    if (addressOf(subpatcher) == addressOf(this->p_05[0]))
        return 9 + this->p_01->getNumParameters() + this->p_02[0]->getNumParameters() + this->p_03->getNumParameters() + this->p_04->getNumParameters();

    if (addressOf(subpatcher) == addressOf(this->p_06))
        return 9 + this->p_01->getNumParameters() + this->p_02[0]->getNumParameters() + this->p_03->getNumParameters() + this->p_04->getNumParameters() + this->p_05[0]->getNumParameters();

    return 0;
}

void processClockEvent(MillisecondTime time, ClockId index, bool hasValue, ParameterValue value) {
    RNBO_UNUSED(hasValue);
    this->updateTime(time, (ENGINE*)nullptr);

    switch (index) {
    case -1621164530:
        {
        this->linetilde_05_target_bang();
        break;
        }
    case 1159774702:
        {
        this->metertilde_01_value_set(value);
        break;
        }
    case -1029704085:
        {
        this->metertilde_02_value_set(value);
        break;
        }
    case -62043057:
        {
        this->linetilde_06_target_bang();
        break;
        }
    case 1861604205:
        {
        this->metertilde_04_value_set(value);
        break;
        }
    case 1075784424:
        {
        this->metertilde_03_value_set(value);
        break;
        }
    }
}

void processOutletAtCurrentTime(EngineLink* , OutletIndex , ParameterValue ) {}

void processOutletEvent(
    EngineLink* sender,
    OutletIndex index,
    ParameterValue value,
    MillisecondTime time
) {
    this->updateTime(time, (ENGINE*)nullptr);
    this->processOutletAtCurrentTime(sender, index, value);
}

void sendOutlet(OutletIndex index, ParameterValue value) {
    this->getEngine()->sendOutlet(this, index, value);
}

void startup() {
    this->updateTime(this->getEngine()->getCurrentTime(), (ENGINE*)nullptr);
    this->p_01->startup();

    for (Index i = 0; i < 8; i++) {
        this->p_02[i]->startup();
    }

    this->p_03->startup();
    this->p_04->startup();

    for (Index i = 0; i < 8; i++) {
        this->p_05[i]->startup();
    }

    this->p_06->startup();

    {
        this->scheduleParamInit(0, 2);
    }

    {
        this->scheduleParamInit(1, 3);
    }

    {
        this->scheduleParamInit(2, 1);
    }

    {
        this->scheduleParamInit(3, 9);
    }

    {
        this->scheduleParamInit(4, 10);
    }

    {
        this->scheduleParamInit(5, 12);
    }

    {
        this->scheduleParamInit(6, 5);
    }

    {
        this->scheduleParamInit(7, 6);
    }

    {
        this->scheduleParamInit(8, 11);
    }

    this->processParamInitEvents();
}

number param_01_value_constrain(number v) const {
    v = (v > 100 ? 100 : (v < 0 ? 0 : v));
    return v;
}

void p_01_in3_number_set(number v) {
    this->p_01->updateTime(this->_currentTime, (ENGINE*)nullptr);
    this->p_01->eventinlet_01_out1_number_set(v);
}

void numberobj_17_output_set(number v) {
    this->p_01_in3_number_set(v);
}

void numberobj_17_value_set(number v) {
    this->numberobj_17_value_setter(v);
    v = this->numberobj_17_value;
    number localvalue = v;

    if ((bool)(true) && localvalue < 0) {
        localvalue = 0;
    }

    if ((bool)(true) && localvalue > 100) {
        localvalue = 100;
    }

    if (this->numberobj_17_currentFormat != 6) {
        localvalue = trunc(localvalue);
    }

    this->numberobj_17_output_set(localvalue);
}

number param_02_value_constrain(number v) const {
    v = (v > 100 ? 100 : (v < 0 ? 0 : v));
    return v;
}

void p_01_in4_number_set(number v) {
    this->p_01->updateTime(this->_currentTime, (ENGINE*)nullptr);
    this->p_01->eventinlet_02_out1_number_set(v);
}

void numberobj_18_output_set(number v) {
    this->p_01_in4_number_set(v);
}

void numberobj_18_value_set(number v) {
    this->numberobj_18_value_setter(v);
    v = this->numberobj_18_value;
    number localvalue = v;

    if ((bool)(true) && localvalue < 0) {
        localvalue = 0;
    }

    if ((bool)(true) && localvalue > 100) {
        localvalue = 100;
    }

    if (this->numberobj_18_currentFormat != 6) {
        localvalue = trunc(localvalue);
    }

    this->numberobj_18_output_set(localvalue);
}

number param_03_value_constrain(number v) const {
    v = (v > 1 ? 1 : (v < 0 ? 0 : v));

    {
        number oneStep = (number)1 / (number)1;
        number oneStepInv = (oneStep != 0 ? (number)1 / oneStep : 0);
        number numberOfSteps = rnbo_fround((v - 0) * oneStepInv * 1 / (number)1) * 1;
        v = numberOfSteps * oneStep + 0;
    }

    return v;
}

void recordtilde_02_record_set(number v) {
    this->recordtilde_02_record = v;

    if ((bool)(v)) {
        this->recordtilde_02_lastRecord = 0;
    }
}

void toggle_09_output_set(number v) {
    this->recordtilde_02_record_set(v);
}

void toggle_09_value_number_set(number v) {
    this->toggle_09_value_number_setter(v);
    v = this->toggle_09_value_number;
    this->toggle_09_output_set(v);
}

void p_04_out4_number_set(number v) {
    this->toggle_09_value_number_set(v);
}

void linetilde_06_time_set(number v) {
    this->linetilde_06_time = v;
}

template<typename LISTTYPE> void linetilde_06_segments_set(const LISTTYPE& v) {
    this->linetilde_06_segments = jsCreateListCopy(v);

    if ((bool)(v->length)) {
        if (v->length == 1 && this->linetilde_06_time == 0) {
            this->linetilde_06_activeRamps->length = 0;
            this->linetilde_06_currentValue = v[0];
        } else {
            auto currentTime = this->currentsampletime();
            number lastRampValue = this->linetilde_06_currentValue;
            number rampEnd = currentTime - this->sampleOffsetIntoNextAudioBuffer;

            for (Index i = 0; i < this->linetilde_06_activeRamps->length; i += 3) {
                rampEnd = this->linetilde_06_activeRamps[(Index)(i + 2)];

                if (rampEnd > currentTime) {
                    this->linetilde_06_activeRamps[(Index)(i + 2)] = currentTime;
                    number diff = rampEnd - currentTime;
                    number valueDiff = diff * this->linetilde_06_activeRamps[(Index)(i + 1)];
                    lastRampValue = this->linetilde_06_activeRamps[(Index)i] - valueDiff;
                    this->linetilde_06_activeRamps[(Index)i] = lastRampValue;
                    this->linetilde_06_activeRamps->length = i + 3;
                    rampEnd = currentTime;
                } else {
                    lastRampValue = this->linetilde_06_activeRamps[(Index)i];
                }
            }

            if (rampEnd < currentTime) {
                this->linetilde_06_activeRamps->push(lastRampValue);
                this->linetilde_06_activeRamps->push(0);
                this->linetilde_06_activeRamps->push(currentTime);
            }

            number lastRampEnd = currentTime;

            for (Index i = 0; i < v->length; i += 2) {
                number destinationValue = v[(Index)i];
                number inc = 0;
                number rampTimeInSamples;

                if (v->length > i + 1) {
                    rampTimeInSamples = this->mstosamps(v[(Index)(i + 1)]);

                    if ((bool)(this->linetilde_06_keepramp)) {
                        this->linetilde_06_time_set(v[(Index)(i + 1)]);
                    }
                } else {
                    rampTimeInSamples = this->mstosamps(this->linetilde_06_time);
                }

                if (rampTimeInSamples <= 0) {
                    rampTimeInSamples = 1;
                }

                inc = (destinationValue - lastRampValue) / rampTimeInSamples;
                lastRampEnd += rampTimeInSamples;
                this->linetilde_06_activeRamps->push(destinationValue);
                this->linetilde_06_activeRamps->push(inc);
                this->linetilde_06_activeRamps->push(lastRampEnd);
                lastRampValue = destinationValue;
            }
        }
    }
}

void numberobj_22_output_set(number v) {
    {
        listbase<number, 1> converted = {v};
        this->linetilde_06_segments_set(converted);
    }
}

void numberobj_22_value_set(number v) {
    this->numberobj_22_value_setter(v);
    v = this->numberobj_22_value;
    number localvalue = v;

    if (this->numberobj_22_currentFormat != 6) {
        localvalue = trunc(localvalue);
    }

    this->numberobj_22_output_set(localvalue);
}

void p_04_out3_number_set(number v) {
    this->numberobj_22_value_set(v);
}

void recordtilde_01_record_set(number v) {
    this->recordtilde_01_record = v;

    if ((bool)(v)) {
        this->recordtilde_01_lastRecord = 0;
    }
}

void toggle_08_output_set(number v) {
    this->recordtilde_01_record_set(v);
}

void toggle_08_value_number_set(number v) {
    this->toggle_08_value_number_setter(v);
    v = this->toggle_08_value_number;
    this->toggle_08_output_set(v);
}

void p_04_out2_number_set(number v) {
    this->toggle_08_value_number_set(v);
}

void linetilde_05_time_set(number v) {
    this->linetilde_05_time = v;
}

template<typename LISTTYPE> void linetilde_05_segments_set(const LISTTYPE& v) {
    this->linetilde_05_segments = jsCreateListCopy(v);

    if ((bool)(v->length)) {
        if (v->length == 1 && this->linetilde_05_time == 0) {
            this->linetilde_05_activeRamps->length = 0;
            this->linetilde_05_currentValue = v[0];
        } else {
            auto currentTime = this->currentsampletime();
            number lastRampValue = this->linetilde_05_currentValue;
            number rampEnd = currentTime - this->sampleOffsetIntoNextAudioBuffer;

            for (Index i = 0; i < this->linetilde_05_activeRamps->length; i += 3) {
                rampEnd = this->linetilde_05_activeRamps[(Index)(i + 2)];

                if (rampEnd > currentTime) {
                    this->linetilde_05_activeRamps[(Index)(i + 2)] = currentTime;
                    number diff = rampEnd - currentTime;
                    number valueDiff = diff * this->linetilde_05_activeRamps[(Index)(i + 1)];
                    lastRampValue = this->linetilde_05_activeRamps[(Index)i] - valueDiff;
                    this->linetilde_05_activeRamps[(Index)i] = lastRampValue;
                    this->linetilde_05_activeRamps->length = i + 3;
                    rampEnd = currentTime;
                } else {
                    lastRampValue = this->linetilde_05_activeRamps[(Index)i];
                }
            }

            if (rampEnd < currentTime) {
                this->linetilde_05_activeRamps->push(lastRampValue);
                this->linetilde_05_activeRamps->push(0);
                this->linetilde_05_activeRamps->push(currentTime);
            }

            number lastRampEnd = currentTime;

            for (Index i = 0; i < v->length; i += 2) {
                number destinationValue = v[(Index)i];
                number inc = 0;
                number rampTimeInSamples;

                if (v->length > i + 1) {
                    rampTimeInSamples = this->mstosamps(v[(Index)(i + 1)]);

                    if ((bool)(this->linetilde_05_keepramp)) {
                        this->linetilde_05_time_set(v[(Index)(i + 1)]);
                    }
                } else {
                    rampTimeInSamples = this->mstosamps(this->linetilde_05_time);
                }

                if (rampTimeInSamples <= 0) {
                    rampTimeInSamples = 1;
                }

                inc = (destinationValue - lastRampValue) / rampTimeInSamples;
                lastRampEnd += rampTimeInSamples;
                this->linetilde_05_activeRamps->push(destinationValue);
                this->linetilde_05_activeRamps->push(inc);
                this->linetilde_05_activeRamps->push(lastRampEnd);
                lastRampValue = destinationValue;
            }
        }
    }
}

void numberobj_19_output_set(number v) {
    {
        listbase<number, 1> converted = {v};
        this->linetilde_05_segments_set(converted);
    }
}

void numberobj_19_value_set(number v) {
    this->numberobj_19_value_setter(v);
    v = this->numberobj_19_value;
    number localvalue = v;

    if (this->numberobj_19_currentFormat != 6) {
        localvalue = trunc(localvalue);
    }

    this->numberobj_19_output_set(localvalue);
}

void p_04_out1_number_set(number v) {
    this->numberobj_19_value_set(v);
}

void p_04_in1_number_set(number v) {
    this->p_04->updateTime(this->_currentTime, (ENGINE*)nullptr);
    this->p_04->eventinlet_06_out1_number_set(v);
}

void toggle_07_output_set(number v) {
    this->p_04_in1_number_set(v);
}

void toggle_07_value_number_set(number v) {
    this->toggle_07_value_number_setter(v);
    v = this->toggle_07_value_number;
    this->toggle_07_output_set(v);
}

number param_04_value_constrain(number v) const {
    v = (v > 100 ? 100 : (v < 0 ? 0 : v));
    return v;
}

void p_03_in3_number_set(number v) {
    this->p_03->updateTime(this->_currentTime, (ENGINE*)nullptr);
    this->p_03->eventinlet_03_out1_number_set(v);
}

void numberobj_20_output_set(number v) {
    this->p_03_in3_number_set(v);
}

void numberobj_20_value_set(number v) {
    this->numberobj_20_value_setter(v);
    v = this->numberobj_20_value;
    number localvalue = v;

    if ((bool)(true) && localvalue < 0) {
        localvalue = 0;
    }

    if ((bool)(true) && localvalue > 100) {
        localvalue = 100;
    }

    if (this->numberobj_20_currentFormat != 6) {
        localvalue = trunc(localvalue);
    }

    this->numberobj_20_output_set(localvalue);
}

number param_05_value_constrain(number v) const {
    v = (v > 100 ? 100 : (v < 0 ? 0 : v));
    return v;
}

void p_03_in4_number_set(number v) {
    this->p_03->updateTime(this->_currentTime, (ENGINE*)nullptr);
    this->p_03->eventinlet_05_out1_number_set(v);
}

void numberobj_21_output_set(number v) {
    this->p_03_in4_number_set(v);
}

void numberobj_21_value_set(number v) {
    this->numberobj_21_value_setter(v);
    v = this->numberobj_21_value;
    number localvalue = v;

    if ((bool)(true) && localvalue < 0) {
        localvalue = 0;
    }

    if ((bool)(true) && localvalue > 100) {
        localvalue = 100;
    }

    if (this->numberobj_21_currentFormat != 6) {
        localvalue = trunc(localvalue);
    }

    this->numberobj_21_output_set(localvalue);
}

number param_06_value_constrain(number v) const {
    v = (v > 100 ? 100 : (v < -100 ? -100 : v));
    return v;
}

void p_03_in5_number_set(number v) {
    this->p_03->updateTime(this->_currentTime, (ENGINE*)nullptr);
    this->p_03->eventinlet_04_out1_number_set(v);
}

void numberobj_23_output_set(number v) {
    this->p_03_in5_number_set(v);
}

void numberobj_23_value_set(number v) {
    this->numberobj_23_value_setter(v);
    v = this->numberobj_23_value;
    number localvalue = v;

    if ((bool)(true) && localvalue < -100) {
        localvalue = -100;
    }

    if ((bool)(true) && localvalue > 100) {
        localvalue = 100;
    }

    if (this->numberobj_23_currentFormat != 6) {
        localvalue = trunc(localvalue);
    }

    this->numberobj_23_output_set(localvalue);
}

number param_07_value_constrain(number v) const {
    v = (v > 100 ? 100 : (v < 0 ? 0 : v));
    return v;
}

void numberobj_25_output_set(number v) {
    this->linetilde_06_time_set(v);
    this->linetilde_05_time_set(v);
}

void numberobj_25_value_set(number v) {
    this->numberobj_25_value_setter(v);
    v = this->numberobj_25_value;
    number localvalue = v;

    if (this->numberobj_25_currentFormat != 6) {
        localvalue = trunc(localvalue);
    }

    this->numberobj_25_output_set(localvalue);
}

template<typename LISTTYPE> void scale_07_out_set(const LISTTYPE& v) {
    this->scale_07_out = jsCreateListCopy(v);

    {
        number converted = (v->length > 0 ? v[0] : 0);
        this->numberobj_25_value_set(converted);
    }
}

template<typename LISTTYPE> void scale_07_input_set(const LISTTYPE& v) {
    this->scale_07_input = jsCreateListCopy(v);
    list tmp = {};

    for (Index i = 0; i < v->length; i++) {
        tmp->push(this->scale(
            v[(Index)i],
            this->scale_07_inlow,
            this->scale_07_inhigh,
            this->scale_07_outlow,
            this->scale_07_outhigh,
            this->scale_07_power
        ));
    }

    this->scale_07_out_set(tmp);
}

void numberobj_24_output_set(number v) {
    {
        listbase<number, 1> converted = {v};
        this->scale_07_input_set(converted);
    }
}

void numberobj_24_value_set(number v) {
    this->numberobj_24_value_setter(v);
    v = this->numberobj_24_value;
    number localvalue = v;

    if ((bool)(true) && localvalue < 0) {
        localvalue = 0;
    }

    if ((bool)(true) && localvalue > 100) {
        localvalue = 100;
    }

    if (this->numberobj_24_currentFormat != 6) {
        localvalue = trunc(localvalue);
    }

    this->numberobj_24_output_set(localvalue);
}

number param_08_value_constrain(number v) const {
    v = (v > 100 ? 100 : (v < 0 ? 0 : v));
    return v;
}

void p_02_target_PosJitterDepth_number_set(number v) {
    for (Index i = 0; i < 8; i++) {
        if (i + 1 == this->p_02_target || 0 == this->p_02_target) {
            this->p_02[i]->updateTime(this->_currentTime, (ENGINE*)nullptr);
        }
    }

    for (Index i = 0; i < 8; i++) {
        this->p_02[i]->receive_04_output_number_set(v);
    }
}

void p_05_target_PosJitterDepth_number_set(number v) {
    for (Index i = 0; i < 8; i++) {
        if (i + 1 == this->p_05_target || 0 == this->p_05_target) {
            this->p_05[i]->updateTime(this->_currentTime, (ENGINE*)nullptr);
        }
    }

    for (Index i = 0; i < 8; i++) {
        this->p_05[i]->receive_10_output_number_set(v);
    }
}

void send_01_input_number_set(number v) {
    this->send_01_input_number = v;
    this->p_02_target_PosJitterDepth_number_set(v);
    this->p_05_target_PosJitterDepth_number_set(v);
}

void numberobj_27_output_set(number v) {
    this->send_01_input_number_set(v);
}

void numberobj_27_value_set(number v) {
    this->numberobj_27_value_setter(v);
    v = this->numberobj_27_value;
    number localvalue = v;

    if (this->numberobj_27_currentFormat != 6) {
        localvalue = trunc(localvalue);
    }

    this->numberobj_27_output_set(localvalue);
}

template<typename LISTTYPE> void scale_08_out_set(const LISTTYPE& v) {
    this->scale_08_out = jsCreateListCopy(v);

    {
        number converted = (v->length > 0 ? v[0] : 0);
        this->numberobj_27_value_set(converted);
    }
}

template<typename LISTTYPE> void scale_08_input_set(const LISTTYPE& v) {
    this->scale_08_input = jsCreateListCopy(v);
    list tmp = {};

    for (Index i = 0; i < v->length; i++) {
        tmp->push(this->scale(
            v[(Index)i],
            this->scale_08_inlow,
            this->scale_08_inhigh,
            this->scale_08_outlow,
            this->scale_08_outhigh,
            this->scale_08_power
        ));
    }

    this->scale_08_out_set(tmp);
}

void numberobj_26_output_set(number v) {
    {
        listbase<number, 1> converted = {v};
        this->scale_08_input_set(converted);
    }
}

void numberobj_26_value_set(number v) {
    this->numberobj_26_value_setter(v);
    v = this->numberobj_26_value;
    number localvalue = v;

    if ((bool)(true) && localvalue < 0) {
        localvalue = 0;
    }

    if ((bool)(true) && localvalue > 100) {
        localvalue = 100;
    }

    if (this->numberobj_26_currentFormat != 6) {
        localvalue = trunc(localvalue);
    }

    this->numberobj_26_output_set(localvalue);
}

number param_09_value_constrain(number v) const {
    v = (v > 1 ? 1 : (v < 0 ? 0 : v));

    {
        number oneStep = (number)1 / (number)1;
        number oneStepInv = (oneStep != 0 ? (number)1 / oneStep : 0);
        number numberOfSteps = rnbo_fround((v - 0) * oneStepInv * 1 / (number)1) * 1;
        v = numberOfSteps * oneStep + 0;
    }

    return v;
}

void p_06_in5_number_set(number v) {
    this->p_06->updateTime(this->_currentTime, (ENGINE*)nullptr);
    this->p_06->eventinlet_07_out1_number_set(v);
}

void toggle_10_output_set(number v) {
    this->p_06_in5_number_set(v);
}

void toggle_10_value_number_set(number v) {
    this->toggle_10_value_number_setter(v);
    v = this->toggle_10_value_number;
    this->toggle_10_output_set(v);
}

template<typename LISTTYPE> void message_10_set_set(const LISTTYPE& v) {
    this->message_10_set = jsCreateListCopy(v);
}

void param_03_normalizedvalue_set(number v) {
    this->param_03_value_set(this->fromnormalized(2, v));
}

template<typename LISTTYPE> void message_10_out_set(const LISTTYPE& v) {
    {
        if (v->length > 1)
            this->param_03_normalizedvalue_set(v[1]);

        number converted = (v->length > 0 ? v[0] : 0);
        this->param_03_value_set(converted);
    }
}

void message_10_trigger_bang() {
    if ((bool)(this->message_10_set->length) || (bool)(false)) {
        this->message_10_out_set(this->message_10_set);
    }
}

void button_09_output_bang() {
    this->message_10_trigger_bang();
}

void button_09_bangval_bang() {
    ;
    this->button_09_output_bang();
}

void ctlin_01_outchannel_set(number ) {}

void ctlin_01_outcontroller_set(number ) {}

void fromnormalized_01_output_set(number v) {
    this->param_01_value_set(v);
}

void fromnormalized_01_input_set(number v) {
    this->fromnormalized_01_output_set(this->fromnormalized(0, v));
}

void expr_08_out1_set(number v) {
    this->expr_08_out1 = v;
    this->fromnormalized_01_input_set(this->expr_08_out1);
}

void expr_08_in1_set(number in1) {
    this->expr_08_in1 = in1;
    this->expr_08_out1_set(this->expr_08_in1 * this->expr_08_in2);//#map:expr_08:1
}

void ctlin_01_value_set(number v) {
    this->expr_08_in1_set(v);
}

void ctlin_01_midihandler(int status, int channel, int port, ConstByteArray data, Index length) {
    RNBO_UNUSED(length);
    RNBO_UNUSED(port);

    if (status == 0xB0 && (channel == this->ctlin_01_channel || this->ctlin_01_channel == -1) && (data[1] == this->ctlin_01_controller || this->ctlin_01_controller == -1)) {
        this->ctlin_01_outchannel_set(channel);
        this->ctlin_01_outcontroller_set(data[1]);
        this->ctlin_01_value_set(data[2]);
        this->ctlin_01_status = 0;
    }
}

void ctlin_02_outchannel_set(number ) {}

void ctlin_02_outcontroller_set(number ) {}

void fromnormalized_02_output_set(number v) {
    this->param_02_value_set(v);
}

void fromnormalized_02_input_set(number v) {
    this->fromnormalized_02_output_set(this->fromnormalized(1, v));
}

void expr_09_out1_set(number v) {
    this->expr_09_out1 = v;
    this->fromnormalized_02_input_set(this->expr_09_out1);
}

void expr_09_in1_set(number in1) {
    this->expr_09_in1 = in1;
    this->expr_09_out1_set(this->expr_09_in1 * this->expr_09_in2);//#map:expr_09:1
}

void ctlin_02_value_set(number v) {
    this->expr_09_in1_set(v);
}

void ctlin_02_midihandler(int status, int channel, int port, ConstByteArray data, Index length) {
    RNBO_UNUSED(length);
    RNBO_UNUSED(port);

    if (status == 0xB0 && (channel == this->ctlin_02_channel || this->ctlin_02_channel == -1) && (data[1] == this->ctlin_02_controller || this->ctlin_02_controller == -1)) {
        this->ctlin_02_outchannel_set(channel);
        this->ctlin_02_outcontroller_set(data[1]);
        this->ctlin_02_value_set(data[2]);
        this->ctlin_02_status = 0;
    }
}

void ctlin_03_outchannel_set(number ) {}

void ctlin_03_outcontroller_set(number ) {}

void fromnormalized_03_output_set(number v) {
    this->param_03_value_set(v);
}

void fromnormalized_03_input_set(number v) {
    this->fromnormalized_03_output_set(this->fromnormalized(2, v));
}

void expr_10_out1_set(number v) {
    this->expr_10_out1 = v;
    this->fromnormalized_03_input_set(this->expr_10_out1);
}

void expr_10_in1_set(number in1) {
    this->expr_10_in1 = in1;
    this->expr_10_out1_set(this->expr_10_in1 * this->expr_10_in2);//#map:expr_10:1
}

void ctlin_03_value_set(number v) {
    this->expr_10_in1_set(v);
}

void ctlin_03_midihandler(int status, int channel, int port, ConstByteArray data, Index length) {
    RNBO_UNUSED(length);
    RNBO_UNUSED(port);

    if (status == 0xB0 && (channel == this->ctlin_03_channel || this->ctlin_03_channel == -1) && (data[1] == this->ctlin_03_controller || this->ctlin_03_controller == -1)) {
        this->ctlin_03_outchannel_set(channel);
        this->ctlin_03_outcontroller_set(data[1]);
        this->ctlin_03_value_set(data[2]);
        this->ctlin_03_status = 0;
    }
}

void ctlin_04_outchannel_set(number ) {}

void ctlin_04_outcontroller_set(number ) {}

void fromnormalized_04_output_set(number v) {
    this->param_04_value_set(v);
}

void fromnormalized_04_input_set(number v) {
    this->fromnormalized_04_output_set(this->fromnormalized(3, v));
}

void expr_11_out1_set(number v) {
    this->expr_11_out1 = v;
    this->fromnormalized_04_input_set(this->expr_11_out1);
}

void expr_11_in1_set(number in1) {
    this->expr_11_in1 = in1;
    this->expr_11_out1_set(this->expr_11_in1 * this->expr_11_in2);//#map:expr_11:1
}

void ctlin_04_value_set(number v) {
    this->expr_11_in1_set(v);
}

void ctlin_04_midihandler(int status, int channel, int port, ConstByteArray data, Index length) {
    RNBO_UNUSED(length);
    RNBO_UNUSED(port);

    if (status == 0xB0 && (channel == this->ctlin_04_channel || this->ctlin_04_channel == -1) && (data[1] == this->ctlin_04_controller || this->ctlin_04_controller == -1)) {
        this->ctlin_04_outchannel_set(channel);
        this->ctlin_04_outcontroller_set(data[1]);
        this->ctlin_04_value_set(data[2]);
        this->ctlin_04_status = 0;
    }
}

void ctlin_05_outchannel_set(number ) {}

void ctlin_05_outcontroller_set(number ) {}

void fromnormalized_05_output_set(number v) {
    this->param_05_value_set(v);
}

void fromnormalized_05_input_set(number v) {
    this->fromnormalized_05_output_set(this->fromnormalized(4, v));
}

void expr_12_out1_set(number v) {
    this->expr_12_out1 = v;
    this->fromnormalized_05_input_set(this->expr_12_out1);
}

void expr_12_in1_set(number in1) {
    this->expr_12_in1 = in1;
    this->expr_12_out1_set(this->expr_12_in1 * this->expr_12_in2);//#map:expr_12:1
}

void ctlin_05_value_set(number v) {
    this->expr_12_in1_set(v);
}

void ctlin_05_midihandler(int status, int channel, int port, ConstByteArray data, Index length) {
    RNBO_UNUSED(length);
    RNBO_UNUSED(port);

    if (status == 0xB0 && (channel == this->ctlin_05_channel || this->ctlin_05_channel == -1) && (data[1] == this->ctlin_05_controller || this->ctlin_05_controller == -1)) {
        this->ctlin_05_outchannel_set(channel);
        this->ctlin_05_outcontroller_set(data[1]);
        this->ctlin_05_value_set(data[2]);
        this->ctlin_05_status = 0;
    }
}

void ctlin_06_outchannel_set(number ) {}

void ctlin_06_outcontroller_set(number ) {}

void fromnormalized_06_output_set(number v) {
    this->param_06_value_set(v);
}

void fromnormalized_06_input_set(number v) {
    this->fromnormalized_06_output_set(this->fromnormalized(5, v));
}

void expr_13_out1_set(number v) {
    this->expr_13_out1 = v;
    this->fromnormalized_06_input_set(this->expr_13_out1);
}

void expr_13_in1_set(number in1) {
    this->expr_13_in1 = in1;
    this->expr_13_out1_set(this->expr_13_in1 * this->expr_13_in2);//#map:expr_13:1
}

void ctlin_06_value_set(number v) {
    this->expr_13_in1_set(v);
}

void ctlin_06_midihandler(int status, int channel, int port, ConstByteArray data, Index length) {
    RNBO_UNUSED(length);
    RNBO_UNUSED(port);

    if (status == 0xB0 && (channel == this->ctlin_06_channel || this->ctlin_06_channel == -1) && (data[1] == this->ctlin_06_controller || this->ctlin_06_controller == -1)) {
        this->ctlin_06_outchannel_set(channel);
        this->ctlin_06_outcontroller_set(data[1]);
        this->ctlin_06_value_set(data[2]);
        this->ctlin_06_status = 0;
    }
}

void ctlin_07_outchannel_set(number ) {}

void ctlin_07_outcontroller_set(number ) {}

void fromnormalized_07_output_set(number v) {
    this->param_07_value_set(v);
}

void fromnormalized_07_input_set(number v) {
    this->fromnormalized_07_output_set(this->fromnormalized(6, v));
}

void expr_14_out1_set(number v) {
    this->expr_14_out1 = v;
    this->fromnormalized_07_input_set(this->expr_14_out1);
}

void expr_14_in1_set(number in1) {
    this->expr_14_in1 = in1;
    this->expr_14_out1_set(this->expr_14_in1 * this->expr_14_in2);//#map:expr_14:1
}

void ctlin_07_value_set(number v) {
    this->expr_14_in1_set(v);
}

void ctlin_07_midihandler(int status, int channel, int port, ConstByteArray data, Index length) {
    RNBO_UNUSED(length);
    RNBO_UNUSED(port);

    if (status == 0xB0 && (channel == this->ctlin_07_channel || this->ctlin_07_channel == -1) && (data[1] == this->ctlin_07_controller || this->ctlin_07_controller == -1)) {
        this->ctlin_07_outchannel_set(channel);
        this->ctlin_07_outcontroller_set(data[1]);
        this->ctlin_07_value_set(data[2]);
        this->ctlin_07_status = 0;
    }
}

void ctlin_08_outchannel_set(number ) {}

void ctlin_08_outcontroller_set(number ) {}

void fromnormalized_08_output_set(number v) {
    this->param_08_value_set(v);
}

void fromnormalized_08_input_set(number v) {
    this->fromnormalized_08_output_set(this->fromnormalized(7, v));
}

void expr_15_out1_set(number v) {
    this->expr_15_out1 = v;
    this->fromnormalized_08_input_set(this->expr_15_out1);
}

void expr_15_in1_set(number in1) {
    this->expr_15_in1 = in1;
    this->expr_15_out1_set(this->expr_15_in1 * this->expr_15_in2);//#map:expr_15:1
}

void ctlin_08_value_set(number v) {
    this->expr_15_in1_set(v);
}

void ctlin_08_midihandler(int status, int channel, int port, ConstByteArray data, Index length) {
    RNBO_UNUSED(length);
    RNBO_UNUSED(port);

    if (status == 0xB0 && (channel == this->ctlin_08_channel || this->ctlin_08_channel == -1) && (data[1] == this->ctlin_08_controller || this->ctlin_08_controller == -1)) {
        this->ctlin_08_outchannel_set(channel);
        this->ctlin_08_outcontroller_set(data[1]);
        this->ctlin_08_value_set(data[2]);
        this->ctlin_08_status = 0;
    }
}

void ctlin_09_outchannel_set(number ) {}

void ctlin_09_outcontroller_set(number ) {}

void fromnormalized_09_output_set(number v) {
    this->param_09_value_set(v);
}

void fromnormalized_09_input_set(number v) {
    this->fromnormalized_09_output_set(this->fromnormalized(8, v));
}

void expr_16_out1_set(number v) {
    this->expr_16_out1 = v;
    this->fromnormalized_09_input_set(this->expr_16_out1);
}

void expr_16_in1_set(number in1) {
    this->expr_16_in1 = in1;
    this->expr_16_out1_set(this->expr_16_in1 * this->expr_16_in2);//#map:expr_16:1
}

void ctlin_09_value_set(number v) {
    this->expr_16_in1_set(v);
}

void ctlin_09_midihandler(int status, int channel, int port, ConstByteArray data, Index length) {
    RNBO_UNUSED(length);
    RNBO_UNUSED(port);

    if (status == 0xB0 && (channel == this->ctlin_09_channel || this->ctlin_09_channel == -1) && (data[1] == this->ctlin_09_controller || this->ctlin_09_controller == -1)) {
        this->ctlin_09_outchannel_set(channel);
        this->ctlin_09_outcontroller_set(data[1]);
        this->ctlin_09_value_set(data[2]);
        this->ctlin_09_status = 0;
    }
}

void p_01_out1_bang_bang() {
    this->button_09_bangval_bang();
}

void p_01_perform(const SampleValue * in1, const SampleValue * in2, Index n) {
    // subpatcher: Attack~
    ConstSampleArray<2> ins = {in1, in2};

    this->p_01->process(ins, 2, nullptr, 0, n);
}

void recordtilde_01_perform(
    number record,
    number begin,
    number end,
    const SampleValue * input1,
    const SampleValue * input2,
    SampleValue * sync,
    Index n
) {
    RNBO_UNUSED(input2);
    RNBO_UNUSED(input1);
    RNBO_UNUSED(end);
    RNBO_UNUSED(begin);
    auto __recordtilde_01_loop = this->recordtilde_01_loop;
    auto __recordtilde_01_wIndex = this->recordtilde_01_wIndex;
    auto __recordtilde_01_lastRecord = this->recordtilde_01_lastRecord;
    auto __recordtilde_01_append = this->recordtilde_01_append;
    auto __recordtilde_01_buffer = this->recordtilde_01_buffer;
    ConstSampleArray<2> input = {input1, input2};
    number bufferSize = __recordtilde_01_buffer->getSize();
    number srInv = (number)1 / this->sr;

    if (bufferSize > 0) {
        number maxChannel = __recordtilde_01_buffer->getChannels();
        number touched = false;

        for (Index i = 0; i < n; i++) {
            number loopBegin = 0;
            number loopEnd = bufferSize;

            if (loopEnd > loopBegin) {
                if ((bool)(__recordtilde_01_append)) {
                    if ((bool)(record) && __recordtilde_01_lastRecord != record && __recordtilde_01_wIndex >= loopEnd) {
                        __recordtilde_01_wIndex = loopBegin;
                    }
                } else {
                    if ((bool)(record) && __recordtilde_01_lastRecord != record) {
                        __recordtilde_01_wIndex = loopBegin;
                    }
                }

                if (record != 0 && __recordtilde_01_wIndex < loopEnd) {
                    for (number channel = 0; channel < 2; channel++) {
                        number effectiveChannel = channel + 0;

                        if (effectiveChannel < maxChannel) {
                            __recordtilde_01_buffer->setSample(channel, __recordtilde_01_wIndex, input[(Index)channel][(Index)i]);
                            touched = true;
                        } else
                            break;
                    }

                    __recordtilde_01_wIndex++;

                    if ((bool)(__recordtilde_01_loop) && __recordtilde_01_wIndex >= loopEnd) {
                        __recordtilde_01_wIndex = loopBegin;
                    }
                } else {
                    sync[(Index)i] = 0;
                }

                if ((bool)(!(bool)(__recordtilde_01_append)))
                    __recordtilde_01_lastRecord = record;
            }
        }

        if ((bool)(touched)) {
            __recordtilde_01_buffer->setTouched(true);
            __recordtilde_01_buffer->setSampleRate(this->sr);
        }
    }

    this->recordtilde_01_lastRecord = __recordtilde_01_lastRecord;
    this->recordtilde_01_wIndex = __recordtilde_01_wIndex;
}

void recordtilde_02_perform(
    number record,
    number begin,
    number end,
    const SampleValue * input1,
    const SampleValue * input2,
    SampleValue * sync,
    Index n
) {
    RNBO_UNUSED(input2);
    RNBO_UNUSED(input1);
    RNBO_UNUSED(end);
    RNBO_UNUSED(begin);
    auto __recordtilde_02_loop = this->recordtilde_02_loop;
    auto __recordtilde_02_wIndex = this->recordtilde_02_wIndex;
    auto __recordtilde_02_lastRecord = this->recordtilde_02_lastRecord;
    auto __recordtilde_02_append = this->recordtilde_02_append;
    auto __recordtilde_02_buffer = this->recordtilde_02_buffer;
    ConstSampleArray<2> input = {input1, input2};
    number bufferSize = __recordtilde_02_buffer->getSize();
    number srInv = (number)1 / this->sr;

    if (bufferSize > 0) {
        number maxChannel = __recordtilde_02_buffer->getChannels();
        number touched = false;

        for (Index i = 0; i < n; i++) {
            number loopBegin = 0;
            number loopEnd = bufferSize;

            if (loopEnd > loopBegin) {
                if ((bool)(__recordtilde_02_append)) {
                    if ((bool)(record) && __recordtilde_02_lastRecord != record && __recordtilde_02_wIndex >= loopEnd) {
                        __recordtilde_02_wIndex = loopBegin;
                    }
                } else {
                    if ((bool)(record) && __recordtilde_02_lastRecord != record) {
                        __recordtilde_02_wIndex = loopBegin;
                    }
                }

                if (record != 0 && __recordtilde_02_wIndex < loopEnd) {
                    for (number channel = 0; channel < 2; channel++) {
                        number effectiveChannel = channel + 0;

                        if (effectiveChannel < maxChannel) {
                            __recordtilde_02_buffer->setSample(channel, __recordtilde_02_wIndex, input[(Index)channel][(Index)i]);
                            touched = true;
                        } else
                            break;
                    }

                    __recordtilde_02_wIndex++;

                    if ((bool)(__recordtilde_02_loop) && __recordtilde_02_wIndex >= loopEnd) {
                        __recordtilde_02_wIndex = loopBegin;
                    }
                } else {
                    sync[(Index)i] = 0;
                }

                if ((bool)(!(bool)(__recordtilde_02_append)))
                    __recordtilde_02_lastRecord = record;
            }
        }

        if ((bool)(touched)) {
            __recordtilde_02_buffer->setTouched(true);
            __recordtilde_02_buffer->setSampleRate(this->sr);
        }
    }

    this->recordtilde_02_lastRecord = __recordtilde_02_lastRecord;
    this->recordtilde_02_wIndex = __recordtilde_02_wIndex;
}

void p_06_perform(
    const SampleValue * in1,
    const SampleValue * in2,
    SampleValue * out1,
    SampleValue * out2,
    Index n
) {
    // subpatcher: Direct~
    ConstSampleArray<2> ins = {in1, in2};

    SampleArray<2> outs = {out1, out2};
    this->p_06->process(ins, 2, outs, 2, n);
}

void phasor_01_perform(number freq, SampleValue * out, Index n) {
    RNBO_UNUSED(freq);

    for (Index i = 0; i < n; i++) {
        out[(Index)i] = this->phasor_01_ph_next(5, -1);
        this->phasor_01_sigbuf[(Index)i] = -1;
    }
}

void p_02_perform(const SampleValue * in1, SampleValue * out1, SampleValue * out2, Index n) {
    ConstSampleArray<1> ins = {in1};
    SampleArray<2> outs = {out1, out2};

    for (number chan = 0; chan < 2; chan++)
        zeroSignal(outs[(Index)chan], n);

    for (Index i = 0; i < 8; i++)
        this->p_02[(Index)i]->process(ins, 1, outs, 2, n);
}

void p_05_perform(const SampleValue * in1, SampleValue * out1, SampleValue * out2, Index n) {
    ConstSampleArray<1> ins = {in1};
    SampleArray<2> outs = {out1, out2};

    for (number chan = 0; chan < 2; chan++)
        zeroSignal(outs[(Index)chan], n);

    for (Index i = 0; i < 8; i++)
        this->p_05[(Index)i]->process(ins, 1, outs, 2, n);
}

void p_04_perform(Index n) {
    // subpatcher: Freeze
    this->p_04->process(nullptr, 0, nullptr, 0, n);
}

void linetilde_05_perform(SampleValue * out, Index n) {
    auto __linetilde_05_time = this->linetilde_05_time;
    auto __linetilde_05_keepramp = this->linetilde_05_keepramp;
    auto __linetilde_05_currentValue = this->linetilde_05_currentValue;
    Index i = 0;

    if ((bool)(this->linetilde_05_activeRamps->length)) {
        while ((bool)(this->linetilde_05_activeRamps->length) && i < n) {
            number destinationValue = this->linetilde_05_activeRamps[0];
            number inc = this->linetilde_05_activeRamps[1];
            number rampTimeInSamples = this->linetilde_05_activeRamps[2] - this->audioProcessSampleCount - i;
            number val = __linetilde_05_currentValue;

            while (rampTimeInSamples > 0 && i < n) {
                out[(Index)i] = val;
                val += inc;
                i++;
                rampTimeInSamples--;
            }

            if (rampTimeInSamples <= 0) {
                val = destinationValue;
                this->linetilde_05_activeRamps->splice(0, 3);

                if ((bool)(!(bool)(this->linetilde_05_activeRamps->length))) {
                    this->getEngine()->scheduleClockEventWithValue(
                        this,
                        -1621164530,
                        this->sampsToMs((SampleIndex)(this->vs)) + this->_currentTime,
                        0
                    );;

                    if ((bool)(!(bool)(__linetilde_05_keepramp))) {
                        __linetilde_05_time = 0;
                    }
                }
            }

            __linetilde_05_currentValue = val;
        }
    }

    while (i < n) {
        out[(Index)i] = __linetilde_05_currentValue;
        i++;
    }

    this->linetilde_05_currentValue = __linetilde_05_currentValue;
    this->linetilde_05_time = __linetilde_05_time;
}

void hann_tilde_03_perform(const Sample * x, SampleValue * out1, Index n) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        number __x = x[(Index)i];
        __x = (__x > 1 ? 1 : (__x < 0 ? 0 : __x));
        out1[(Index)i] = 0.5 * (1 - rnbo_cos(6.28318530717958647692 * __x));
    }
}

void dspexpr_40_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        out1[(Index)i] = in1[(Index)i] * in2[(Index)i];//#map:_###_obj_###_:1
    }
}

void metertilde_01_perform(const SampleValue * input, Index n) {
    auto __metertilde_01_currentIntervalInSamples = this->metertilde_01_currentIntervalInSamples;
    auto __metertilde_01_currentInterval = this->metertilde_01_currentInterval;
    auto __metertilde_01_currentPeak = this->metertilde_01_currentPeak;

    for (Index i = 0; i < n; i++) {
        number value = (input[(Index)i] < 0 ? -input[(Index)i] : input[(Index)i]);

        if (value > __metertilde_01_currentPeak)
            __metertilde_01_currentPeak = value;
    }

    __metertilde_01_currentInterval -= n;

    if (__metertilde_01_currentInterval <= 0) {
        __metertilde_01_currentInterval = __metertilde_01_currentIntervalInSamples;
        ;

        this->getEngine()->scheduleClockEventWithValue(
            this,
            1159774702,
            this->sampsToMs((SampleIndex)(this->vs)) + this->_currentTime,
            __metertilde_01_currentPeak
        );;

        __metertilde_01_currentPeak = 0;
    }

    this->metertilde_01_currentPeak = __metertilde_01_currentPeak;
    this->metertilde_01_currentInterval = __metertilde_01_currentInterval;
}

void dspexpr_42_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        out1[(Index)i] = in1[(Index)i] * in2[(Index)i];//#map:_###_obj_###_:1
    }
}

void metertilde_02_perform(const SampleValue * input, Index n) {
    auto __metertilde_02_currentIntervalInSamples = this->metertilde_02_currentIntervalInSamples;
    auto __metertilde_02_currentInterval = this->metertilde_02_currentInterval;
    auto __metertilde_02_currentPeak = this->metertilde_02_currentPeak;

    for (Index i = 0; i < n; i++) {
        number value = (input[(Index)i] < 0 ? -input[(Index)i] : input[(Index)i]);

        if (value > __metertilde_02_currentPeak)
            __metertilde_02_currentPeak = value;
    }

    __metertilde_02_currentInterval -= n;

    if (__metertilde_02_currentInterval <= 0) {
        __metertilde_02_currentInterval = __metertilde_02_currentIntervalInSamples;
        ;

        this->getEngine()->scheduleClockEventWithValue(
            this,
            -1029704085,
            this->sampsToMs((SampleIndex)(this->vs)) + this->_currentTime,
            __metertilde_02_currentPeak
        );;

        __metertilde_02_currentPeak = 0;
    }

    this->metertilde_02_currentPeak = __metertilde_02_currentPeak;
    this->metertilde_02_currentInterval = __metertilde_02_currentInterval;
}

void linetilde_06_perform(SampleValue * out, Index n) {
    auto __linetilde_06_time = this->linetilde_06_time;
    auto __linetilde_06_keepramp = this->linetilde_06_keepramp;
    auto __linetilde_06_currentValue = this->linetilde_06_currentValue;
    Index i = 0;

    if ((bool)(this->linetilde_06_activeRamps->length)) {
        while ((bool)(this->linetilde_06_activeRamps->length) && i < n) {
            number destinationValue = this->linetilde_06_activeRamps[0];
            number inc = this->linetilde_06_activeRamps[1];
            number rampTimeInSamples = this->linetilde_06_activeRamps[2] - this->audioProcessSampleCount - i;
            number val = __linetilde_06_currentValue;

            while (rampTimeInSamples > 0 && i < n) {
                out[(Index)i] = val;
                val += inc;
                i++;
                rampTimeInSamples--;
            }

            if (rampTimeInSamples <= 0) {
                val = destinationValue;
                this->linetilde_06_activeRamps->splice(0, 3);

                if ((bool)(!(bool)(this->linetilde_06_activeRamps->length))) {
                    this->getEngine()->scheduleClockEventWithValue(
                        this,
                        -62043057,
                        this->sampsToMs((SampleIndex)(this->vs)) + this->_currentTime,
                        0
                    );;

                    if ((bool)(!(bool)(__linetilde_06_keepramp))) {
                        __linetilde_06_time = 0;
                    }
                }
            }

            __linetilde_06_currentValue = val;
        }
    }

    while (i < n) {
        out[(Index)i] = __linetilde_06_currentValue;
        i++;
    }

    this->linetilde_06_currentValue = __linetilde_06_currentValue;
    this->linetilde_06_time = __linetilde_06_time;
}

void hann_tilde_04_perform(const Sample * x, SampleValue * out1, Index n) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        number __x = x[(Index)i];
        __x = (__x > 1 ? 1 : (__x < 0 ? 0 : __x));
        out1[(Index)i] = 0.5 * (1 - rnbo_cos(6.28318530717958647692 * __x));
    }
}

void dspexpr_45_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        out1[(Index)i] = in1[(Index)i] * in2[(Index)i];//#map:_###_obj_###_:1
    }
}

void metertilde_04_perform(const SampleValue * input, Index n) {
    auto __metertilde_04_currentIntervalInSamples = this->metertilde_04_currentIntervalInSamples;
    auto __metertilde_04_currentInterval = this->metertilde_04_currentInterval;
    auto __metertilde_04_currentPeak = this->metertilde_04_currentPeak;

    for (Index i = 0; i < n; i++) {
        number value = (input[(Index)i] < 0 ? -input[(Index)i] : input[(Index)i]);

        if (value > __metertilde_04_currentPeak)
            __metertilde_04_currentPeak = value;
    }

    __metertilde_04_currentInterval -= n;

    if (__metertilde_04_currentInterval <= 0) {
        __metertilde_04_currentInterval = __metertilde_04_currentIntervalInSamples;
        ;

        this->getEngine()->scheduleClockEventWithValue(
            this,
            1861604205,
            this->sampsToMs((SampleIndex)(this->vs)) + this->_currentTime,
            __metertilde_04_currentPeak
        );;

        __metertilde_04_currentPeak = 0;
    }

    this->metertilde_04_currentPeak = __metertilde_04_currentPeak;
    this->metertilde_04_currentInterval = __metertilde_04_currentInterval;
}

void dspexpr_43_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        out1[(Index)i] = in1[(Index)i] + in2[(Index)i];//#map:_###_obj_###_:1
    }
}

void dspexpr_44_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        out1[(Index)i] = in1[(Index)i] * in2[(Index)i];//#map:_###_obj_###_:1
    }
}

void metertilde_03_perform(const SampleValue * input, Index n) {
    auto __metertilde_03_currentIntervalInSamples = this->metertilde_03_currentIntervalInSamples;
    auto __metertilde_03_currentInterval = this->metertilde_03_currentInterval;
    auto __metertilde_03_currentPeak = this->metertilde_03_currentPeak;

    for (Index i = 0; i < n; i++) {
        number value = (input[(Index)i] < 0 ? -input[(Index)i] : input[(Index)i]);

        if (value > __metertilde_03_currentPeak)
            __metertilde_03_currentPeak = value;
    }

    __metertilde_03_currentInterval -= n;

    if (__metertilde_03_currentInterval <= 0) {
        __metertilde_03_currentInterval = __metertilde_03_currentIntervalInSamples;
        ;

        this->getEngine()->scheduleClockEventWithValue(
            this,
            1075784424,
            this->sampsToMs((SampleIndex)(this->vs)) + this->_currentTime,
            __metertilde_03_currentPeak
        );;

        __metertilde_03_currentPeak = 0;
    }

    this->metertilde_03_currentPeak = __metertilde_03_currentPeak;
    this->metertilde_03_currentInterval = __metertilde_03_currentInterval;
}

void dspexpr_41_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        out1[(Index)i] = in1[(Index)i] + in2[(Index)i];//#map:_###_obj_###_:1
    }
}

void p_03_perform(
    const SampleValue * in1,
    const SampleValue * in2,
    SampleValue * out1,
    SampleValue * out2,
    Index n
) {
    // subpatcher: EQ&Volume~
    ConstSampleArray<2> ins = {in1, in2};

    SampleArray<2> outs = {out1, out2};
    this->p_03->process(ins, 2, outs, 2, n);
}

void signaladder_01_perform(
    const SampleValue * in1,
    const SampleValue * in2,
    SampleValue * out,
    Index n
) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        out[(Index)i] = in1[(Index)i] + in2[(Index)i];
    }
}

void signaladder_02_perform(
    const SampleValue * in1,
    const SampleValue * in2,
    SampleValue * out,
    Index n
) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        out[(Index)i] = in1[(Index)i] + in2[(Index)i];
    }
}

void stackprotect_perform(Index n) {
    RNBO_UNUSED(n);
    auto __stackprotect_count = this->stackprotect_count;
    __stackprotect_count = 0;
    this->stackprotect_count = __stackprotect_count;
}

void data_01_srout_set(number ) {}

void data_01_chanout_set(number ) {}

void data_01_sizeout_set(number v) {
    this->data_01_sizeout = v;
}

void data_02_srout_set(number ) {}

void data_02_chanout_set(number ) {}

void data_02_sizeout_set(number v) {
    this->data_02_sizeout = v;
}

void numberobj_17_value_setter(number v) {
    number localvalue = v;

    if ((bool)(true) && localvalue < 0) {
        localvalue = 0;
    }

    if ((bool)(true) && localvalue > 100) {
        localvalue = 100;
    }

    if (this->numberobj_17_currentFormat != 6) {
        localvalue = trunc(localvalue);
    }

    this->numberobj_17_value = localvalue;
}

void numberobj_18_value_setter(number v) {
    number localvalue = v;

    if ((bool)(true) && localvalue < 0) {
        localvalue = 0;
    }

    if ((bool)(true) && localvalue > 100) {
        localvalue = 100;
    }

    if (this->numberobj_18_currentFormat != 6) {
        localvalue = trunc(localvalue);
    }

    this->numberobj_18_value = localvalue;
}

void toggle_07_value_number_setter(number v) {
    this->toggle_07_value_number = (v != 0 ? 1 : 0);
}

void numberobj_19_value_setter(number v) {
    number localvalue = v;

    if (this->numberobj_19_currentFormat != 6) {
        localvalue = trunc(localvalue);
    }

    this->numberobj_19_value = localvalue;
}

void toggle_08_value_number_setter(number v) {
    this->toggle_08_value_number = (v != 0 ? 1 : 0);
}

void numberobj_20_value_setter(number v) {
    number localvalue = v;

    if ((bool)(true) && localvalue < 0) {
        localvalue = 0;
    }

    if ((bool)(true) && localvalue > 100) {
        localvalue = 100;
    }

    if (this->numberobj_20_currentFormat != 6) {
        localvalue = trunc(localvalue);
    }

    this->numberobj_20_value = localvalue;
}

void numberobj_21_value_setter(number v) {
    number localvalue = v;

    if ((bool)(true) && localvalue < 0) {
        localvalue = 0;
    }

    if ((bool)(true) && localvalue > 100) {
        localvalue = 100;
    }

    if (this->numberobj_21_currentFormat != 6) {
        localvalue = trunc(localvalue);
    }

    this->numberobj_21_value = localvalue;
}

void numberobj_22_value_setter(number v) {
    number localvalue = v;

    if (this->numberobj_22_currentFormat != 6) {
        localvalue = trunc(localvalue);
    }

    this->numberobj_22_value = localvalue;
}

void numberobj_23_value_setter(number v) {
    number localvalue = v;

    if ((bool)(true) && localvalue < -100) {
        localvalue = -100;
    }

    if ((bool)(true) && localvalue > 100) {
        localvalue = 100;
    }

    if (this->numberobj_23_currentFormat != 6) {
        localvalue = trunc(localvalue);
    }

    this->numberobj_23_value = localvalue;
}

void numberobj_24_value_setter(number v) {
    number localvalue = v;

    if ((bool)(true) && localvalue < 0) {
        localvalue = 0;
    }

    if ((bool)(true) && localvalue > 100) {
        localvalue = 100;
    }

    if (this->numberobj_24_currentFormat != 6) {
        localvalue = trunc(localvalue);
    }

    this->numberobj_24_value = localvalue;
}

void numberobj_25_value_setter(number v) {
    number localvalue = v;

    if (this->numberobj_25_currentFormat != 6) {
        localvalue = trunc(localvalue);
    }

    this->numberobj_25_value = localvalue;
}

void toggle_09_value_number_setter(number v) {
    this->toggle_09_value_number = (v != 0 ? 1 : 0);
}

void numberobj_26_value_setter(number v) {
    number localvalue = v;

    if ((bool)(true) && localvalue < 0) {
        localvalue = 0;
    }

    if ((bool)(true) && localvalue > 100) {
        localvalue = 100;
    }

    if (this->numberobj_26_currentFormat != 6) {
        localvalue = trunc(localvalue);
    }

    this->numberobj_26_value = localvalue;
}

void numberobj_27_value_setter(number v) {
    number localvalue = v;

    if (this->numberobj_27_currentFormat != 6) {
        localvalue = trunc(localvalue);
    }

    this->numberobj_27_value = localvalue;
}

void toggle_10_value_number_setter(number v) {
    this->toggle_10_value_number = (v != 0 ? 1 : 0);
}

void message_10_init() {
    this->message_10_set_set(listbase<number, 1>{1});
}

void numberobj_17_init() {
    this->numberobj_17_currentFormat = 6;
}

void numberobj_17_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->numberobj_17_value;
}

void numberobj_17_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->numberobj_17_value_set(preset["value"]);
}

void param_01_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->param_01_value;
}

void param_01_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->param_01_value_set(preset["value"]);
}

void phasor_01_onBeatTimeChanged(number beattime) {
    RNBO_UNUSED(beattime);
    this->phasor_01_recalcInc = true;
    this->phasor_01_recalcPhase = true;
}

number phasor_01_ph_next(number freq, number reset) {
    RNBO_UNUSED(reset);
    RNBO_UNUSED(freq);
    number pincr = 5 * this->phasor_01_ph_conv;

    if (this->phasor_01_ph_currentPhase < 0.)
        this->phasor_01_ph_currentPhase = 1. + this->phasor_01_ph_currentPhase;

    if (this->phasor_01_ph_currentPhase > 1.)
        this->phasor_01_ph_currentPhase = this->phasor_01_ph_currentPhase - 1.;

    number tmp = this->phasor_01_ph_currentPhase;
    this->phasor_01_ph_currentPhase += pincr;
    return tmp;
}

void phasor_01_ph_reset() {
    this->phasor_01_ph_currentPhase = 0;
}

void phasor_01_ph_dspsetup() {
    this->phasor_01_ph_conv = (number)1 / this->sr;
}

void phasor_01_dspsetup(bool force) {
    if ((bool)(this->phasor_01_setupDone) && (bool)(!(bool)(force)))
        return;

    this->phasor_01_setupDone = true;
    this->phasor_01_ph_dspsetup();
}

void metertilde_01_init() {}

void metertilde_01_dspsetup(bool force) {
    if ((bool)(this->metertilde_01_setupDone) && (bool)(!(bool)(force)))
        return;

    this->metertilde_01_currentIntervalInSamples = this->mstosamps(50);
    this->metertilde_01_currentInterval = this->metertilde_01_currentIntervalInSamples;
    this->metertilde_01_setupDone = true;
}

void metertilde_02_init() {}

void metertilde_02_dspsetup(bool force) {
    if ((bool)(this->metertilde_02_setupDone) && (bool)(!(bool)(force)))
        return;

    this->metertilde_02_currentIntervalInSamples = this->mstosamps(50);
    this->metertilde_02_currentInterval = this->metertilde_02_currentIntervalInSamples;
    this->metertilde_02_setupDone = true;
}

void numberobj_18_init() {
    this->numberobj_18_currentFormat = 6;
}

void numberobj_18_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->numberobj_18_value;
}

void numberobj_18_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->numberobj_18_value_set(preset["value"]);
}

void param_02_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->param_02_value;
}

void param_02_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->param_02_value_set(preset["value"]);
}

void toggle_07_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->toggle_07_value_number;
}

void toggle_07_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->toggle_07_value_number_set(preset["value"]);
}

void param_03_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->param_03_value;
}

void param_03_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->param_03_value_set(preset["value"]);
}

void numberobj_19_init() {
    this->numberobj_19_currentFormat = 6;
}

void numberobj_19_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->numberobj_19_value;
}

void numberobj_19_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->numberobj_19_value_set(preset["value"]);
}

void toggle_08_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->toggle_08_value_number;
}

void toggle_08_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->toggle_08_value_number_set(preset["value"]);
}

void numberobj_20_init() {
    this->numberobj_20_currentFormat = 6;
}

void numberobj_20_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->numberobj_20_value;
}

void numberobj_20_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->numberobj_20_value_set(preset["value"]);
}

void param_04_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->param_04_value;
}

void param_04_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->param_04_value_set(preset["value"]);
}

void metertilde_03_init() {}

void metertilde_03_dspsetup(bool force) {
    if ((bool)(this->metertilde_03_setupDone) && (bool)(!(bool)(force)))
        return;

    this->metertilde_03_currentIntervalInSamples = this->mstosamps(50);
    this->metertilde_03_currentInterval = this->metertilde_03_currentIntervalInSamples;
    this->metertilde_03_setupDone = true;
}

void numberobj_21_init() {
    this->numberobj_21_currentFormat = 6;
}

void numberobj_21_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->numberobj_21_value;
}

void numberobj_21_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->numberobj_21_value_set(preset["value"]);
}

void param_05_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->param_05_value;
}

void param_05_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->param_05_value_set(preset["value"]);
}

void metertilde_04_init() {}

void metertilde_04_dspsetup(bool force) {
    if ((bool)(this->metertilde_04_setupDone) && (bool)(!(bool)(force)))
        return;

    this->metertilde_04_currentIntervalInSamples = this->mstosamps(50);
    this->metertilde_04_currentInterval = this->metertilde_04_currentIntervalInSamples;
    this->metertilde_04_setupDone = true;
}

void numberobj_22_init() {
    this->numberobj_22_currentFormat = 6;
}

void numberobj_22_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->numberobj_22_value;
}

void numberobj_22_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->numberobj_22_value_set(preset["value"]);
}

void numberobj_23_init() {
    this->numberobj_23_currentFormat = 6;
}

void numberobj_23_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->numberobj_23_value;
}

void numberobj_23_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->numberobj_23_value_set(preset["value"]);
}

void param_06_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->param_06_value;
}

void param_06_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->param_06_value_set(preset["value"]);
}

void numberobj_24_init() {
    this->numberobj_24_currentFormat = 6;
}

void numberobj_24_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->numberobj_24_value;
}

void numberobj_24_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->numberobj_24_value_set(preset["value"]);
}

void param_07_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->param_07_value;
}

void param_07_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->param_07_value_set(preset["value"]);
}

void numberobj_25_init() {
    this->numberobj_25_currentFormat = 6;
}

void numberobj_25_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->numberobj_25_value;
}

void numberobj_25_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->numberobj_25_value_set(preset["value"]);
}

void data_01_init() {
    this->data_01_buffer->setWantsFill(true);
}

Index data_01_evaluateSizeExpr(number samplerate, number vectorsize) {
    RNBO_UNUSED(vectorsize);
    RNBO_UNUSED(samplerate);
    number size = 0;
    return (Index)(size);
}

void data_01_dspsetup(bool force) {
    if ((bool)(this->data_01_setupDone) && (bool)(!(bool)(force)))
        return;

    {
        if (this->data_01_sizemode == 2) {
            this->data_01_buffer = this->data_01_buffer->setSize((Index)(this->mstosamps(this->data_01_sizems)));
            updateDataRef(this, this->data_01_buffer);
        } else if (this->data_01_sizemode == 3) {
            this->data_01_buffer = this->data_01_buffer->setSize(this->data_01_evaluateSizeExpr(this->sr, this->vs));
            updateDataRef(this, this->data_01_buffer);
        }
    }

    this->data_01_setupDone = true;
}

void data_01_afterBufferUpdate() {
    this->data_01_report();
}

void data_01_report() {
    {
        this->data_01_srout_set(this->data_01_buffer->getSampleRate());
        this->data_01_chanout_set(this->data_01_buffer->getChannels());
    }

    this->data_01_sizeout_set(this->data_01_buffer->getSize());
}

void data_02_init() {
    this->data_02_buffer->setWantsFill(true);
}

Index data_02_evaluateSizeExpr(number samplerate, number vectorsize) {
    RNBO_UNUSED(vectorsize);
    RNBO_UNUSED(samplerate);
    number size = 0;
    return (Index)(size);
}

void data_02_dspsetup(bool force) {
    if ((bool)(this->data_02_setupDone) && (bool)(!(bool)(force)))
        return;

    {
        if (this->data_02_sizemode == 2) {
            this->data_02_buffer = this->data_02_buffer->setSize((Index)(this->mstosamps(this->data_02_sizems)));
            updateDataRef(this, this->data_02_buffer);
        } else if (this->data_02_sizemode == 3) {
            this->data_02_buffer = this->data_02_buffer->setSize(this->data_02_evaluateSizeExpr(this->sr, this->vs));
            updateDataRef(this, this->data_02_buffer);
        }
    }

    this->data_02_setupDone = true;
}

void data_02_afterBufferUpdate() {
    this->data_02_report();
}

void data_02_report() {
    {
        this->data_02_srout_set(this->data_02_buffer->getSampleRate());
        this->data_02_chanout_set(this->data_02_buffer->getChannels());
    }

    this->data_02_sizeout_set(this->data_02_buffer->getSize());
}

void toggle_09_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->toggle_09_value_number;
}

void toggle_09_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->toggle_09_value_number_set(preset["value"]);
}

void numberobj_26_init() {
    this->numberobj_26_currentFormat = 6;
}

void numberobj_26_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->numberobj_26_value;
}

void numberobj_26_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->numberobj_26_value_set(preset["value"]);
}

void param_08_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->param_08_value;
}

void param_08_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->param_08_value_set(preset["value"]);
}

void numberobj_27_init() {
    this->numberobj_27_currentFormat = 6;
}

void numberobj_27_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->numberobj_27_value;
}

void numberobj_27_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->numberobj_27_value_set(preset["value"]);
}

void toggle_10_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->toggle_10_value_number;
}

void toggle_10_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->toggle_10_value_number_set(preset["value"]);
}

void param_09_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->param_09_value;
}

void param_09_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->param_09_value_set(preset["value"]);
}

number globaltransport_getSampleOffset(MillisecondTime time) {
    return this->mstosamps(this->maximum(0, time - this->getEngine()->getCurrentTime()));
}

number globaltransport_getTempoAtSample(SampleIndex sampleOffset) {
    return (sampleOffset >= 0 && sampleOffset < (SampleIndex)(this->vs) ? this->globaltransport_tempo[(Index)sampleOffset] : this->globaltransport_lastTempo);
}

number globaltransport_getStateAtSample(SampleIndex sampleOffset) {
    return (sampleOffset >= 0 && sampleOffset < (SampleIndex)(this->vs) ? this->globaltransport_state[(Index)sampleOffset] : this->globaltransport_lastState);
}

number globaltransport_getState(MillisecondTime time) {
    return this->globaltransport_getStateAtSample((SampleIndex)(this->globaltransport_getSampleOffset(time)));
}

number globaltransport_getTempo(MillisecondTime time) {
    return this->globaltransport_getTempoAtSample((SampleIndex)(this->globaltransport_getSampleOffset(time)));
}

number globaltransport_getBeatTime(MillisecondTime time) {
    number i = 2;

    while (i < this->globaltransport_beatTimeChanges->length && this->globaltransport_beatTimeChanges[(Index)(i + 1)] <= time) {
        i += 2;
    }

    i -= 2;
    number beatTimeBase = this->globaltransport_beatTimeChanges[(Index)i];

    if (this->globaltransport_getState(time) == 0)
        return beatTimeBase;

    number beatTimeBaseMsTime = this->globaltransport_beatTimeChanges[(Index)(i + 1)];
    number diff = time - beatTimeBaseMsTime;
    number diffInBeats = diff * this->globaltransport_getTempo(time) * 0.008 / (number)480;
    return beatTimeBase + diffInBeats;
}

bool globaltransport_setTempo(MillisecondTime time, number tempo, bool notify) {
    if ((bool)(notify)) {
        this->processTempoEvent(time, tempo);
        this->globaltransport_notify = true;
    } else {
        Index offset = (Index)(this->globaltransport_getSampleOffset(time));

        if (this->globaltransport_getTempoAtSample((SampleIndex)(offset)) != tempo) {
            this->globaltransport_beatTimeChanges->push(this->globaltransport_getBeatTime(time));
            this->globaltransport_beatTimeChanges->push(time);
            fillSignal(this->globaltransport_tempo, this->vs, tempo, offset);
            this->globaltransport_lastTempo = tempo;
            this->globaltransport_tempoNeedsReset = true;
            return true;
        }
    }

    return false;
}

bool globaltransport_setState(MillisecondTime time, number state, bool notify) {
    if ((bool)(notify)) {
        this->processTransportEvent(time, TransportState(state));
        this->globaltransport_notify = true;
    } else {
        SampleIndex offset = (SampleIndex)(this->globaltransport_getSampleOffset(time));

        if (this->globaltransport_getStateAtSample(offset) != state) {
            this->globaltransport_beatTimeChanges->push(this->globaltransport_getBeatTime(time));
            this->globaltransport_beatTimeChanges->push(time);
            fillSignal(this->globaltransport_state, this->vs, state, (Index)(offset));
            this->globaltransport_lastState = TransportState(state);
            this->globaltransport_stateNeedsReset = true;
            return true;
        }
    }

    return false;
}

bool globaltransport_setBeatTime(MillisecondTime time, number beattime, bool notify) {
    if ((bool)(notify)) {
        this->processBeatTimeEvent(time, beattime);
        this->globaltransport_notify = true;
        return false;
    } else {
        bool beatTimeHasChanged = false;
        number currentBeatTime = this->globaltransport_getBeatTime(time);

        if (rnbo_abs(beattime - currentBeatTime) > 0.001) {
            beatTimeHasChanged = true;
            this->globaltransport_beatTimeChanges->push(beattime);
            this->globaltransport_beatTimeChanges->push(time);
        } else {
            this->globaltransport_beatTimeChanges->push(currentBeatTime);
            this->globaltransport_beatTimeChanges->push(time);
        }

        return beatTimeHasChanged;
    }
}

number globaltransport_getBeatTimeAtSample(SampleIndex sampleOffset) {
    auto msOffset = this->sampstoms(sampleOffset);
    return this->globaltransport_getBeatTime(this->getEngine()->getCurrentTime() + msOffset);
}

array<number, 2> globaltransport_getTimeSignature(MillisecondTime time) {
    number i = 3;

    while (i < this->globaltransport_timeSignatureChanges->length && this->globaltransport_timeSignatureChanges[(Index)(i + 2)] <= time) {
        i += 3;
    }

    i -= 3;

    return {
        this->globaltransport_timeSignatureChanges[(Index)i],
        this->globaltransport_timeSignatureChanges[(Index)(i + 1)]
    };
}

array<number, 2> globaltransport_getTimeSignatureAtSample(SampleIndex sampleOffset) {
    MillisecondTime msOffset = (MillisecondTime)(this->sampstoms(sampleOffset));
    return this->globaltransport_getTimeSignature(this->getEngine()->getCurrentTime() + msOffset);
}

void globaltransport_setBBUBase(
    MillisecondTime time,
    number numerator,
    number denominator,
    number bars,
    number beats,
    number units
) {
    number beatsInQuarterNotes = this->globaltransport_getBeatTime(time);
    bars--;
    beats--;
    number beatsIncCurrenttDenom = beatsInQuarterNotes * (denominator * 0.25);
    number beatLength = (number)4 / denominator;
    number beatLengthInUnits = beatLength * 480;

    while (units > beatLengthInUnits) {
        units -= beatLengthInUnits;
        beats++;
    }

    number targetBeatTime = bars * numerator + beats + units / beatLengthInUnits;
    this->globaltransport_bbuBase = targetBeatTime - beatsIncCurrenttDenom;
}

array<number, 3> globaltransport_getBBU(MillisecondTime time) {
    array<number, 2> currentSig = this->globaltransport_getTimeSignature(time);
    number numerator = currentSig[0];
    number denominator = currentSig[1];
    number beatsInQuarterNotes = this->globaltransport_getBeatTime(time);
    number beatsIncCurrenttDenom = beatsInQuarterNotes * (denominator * 0.25);
    number beatLength = (number)4 / denominator;
    number beatLengthInUnits = beatLength * 480;
    number targetBeatTime = beatsIncCurrenttDenom + this->globaltransport_bbuBase;
    number currentBars = 0;
    number currentBeats = 0;
    number currentUnits = 0;

    if (targetBeatTime >= 0) {
        currentBars = trunc(targetBeatTime / numerator);
        targetBeatTime -= currentBars * numerator;
        currentBeats = trunc(targetBeatTime);
        targetBeatTime -= currentBeats;
        currentUnits = targetBeatTime * beatLengthInUnits;
    } else {
        currentBars = trunc(targetBeatTime / numerator);
        targetBeatTime -= currentBars * numerator;

        if (targetBeatTime != 0) {
            currentBars -= 1;
            currentBeats = trunc(targetBeatTime);
            targetBeatTime -= currentBeats;
            currentBeats = numerator + currentBeats;
            currentUnits = targetBeatTime * beatLengthInUnits;

            if (currentUnits != 0) {
                currentUnits = beatLengthInUnits + currentUnits;
                currentBeats -= 1;
            }
        }
    }

    return {currentBars + 1, currentBeats + 1, currentUnits};
}

bool globaltransport_setTimeSignature(MillisecondTime time, number numerator, number denominator, bool notify) {
    if ((bool)(notify)) {
        this->processTimeSignatureEvent(time, (Int)(numerator), (Int)(denominator));
        this->globaltransport_notify = true;
    } else {
        array<number, 2> currentSig = this->globaltransport_getTimeSignature(time);

        if (currentSig[0] != numerator || currentSig[1] != denominator) {
            array<number, 3> bbu = this->globaltransport_getBBU(time);
            this->globaltransport_setBBUBase(time, numerator, denominator, bbu[0], bbu[1], bbu[2]);
            this->globaltransport_timeSignatureChanges->push(numerator);
            this->globaltransport_timeSignatureChanges->push(denominator);
            this->globaltransport_timeSignatureChanges->push(time);
            return true;
        }
    }

    return false;
}

array<number, 3> globaltransport_getBBUAtSample(SampleIndex sampleOffset) {
    auto msOffset = this->sampstoms(sampleOffset);
    return this->globaltransport_getBBU(this->getEngine()->getCurrentTime() + msOffset);
}

bool globaltransport_setBBU(MillisecondTime time, number bars, number beats, number units, bool notify) {
    RNBO_UNUSED(notify);
    array<number, 2> sig = this->globaltransport_getTimeSignature(time);
    number numerator = sig[0];
    number denominator = sig[1];
    this->globaltransport_setBBUBase(time, numerator, denominator, bars, beats, units);
    return true;
}

void globaltransport_advance() {
    if ((bool)(this->globaltransport_tempoNeedsReset)) {
        fillSignal(this->globaltransport_tempo, this->vs, this->globaltransport_lastTempo);
        this->globaltransport_tempoNeedsReset = false;

        if ((bool)(this->globaltransport_notify)) {
            this->getEngine()->sendTempoEvent(this->globaltransport_lastTempo);
        }
    }

    if ((bool)(this->globaltransport_stateNeedsReset)) {
        fillSignal(this->globaltransport_state, this->vs, this->globaltransport_lastState);
        this->globaltransport_stateNeedsReset = false;

        if ((bool)(this->globaltransport_notify)) {
            this->getEngine()->sendTransportEvent(TransportState(this->globaltransport_lastState));
        }
    }

    if (this->globaltransport_beatTimeChanges->length > 2) {
        this->globaltransport_beatTimeChanges[0] = this->globaltransport_beatTimeChanges[(Index)(this->globaltransport_beatTimeChanges->length - 2)];
        this->globaltransport_beatTimeChanges[1] = this->globaltransport_beatTimeChanges[(Index)(this->globaltransport_beatTimeChanges->length - 1)];
        this->globaltransport_beatTimeChanges->length = 2;

        if ((bool)(this->globaltransport_notify)) {
            this->getEngine()->sendBeatTimeEvent(this->globaltransport_beatTimeChanges[0]);
        }
    }

    if (this->globaltransport_timeSignatureChanges->length > 3) {
        this->globaltransport_timeSignatureChanges[0] = this->globaltransport_timeSignatureChanges[(Index)(this->globaltransport_timeSignatureChanges->length - 3)];
        this->globaltransport_timeSignatureChanges[1] = this->globaltransport_timeSignatureChanges[(Index)(this->globaltransport_timeSignatureChanges->length - 2)];
        this->globaltransport_timeSignatureChanges[2] = this->globaltransport_timeSignatureChanges[(Index)(this->globaltransport_timeSignatureChanges->length - 1)];
        this->globaltransport_timeSignatureChanges->length = 3;

        if ((bool)(this->globaltransport_notify)) {
            this->getEngine()->sendTimeSignatureEvent(
                (Int)(this->globaltransport_timeSignatureChanges[0]),
                (Int)(this->globaltransport_timeSignatureChanges[1])
            );
        }
    }

    this->globaltransport_notify = false;
}

void globaltransport_dspsetup(bool force) {
    if ((bool)(this->globaltransport_setupDone) && (bool)(!(bool)(force)))
        return;

    fillSignal(this->globaltransport_tempo, this->vs, this->globaltransport_lastTempo);
    this->globaltransport_tempoNeedsReset = false;
    fillSignal(this->globaltransport_state, this->vs, this->globaltransport_lastState);
    this->globaltransport_stateNeedsReset = false;
    this->globaltransport_setupDone = true;
}

bool stackprotect_check() {
    this->stackprotect_count++;

    if (this->stackprotect_count > 128) {
        console->log("STACK OVERFLOW DETECTED - stopped processing branch !");
        return true;
    }

    return false;
}

Index getPatcherSerial() const {
    return 0;
}

void sendParameter(ParameterIndex index, bool ignoreValue) {
    this->getEngine()->notifyParameterValueChanged(index, (ignoreValue ? 0 : this->getParameterValue(index)), ignoreValue);
}

void scheduleParamInit(ParameterIndex index, Index order) {
    this->paramInitIndices->push(index);
    this->paramInitOrder->push(order);
}

void processParamInitEvents() {
    this->listquicksort(
        this->paramInitOrder,
        this->paramInitIndices,
        0,
        (int)(this->paramInitOrder->length - 1),
        true
    );

    for (Index i = 0; i < this->paramInitOrder->length; i++) {
        this->getEngine()->scheduleParameterBang(this->paramInitIndices[i], 0);
    }
}

void updateTime(MillisecondTime time, EXTERNALENGINE* engine, bool inProcess = false) {
    RNBO_UNUSED(inProcess);
    RNBO_UNUSED(engine);
    this->_currentTime = time;
    auto offset = rnbo_fround(this->msToSamps(time - this->getEngine()->getCurrentTime(), this->sr));

    if (offset >= (SampleIndex)(this->vs))
        offset = (SampleIndex)(this->vs) - 1;

    if (offset < 0)
        offset = 0;

    this->sampleOffsetIntoNextAudioBuffer = (Index)(offset);
}

void assign_defaults()
{
    p_01_target = 0;
    numberobj_17_value = 0;
    numberobj_17_value_setter(numberobj_17_value);
    param_01_value = 0;
    p_02_target = 0;
    dspexpr_40_in1 = 0;
    dspexpr_40_in2 = 0;
    phasor_01_freq = 5;
    numberobj_18_value = 0;
    numberobj_18_value_setter(numberobj_18_value);
    param_02_value = 0;
    p_03_target = 0;
    dspexpr_41_in1 = 0;
    dspexpr_41_in2 = 0;
    dspexpr_42_in1 = 0;
    dspexpr_42_in2 = 0;
    p_04_target = 0;
    toggle_07_value_number = 0;
    toggle_07_value_number_setter(toggle_07_value_number);
    param_03_value = 0;
    numberobj_19_value = 0;
    numberobj_19_value_setter(numberobj_19_value);
    linetilde_05_time = 1000;
    linetilde_05_keepramp = true;
    hann_tilde_03_x = 0;
    dspexpr_43_in1 = 0;
    dspexpr_43_in2 = 0;
    p_05_target = 0;
    toggle_08_value_number = 0;
    toggle_08_value_number_setter(toggle_08_value_number);
    dspexpr_44_in1 = 0;
    dspexpr_44_in2 = 0;
    recordtilde_01_append = 0;
    recordtilde_01_record = 0;
    recordtilde_01_begin = 0;
    recordtilde_01_end = -1;
    recordtilde_01_loop = 0;
    numberobj_20_value = 0;
    numberobj_20_value_setter(numberobj_20_value);
    param_04_value = 0;
    numberobj_21_value = 0;
    numberobj_21_value_setter(numberobj_21_value);
    param_05_value = 0;
    dspexpr_45_in1 = 0;
    dspexpr_45_in2 = 0;
    numberobj_22_value = 0;
    numberobj_22_value_setter(numberobj_22_value);
    linetilde_06_time = 1000;
    linetilde_06_keepramp = true;
    hann_tilde_04_x = 0;
    numberobj_23_value = 0;
    numberobj_23_value_setter(numberobj_23_value);
    param_06_value = 0;
    scale_07_inlow = 0;
    scale_07_inhigh = 100;
    scale_07_outlow = 250;
    scale_07_outhigh = 5000;
    scale_07_power = 1.3313;
    numberobj_24_value = 0;
    numberobj_24_value_setter(numberobj_24_value);
    param_07_value = 0;
    numberobj_25_value = 0;
    numberobj_25_value_setter(numberobj_25_value);
    data_01_sizeout = 0;
    data_01_size = 0;
    data_01_sizems = 200;
    data_01_normalize = 0.995;
    data_01_channels = 2;
    data_02_sizeout = 0;
    data_02_size = 0;
    data_02_sizems = 200;
    data_02_normalize = 0.995;
    data_02_channels = 2;
    toggle_09_value_number = 0;
    toggle_09_value_number_setter(toggle_09_value_number);
    recordtilde_02_append = 0;
    recordtilde_02_record = 0;
    recordtilde_02_begin = 0;
    recordtilde_02_end = -1;
    recordtilde_02_loop = 0;
    p_06_target = 0;
    send_01_input_number = 0;
    scale_08_inlow = 0;
    scale_08_inhigh = 100;
    scale_08_outlow = 0;
    scale_08_outhigh = 0.025;
    scale_08_power = 1.75;
    numberobj_26_value = 0;
    numberobj_26_value_setter(numberobj_26_value);
    param_08_value = 0;
    numberobj_27_value = 0;
    numberobj_27_value_setter(numberobj_27_value);
    toggle_10_value_number = 0;
    toggle_10_value_number_setter(toggle_10_value_number);
    param_09_value = 0;
    ctlin_01_input = 0;
    ctlin_01_controller = 2;
    ctlin_01_channel = -1;
    expr_08_in1 = 0;
    expr_08_in2 = 0.007874015748;
    expr_08_out1 = 0;
    ctlin_02_input = 0;
    ctlin_02_controller = 3;
    ctlin_02_channel = -1;
    expr_09_in1 = 0;
    expr_09_in2 = 0.007874015748;
    expr_09_out1 = 0;
    ctlin_03_input = 0;
    ctlin_03_controller = 1;
    ctlin_03_channel = -1;
    expr_10_in1 = 0;
    expr_10_in2 = 0.007874015748;
    expr_10_out1 = 0;
    ctlin_04_input = 0;
    ctlin_04_controller = 9;
    ctlin_04_channel = -1;
    expr_11_in1 = 0;
    expr_11_in2 = 0.007874015748;
    expr_11_out1 = 0;
    ctlin_05_input = 0;
    ctlin_05_controller = 10;
    ctlin_05_channel = -1;
    expr_12_in1 = 0;
    expr_12_in2 = 0.007874015748;
    expr_12_out1 = 0;
    ctlin_06_input = 0;
    ctlin_06_controller = 12;
    ctlin_06_channel = -1;
    expr_13_in1 = 0;
    expr_13_in2 = 0.007874015748;
    expr_13_out1 = 0;
    ctlin_07_input = 0;
    ctlin_07_controller = 5;
    ctlin_07_channel = -1;
    expr_14_in1 = 0;
    expr_14_in2 = 0.007874015748;
    expr_14_out1 = 0;
    ctlin_08_input = 0;
    ctlin_08_controller = 6;
    ctlin_08_channel = -1;
    expr_15_in1 = 0;
    expr_15_in2 = 0.007874015748;
    expr_15_out1 = 0;
    ctlin_09_input = 0;
    ctlin_09_controller = 11;
    ctlin_09_channel = -1;
    expr_16_in1 = 0;
    expr_16_in2 = 0.007874015748;
    expr_16_out1 = 0;
    _currentTime = 0;
    audioProcessSampleCount = 0;
    sampleOffsetIntoNextAudioBuffer = 0;
    zeroBuffer = nullptr;
    dummyBuffer = nullptr;
    signals[0] = nullptr;
    signals[1] = nullptr;
    signals[2] = nullptr;
    signals[3] = nullptr;
    signals[4] = nullptr;
    signals[5] = nullptr;
    signals[6] = nullptr;
    signals[7] = nullptr;
    didAllocateSignals = 0;
    vs = 0;
    maxvs = 0;
    sr = 44100;
    invsr = 0.000022675736961451248;
    numberobj_17_currentFormat = 6;
    numberobj_17_lastValue = 0;
    param_01_lastValue = 0;
    phasor_01_sigbuf = nullptr;
    phasor_01_lastLockedPhase = 0;
    phasor_01_lastQuantum = 0;
    phasor_01_lastTempo = 0;
    phasor_01_nextJumpInSamples = 0;
    phasor_01_inc = 0;
    phasor_01_recalcInc = true;
    phasor_01_recalcPhase = true;
    phasor_01_ph_currentPhase = 0;
    phasor_01_ph_conv = 0;
    phasor_01_setupDone = false;
    metertilde_01_currentPeak = 0;
    metertilde_01_currentInterval = 0;
    metertilde_01_currentIntervalInSamples = 0;
    metertilde_01_setupDone = false;
    metertilde_02_currentPeak = 0;
    metertilde_02_currentInterval = 0;
    metertilde_02_currentIntervalInSamples = 0;
    metertilde_02_setupDone = false;
    numberobj_18_currentFormat = 6;
    numberobj_18_lastValue = 0;
    param_02_lastValue = 0;
    toggle_07_lastValue = 0;
    param_03_lastValue = 0;
    numberobj_19_currentFormat = 6;
    numberobj_19_lastValue = 0;
    linetilde_05_currentValue = 0;
    toggle_08_lastValue = 0;
    recordtilde_01_wIndex = 0;
    recordtilde_01_lastRecord = 0;
    numberobj_20_currentFormat = 6;
    numberobj_20_lastValue = 0;
    param_04_lastValue = 0;
    metertilde_03_currentPeak = 0;
    metertilde_03_currentInterval = 0;
    metertilde_03_currentIntervalInSamples = 0;
    metertilde_03_setupDone = false;
    numberobj_21_currentFormat = 6;
    numberobj_21_lastValue = 0;
    param_05_lastValue = 0;
    metertilde_04_currentPeak = 0;
    metertilde_04_currentInterval = 0;
    metertilde_04_currentIntervalInSamples = 0;
    metertilde_04_setupDone = false;
    numberobj_22_currentFormat = 6;
    numberobj_22_lastValue = 0;
    linetilde_06_currentValue = 0;
    numberobj_23_currentFormat = 6;
    numberobj_23_lastValue = 0;
    param_06_lastValue = 0;
    numberobj_24_currentFormat = 6;
    numberobj_24_lastValue = 0;
    param_07_lastValue = 0;
    numberobj_25_currentFormat = 6;
    numberobj_25_lastValue = 0;
    data_01_sizemode = 2;
    data_01_setupDone = false;
    data_02_sizemode = 2;
    data_02_setupDone = false;
    toggle_09_lastValue = 0;
    recordtilde_02_wIndex = 0;
    recordtilde_02_lastRecord = 0;
    numberobj_26_currentFormat = 6;
    numberobj_26_lastValue = 0;
    param_08_lastValue = 0;
    numberobj_27_currentFormat = 6;
    numberobj_27_lastValue = 0;
    toggle_10_lastValue = 0;
    param_09_lastValue = 0;
    ctlin_01_status = 0;
    ctlin_01_byte1 = -1;
    ctlin_01_inchan = 0;
    ctlin_02_status = 0;
    ctlin_02_byte1 = -1;
    ctlin_02_inchan = 0;
    ctlin_03_status = 0;
    ctlin_03_byte1 = -1;
    ctlin_03_inchan = 0;
    ctlin_04_status = 0;
    ctlin_04_byte1 = -1;
    ctlin_04_inchan = 0;
    ctlin_05_status = 0;
    ctlin_05_byte1 = -1;
    ctlin_05_inchan = 0;
    ctlin_06_status = 0;
    ctlin_06_byte1 = -1;
    ctlin_06_inchan = 0;
    ctlin_07_status = 0;
    ctlin_07_byte1 = -1;
    ctlin_07_inchan = 0;
    ctlin_08_status = 0;
    ctlin_08_byte1 = -1;
    ctlin_08_inchan = 0;
    ctlin_09_status = 0;
    ctlin_09_byte1 = -1;
    ctlin_09_inchan = 0;
    globaltransport_tempo = nullptr;
    globaltransport_tempoNeedsReset = false;
    globaltransport_lastTempo = 120;
    globaltransport_state = nullptr;
    globaltransport_stateNeedsReset = false;
    globaltransport_lastState = 0;
    globaltransport_beatTimeChanges = { 0, 0 };
    globaltransport_timeSignatureChanges = { 4, 4, 0 };
    globaltransport_notify = false;
    globaltransport_bbuBase = 0;
    globaltransport_setupDone = false;
    stackprotect_count = 0;
    _voiceIndex = 0;
    _noteNumber = 0;
    isMuted = 1;
}

    // data ref strings
    struct DataRefStrings {
    	static constexpr auto& name0 = "FreezeA";
    	static constexpr auto& file0 = "";
    	static constexpr auto& tag0 = "data";
    	static constexpr auto& name1 = "RNBODefaultSinus";
    	static constexpr auto& file1 = "";
    	static constexpr auto& tag1 = "buffer~";
    	static constexpr auto& name2 = "FreezeB";
    	static constexpr auto& file2 = "";
    	static constexpr auto& tag2 = "data";
    	DataRefStrings* operator->() { return this; }
    	const DataRefStrings* operator->() const { return this; }
    };

    DataRefStrings dataRefStrings;

// member variables

    list message_10_set;
    number p_01_target;
    number numberobj_17_value;
    number param_01_value;
    number p_02_target;
    number dspexpr_40_in1;
    number dspexpr_40_in2;
    number phasor_01_freq;
    number numberobj_18_value;
    number param_02_value;
    number p_03_target;
    number dspexpr_41_in1;
    number dspexpr_41_in2;
    number dspexpr_42_in1;
    number dspexpr_42_in2;
    number p_04_target;
    number toggle_07_value_number;
    number param_03_value;
    number numberobj_19_value;
    list linetilde_05_segments;
    number linetilde_05_time;
    number linetilde_05_keepramp;
    number hann_tilde_03_x;
    number dspexpr_43_in1;
    number dspexpr_43_in2;
    number p_05_target;
    number toggle_08_value_number;
    number dspexpr_44_in1;
    number dspexpr_44_in2;
    number recordtilde_01_append;
    number recordtilde_01_record;
    number recordtilde_01_begin;
    number recordtilde_01_end;
    number recordtilde_01_loop;
    number numberobj_20_value;
    number param_04_value;
    number numberobj_21_value;
    number param_05_value;
    number dspexpr_45_in1;
    number dspexpr_45_in2;
    number numberobj_22_value;
    list linetilde_06_segments;
    number linetilde_06_time;
    number linetilde_06_keepramp;
    number hann_tilde_04_x;
    number numberobj_23_value;
    number param_06_value;
    list scale_07_input;
    number scale_07_inlow;
    number scale_07_inhigh;
    number scale_07_outlow;
    number scale_07_outhigh;
    number scale_07_power;
    list scale_07_out;
    number numberobj_24_value;
    number param_07_value;
    number numberobj_25_value;
    number data_01_sizeout;
    number data_01_size;
    number data_01_sizems;
    number data_01_normalize;
    number data_01_channels;
    number data_02_sizeout;
    number data_02_size;
    number data_02_sizems;
    number data_02_normalize;
    number data_02_channels;
    number toggle_09_value_number;
    number recordtilde_02_append;
    number recordtilde_02_record;
    number recordtilde_02_begin;
    number recordtilde_02_end;
    number recordtilde_02_loop;
    number p_06_target;
    number send_01_input_number;
    list send_01_input_list;
    list scale_08_input;
    number scale_08_inlow;
    number scale_08_inhigh;
    number scale_08_outlow;
    number scale_08_outhigh;
    number scale_08_power;
    list scale_08_out;
    number numberobj_26_value;
    number param_08_value;
    number numberobj_27_value;
    number toggle_10_value_number;
    number param_09_value;
    number ctlin_01_input;
    number ctlin_01_controller;
    number ctlin_01_channel;
    number expr_08_in1;
    number expr_08_in2;
    number expr_08_out1;
    number ctlin_02_input;
    number ctlin_02_controller;
    number ctlin_02_channel;
    number expr_09_in1;
    number expr_09_in2;
    number expr_09_out1;
    number ctlin_03_input;
    number ctlin_03_controller;
    number ctlin_03_channel;
    number expr_10_in1;
    number expr_10_in2;
    number expr_10_out1;
    number ctlin_04_input;
    number ctlin_04_controller;
    number ctlin_04_channel;
    number expr_11_in1;
    number expr_11_in2;
    number expr_11_out1;
    number ctlin_05_input;
    number ctlin_05_controller;
    number ctlin_05_channel;
    number expr_12_in1;
    number expr_12_in2;
    number expr_12_out1;
    number ctlin_06_input;
    number ctlin_06_controller;
    number ctlin_06_channel;
    number expr_13_in1;
    number expr_13_in2;
    number expr_13_out1;
    number ctlin_07_input;
    number ctlin_07_controller;
    number ctlin_07_channel;
    number expr_14_in1;
    number expr_14_in2;
    number expr_14_out1;
    number ctlin_08_input;
    number ctlin_08_controller;
    number ctlin_08_channel;
    number expr_15_in1;
    number expr_15_in2;
    number expr_15_out1;
    number ctlin_09_input;
    number ctlin_09_controller;
    number ctlin_09_channel;
    number expr_16_in1;
    number expr_16_in2;
    number expr_16_out1;
    MillisecondTime _currentTime;
    ENGINE _internalEngine;
    UInt64 audioProcessSampleCount;
    Index sampleOffsetIntoNextAudioBuffer;
    signal zeroBuffer;
    signal dummyBuffer;
    SampleValue * signals[8];
    bool didAllocateSignals;
    Index vs;
    Index maxvs;
    number sr;
    number invsr;
    Int numberobj_17_currentFormat;
    number numberobj_17_lastValue;
    number param_01_lastValue;
    signal phasor_01_sigbuf;
    number phasor_01_lastLockedPhase;
    number phasor_01_lastQuantum;
    number phasor_01_lastTempo;
    number phasor_01_nextJumpInSamples;
    number phasor_01_inc;
    bool phasor_01_recalcInc;
    bool phasor_01_recalcPhase;
    number phasor_01_ph_currentPhase;
    number phasor_01_ph_conv;
    bool phasor_01_setupDone;
    number metertilde_01_currentPeak;
    SampleIndex metertilde_01_currentInterval;
    SampleIndex metertilde_01_currentIntervalInSamples;
    bool metertilde_01_setupDone;
    number metertilde_02_currentPeak;
    SampleIndex metertilde_02_currentInterval;
    SampleIndex metertilde_02_currentIntervalInSamples;
    bool metertilde_02_setupDone;
    Int numberobj_18_currentFormat;
    number numberobj_18_lastValue;
    number param_02_lastValue;
    number toggle_07_lastValue;
    number param_03_lastValue;
    Int numberobj_19_currentFormat;
    number numberobj_19_lastValue;
    list linetilde_05_activeRamps;
    number linetilde_05_currentValue;
    number toggle_08_lastValue;
    Float64BufferRef recordtilde_01_buffer;
    SampleIndex recordtilde_01_wIndex;
    number recordtilde_01_lastRecord;
    Int numberobj_20_currentFormat;
    number numberobj_20_lastValue;
    number param_04_lastValue;
    number metertilde_03_currentPeak;
    SampleIndex metertilde_03_currentInterval;
    SampleIndex metertilde_03_currentIntervalInSamples;
    bool metertilde_03_setupDone;
    Int numberobj_21_currentFormat;
    number numberobj_21_lastValue;
    number param_05_lastValue;
    number metertilde_04_currentPeak;
    SampleIndex metertilde_04_currentInterval;
    SampleIndex metertilde_04_currentIntervalInSamples;
    bool metertilde_04_setupDone;
    Int numberobj_22_currentFormat;
    number numberobj_22_lastValue;
    list linetilde_06_activeRamps;
    number linetilde_06_currentValue;
    Int numberobj_23_currentFormat;
    number numberobj_23_lastValue;
    number param_06_lastValue;
    Int numberobj_24_currentFormat;
    number numberobj_24_lastValue;
    number param_07_lastValue;
    Int numberobj_25_currentFormat;
    number numberobj_25_lastValue;
    Float64BufferRef data_01_buffer;
    Int data_01_sizemode;
    bool data_01_setupDone;
    Float64BufferRef data_02_buffer;
    Int data_02_sizemode;
    bool data_02_setupDone;
    number toggle_09_lastValue;
    Float64BufferRef recordtilde_02_buffer;
    SampleIndex recordtilde_02_wIndex;
    number recordtilde_02_lastRecord;
    Int numberobj_26_currentFormat;
    number numberobj_26_lastValue;
    number param_08_lastValue;
    Int numberobj_27_currentFormat;
    number numberobj_27_lastValue;
    number toggle_10_lastValue;
    number param_09_lastValue;
    Int ctlin_01_status;
    Int ctlin_01_byte1;
    Int ctlin_01_inchan;
    Int ctlin_02_status;
    Int ctlin_02_byte1;
    Int ctlin_02_inchan;
    Int ctlin_03_status;
    Int ctlin_03_byte1;
    Int ctlin_03_inchan;
    Int ctlin_04_status;
    Int ctlin_04_byte1;
    Int ctlin_04_inchan;
    Int ctlin_05_status;
    Int ctlin_05_byte1;
    Int ctlin_05_inchan;
    Int ctlin_06_status;
    Int ctlin_06_byte1;
    Int ctlin_06_inchan;
    Int ctlin_07_status;
    Int ctlin_07_byte1;
    Int ctlin_07_inchan;
    Int ctlin_08_status;
    Int ctlin_08_byte1;
    Int ctlin_08_inchan;
    Int ctlin_09_status;
    Int ctlin_09_byte1;
    Int ctlin_09_inchan;
    signal globaltransport_tempo;
    bool globaltransport_tempoNeedsReset;
    number globaltransport_lastTempo;
    signal globaltransport_state;
    bool globaltransport_stateNeedsReset;
    number globaltransport_lastState;
    list globaltransport_beatTimeChanges;
    list globaltransport_timeSignatureChanges;
    bool globaltransport_notify;
    number globaltransport_bbuBase;
    bool globaltransport_setupDone;
    number stackprotect_count;
    DataRef FreezeA;
    DataRef RNBODefaultSinus;
    DataRef FreezeB;
    Index _voiceIndex;
    Int _noteNumber;
    Index isMuted;
    indexlist paramInitIndices;
    indexlist paramInitOrder;
    RNBOSubpatcher_244 p_01;
    RNBOSubpatcher_245 p_02[8];
    RNBOSubpatcher_246 p_03;
    RNBOSubpatcher_247 p_04;
    RNBOSubpatcher_248 p_05[8];
    RNBOSubpatcher_249 p_06;
    bool _isInitialized = false;
};

static PatcherInterface* creaternbomatic()
{
    return new rnbomatic<EXTERNALENGINE>();
}

#ifndef RNBO_NO_PATCHERFACTORY
extern "C" PatcherFactoryFunctionPtr GetPatcherFactoryFunction()
#else
extern "C" PatcherFactoryFunctionPtr rnbomaticFactoryFunction()
#endif
{
    return creaternbomatic;
}

#ifndef RNBO_NO_PATCHERFACTORY
extern "C" void SetLogger(Logger* logger)
#else
void rnbomaticSetLogger(Logger* logger)
#endif
{
    console = logger;
}

} // end RNBO namespace

