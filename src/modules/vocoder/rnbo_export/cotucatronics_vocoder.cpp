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
    this->signaladder_01_perform(in1, in1, this->signals[1], n);
    this->dspexpr_09_perform(this->signals[1], this->dspexpr_09_in2, this->signals[0], n);

    this->fftstream_tilde_01_perform(
        in1,
        this->fftstream_tilde_01_imagIn,
        this->signals[2],
        this->signals[3],
        this->signals[4],
        n
    );

    this->dspexpr_03_perform(this->signals[2], this->signals[2], this->signals[5], n);
    this->dspexpr_04_perform(this->signals[3], this->signals[3], this->signals[2], n);
    this->signaladder_02_perform(this->signals[2], this->signals[5], this->signals[5], n);
    this->dspexpr_02_perform(this->signals[5], this->signals[3], n);

    this->poke_tilde_01_perform(
        this->signals[4],
        this->poke_tilde_01_channel,
        this->poke_tilde_01_overdub,
        this->signals[3],
        n
    );

    this->fftstream_tilde_04_perform(
        in1,
        this->fftstream_tilde_04_imagIn,
        this->signals[3],
        this->signals[2],
        this->signals[6],
        n
    );

    this->dspexpr_14_perform(this->signals[3], this->signals[3], this->signals[7], n);
    this->dspexpr_15_perform(this->signals[2], this->signals[2], this->signals[3], n);
    this->signaladder_03_perform(this->signals[7], this->signals[3], this->signals[3], n);
    this->dspexpr_13_perform(this->signals[3], this->signals[2], n);

    this->poke_tilde_02_perform(
        this->signals[6],
        this->poke_tilde_02_channel,
        this->poke_tilde_02_overdub,
        this->signals[2],
        n
    );

    this->fftstream_tilde_02_perform(
        in2,
        this->fftstream_tilde_02_imagIn,
        this->signals[2],
        this->signals[7],
        this->dummyBuffer,
        n
    );

    this->fftstream_tilde_03_perform(
        in2,
        this->fftstream_tilde_03_imagIn,
        this->signals[8],
        this->signals[9],
        this->dummyBuffer,
        n
    );

    this->ip_01_perform(this->signals[10], n);
    this->dspexpr_10_perform(this->signals[6], this->signals[10], this->signals[11], n);

    this->peek_tilde_02_perform(
        this->signals[11],
        this->peek_tilde_02_offset,
        this->signals[6],
        this->dummyBuffer,
        n
    );

    this->dspexpr_12_perform(this->signals[6], this->signals[9], this->signals[11], n);
    this->dspexpr_11_perform(this->signals[6], this->signals[8], this->signals[9], n);

    this->ifftstream_tilde_02_perform(
        this->signals[9],
        this->signals[11],
        this->signals[8],
        this->dummyBuffer,
        this->dummyBuffer,
        n
    );

    this->dspexpr_08_perform(this->signals[4], this->signals[10], this->signals[11], n);

    this->peek_tilde_01_perform(
        this->signals[11],
        this->peek_tilde_01_offset,
        this->signals[10],
        this->dummyBuffer,
        n
    );

    this->dspexpr_05_perform(this->signals[10], this->signals[7], this->signals[11], n);
    this->dspexpr_01_perform(this->signals[10], this->signals[2], this->signals[7], n);

    this->ifftstream_tilde_01_perform(
        this->signals[7],
        this->signals[11],
        this->signals[2],
        this->dummyBuffer,
        this->dummyBuffer,
        n
    );

    this->signaladder_04_perform(this->signals[8], this->signals[2], this->signals[2], n);
    this->dspexpr_07_perform(this->signals[2], this->dspexpr_07_in2, this->signals[11], n);
    this->dspexpr_06_perform(this->signals[11], this->signals[0], this->signals[8], n);
    this->signalforwarder_01_perform(this->signals[8], out1, n);
    this->signalforwarder_02_perform(this->signals[8], out2, n);
    this->stackprotect_perform(n);
    this->globaltransport_advance();
    this->advanceTime((ENGINE*)nullptr);
    this->audioProcessSampleCount += this->vs;
}

void prepareToProcess(number sampleRate, Index maxBlockSize, bool force) {
    RNBO_ASSERT(this->_isInitialized);

    if (this->maxvs < maxBlockSize || !this->didAllocateSignals) {
        Index i;

        for (i = 0; i < 12; i++) {
            this->signals[i] = resizeSignal(this->signals[i], this->maxvs, maxBlockSize);
        }

        this->ip_01_sigbuf = resizeSignal(this->ip_01_sigbuf, this->maxvs, maxBlockSize);
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

    this->data_01_dspsetup(forceDSPSetup);
    this->fftstream_tilde_01_dspsetup(forceDSPSetup);
    this->fftstream_tilde_04_dspsetup(forceDSPSetup);
    this->fftstream_tilde_02_dspsetup(forceDSPSetup);
    this->fftstream_tilde_03_dspsetup(forceDSPSetup);
    this->data_02_dspsetup(forceDSPSetup);
    this->ip_01_dspsetup(forceDSPSetup);
    this->ifftstream_tilde_02_dspsetup(forceDSPSetup);
    this->ifftstream_tilde_01_dspsetup(forceDSPSetup);
    this->data_03_dspsetup(forceDSPSetup);
    this->globaltransport_dspsetup(forceDSPSetup);

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
        return addressOf(this->frzmag1);
        break;
        }
    case 1:
        {
        return addressOf(this->r_window);
        break;
        }
    case 2:
        {
        return addressOf(this->frzmag2);
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
        this->poke_tilde_01_buffer = reInitDataView(this->poke_tilde_01_buffer, this->frzmag1);
        this->peek_tilde_01_buffer = reInitDataView(this->peek_tilde_01_buffer, this->frzmag1);
        this->data_02_buffer = reInitDataView(this->data_02_buffer, this->frzmag1);
        this->data_02_afterBufferUpdate();
    }

    if (index == 1) {
        this->data_01_buffer = reInitDataView(this->data_01_buffer, this->r_window);
        this->ifftstream_tilde_01_win_buf = reInitDataView(this->ifftstream_tilde_01_win_buf, this->r_window);
        this->fftstream_tilde_01_win_buf = reInitDataView(this->fftstream_tilde_01_win_buf, this->r_window);
        this->fftstream_tilde_02_win_buf = reInitDataView(this->fftstream_tilde_02_win_buf, this->r_window);
        this->ifftstream_tilde_02_win_buf = reInitDataView(this->ifftstream_tilde_02_win_buf, this->r_window);
        this->fftstream_tilde_03_win_buf = reInitDataView(this->fftstream_tilde_03_win_buf, this->r_window);
        this->fftstream_tilde_04_win_buf = reInitDataView(this->fftstream_tilde_04_win_buf, this->r_window);
        this->data_01_afterBufferUpdate();
    }

    if (index == 2) {
        this->peek_tilde_02_buffer = reInitDataView(this->peek_tilde_02_buffer, this->frzmag2);
        this->data_03_buffer = reInitDataView(this->data_03_buffer, this->frzmag2);
        this->poke_tilde_02_buffer = reInitDataView(this->poke_tilde_02_buffer, this->frzmag2);
        this->data_03_afterBufferUpdate();
    }
}

void initialize() {
    RNBO_ASSERT(!this->_isInitialized);

    this->frzmag1 = initDataRef(
        this->frzmag1,
        this->dataRefStrings->name0,
        true,
        this->dataRefStrings->file0,
        this->dataRefStrings->tag0
    );

    this->r_window = initDataRef(
        this->r_window,
        this->dataRefStrings->name1,
        false,
        this->dataRefStrings->file1,
        this->dataRefStrings->tag1
    );

    this->frzmag2 = initDataRef(
        this->frzmag2,
        this->dataRefStrings->name2,
        true,
        this->dataRefStrings->file2,
        this->dataRefStrings->tag2
    );

    this->assign_defaults();
    this->applyState();
    this->frzmag1->setIndex(0);
    this->poke_tilde_01_buffer = new Float64Buffer(this->frzmag1);
    this->peek_tilde_01_buffer = new Float64Buffer(this->frzmag1);
    this->data_02_buffer = new Float64Buffer(this->frzmag1);
    this->r_window->setIndex(1);
    this->data_01_buffer = new Float32Buffer(this->r_window);
    this->ifftstream_tilde_01_win_buf = new Float32Buffer(this->r_window);
    this->fftstream_tilde_01_win_buf = new Float32Buffer(this->r_window);
    this->fftstream_tilde_02_win_buf = new Float32Buffer(this->r_window);
    this->ifftstream_tilde_02_win_buf = new Float32Buffer(this->r_window);
    this->fftstream_tilde_03_win_buf = new Float32Buffer(this->r_window);
    this->fftstream_tilde_04_win_buf = new Float32Buffer(this->r_window);
    this->frzmag2->setIndex(2);
    this->peek_tilde_02_buffer = new Float64Buffer(this->frzmag2);
    this->data_03_buffer = new Float64Buffer(this->frzmag2);
    this->poke_tilde_02_buffer = new Float64Buffer(this->frzmag2);
    this->initializeObjects();
    this->allocateDataRefs();
    this->startup();
    this->_isInitialized = true;
}

void getPreset(PatcherStateInterface& preset) {
    this->updateTime(this->getEngine()->getCurrentTime(), (ENGINE*)nullptr);
    preset["__presetid"] = "rnbo";
    this->param_01_getPresetValue(getSubState(preset, "voc_mix"));
    this->param_02_getPresetValue(getSubState(preset, "voc_pitch"));
}

void setPreset(MillisecondTime time, PatcherStateInterface& preset) {
    this->updateTime(time, (ENGINE*)nullptr);
    this->param_01_setPresetValue(getSubState(preset, "voc_mix"));
    this->param_02_setPresetValue(getSubState(preset, "voc_pitch"));
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
    return 2;
}

ConstCharPointer getParameterName(ParameterIndex index) const {
    switch (index) {
    case 0:
        {
        return "voc_mix";
        }
    case 1:
        {
        return "voc_pitch";
        }
    default:
        {
        return "bogus";
        }
    }
}

