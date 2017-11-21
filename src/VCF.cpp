/*
The filter DSP code has been derived from
Miller Puckette's code hosted at
https://github.com/ddiakopoulos/MoogLadders/blob/master/src/RKSimulationModel.h
which is licensed for use under the following terms (MIT license):


Copyright (c) 2015, Miller Puckette. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "Gratrix.hpp"


namespace GTX {
namespace VCF_F1 {


// The clipping function of a transistor pair is approximately tanh(x)
// TODO: Put this in a lookup table. 5th order approx doesn't seem to cut it
inline float clip(float x) {
	return tanhf(x);
}

struct LadderFilter {
	float cutoff = 1000.0;
	float resonance = 1.0;
	float state[4] = {};

	void calculateDerivatives(float input, float *dstate, const float *state) {
		float cutoff2Pi = 2*M_PI * cutoff;

		float satstate0 = clip(state[0]);
		float satstate1 = clip(state[1]);
		float satstate2 = clip(state[2]);

		dstate[0] = cutoff2Pi * (clip(input - resonance * state[3]) - satstate0);
		dstate[1] = cutoff2Pi * (satstate0 - satstate1);
		dstate[2] = cutoff2Pi * (satstate1 - satstate2);
		dstate[3] = cutoff2Pi * (satstate2 - clip(state[3]));
	}

	void process(float input, float dt) {
		float deriv1[4], deriv2[4], deriv3[4], deriv4[4], tempState[4];

		calculateDerivatives(input, deriv1, state);
		for (int i = 0; i < 4; i++)
			tempState[i] = state[i] + 0.5 * dt * deriv1[i];

		calculateDerivatives(input, deriv2, tempState);
		for (int i = 0; i < 4; i++)
			tempState[i] = state[i] + 0.5 * dt * deriv2[i];

		calculateDerivatives(input, deriv3, tempState);
		for (int i = 0; i < 4; i++)
			tempState[i] = state[i] + dt * deriv3[i];

		calculateDerivatives(input, deriv4, tempState);
		for (int i = 0; i < 4; i++)
			state[i] += (1.0 / 6.0) * dt * (deriv1[i] + 2.0 * deriv2[i] + 2.0 * deriv3[i] + deriv4[i]);
	}
	void reset() {
		for (int i = 0; i < 4; i++) {
			state[i] = 0.0;
		}
	}
};


//============================================================================================================

struct VCF : MicroModule {
	enum ParamIds {
		FREQ_PARAM,
		FINE_PARAM,
		RES_PARAM,
		FREQ_CV_PARAM,
		DRIVE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		FREQ_INPUT,
		RES_INPUT,
		DRIVE_INPUT,
		IN_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		LPF_OUTPUT,
		HPF_OUTPUT,
		NUM_OUTPUTS
	};

	LadderFilter filter;

	VCF() : MicroModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS) {}
	void step();
	void reset() {
		filter.reset();
	}
};


//============================================================================================================

void VCF::step() {
	float input = inputs[IN_INPUT].value / 5.0;
	float drive = params[DRIVE_PARAM].value + inputs[DRIVE_INPUT].value / 10.0;
	float gain = powf(100.0, drive);
	input *= gain;
	// Add -60dB noise to bootstrap self-oscillation
	input += 1.0e-6 * (2.0*randomf() - 1.0);

	// Set resonance
	float res = params[RES_PARAM].value + inputs[RES_INPUT].value / 5.0;
	res = 5.5 * clampf(res, 0.0, 1.0);
	filter.resonance = res;

	// Set cutoff frequency
	float cutoffExp = params[FREQ_PARAM].value + params[FREQ_CV_PARAM].value * inputs[FREQ_INPUT].value / 5.0;
	cutoffExp = clampf(cutoffExp, 0.0, 1.0);
	const float minCutoff = 15.0;
	const float maxCutoff = 8400.0;
	filter.cutoff = minCutoff * powf(maxCutoff / minCutoff, cutoffExp);

	// Push a sample to the state filter
	filter.process(input, 1.0/engineGetSampleRate());

	// Set outputs
	outputs[LPF_OUTPUT].value = 5.0 * filter.state[3];
	outputs[HPF_OUTPUT].value = 5.0 * (input - filter.state[3]);
}


//============================================================================================================

struct VCFBank : Module
{
	std::array<VCF, GTX__N> inst;

	VCFBank() : Module(VCF::NUM_PARAMS, (GTX__N+1) * VCF::NUM_INPUTS, GTX__N * VCF::NUM_OUTPUTS) {}

	static std::size_t imap(std::size_t port, std::size_t bank)
	{
		return port + bank * VCF::NUM_INPUTS;
	}

	static std::size_t omap(std::size_t port, std::size_t bank)
	{
		return port + bank * VCF::NUM_OUTPUTS;
	}

	void step() override
	{
		for (std::size_t i=0; i<GTX__N; ++i)
		{
			for (std::size_t p=0; p<VCF::NUM_PARAMS;  ++p) inst[i].params[p]  = params[p];
			for (std::size_t p=0; p<VCF::NUM_INPUTS;  ++p) inst[i].inputs[p]  = inputs[imap(p, i)].active ? inputs[imap(p, i)] : inputs[imap(p, GTX__N)];
			for (std::size_t p=0; p<VCF::NUM_OUTPUTS; ++p) inst[i].outputs[p] = outputs[omap(p, i)];

			inst[i].step();

			for (std::size_t p=0; p<VCF::NUM_OUTPUTS; ++p) outputs[omap(p, i)].value = inst[i].outputs[p].value;
		}
	}

	void reset() override
	{
		for (std::size_t i=0; i<GTX__N; ++i)
		{
			inst[i].reset();
		}
	}
};


//============================================================================================================

Widget::Widget()
{
	GTX__WIDGET();

	VCFBank *module = new VCFBank();
	setModule(module);
	box.size = Vec(18*15, 380);

	#if GTX__SAVE_SVG
	{
		PanelGen pg(assetPlugin(plugin, "build/res/VCF-F1.svg"), box.size, "VCF-F1");

		pg.nob_big(0, 0, "FREQ");

		pg.nob_med(1.1, -0.28, "FINE");     pg.nob_med(1.9, -0.28, "RES");
		pg.nob_med(1.1, +0.28, "FREQ  CV"); pg.nob_med(1.9, +0.28, "DRIVE");

		pg.bus_in(0, 1, "FREQ"); pg.bus_in(1, 1, "RES");   pg.bus_out(2, 1, "HPF");
		pg.bus_in(0, 2, "IN");   pg.bus_in(1, 2, "DRIVE"); pg.bus_out(2, 2, "LPF");
	}
	#endif

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/VCF-F1.svg")));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

	addParam(createParam<RoundHugeBlackKnob>(n_b(fx(0), fy(0)), module, VCF::FREQ_PARAM, 0.0, 1.0, 0.5));

	addParam(createParam<RoundBlackKnob>(n_m(fx(1.1), fy(-0.28)), module, VCF::FINE_PARAM,     0.0, 1.0, 0.5));
	addParam(createParam<RoundBlackKnob>(n_m(fx(1.9), fy(-0.28)), module, VCF::RES_PARAM,      0.0, 1.0, 0.0));
	addParam(createParam<RoundBlackKnob>(n_m(fx(1.1), fy(+0.28)), module, VCF::FREQ_CV_PARAM, -1.0, 1.0, 0.0));
	addParam(createParam<RoundBlackKnob>(n_m(fx(1.9), fy(+0.28)), module, VCF::DRIVE_PARAM,    0.0, 1.0, 0.0));

	for (std::size_t i=0; i<GTX__N; ++i)
	{
		addInput(createInput<PJ301MPort>  (prt(px(0, i), py(1, i)), module, VCFBank::imap(VCF::FREQ_INPUT,  i)));
		addInput(createInput<PJ301MPort>  (prt(px(1, i), py(1, i)), module, VCFBank::imap(VCF::RES_INPUT,   i)));
		addInput(createInput<PJ301MPort>  (prt(px(1, i), py(2, i)), module, VCFBank::imap(VCF::DRIVE_INPUT, i)));
		addInput(createInput<PJ301MPort>  (prt(px(0, i), py(2, i)), module, VCFBank::imap(VCF::IN_INPUT,    i)));

		addOutput(createOutput<PJ301MPort>(prt(px(2, i), py(2, i)), module, VCFBank::omap(VCF::LPF_OUTPUT,  i)));
		addOutput(createOutput<PJ301MPort>(prt(px(2, i), py(1, i)), module, VCFBank::omap(VCF::HPF_OUTPUT,  i)));
	}

	addInput(createInput<PJ301MPort>  (prt(gx(0), gy(1)), module, VCFBank::imap(VCF::FREQ_INPUT,  GTX__N)));
	addInput(createInput<PJ301MPort>  (prt(gx(1), gy(1)), module, VCFBank::imap(VCF::RES_INPUT,   GTX__N)));
	addInput(createInput<PJ301MPort>  (prt(gx(1), gy(2)), module, VCFBank::imap(VCF::DRIVE_INPUT, GTX__N)));
	addInput(createInput<PJ301MPort>  (prt(gx(0), gy(2)), module, VCFBank::imap(VCF::IN_INPUT,    GTX__N)));
}


} // VCF_F1
} // GTX