ConstCharPointer getParameterId(ParameterIndex index) const {
    switch (index) {
    case 0:
        {
        return "voc_mix";
        }
    case 1:
        {
        return "voc_pitch";
        }
    default:
        {
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
            info->initialValue = 1;
            info->min = 0;
            info->max = 1;
            info->exponent = 1;
            info->steps = 0;
            info->debug = false;
            info->saveable = true;
            info->transmittable = true;
            info->initialized = true;
            info->visible = true;
            info->displayName = "";
            info->unit = "";
            info->ioType = IOTypeUndefined;
            info->signalIndex = INVALID_INDEX;
            break;
            }
        case 1:
            {
            info->type = ParameterTypeNumber;
            info->initialValue = 0.5;
            info->min = 0;
            info->max = 1;
            info->exponent = 1;
            info->steps = 0;
            info->debug = false;
            info->saveable = true;
            info->transmittable = true;
            info->initialized = true;
            info->visible = true;
            info->displayName = "";
            info->unit = "";
            info->ioType = IOTypeUndefined;
            info->signalIndex = INVALID_INDEX;
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
    case 0:
    case 1:
        {
        {
            value = (value < 0 ? 0 : (value > 1 ? 1 : value));
            ParameterValue normalizedValue = (value - 0) / (1 - 0);
            return normalizedValue;
        }
        }
    default:
        {
        return value;
        }
    }
}

ParameterValue convertFromNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
    value = (value < 0 ? 0 : (value > 1 ? 1 : value));

    switch (index) {
    case 0:
    case 1:
        {
        {
            {
                return 0 + value * (1 - 0);
            }
        }
        }
    default:
        {
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
    default:
        {
        return value;
        }
    }
}

void processNumMessage(MessageTag , MessageTag , MillisecondTime , number ) {}

void processListMessage(MessageTag , MessageTag , MillisecondTime , const list& ) {}

void processBangMessage(MessageTag , MessageTag , MillisecondTime ) {}

MessageTagInfo resolveTag(MessageTag tag) const {
    switch (tag) {

    }

    return "";
}

MessageIndex getNumMessages() const {
    return 0;
}

const MessageInfo& getMessageInfo(MessageIndex index) const {
    switch (index) {

    }

    return NullMessageInfo;
}

protected:

		
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
}

number hann(number x) {
    x = (x > 1 ? 1 : (x < 0 ? 0 : x));
    return 0.5 * (1 - rnbo_cos(6.28318530717958647692 * x));
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

template<typename BUFFERTYPE> void poke_boundmode_ignore_channelmode_ignore_indexmode_samples_overdubmode_accum(
    BUFFERTYPE& buffer,
    SampleValue value,
    SampleValue sampleIndex,
    Int channel,
    number overdub
) {
    number bufferSize = buffer->getSize();
    const Index bufferChannels = (const Index)(buffer->getChannels());

    if (bufferSize > 0 && (5 != 5 || (sampleIndex >= 0 && sampleIndex < bufferSize)) && (5 != 5 || (channel >= 0 && channel < bufferChannels))) {
        if (overdub != 0) {
            number currentValue = buffer->getSample(channel, sampleIndex);

            {
                value = value + currentValue * overdub;
            }
        }

        buffer->setSample(channel, sampleIndex, value);
        buffer->setTouched(true);
    }
}

number mstosamps(MillisecondTime ms) {
    return ms * this->sr * 0.001;
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

template<typename BUFFERTYPE> array<SampleValue, 1 + 1> peek_boundmode_ignore_channelmode_ignore_indexmode_samples_interp_none(BUFFERTYPE& buffer, SampleValue sampleIndex, Index channelOffset) {
    number bufferSize = buffer->getSize();
    const Index bufferChannels = (const Index)(buffer->getChannels());
    constexpr int ___N2 = 1 + 1;
    array<SampleValue, ___N2> out = FIXEDSIZEARRAYINIT(1 + 1);

    if (bufferSize == 0 || (5 == 5 && (sampleIndex < 0 || sampleIndex >= bufferSize))) {
        return out;
    } else {
        for (Index channel = 0; channel < 1; channel++) {
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
                    bufferreadsample_result = __buffer->getSample(channelIndex, truncIndex);
                }
            }

            out[(Index)channel] = bufferreadsample_result;
        }

        out[1] = sampleIndex;
        return out;
    }
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

    this->expr_01_in1_set(v);
    this->dspexpr_07_in2_set(v);
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

    {
        listbase<number, 1> converted = {v};
        this->scale_01_input_set(converted);
    }
}

MillisecondTime getPatcherTime() const {
    return this->_currentTime;
}

void deallocateSignals() {
    Index i;

    for (i = 0; i < 12; i++) {
        this->signals[i] = freeSignal(this->signals[i]);
    }

    this->ip_01_sigbuf = freeSignal(this->ip_01_sigbuf);
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

void fillr_window(DataRef& ref) {
    Float32Buffer buffer(ref);
    number bufsize = buffer->getSize();

    for (number channel = 0; channel < buffer->getChannels(); channel++) {
        for (Int index = 0; index < bufsize; index++) {
            number x = index / bufsize;
            number value = rnbo_sqrt(this->hann(x));
            buffer->setSample(channel, index, value);
        }
    }
}

void fillDataRef(DataRefIndex index, DataRef& ref) {
    switch (index) {
    case 1:
        {
        this->fillr_window(ref);
        break;
        }
    }
}

void zeroDataRef(DataRef& ref) {
    ref->setZero();
}

void allocateDataRefs() {
    this->data_02_buffer->requestSize(2048, 1);
    this->data_02_buffer->setSampleRate(this->sr);
    this->data_01_buffer->requestSize(2048, 1);
    this->data_01_buffer->setSampleRate(this->sr);
    this->data_03_buffer->requestSize(2048, 1);
    this->data_03_buffer->setSampleRate(this->sr);
    this->poke_tilde_01_buffer = this->poke_tilde_01_buffer->allocateIfNeeded();
    this->peek_tilde_01_buffer = this->peek_tilde_01_buffer->allocateIfNeeded();
    this->data_02_buffer = this->data_02_buffer->allocateIfNeeded();

    if (this->frzmag1->hasRequestedSize()) {
        if (this->frzmag1->wantsFill())
            this->zeroDataRef(this->frzmag1);

        this->getEngine()->sendDataRefUpdated(0);
    }

    this->data_01_buffer = this->data_01_buffer->allocateIfNeeded();
    this->ifftstream_tilde_01_win_buf = this->ifftstream_tilde_01_win_buf->allocateIfNeeded();
    this->fftstream_tilde_01_win_buf = this->fftstream_tilde_01_win_buf->allocateIfNeeded();
    this->fftstream_tilde_02_win_buf = this->fftstream_tilde_02_win_buf->allocateIfNeeded();
    this->ifftstream_tilde_02_win_buf = this->ifftstream_tilde_02_win_buf->allocateIfNeeded();
    this->fftstream_tilde_03_win_buf = this->fftstream_tilde_03_win_buf->allocateIfNeeded();
    this->fftstream_tilde_04_win_buf = this->fftstream_tilde_04_win_buf->allocateIfNeeded();

    if (this->r_window->hasRequestedSize()) {
        if (this->r_window->wantsFill())
            this->fillr_window(this->r_window);

        this->getEngine()->sendDataRefUpdated(1);
    }

    this->peek_tilde_02_buffer = this->peek_tilde_02_buffer->allocateIfNeeded();
    this->data_03_buffer = this->data_03_buffer->allocateIfNeeded();
    this->poke_tilde_02_buffer = this->poke_tilde_02_buffer->allocateIfNeeded();

    if (this->frzmag2->hasRequestedSize()) {
        if (this->frzmag2->wantsFill())
            this->zeroDataRef(this->frzmag2);

        this->getEngine()->sendDataRefUpdated(2);
    }
}

void initializeObjects() {
    this->data_01_init();
    this->data_02_init();
    this->ip_01_init();
    this->data_03_init();
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

void processClockEvent(MillisecondTime , ClockId , bool , ParameterValue ) {}

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

    {
        this->scheduleParamInit(0, 0);
    }

    {
        this->scheduleParamInit(1, 0);
    }

    this->processParamInitEvents();
}

number param_01_value_constrain(number v) const {
    v = (v > 1 ? 1 : (v < 0 ? 0 : v));
    return v;
}

void dspexpr_09_in2_set(number v) {
    this->dspexpr_09_in2 = v;
}

void expr_01_out1_set(number v) {
    this->expr_01_out1 = v;
    this->dspexpr_09_in2_set(this->expr_01_out1);
}

void expr_01_in1_set(number in1) {
    this->expr_01_in1 = in1;
    this->expr_01_out1_set(this->expr_01_in2 - this->expr_01_in1);//#map:!-_obj-29:1
}

void dspexpr_07_in2_set(number v) {
    this->dspexpr_07_in2 = v;
}

number param_02_value_constrain(number v) const {
    v = (v > 1 ? 1 : (v < 0 ? 0 : v));
    return v;
}

void ip_01_value_set(number v) {
    this->ip_01_value = v;
    this->ip_01_fillSigBuf();
    this->ip_01_lastValue = v;
}

void ip_01_impulse_set(number v) {
    this->ip_01_impulse = v;
    this->ip_01_fillSigBuf();
    this->ip_01_lastValue = 0;
    SampleIndex lengthInSamples = (SampleIndex)(this->maximum(1, this->msToSamps(v, this->sr)));
    this->ip_01_resetCount = lengthInSamples;
}

template<typename LISTTYPE> void scale_01_out_set(const LISTTYPE& v) {
    this->scale_01_out = jsCreateListCopy(v);

    {
        if (v->length > 1)
            this->ip_01_impulse_set(v[1]);

        number converted = (v->length > 0 ? v[0] : 0);
        this->ip_01_value_set(converted);
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

void dspexpr_09_perform(const Sample * in1, number in2, SampleValue * out1, Index n) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        out1[(Index)i] = in1[(Index)i] * in2;//#map:_###_obj_###_:1
    }
}

void fftstream_tilde_01_perform(
    const Sample * realIn,
    number imagIn,
    SampleValue * out1,
    SampleValue * out2,
    SampleValue * out3,
    Index n
) {
    RNBO_UNUSED(imagIn);
    auto __fftstream_tilde_01_win_buf = this->fftstream_tilde_01_win_buf;
    auto __fftstream_tilde_01_datapos = this->fftstream_tilde_01_datapos;
    number invFftSize = (number)1 / (number)2048;
    Index i0;

    for (i0 = 0; i0 < (Index)n; i0++) {
        SampleIndex i = (SampleIndex)(__fftstream_tilde_01_datapos);
        array<number, 3> out = {0, 0, i};
        number windowFactor = 1;
        windowFactor = __fftstream_tilde_01_win_buf->getSample(0, i);

        if (__fftstream_tilde_01_datapos < 2048) {
            this->fftstream_tilde_01_inWorkspace[(Index)(2 * i)] = windowFactor * realIn[(Index)i0];
            out[0] = this->fftstream_tilde_01_outWorkspace[(Index)(2 * i)];
            this->fftstream_tilde_01_inWorkspace[(Index)(2 * i + 1)] = windowFactor * 0;
            out[1] = this->fftstream_tilde_01_outWorkspace[(Index)(2 * i + 1)];

            if (i == 2048 - 1) {
                this->fftstream_tilde_01_fft_next(this->fftstream_tilde_01_inWorkspace, 2048);

                for (SampleIndex j = 0; j < 2048 * 2; j++) {
                    this->fftstream_tilde_01_outWorkspace[(Index)j] = this->fftstream_tilde_01_inWorkspace[(Index)j];
                }
            }

            __fftstream_tilde_01_datapos = (__fftstream_tilde_01_datapos + 1) % 2048;
        }

        out1[(Index)i0] = out[0];
        out2[(Index)i0] = out[1];
        out3[(Index)i0] = out[2];
    }

    this->fftstream_tilde_01_datapos = __fftstream_tilde_01_datapos;
}

void dspexpr_03_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        out1[(Index)i] = in1[(Index)i] * in2[(Index)i];//#map:_###_obj_###_:1
    }
}

void dspexpr_04_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        out1[(Index)i] = in1[(Index)i] * in2[(Index)i];//#map:_###_obj_###_:1
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

void dspexpr_02_perform(const Sample * in1, SampleValue * out1, Index n) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        out1[(Index)i] = (in1[(Index)i] <= 0 ? 0 : rnbo_sqrt(in1[(Index)i]));//#map:_###_obj_###_:1
    }
}

void poke_tilde_01_perform(
    const Sample * index,
    number channel,
    number overdub,
    const SampleValue * value_signal,
    Index n
) {
    RNBO_UNUSED(overdub);
    RNBO_UNUSED(channel);
    auto __poke_tilde_01_buffer = this->poke_tilde_01_buffer;

    for (Index i = 0; i < n; i++) {
        this->poke_boundmode_ignore_channelmode_ignore_indexmode_samples_overdubmode_accum(__poke_tilde_01_buffer, value_signal[(Index)i], index[(Index)i], 0, 0);
    }
}

void fftstream_tilde_04_perform(
    const Sample * realIn,
    number imagIn,
    SampleValue * out1,
    SampleValue * out2,
    SampleValue * out3,
    Index n
) {
    RNBO_UNUSED(imagIn);
    auto __fftstream_tilde_04_win_buf = this->fftstream_tilde_04_win_buf;
    auto __fftstream_tilde_04_datapos = this->fftstream_tilde_04_datapos;
    number invFftSize = (number)1 / (number)2048;
    Index i0;

    for (i0 = 0; i0 < (Index)n; i0++) {
        SampleIndex i = (SampleIndex)(__fftstream_tilde_04_datapos);
        array<number, 3> out = {0, 0, i};
        number windowFactor = 1;
        windowFactor = __fftstream_tilde_04_win_buf->getSample(0, i);

        if (__fftstream_tilde_04_datapos < 2048) {
            this->fftstream_tilde_04_inWorkspace[(Index)(2 * i)] = windowFactor * realIn[(Index)i0];
            out[0] = this->fftstream_tilde_04_outWorkspace[(Index)(2 * i)];
            this->fftstream_tilde_04_inWorkspace[(Index)(2 * i + 1)] = windowFactor * 0;
            out[1] = this->fftstream_tilde_04_outWorkspace[(Index)(2 * i + 1)];

            if (i == 2048 - 1) {
                this->fftstream_tilde_04_fft_next(this->fftstream_tilde_04_inWorkspace, 2048);

                for (SampleIndex j = 0; j < 2048 * 2; j++) {
                    this->fftstream_tilde_04_outWorkspace[(Index)j] = this->fftstream_tilde_04_inWorkspace[(Index)j];
                }
            }

            __fftstream_tilde_04_datapos = (__fftstream_tilde_04_datapos + 1) % 2048;
        }

        out1[(Index)i0] = out[0];
        out2[(Index)i0] = out[1];
        out3[(Index)i0] = out[2];
    }

    this->fftstream_tilde_04_datapos = __fftstream_tilde_04_datapos;
}

void dspexpr_14_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        out1[(Index)i] = in1[(Index)i] * in2[(Index)i];//#map:_###_obj_###_:1
    }
}

void dspexpr_15_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        out1[(Index)i] = in1[(Index)i] * in2[(Index)i];//#map:_###_obj_###_:1
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

void dspexpr_13_perform(const Sample * in1, SampleValue * out1, Index n) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        out1[(Index)i] = (in1[(Index)i] <= 0 ? 0 : rnbo_sqrt(in1[(Index)i]));//#map:_###_obj_###_:1
    }
}

void poke_tilde_02_perform(
    const Sample * index,
    number channel,
    number overdub,
    const SampleValue * value_signal,
    Index n
) {
    RNBO_UNUSED(overdub);
    RNBO_UNUSED(channel);
    auto __poke_tilde_02_buffer = this->poke_tilde_02_buffer;

    for (Index i = 0; i < n; i++) {
        this->poke_boundmode_ignore_channelmode_ignore_indexmode_samples_overdubmode_accum(__poke_tilde_02_buffer, value_signal[(Index)i], index[(Index)i], 0, 0);
    }
}

void fftstream_tilde_02_perform(
    const Sample * realIn,
    number imagIn,
    SampleValue * out1,
    SampleValue * out2,
    SampleValue * out3,
    Index n
) {
    RNBO_UNUSED(imagIn);
    auto __fftstream_tilde_02_win_buf = this->fftstream_tilde_02_win_buf;
    auto __fftstream_tilde_02_datapos = this->fftstream_tilde_02_datapos;
    number invFftSize = (number)1 / (number)2048;
    Index i0;

    for (i0 = 0; i0 < (Index)n; i0++) {
        SampleIndex i = (SampleIndex)(__fftstream_tilde_02_datapos);
        array<number, 3> out = {0, 0, i};
        number windowFactor = 1;
        windowFactor = __fftstream_tilde_02_win_buf->getSample(0, i);

        if (__fftstream_tilde_02_datapos < 2048) {
            this->fftstream_tilde_02_inWorkspace[(Index)(2 * i)] = windowFactor * realIn[(Index)i0];
            out[0] = this->fftstream_tilde_02_outWorkspace[(Index)(2 * i)];
            this->fftstream_tilde_02_inWorkspace[(Index)(2 * i + 1)] = windowFactor * 0;
            out[1] = this->fftstream_tilde_02_outWorkspace[(Index)(2 * i + 1)];

            if (i == 2048 - 1) {
                this->fftstream_tilde_02_fft_next(this->fftstream_tilde_02_inWorkspace, 2048);

                for (SampleIndex j = 0; j < 2048 * 2; j++) {
                    this->fftstream_tilde_02_outWorkspace[(Index)j] = this->fftstream_tilde_02_inWorkspace[(Index)j];
                }
            }

            __fftstream_tilde_02_datapos = (__fftstream_tilde_02_datapos + 1) % 2048;
        }

        out1[(Index)i0] = out[0];
        out2[(Index)i0] = out[1];
        out3[(Index)i0] = out[2];
    }

    this->fftstream_tilde_02_datapos = __fftstream_tilde_02_datapos;
}

void fftstream_tilde_03_perform(
    const Sample * realIn,
    number imagIn,
    SampleValue * out1,
    SampleValue * out2,
    SampleValue * out3,
    Index n
) {
    RNBO_UNUSED(imagIn);
    auto __fftstream_tilde_03_win_buf = this->fftstream_tilde_03_win_buf;
    auto __fftstream_tilde_03_datapos = this->fftstream_tilde_03_datapos;
    number invFftSize = (number)1 / (number)2048;
    Index i0;

    for (i0 = 0; i0 < (Index)n; i0++) {
        SampleIndex i = (SampleIndex)(__fftstream_tilde_03_datapos);
        array<number, 3> out = {0, 0, i};
        number windowFactor = 1;
        windowFactor = __fftstream_tilde_03_win_buf->getSample(0, i);

        if (__fftstream_tilde_03_datapos < 2048) {
            this->fftstream_tilde_03_inWorkspace[(Index)(2 * i)] = windowFactor * realIn[(Index)i0];
            out[0] = this->fftstream_tilde_03_outWorkspace[(Index)(2 * i)];
            this->fftstream_tilde_03_inWorkspace[(Index)(2 * i + 1)] = windowFactor * 0;
            out[1] = this->fftstream_tilde_03_outWorkspace[(Index)(2 * i + 1)];

            if (i == 2048 - 1) {
                this->fftstream_tilde_03_fft_next(this->fftstream_tilde_03_inWorkspace, 2048);

                for (SampleIndex j = 0; j < 2048 * 2; j++) {
                    this->fftstream_tilde_03_outWorkspace[(Index)j] = this->fftstream_tilde_03_inWorkspace[(Index)j];
                }
            }

            __fftstream_tilde_03_datapos = (__fftstream_tilde_03_datapos + 1) % 2048;
        }

        out1[(Index)i0] = out[0];
        out2[(Index)i0] = out[1];
        out3[(Index)i0] = out[2];
    }

    this->fftstream_tilde_03_datapos = __fftstream_tilde_03_datapos;
}

void ip_01_perform(SampleValue * out, Index n) {
    auto __ip_01_lastValue = this->ip_01_lastValue;
    auto __ip_01_lastIndex = this->ip_01_lastIndex;

    for (Index i = 0; i < n; i++) {
        out[(Index)i] = ((SampleIndex)(i) >= __ip_01_lastIndex ? __ip_01_lastValue : this->ip_01_sigbuf[(Index)i]);
    }

    __ip_01_lastIndex = 0;
    this->ip_01_lastIndex = __ip_01_lastIndex;
}

void dspexpr_10_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        out1[(Index)i] = in1[(Index)i] + in2[(Index)i];//#map:_###_obj_###_:1
    }
}

void peek_tilde_02_perform(
    const Sample * index,
    number offset,
    SampleValue * out1,
    SampleValue * outindex,
    Index n
) {
    RNBO_UNUSED(out1);
    RNBO_UNUSED(offset);
    auto __peek_tilde_02_buffer = this->peek_tilde_02_buffer;
    SampleArray<1> out = {out1};

    for (Index i = 0; i < n; i++) {
        auto result = this->peek_boundmode_ignore_channelmode_ignore_indexmode_samples_interp_none(__peek_tilde_02_buffer, index[(Index)i], 0);

        for (Index j = 0; j < 1; j++) {
            out[(Index)j][(Index)i] = result[(Index)j];
        }

        outindex[(Index)i] = result[1];
    }
}

void dspexpr_12_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
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

void ifftstream_tilde_02_perform(
    const Sample * realIn,
    const Sample * imagIn,
    SampleValue * out1,
    SampleValue * out2,
    SampleValue * out3,
    Index n
) {
    auto __ifftstream_tilde_02_win_buf = this->ifftstream_tilde_02_win_buf;
    auto __ifftstream_tilde_02_datapos = this->ifftstream_tilde_02_datapos;
    Index i0;

    for (i0 = 0; i0 < (Index)n; i0++) {
        number invFftSize = (number)1 / (number)2048;
        SampleIndex i = (SampleIndex)(__ifftstream_tilde_02_datapos);
        list out = list(0, 0, i);
        number windowFactor = 1;
        windowFactor = __ifftstream_tilde_02_win_buf->getSample(0, i);

        if (__ifftstream_tilde_02_datapos < 2048) {
            this->ifftstream_tilde_02_inWorkspace[(Index)(2 * i + 1)] = realIn[(Index)i0];
            out[0] = windowFactor * this->ifftstream_tilde_02_outWorkspace[(Index)(2 * i + 1)];
            this->ifftstream_tilde_02_inWorkspace[(Index)(2 * i)] = imagIn[(Index)i0];
            out[1] = windowFactor * this->ifftstream_tilde_02_outWorkspace[(Index)(2 * i)];

            if (i == 2048 - 1) {
                this->ifftstream_tilde_02_fft_next(this->ifftstream_tilde_02_inWorkspace, 2048);

                for (SampleIndex j = 0; j < 2048 * 2; j++) {
                    this->ifftstream_tilde_02_outWorkspace[(Index)j] = this->ifftstream_tilde_02_inWorkspace[(Index)j] * invFftSize;
                }
            }

            __ifftstream_tilde_02_datapos = (__ifftstream_tilde_02_datapos + 1) % 2048;
        }

        out1[(Index)i0] = out[0];
        out2[(Index)i0] = out[1];
        out3[(Index)i0] = out[2];
    }

    this->ifftstream_tilde_02_datapos = __ifftstream_tilde_02_datapos;
}

void dspexpr_08_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        out1[(Index)i] = in1[(Index)i] + in2[(Index)i];//#map:_###_obj_###_:1
    }
}

void peek_tilde_01_perform(
    const Sample * index,
    number offset,
    SampleValue * out1,
    SampleValue * outindex,
    Index n
) {
    RNBO_UNUSED(out1);
    RNBO_UNUSED(offset);
    auto __peek_tilde_01_buffer = this->peek_tilde_01_buffer;
    SampleArray<1> out = {out1};

    for (Index i = 0; i < n; i++) {
        auto result = this->peek_boundmode_ignore_channelmode_ignore_indexmode_samples_interp_none(__peek_tilde_01_buffer, index[(Index)i], 0);

        for (Index j = 0; j < 1; j++) {
            out[(Index)j][(Index)i] = result[(Index)j];
        }

        outindex[(Index)i] = result[1];
    }
}

void dspexpr_05_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        out1[(Index)i] = in1[(Index)i] * in2[(Index)i];//#map:_###_obj_###_:1
    }
}

void dspexpr_01_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        out1[(Index)i] = in1[(Index)i] * in2[(Index)i];//#map:_###_obj_###_:1
    }
}

void ifftstream_tilde_01_perform(
    const Sample * realIn,
    const Sample * imagIn,
    SampleValue * out1,
    SampleValue * out2,
    SampleValue * out3,
    Index n
) {
    auto __ifftstream_tilde_01_win_buf = this->ifftstream_tilde_01_win_buf;
    auto __ifftstream_tilde_01_datapos = this->ifftstream_tilde_01_datapos;
    Index i0;

    for (i0 = 0; i0 < (Index)n; i0++) {
        number invFftSize = (number)1 / (number)2048;
        SampleIndex i = (SampleIndex)(__ifftstream_tilde_01_datapos);
        list out = list(0, 0, i);
        number windowFactor = 1;
        windowFactor = __ifftstream_tilde_01_win_buf->getSample(0, i);

        if (__ifftstream_tilde_01_datapos < 2048) {
            this->ifftstream_tilde_01_inWorkspace[(Index)(2 * i + 1)] = realIn[(Index)i0];
            out[0] = windowFactor * this->ifftstream_tilde_01_outWorkspace[(Index)(2 * i + 1)];
            this->ifftstream_tilde_01_inWorkspace[(Index)(2 * i)] = imagIn[(Index)i0];
            out[1] = windowFactor * this->ifftstream_tilde_01_outWorkspace[(Index)(2 * i)];

            if (i == 2048 - 1) {
                this->ifftstream_tilde_01_fft_next(this->ifftstream_tilde_01_inWorkspace, 2048);

                for (SampleIndex j = 0; j < 2048 * 2; j++) {
                    this->ifftstream_tilde_01_outWorkspace[(Index)j] = this->ifftstream_tilde_01_inWorkspace[(Index)j] * invFftSize;
                }
            }

            __ifftstream_tilde_01_datapos = (__ifftstream_tilde_01_datapos + 1) % 2048;
        }

        out1[(Index)i0] = out[0];
        out2[(Index)i0] = out[1];
        out3[(Index)i0] = out[2];
    }

    this->ifftstream_tilde_01_datapos = __ifftstream_tilde_01_datapos;
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

void dspexpr_07_perform(const Sample * in1, number in2, SampleValue * out1, Index n) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        out1[(Index)i] = in1[(Index)i] * in2;//#map:_###_obj_###_:1
    }
}

void dspexpr_06_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        out1[(Index)i] = in1[(Index)i] + in2[(Index)i];//#map:_###_obj_###_:1
    }
}

void signalforwarder_01_perform(const SampleValue * input, SampleValue * output, Index n) {
    copySignal(output, input, n);
}

void signalforwarder_02_perform(const SampleValue * input, SampleValue * output, Index n) {
    copySignal(output, input, n);
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

void data_03_srout_set(number ) {}

void data_03_chanout_set(number ) {}

void data_03_sizeout_set(number v) {
    this->data_03_sizeout = v;
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

template<typename BUFFERTYPE> void ifftstream_tilde_01_fft_next(BUFFERTYPE& buffer, Int fftsize) {
    if (this->ifftstream_tilde_01_fft_lastsize != fftsize) {
        for (Index i = 0; i < 32; i++) {
            if ((BinOpInt)((BinOpInt)1 << imod_nocast((UBinOpInt)i, 32)) == fftsize) {
                this->ifftstream_tilde_01_fft_levels = i;
            }
        }

        for (Index i = 0; i < fftsize; i++) {
            this->ifftstream_tilde_01_fft_costab->push(rnbo_cos(6.28318530717958647692 * i / fftsize));
            this->ifftstream_tilde_01_fft_sintab->push(rnbo_sin(6.28318530717958647692 * i / fftsize));
        }

        this->ifftstream_tilde_01_fft_lastsize = fftsize;
    }

    if (this->ifftstream_tilde_01_fft_levels != -1) {
        for (Index i = 0; i < fftsize; i++) {
            Index x = (Index)(i);
            Index j = 0;

            for (Index k = 0; k < this->ifftstream_tilde_01_fft_levels; k++) {
                j = (BinOpInt)((BinOpInt)((BinOpInt)j << imod_nocast((UBinOpInt)1, 32)) | (BinOpInt)((BinOpInt)x & (BinOpInt)1));
                x = (BinOpInt)((BinOpInt)x >> imod_nocast((UBinOpInt)1, 32));
            }

            if (j > i) {
                number temp = buffer[(Index)(2 * i)];
                buffer[(Index)(2 * i)] = buffer[(Index)(2 * j)];
                buffer[(Index)(2 * j)] = temp;
                temp = buffer[(Index)(2 * i + 1)];
                buffer[(Index)(2 * i + 1)] = buffer[(Index)(2 * j + 1)];
                buffer[(Index)(2 * j + 1)] = temp;
            }
        }

        for (Index size = 2; size <= fftsize; size *= 2) {
            Index halfsize = (Index)(size / (number)2);
            Index tablestep = (Index)(fftsize / size);

            for (Index i = 0; i < fftsize; i += size) {
                Index k = 0;

                for (Index j = (Index)(i); j < i + halfsize; j++) {
                    Index l = (Index)(j + halfsize);
                    number real_l = buffer[(Index)(2 * l)];
                    number imag_l = buffer[(Index)(2 * l + 1)];
                    number tpre = real_l * this->ifftstream_tilde_01_fft_costab[(Index)k] + imag_l * this->ifftstream_tilde_01_fft_sintab[(Index)k];
                    number tpim = -real_l * this->ifftstream_tilde_01_fft_sintab[(Index)k] + imag_l * this->ifftstream_tilde_01_fft_costab[(Index)k];
                    number real_j = buffer[(Index)(2 * j)];
                    number imag_j = buffer[(Index)(2 * j + 1)];
                    buffer[(Index)(2 * l)] = real_j - tpre;
                    buffer[(Index)(2 * l + 1)] = imag_j - tpim;
                    buffer[(Index)(2 * j)] = real_j + tpre;
                    buffer[(Index)(2 * j + 1)] = imag_j + tpim;
                    k += tablestep;
                }
            }
        }
    }
}

void ifftstream_tilde_01_fft_reset() {
    this->ifftstream_tilde_01_fft_lastsize = 0;
    this->ifftstream_tilde_01_fft_costab = {};
    this->ifftstream_tilde_01_fft_sintab = {};
    this->ifftstream_tilde_01_fft_levels = -1;
}

void ifftstream_tilde_01_dspsetup(bool force) {
    if ((bool)(this->ifftstream_tilde_01_setupDone) && (bool)(!(bool)(force)))
        return;

    for (Int i = 0; i < 2048 * 2; i++) {
        this->ifftstream_tilde_01_inWorkspace[(Index)i] = 0;
        this->ifftstream_tilde_01_outWorkspace[(Index)i] = 0;
    }

    Int safeframesize = (Int)(2048);

    {
        safeframesize = nextpoweroftwo(2048);
    }

    Int framepos = (Int)(0 % safeframesize);

    if (framepos < 2048) {
        this->ifftstream_tilde_01_datapos = framepos;
    } else {
        this->ifftstream_tilde_01_datapos = 0;
    }

    this->ifftstream_tilde_01_datapos = (this->audioProcessSampleCount + this->ifftstream_tilde_01_datapos) % 2048;
    this->ifftstream_tilde_01_setupDone = true;
}

template<typename BUFFERTYPE> void fftstream_tilde_01_fft_next(BUFFERTYPE& buffer, Int fftsize) {
    if (this->fftstream_tilde_01_fft_lastsize != fftsize) {
        for (Index i = 0; i < 32; i++) {
            if ((BinOpInt)((BinOpInt)1 << imod_nocast((UBinOpInt)i, 32)) == fftsize) {
                this->fftstream_tilde_01_fft_levels = i;
            }
        }

        for (Index i = 0; i < fftsize; i++) {
            this->fftstream_tilde_01_fft_costab->push(rnbo_cos(6.28318530717958647692 * i / fftsize));
            this->fftstream_tilde_01_fft_sintab->push(rnbo_sin(6.28318530717958647692 * i / fftsize));
        }

        this->fftstream_tilde_01_fft_lastsize = fftsize;
    }

    if (this->fftstream_tilde_01_fft_levels != -1) {
        for (Index i = 0; i < fftsize; i++) {
            Index x = (Index)(i);
            Index j = 0;

            for (Index k = 0; k < this->fftstream_tilde_01_fft_levels; k++) {
                j = (BinOpInt)((BinOpInt)((BinOpInt)j << imod_nocast((UBinOpInt)1, 32)) | (BinOpInt)((BinOpInt)x & (BinOpInt)1));
                x = (BinOpInt)((BinOpInt)x >> imod_nocast((UBinOpInt)1, 32));
            }

            if (j > i) {
                number temp = buffer[(Index)(2 * i)];
                buffer[(Index)(2 * i)] = buffer[(Index)(2 * j)];
                buffer[(Index)(2 * j)] = temp;
                temp = buffer[(Index)(2 * i + 1)];
                buffer[(Index)(2 * i + 1)] = buffer[(Index)(2 * j + 1)];
                buffer[(Index)(2 * j + 1)] = temp;
            }
        }

        for (Index size = 2; size <= fftsize; size *= 2) {
            Index halfsize = (Index)(size / (number)2);
            Index tablestep = (Index)(fftsize / size);

            for (Index i = 0; i < fftsize; i += size) {
                Index k = 0;

                for (Index j = (Index)(i); j < i + halfsize; j++) {
                    Index l = (Index)(j + halfsize);
                    number real_l = buffer[(Index)(2 * l)];
                    number imag_l = buffer[(Index)(2 * l + 1)];
                    number tpre = real_l * this->fftstream_tilde_01_fft_costab[(Index)k] + imag_l * this->fftstream_tilde_01_fft_sintab[(Index)k];
                    number tpim = -real_l * this->fftstream_tilde_01_fft_sintab[(Index)k] + imag_l * this->fftstream_tilde_01_fft_costab[(Index)k];
                    number real_j = buffer[(Index)(2 * j)];
                    number imag_j = buffer[(Index)(2 * j + 1)];
                    buffer[(Index)(2 * l)] = real_j - tpre;
                    buffer[(Index)(2 * l + 1)] = imag_j - tpim;
                    buffer[(Index)(2 * j)] = real_j + tpre;
                    buffer[(Index)(2 * j + 1)] = imag_j + tpim;
                    k += tablestep;
                }
            }
        }
    }
}

void fftstream_tilde_01_fft_reset() {
    this->fftstream_tilde_01_fft_lastsize = 0;
    this->fftstream_tilde_01_fft_costab = {};
    this->fftstream_tilde_01_fft_sintab = {};
    this->fftstream_tilde_01_fft_levels = -1;
}

void fftstream_tilde_01_dspsetup(bool force) {
    if ((bool)(this->fftstream_tilde_01_setupDone) && (bool)(!(bool)(force)))
        return;

    for (Int i = 0; i < 2048 * 2; i++) {
        this->fftstream_tilde_01_inWorkspace[(Index)i] = 0;
        this->fftstream_tilde_01_outWorkspace[(Index)i] = 0;
    }

    Int safeframesize = (Int)(2048);

    {
        safeframesize = nextpoweroftwo(2048);
    }

    Int framepos = (Int)(0 % safeframesize);

    if (framepos < 2048) {
        this->fftstream_tilde_01_datapos = framepos;
    } else {
        this->fftstream_tilde_01_datapos = 0;
    }

    this->fftstream_tilde_01_datapos = (this->audioProcessSampleCount + this->fftstream_tilde_01_datapos) % 2048;
    this->fftstream_tilde_01_setupDone = true;
}

template<typename BUFFERTYPE> void fftstream_tilde_02_fft_next(BUFFERTYPE& buffer, Int fftsize) {
    if (this->fftstream_tilde_02_fft_lastsize != fftsize) {
        for (Index i = 0; i < 32; i++) {
            if ((BinOpInt)((BinOpInt)1 << imod_nocast((UBinOpInt)i, 32)) == fftsize) {
                this->fftstream_tilde_02_fft_levels = i;
            }
        }

        for (Index i = 0; i < fftsize; i++) {
            this->fftstream_tilde_02_fft_costab->push(rnbo_cos(6.28318530717958647692 * i / fftsize));
            this->fftstream_tilde_02_fft_sintab->push(rnbo_sin(6.28318530717958647692 * i / fftsize));
        }

        this->fftstream_tilde_02_fft_lastsize = fftsize;
    }

    if (this->fftstream_tilde_02_fft_levels != -1) {
        for (Index i = 0; i < fftsize; i++) {
            Index x = (Index)(i);
            Index j = 0;

            for (Index k = 0; k < this->fftstream_tilde_02_fft_levels; k++) {
                j = (BinOpInt)((BinOpInt)((BinOpInt)j << imod_nocast((UBinOpInt)1, 32)) | (BinOpInt)((BinOpInt)x & (BinOpInt)1));
                x = (BinOpInt)((BinOpInt)x >> imod_nocast((UBinOpInt)1, 32));
            }

            if (j > i) {
                number temp = buffer[(Index)(2 * i)];
                buffer[(Index)(2 * i)] = buffer[(Index)(2 * j)];
                buffer[(Index)(2 * j)] = temp;
                temp = buffer[(Index)(2 * i + 1)];
                buffer[(Index)(2 * i + 1)] = buffer[(Index)(2 * j + 1)];
                buffer[(Index)(2 * j + 1)] = temp;
            }
        }

        for (Index size = 2; size <= fftsize; size *= 2) {
            Index halfsize = (Index)(size / (number)2);
            Index tablestep = (Index)(fftsize / size);

            for (Index i = 0; i < fftsize; i += size) {
                Index k = 0;

                for (Index j = (Index)(i); j < i + halfsize; j++) {
                    Index l = (Index)(j + halfsize);
                    number real_l = buffer[(Index)(2 * l)];
                    number imag_l = buffer[(Index)(2 * l + 1)];
                    number tpre = real_l * this->fftstream_tilde_02_fft_costab[(Index)k] + imag_l * this->fftstream_tilde_02_fft_sintab[(Index)k];
                    number tpim = -real_l * this->fftstream_tilde_02_fft_sintab[(Index)k] + imag_l * this->fftstream_tilde_02_fft_costab[(Index)k];
                    number real_j = buffer[(Index)(2 * j)];
                    number imag_j = buffer[(Index)(2 * j + 1)];
                    buffer[(Index)(2 * l)] = real_j - tpre;
                    buffer[(Index)(2 * l + 1)] = imag_j - tpim;
                    buffer[(Index)(2 * j)] = real_j + tpre;
                    buffer[(Index)(2 * j + 1)] = imag_j + tpim;
                    k += tablestep;
                }
            }
        }
    }
}

void fftstream_tilde_02_fft_reset() {
    this->fftstream_tilde_02_fft_lastsize = 0;
    this->fftstream_tilde_02_fft_costab = {};
    this->fftstream_tilde_02_fft_sintab = {};
    this->fftstream_tilde_02_fft_levels = -1;
}

void fftstream_tilde_02_dspsetup(bool force) {
    if ((bool)(this->fftstream_tilde_02_setupDone) && (bool)(!(bool)(force)))
        return;

    for (Int i = 0; i < 2048 * 2; i++) {
        this->fftstream_tilde_02_inWorkspace[(Index)i] = 0;
        this->fftstream_tilde_02_outWorkspace[(Index)i] = 0;
    }

    Int safeframesize = (Int)(2048);

    {
        safeframesize = nextpoweroftwo(2048);
    }

    Int framepos = (Int)(0 % safeframesize);

    if (framepos < 2048) {
        this->fftstream_tilde_02_datapos = framepos;
    } else {
        this->fftstream_tilde_02_datapos = 0;
    }

    this->fftstream_tilde_02_datapos = (this->audioProcessSampleCount + this->fftstream_tilde_02_datapos) % 2048;
    this->fftstream_tilde_02_setupDone = true;
}

void param_01_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->param_01_value;
}

void param_01_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->param_01_value_set(preset["value"]);
}

void param_02_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->param_02_value;
}

void param_02_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->param_02_value_set(preset["value"]);
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

void ip_01_init() {
    this->ip_01_lastValue = this->ip_01_value;
}

void ip_01_fillSigBuf() {
    if ((bool)(this->ip_01_sigbuf)) {
        SampleIndex k = (SampleIndex)(this->sampleOffsetIntoNextAudioBuffer);

        if (k >= (SampleIndex)(this->vs))
            k = (SampleIndex)(this->vs) - 1;

        for (SampleIndex i = (SampleIndex)(this->ip_01_lastIndex); i < k; i++) {
            if (this->ip_01_resetCount > 0) {
                this->ip_01_sigbuf[(Index)i] = 1;
                this->ip_01_resetCount--;
            } else {
                this->ip_01_sigbuf[(Index)i] = this->ip_01_lastValue;
            }
        }

        this->ip_01_lastIndex = k;
    }
}

void ip_01_dspsetup(bool force) {
    if ((bool)(this->ip_01_setupDone) && (bool)(!(bool)(force)))
        return;

    this->ip_01_lastIndex = 0;
    this->ip_01_setupDone = true;
}

template<typename BUFFERTYPE> void ifftstream_tilde_02_fft_next(BUFFERTYPE& buffer, Int fftsize) {
    if (this->ifftstream_tilde_02_fft_lastsize != fftsize) {
        for (Index i = 0; i < 32; i++) {
            if ((BinOpInt)((BinOpInt)1 << imod_nocast((UBinOpInt)i, 32)) == fftsize) {
                this->ifftstream_tilde_02_fft_levels = i;
            }
        }

        for (Index i = 0; i < fftsize; i++) {
            this->ifftstream_tilde_02_fft_costab->push(rnbo_cos(6.28318530717958647692 * i / fftsize));
            this->ifftstream_tilde_02_fft_sintab->push(rnbo_sin(6.28318530717958647692 * i / fftsize));
        }

        this->ifftstream_tilde_02_fft_lastsize = fftsize;
    }

    if (this->ifftstream_tilde_02_fft_levels != -1) {
        for (Index i = 0; i < fftsize; i++) {
            Index x = (Index)(i);
            Index j = 0;

            for (Index k = 0; k < this->ifftstream_tilde_02_fft_levels; k++) {
                j = (BinOpInt)((BinOpInt)((BinOpInt)j << imod_nocast((UBinOpInt)1, 32)) | (BinOpInt)((BinOpInt)x & (BinOpInt)1));
                x = (BinOpInt)((BinOpInt)x >> imod_nocast((UBinOpInt)1, 32));
            }

            if (j > i) {
                number temp = buffer[(Index)(2 * i)];
                buffer[(Index)(2 * i)] = buffer[(Index)(2 * j)];
                buffer[(Index)(2 * j)] = temp;
                temp = buffer[(Index)(2 * i + 1)];
                buffer[(Index)(2 * i + 1)] = buffer[(Index)(2 * j + 1)];
                buffer[(Index)(2 * j + 1)] = temp;
            }
        }

        for (Index size = 2; size <= fftsize; size *= 2) {
            Index halfsize = (Index)(size / (number)2);
            Index tablestep = (Index)(fftsize / size);

            for (Index i = 0; i < fftsize; i += size) {
                Index k = 0;

                for (Index j = (Index)(i); j < i + halfsize; j++) {
                    Index l = (Index)(j + halfsize);
                    number real_l = buffer[(Index)(2 * l)];
                    number imag_l = buffer[(Index)(2 * l + 1)];
                    number tpre = real_l * this->ifftstream_tilde_02_fft_costab[(Index)k] + imag_l * this->ifftstream_tilde_02_fft_sintab[(Index)k];
                    number tpim = -real_l * this->ifftstream_tilde_02_fft_sintab[(Index)k] + imag_l * this->ifftstream_tilde_02_fft_costab[(Index)k];
                    number real_j = buffer[(Index)(2 * j)];
                    number imag_j = buffer[(Index)(2 * j + 1)];
                    buffer[(Index)(2 * l)] = real_j - tpre;
                    buffer[(Index)(2 * l + 1)] = imag_j - tpim;
                    buffer[(Index)(2 * j)] = real_j + tpre;
                    buffer[(Index)(2 * j + 1)] = imag_j + tpim;
                    k += tablestep;
                }
            }
        }
    }
}

void ifftstream_tilde_02_fft_reset() {
    this->ifftstream_tilde_02_fft_lastsize = 0;
    this->ifftstream_tilde_02_fft_costab = {};
    this->ifftstream_tilde_02_fft_sintab = {};
    this->ifftstream_tilde_02_fft_levels = -1;
}

void ifftstream_tilde_02_dspsetup(bool force) {
    if ((bool)(this->ifftstream_tilde_02_setupDone) && (bool)(!(bool)(force)))
        return;

    for (Int i = 0; i < 2048 * 2; i++) {
        this->ifftstream_tilde_02_inWorkspace[(Index)i] = 0;
        this->ifftstream_tilde_02_outWorkspace[(Index)i] = 0;
    }

    Int safeframesize = (Int)(2048);

    {
        safeframesize = nextpoweroftwo(2048);
    }

    Int framepos = (Int)(1024 % safeframesize);

    if (framepos < 2048) {
        this->ifftstream_tilde_02_datapos = framepos;
    } else {
        this->ifftstream_tilde_02_datapos = 0;
    }

    this->ifftstream_tilde_02_datapos = (this->audioProcessSampleCount + this->ifftstream_tilde_02_datapos) % 2048;
    this->ifftstream_tilde_02_setupDone = true;
}

void data_03_init() {
    this->data_03_buffer->setWantsFill(true);
}

Index data_03_evaluateSizeExpr(number samplerate, number vectorsize) {
    RNBO_UNUSED(vectorsize);
    RNBO_UNUSED(samplerate);
    number size = 0;
    return (Index)(size);
}

void data_03_dspsetup(bool force) {
    if ((bool)(this->data_03_setupDone) && (bool)(!(bool)(force)))
        return;

    {
        if (this->data_03_sizemode == 2) {
            this->data_03_buffer = this->data_03_buffer->setSize((Index)(this->mstosamps(this->data_03_sizems)));
            updateDataRef(this, this->data_03_buffer);
        } else if (this->data_03_sizemode == 3) {
            this->data_03_buffer = this->data_03_buffer->setSize(this->data_03_evaluateSizeExpr(this->sr, this->vs));
            updateDataRef(this, this->data_03_buffer);
        }
    }

    this->data_03_setupDone = true;
}

void data_03_afterBufferUpdate() {
    this->data_03_report();
}

void data_03_report() {
    {
        this->data_03_srout_set(this->data_03_buffer->getSampleRate());
        this->data_03_chanout_set(this->data_03_buffer->getChannels());
    }

    this->data_03_sizeout_set(this->data_03_buffer->getSize());
}

template<typename BUFFERTYPE> void fftstream_tilde_03_fft_next(BUFFERTYPE& buffer, Int fftsize) {
    if (this->fftstream_tilde_03_fft_lastsize != fftsize) {
        for (Index i = 0; i < 32; i++) {
            if ((BinOpInt)((BinOpInt)1 << imod_nocast((UBinOpInt)i, 32)) == fftsize) {
                this->fftstream_tilde_03_fft_levels = i;
            }
        }

        for (Index i = 0; i < fftsize; i++) {
            this->fftstream_tilde_03_fft_costab->push(rnbo_cos(6.28318530717958647692 * i / fftsize));
            this->fftstream_tilde_03_fft_sintab->push(rnbo_sin(6.28318530717958647692 * i / fftsize));
        }

        this->fftstream_tilde_03_fft_lastsize = fftsize;
    }

    if (this->fftstream_tilde_03_fft_levels != -1) {
        for (Index i = 0; i < fftsize; i++) {
            Index x = (Index)(i);
            Index j = 0;

            for (Index k = 0; k < this->fftstream_tilde_03_fft_levels; k++) {
                j = (BinOpInt)((BinOpInt)((BinOpInt)j << imod_nocast((UBinOpInt)1, 32)) | (BinOpInt)((BinOpInt)x & (BinOpInt)1));
                x = (BinOpInt)((BinOpInt)x >> imod_nocast((UBinOpInt)1, 32));
            }

            if (j > i) {
                number temp = buffer[(Index)(2 * i)];
                buffer[(Index)(2 * i)] = buffer[(Index)(2 * j)];
                buffer[(Index)(2 * j)] = temp;
                temp = buffer[(Index)(2 * i + 1)];
                buffer[(Index)(2 * i + 1)] = buffer[(Index)(2 * j + 1)];
                buffer[(Index)(2 * j + 1)] = temp;
            }
        }

        for (Index size = 2; size <= fftsize; size *= 2) {
            Index halfsize = (Index)(size / (number)2);
            Index tablestep = (Index)(fftsize / size);

            for (Index i = 0; i < fftsize; i += size) {
                Index k = 0;

                for (Index j = (Index)(i); j < i + halfsize; j++) {
                    Index l = (Index)(j + halfsize);
                    number real_l = buffer[(Index)(2 * l)];
                    number imag_l = buffer[(Index)(2 * l + 1)];
                    number tpre = real_l * this->fftstream_tilde_03_fft_costab[(Index)k] + imag_l * this->fftstream_tilde_03_fft_sintab[(Index)k];
                    number tpim = -real_l * this->fftstream_tilde_03_fft_sintab[(Index)k] + imag_l * this->fftstream_tilde_03_fft_costab[(Index)k];
                    number real_j = buffer[(Index)(2 * j)];
                    number imag_j = buffer[(Index)(2 * j + 1)];
                    buffer[(Index)(2 * l)] = real_j - tpre;
                    buffer[(Index)(2 * l + 1)] = imag_j - tpim;
                    buffer[(Index)(2 * j)] = real_j + tpre;
                    buffer[(Index)(2 * j + 1)] = imag_j + tpim;
                    k += tablestep;
                }
            }
        }
    }
}

void fftstream_tilde_03_fft_reset() {
    this->fftstream_tilde_03_fft_lastsize = 0;
    this->fftstream_tilde_03_fft_costab = {};
    this->fftstream_tilde_03_fft_sintab = {};
    this->fftstream_tilde_03_fft_levels = -1;
}

void fftstream_tilde_03_dspsetup(bool force) {
    if ((bool)(this->fftstream_tilde_03_setupDone) && (bool)(!(bool)(force)))
        return;

    for (Int i = 0; i < 2048 * 2; i++) {
        this->fftstream_tilde_03_inWorkspace[(Index)i] = 0;
        this->fftstream_tilde_03_outWorkspace[(Index)i] = 0;
    }

    Int safeframesize = (Int)(2048);

    {
        safeframesize = nextpoweroftwo(2048);
    }

    Int framepos = (Int)(1024 % safeframesize);

    if (framepos < 2048) {
        this->fftstream_tilde_03_datapos = framepos;
    } else {
        this->fftstream_tilde_03_datapos = 0;
    }

    this->fftstream_tilde_03_datapos = (this->audioProcessSampleCount + this->fftstream_tilde_03_datapos) % 2048;
    this->fftstream_tilde_03_setupDone = true;
}

template<typename BUFFERTYPE> void fftstream_tilde_04_fft_next(BUFFERTYPE& buffer, Int fftsize) {
    if (this->fftstream_tilde_04_fft_lastsize != fftsize) {
        for (Index i = 0; i < 32; i++) {
            if ((BinOpInt)((BinOpInt)1 << imod_nocast((UBinOpInt)i, 32)) == fftsize) {
                this->fftstream_tilde_04_fft_levels = i;
            }
        }

        for (Index i = 0; i < fftsize; i++) {
            this->fftstream_tilde_04_fft_costab->push(rnbo_cos(6.28318530717958647692 * i / fftsize));
            this->fftstream_tilde_04_fft_sintab->push(rnbo_sin(6.28318530717958647692 * i / fftsize));
        }

        this->fftstream_tilde_04_fft_lastsize = fftsize;
    }

    if (this->fftstream_tilde_04_fft_levels != -1) {
        for (Index i = 0; i < fftsize; i++) {
            Index x = (Index)(i);
            Index j = 0;

            for (Index k = 0; k < this->fftstream_tilde_04_fft_levels; k++) {
                j = (BinOpInt)((BinOpInt)((BinOpInt)j << imod_nocast((UBinOpInt)1, 32)) | (BinOpInt)((BinOpInt)x & (BinOpInt)1));
                x = (BinOpInt)((BinOpInt)x >> imod_nocast((UBinOpInt)1, 32));
            }

            if (j > i) {
                number temp = buffer[(Index)(2 * i)];
                buffer[(Index)(2 * i)] = buffer[(Index)(2 * j)];
                buffer[(Index)(2 * j)] = temp;
                temp = buffer[(Index)(2 * i + 1)];
                buffer[(Index)(2 * i + 1)] = buffer[(Index)(2 * j + 1)];
                buffer[(Index)(2 * j + 1)] = temp;
            }
        }

        for (Index size = 2; size <= fftsize; size *= 2) {
            Index halfsize = (Index)(size / (number)2);
            Index tablestep = (Index)(fftsize / size);

            for (Index i = 0; i < fftsize; i += size) {
                Index k = 0;

                for (Index j = (Index)(i); j < i + halfsize; j++) {
                    Index l = (Index)(j + halfsize);
                    number real_l = buffer[(Index)(2 * l)];
                    number imag_l = buffer[(Index)(2 * l + 1)];
                    number tpre = real_l * this->fftstream_tilde_04_fft_costab[(Index)k] + imag_l * this->fftstream_tilde_04_fft_sintab[(Index)k];
                    number tpim = -real_l * this->fftstream_tilde_04_fft_sintab[(Index)k] + imag_l * this->fftstream_tilde_04_fft_costab[(Index)k];
                    number real_j = buffer[(Index)(2 * j)];
                    number imag_j = buffer[(Index)(2 * j + 1)];
                    buffer[(Index)(2 * l)] = real_j - tpre;
                    buffer[(Index)(2 * l + 1)] = imag_j - tpim;
                    buffer[(Index)(2 * j)] = real_j + tpre;
                    buffer[(Index)(2 * j + 1)] = imag_j + tpim;
                    k += tablestep;
                }
            }
        }
    }
}

void fftstream_tilde_04_fft_reset() {
    this->fftstream_tilde_04_fft_lastsize = 0;
    this->fftstream_tilde_04_fft_costab = {};
    this->fftstream_tilde_04_fft_sintab = {};
    this->fftstream_tilde_04_fft_levels = -1;
}

void fftstream_tilde_04_dspsetup(bool force) {
    if ((bool)(this->fftstream_tilde_04_setupDone) && (bool)(!(bool)(force)))
        return;

    for (Int i = 0; i < 2048 * 2; i++) {
        this->fftstream_tilde_04_inWorkspace[(Index)i] = 0;
        this->fftstream_tilde_04_outWorkspace[(Index)i] = 0;
    }

    Int safeframesize = (Int)(2048);

    {
        safeframesize = nextpoweroftwo(2048);
    }

    Int framepos = (Int)(1024 % safeframesize);

    if (framepos < 2048) {
        this->fftstream_tilde_04_datapos = framepos;
    } else {
        this->fftstream_tilde_04_datapos = 0;
    }

    this->fftstream_tilde_04_datapos = (this->audioProcessSampleCount + this->fftstream_tilde_04_datapos) % 2048;
    this->fftstream_tilde_04_setupDone = true;
}

void globaltransport_advance() {}

void globaltransport_dspsetup(bool ) {}

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
    poke_tilde_01_value_number = 0;
    poke_tilde_01_index = 0;
    poke_tilde_01_channel = 0;
    poke_tilde_01_overdub = 0;
    data_01_sizeout = 0;
    data_01_size = 2048;
    data_01_sizems = 0;
    data_01_normalize = 0.995;
    data_01_channels = 1;
    dspexpr_01_in1 = 0;
    dspexpr_01_in2 = 0;
    dspexpr_02_in1 = 0;
    dspexpr_03_in1 = 0;
    dspexpr_03_in2 = 0;
    ifftstream_tilde_01_realIn = 0;
    ifftstream_tilde_01_imagIn = 0;
    fftstream_tilde_01_realIn = 0;
    fftstream_tilde_01_imagIn = 0;
    fftstream_tilde_02_realIn = 0;
    fftstream_tilde_02_imagIn = 0;
    dspexpr_04_in1 = 0;
    dspexpr_04_in2 = 0;
    dspexpr_05_in1 = 0;
    dspexpr_05_in2 = 0;
    dspexpr_06_in1 = 0;
    dspexpr_06_in2 = 0;
    dspexpr_07_in1 = 0;
    dspexpr_07_in2 = 0;
    param_01_value = 1;
    peek_tilde_01_index = 0;
    peek_tilde_01_offset = 0;
    dspexpr_08_in1 = 0;
    dspexpr_08_in2 = 0;
    scale_01_inlow = 0;
    scale_01_inhigh = 1;
    scale_01_outlow = -24;
    scale_01_outhigh = 24;
    scale_01_power = 1;
    param_02_value = 0.5;
    dspexpr_09_in1 = 0;
    dspexpr_09_in2 = 0;
    data_02_sizeout = 0;
    data_02_size = 2048;
    data_02_sizems = 0;
    data_02_normalize = 0.995;
    data_02_channels = 1;
    expr_01_in1 = 0;
    expr_01_in2 = 1;
    expr_01_out1 = 0;
    ip_01_value = 0;
    ip_01_impulse = 0;
    dspexpr_10_in1 = 0;
    dspexpr_10_in2 = 0;
    peek_tilde_02_index = 0;
    peek_tilde_02_offset = 0;
    dspexpr_11_in1 = 0;
    dspexpr_11_in2 = 0;
    ifftstream_tilde_02_realIn = 0;
    ifftstream_tilde_02_imagIn = 0;
    data_03_sizeout = 0;
    data_03_size = 2048;
    data_03_sizems = 0;
    data_03_normalize = 0.995;
    data_03_channels = 1;
    dspexpr_12_in1 = 0;
    dspexpr_12_in2 = 0;
    fftstream_tilde_03_realIn = 0;
    fftstream_tilde_03_imagIn = 0;
    poke_tilde_02_value_number = 0;
    poke_tilde_02_index = 0;
    poke_tilde_02_channel = 0;
    poke_tilde_02_overdub = 0;
    dspexpr_13_in1 = 0;
    dspexpr_14_in1 = 0;
    dspexpr_14_in2 = 0;
    fftstream_tilde_04_realIn = 0;
    fftstream_tilde_04_imagIn = 0;
    dspexpr_15_in1 = 0;
    dspexpr_15_in2 = 0;
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
    signals[8] = nullptr;
    signals[9] = nullptr;
    signals[10] = nullptr;
    signals[11] = nullptr;
    didAllocateSignals = 0;
    vs = 0;
    maxvs = 0;
    sr = 44100;
    invsr = 0.000022675736961451248;
    data_01_sizemode = 1;
    data_01_setupDone = false;
    ifftstream_tilde_01_datapos = 0;
    ifftstream_tilde_01_fft_lastsize = 0;
    ifftstream_tilde_01_fft_levels = -1;
    ifftstream_tilde_01_setupDone = false;
    fftstream_tilde_01_datapos = 0;
    fftstream_tilde_01_fft_lastsize = 0;
    fftstream_tilde_01_fft_levels = -1;
    fftstream_tilde_01_setupDone = false;
    fftstream_tilde_02_datapos = 0;
    fftstream_tilde_02_fft_lastsize = 0;
    fftstream_tilde_02_fft_levels = -1;
    fftstream_tilde_02_setupDone = false;
    param_01_lastValue = 0;
    param_02_lastValue = 0;
    data_02_sizemode = 1;
    data_02_setupDone = false;
    ip_01_lastIndex = 0;
    ip_01_lastValue = 0;
    ip_01_resetCount = 0;
    ip_01_sigbuf = nullptr;
    ip_01_setupDone = false;
    ifftstream_tilde_02_datapos = 0;
    ifftstream_tilde_02_fft_lastsize = 0;
    ifftstream_tilde_02_fft_levels = -1;
    ifftstream_tilde_02_setupDone = false;
    data_03_sizemode = 1;
    data_03_setupDone = false;
    fftstream_tilde_03_datapos = 0;
    fftstream_tilde_03_fft_lastsize = 0;
    fftstream_tilde_03_fft_levels = -1;
    fftstream_tilde_03_setupDone = false;
    fftstream_tilde_04_datapos = 0;
    fftstream_tilde_04_fft_lastsize = 0;
    fftstream_tilde_04_fft_levels = -1;
    fftstream_tilde_04_setupDone = false;
    globaltransport_tempo = nullptr;
    globaltransport_state = nullptr;
    stackprotect_count = 0;
    _voiceIndex = 0;
    _noteNumber = 0;
    isMuted = 1;
}

    // data ref strings
    struct DataRefStrings {
    	static constexpr auto& name0 = "frzmag1";
    	static constexpr auto& file0 = "";
    	static constexpr auto& tag0 = "data";
    	static constexpr auto& name1 = "r_window";
    	static constexpr auto& file1 = "";
    	static constexpr auto& tag1 = "buffer~";
    	static constexpr auto& name2 = "frzmag2";
    	static constexpr auto& file2 = "";
    	static constexpr auto& tag2 = "data";
    	DataRefStrings* operator->() { return this; }
    	const DataRefStrings* operator->() const { return this; }
    };

    DataRefStrings dataRefStrings;

// member variables

    number poke_tilde_01_value_number;
    list poke_tilde_01_value_list;
    number poke_tilde_01_index;
    number poke_tilde_01_channel;
    number poke_tilde_01_overdub;
    number data_01_sizeout;
    number data_01_size;
    number data_01_sizems;
    number data_01_normalize;
    number data_01_channels;
    number dspexpr_01_in1;
    number dspexpr_01_in2;
    number dspexpr_02_in1;
    number dspexpr_03_in1;
    number dspexpr_03_in2;
    number ifftstream_tilde_01_realIn;
    number ifftstream_tilde_01_imagIn;
    number fftstream_tilde_01_realIn;
    number fftstream_tilde_01_imagIn;
    number fftstream_tilde_02_realIn;
    number fftstream_tilde_02_imagIn;
    number dspexpr_04_in1;
    number dspexpr_04_in2;
    number dspexpr_05_in1;
    number dspexpr_05_in2;
    number dspexpr_06_in1;
    number dspexpr_06_in2;
    number dspexpr_07_in1;
    number dspexpr_07_in2;
    number param_01_value;
    number peek_tilde_01_index;
    number peek_tilde_01_offset;
    number dspexpr_08_in1;
    number dspexpr_08_in2;
    list scale_01_input;
    number scale_01_inlow;
    number scale_01_inhigh;
    number scale_01_outlow;
    number scale_01_outhigh;
    number scale_01_power;
    list scale_01_out;
    number param_02_value;
    number dspexpr_09_in1;
    number dspexpr_09_in2;
    number data_02_sizeout;
    number data_02_size;
    number data_02_sizems;
    number data_02_normalize;
    number data_02_channels;
    number expr_01_in1;
    number expr_01_in2;
    number expr_01_out1;
    number ip_01_value;
    number ip_01_impulse;
    number dspexpr_10_in1;
    number dspexpr_10_in2;
    number peek_tilde_02_index;
    number peek_tilde_02_offset;
    number dspexpr_11_in1;
    number dspexpr_11_in2;
    number ifftstream_tilde_02_realIn;
    number ifftstream_tilde_02_imagIn;
    number data_03_sizeout;
    number data_03_size;
    number data_03_sizems;
    number data_03_normalize;
    number data_03_channels;
    number dspexpr_12_in1;
    number dspexpr_12_in2;
    number fftstream_tilde_03_realIn;
    number fftstream_tilde_03_imagIn;
    number poke_tilde_02_value_number;
    list poke_tilde_02_value_list;
    number poke_tilde_02_index;
    number poke_tilde_02_channel;
    number poke_tilde_02_overdub;
    number dspexpr_13_in1;
    number dspexpr_14_in1;
    number dspexpr_14_in2;
    number fftstream_tilde_04_realIn;
    number fftstream_tilde_04_imagIn;
    number dspexpr_15_in1;
    number dspexpr_15_in2;
    MillisecondTime _currentTime;
    ENGINE _internalEngine;
    UInt64 audioProcessSampleCount;
    Index sampleOffsetIntoNextAudioBuffer;
    signal zeroBuffer;
    signal dummyBuffer;
    SampleValue * signals[12];
    bool didAllocateSignals;
    Index vs;
    Index maxvs;
    number sr;
    number invsr;
    Float64BufferRef poke_tilde_01_buffer;
    Float32BufferRef data_01_buffer;
    Int data_01_sizemode;
    bool data_01_setupDone;
    SampleValue ifftstream_tilde_01_inWorkspace[4096] = { };
    SampleValue ifftstream_tilde_01_outWorkspace[4096] = { };
    Float32BufferRef ifftstream_tilde_01_win_buf;
    SampleIndex ifftstream_tilde_01_datapos;
    Int ifftstream_tilde_01_fft_lastsize;
    list ifftstream_tilde_01_fft_costab;
    list ifftstream_tilde_01_fft_sintab;
    Int ifftstream_tilde_01_fft_levels;
    bool ifftstream_tilde_01_setupDone;
    SampleValue fftstream_tilde_01_inWorkspace[4096] = { };
    SampleValue fftstream_tilde_01_outWorkspace[4096] = { };
    Float32BufferRef fftstream_tilde_01_win_buf;
    SampleIndex fftstream_tilde_01_datapos;
    Int fftstream_tilde_01_fft_lastsize;
    list fftstream_tilde_01_fft_costab;
    list fftstream_tilde_01_fft_sintab;
    Int fftstream_tilde_01_fft_levels;
    bool fftstream_tilde_01_setupDone;
    SampleValue fftstream_tilde_02_inWorkspace[4096] = { };
    SampleValue fftstream_tilde_02_outWorkspace[4096] = { };
    Float32BufferRef fftstream_tilde_02_win_buf;
    SampleIndex fftstream_tilde_02_datapos;
    Int fftstream_tilde_02_fft_lastsize;
    list fftstream_tilde_02_fft_costab;
    list fftstream_tilde_02_fft_sintab;
    Int fftstream_tilde_02_fft_levels;
    bool fftstream_tilde_02_setupDone;
    number param_01_lastValue;
    Float64BufferRef peek_tilde_01_buffer;
    number param_02_lastValue;
    Float64BufferRef data_02_buffer;
    Int data_02_sizemode;
    bool data_02_setupDone;
    SampleIndex ip_01_lastIndex;
    number ip_01_lastValue;
    SampleIndex ip_01_resetCount;
    signal ip_01_sigbuf;
    bool ip_01_setupDone;
    Float64BufferRef peek_tilde_02_buffer;
    SampleValue ifftstream_tilde_02_inWorkspace[4096] = { };
    SampleValue ifftstream_tilde_02_outWorkspace[4096] = { };
    Float32BufferRef ifftstream_tilde_02_win_buf;
    SampleIndex ifftstream_tilde_02_datapos;
    Int ifftstream_tilde_02_fft_lastsize;
    list ifftstream_tilde_02_fft_costab;
    list ifftstream_tilde_02_fft_sintab;
    Int ifftstream_tilde_02_fft_levels;
    bool ifftstream_tilde_02_setupDone;
    Float64BufferRef data_03_buffer;
    Int data_03_sizemode;
    bool data_03_setupDone;
    SampleValue fftstream_tilde_03_inWorkspace[4096] = { };
    SampleValue fftstream_tilde_03_outWorkspace[4096] = { };
    Float32BufferRef fftstream_tilde_03_win_buf;
    SampleIndex fftstream_tilde_03_datapos;
    Int fftstream_tilde_03_fft_lastsize;
    list fftstream_tilde_03_fft_costab;
    list fftstream_tilde_03_fft_sintab;
    Int fftstream_tilde_03_fft_levels;
    bool fftstream_tilde_03_setupDone;
    Float64BufferRef poke_tilde_02_buffer;
    SampleValue fftstream_tilde_04_inWorkspace[4096] = { };
    SampleValue fftstream_tilde_04_outWorkspace[4096] = { };
    Float32BufferRef fftstream_tilde_04_win_buf;
    SampleIndex fftstream_tilde_04_datapos;
    Int fftstream_tilde_04_fft_lastsize;
    list fftstream_tilde_04_fft_costab;
    list fftstream_tilde_04_fft_sintab;
    Int fftstream_tilde_04_fft_levels;
    bool fftstream_tilde_04_setupDone;
    signal globaltransport_tempo;
    signal globaltransport_state;
    number stackprotect_count;
    DataRef frzmag1;
    DataRef r_window;
    DataRef frzmag2;
    Index _voiceIndex;
    Int _noteNumber;
    Index isMuted;
    indexlist paramInitIndices;
    indexlist paramInitOrder;
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

