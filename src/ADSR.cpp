#include "Gratrix.hpp"
#include "dsp/digital.hpp"


// ===========================================================================================================

struct ADSR : MicroModule {
	enum ParamIds {
		ATTACK_PARAM,
		DECAY_PARAM,
		SUSTAIN_PARAM,
		RELEASE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		ATTACK_INPUT,     // 1
		DECAY_INPUT,      // 1
		SUSTAIN_INPUT,    // 1
		RELEASE_INPUT,    // 1
		GATE_INPUT,       // N+1
		TRIG_INPUT,       // N+1
		NUM_INPUTS,
		OFF_INPUTS = GATE_INPUT
	};
	enum OutputIds {
		ENVELOPE_OUTPUT,  // N
		INVERTED_OUTPUT,  // N
		NUM_OUTPUTS,
		OFF_OUTPUTS = ENVELOPE_OUTPUT
	};

	bool decaying = false;
	float env = 0.0;
	SchmittTrigger trigger;

	ADSR() : MicroModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS) {
		trigger.setThresholds(0.0, 1.0);
	}
	void step();
};


// ===========================================================================================================

void ADSR::step() {
	float attack  = clampf(params[ATTACK_INPUT] .value + inputs[ATTACK_INPUT] .value / 10.0, 0.0, 1.0);
	float decay   = clampf(params[DECAY_PARAM]  .value + inputs[DECAY_INPUT]  .value / 10.0, 0.0, 1.0);
	float sustain = clampf(params[SUSTAIN_PARAM].value + inputs[SUSTAIN_INPUT].value / 10.0, 0.0, 1.0);
	float release = clampf(params[RELEASE_PARAM].value + inputs[RELEASE_PARAM].value / 10.0, 0.0, 1.0);

	// Gate and trigger
	bool gated = inputs[GATE_INPUT].value >= 1.0;
	if (trigger.process(inputs[TRIG_INPUT].value))
		decaying = false;

	const float base = 20000.0;
	const float maxTime = 10.0;
	if (gated) {
		if (decaying) {
			// Decay
			if (decay < 1e-4) {
				env = sustain;
			}
			else {
				env += powf(base, 1 - decay) / maxTime * (sustain - env) / engineGetSampleRate();
			}
		}
		else {
			// Attack
			// Skip ahead if attack is all the way down (infinitely fast)
			if (attack < 1e-4) {
				env = 1.0;
			}
			else {
				env += powf(base, 1 - attack) / maxTime * (1.01 - env) / engineGetSampleRate();
			}
			if (env >= 1.0) {
				env = 1.0;
				decaying = true;
			}
		}
	}
	else {
		// Release
		if (release < 1e-4) {
			env = 0.0;
		}
		else {
			env += powf(base, 1 - release) / maxTime * (0.0 - env) / engineGetSampleRate();
		}
		decaying = false;
	}

	outputs[ENVELOPE_OUTPUT].value = 10.0 * env;
	outputs[INVERTED_OUTPUT].value = 10.0 * (1.0 - env);
}


// ===========================================================================================================

struct ADSRBank : Module
{
	std::array<ADSR, GTX__N> inst;

	ADSRBank()
	:
		Module(ADSR::NUM_PARAMS,
			(GTX__N+1) * (ADSR::NUM_INPUTS  - ADSR::OFF_INPUTS ) + ADSR::OFF_INPUTS,
			(GTX__N  ) * (ADSR::NUM_OUTPUTS - ADSR::OFF_OUTPUTS) + ADSR::OFF_OUTPUTS)
	{}

	static constexpr std::size_t imap(std::size_t port, std::size_t bank)
	{
		return (port < ADSR::OFF_INPUTS)  ? port : port + bank * (ADSR::NUM_INPUTS  - ADSR::OFF_INPUTS);
	}

	static constexpr std::size_t omap(std::size_t port, std::size_t bank)
	{
	//	return (port < ADSR::OFF_OUTPUTS) ? port : port + bank * (ADSR::NUM_OUTPUTS - ADSR::OFF_OUTPUTS);
		return                                     port + bank *  ADSR::NUM_OUTPUTS;
	}

	void step() override
	{
		for (std::size_t i=0; i<GTX__N; ++i)
		{
			for (std::size_t p=0; p<ADSR::NUM_PARAMS;  ++p) inst[i].params[p]  = params[p];
			for (std::size_t p=0; p<ADSR::NUM_INPUTS;  ++p) inst[i].inputs[p]  = inputs[imap(p, i)].active ? inputs[imap(p, i)] : inputs[imap(p, GTX__N)];
			for (std::size_t p=0; p<ADSR::NUM_OUTPUTS; ++p) inst[i].outputs[p] = outputs[omap(p, i)];

			inst[i].step();

			for (std::size_t p=0; p<ADSR::NUM_OUTPUTS; ++p) outputs[omap(p, i)].value = inst[i].outputs[p].value;
		}
	}
};


// ===========================================================================================================

ADSRWidget::ADSRWidget() {
	ADSRBank *module = new ADSRBank();
	setModule(module);
	box.size = Vec(12*15, 380);

	#if GTX__SAVE_SVG
	{
		PanelGen pg(assetPlugin(plugin, "res/Env-F1.svg"), box.size, "ENV-F1");

		pg.nob_med(0, -0.28, "ATTACK");  pg.nob_med(1, -0.28, "DECAY");
		pg.nob_med(0, +0.28, "SUSTAIN"); pg.nob_med(1, +0.28, "RELEASE");

		pg.bus_in(0, 1, "GATE");  pg.bus_out(1, 1, "OUT");
		pg.bus_in(0, 2, "TRIG");  pg.bus_out(1, 2, "INV OUT");
	}
	#endif

	{
		#if GTX__LOAD_SVG
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Env-F1.svg")));
		#else
		Panel *panel = new LightPanel();
		panel->box.size = box.size;
		panel->backgroundImage = Image::load(assetPlugin(plugin, "res/Env-F1.png"));
		#endif
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

	addParam(createParam<RoundBlackKnob>(n_m(fx(0+0.18), fy(-0.28)), module, ADSR::ATTACK_PARAM,  0.0, 1.0, 0.5));
	addParam(createParam<RoundBlackKnob>(n_m(fx(1+0.18), fy(-0.28)), module, ADSR::DECAY_PARAM,   0.0, 1.0, 0.5));
	addParam(createParam<RoundBlackKnob>(n_m(fx(0+0.18), fy(+0.28)), module, ADSR::SUSTAIN_PARAM, 0.0, 1.0, 0.5));
	addParam(createParam<RoundBlackKnob>(n_m(fx(1+0.18), fy(+0.28)), module, ADSR::RELEASE_PARAM, 0.0, 1.0, 0.5));

	addInput(createInput<PJ301MPort>(prt(fx(0-0.28), fy(-0.28)), module, ADSR::ATTACK_INPUT));
	addInput(createInput<PJ301MPort>(prt(fx(1-0.28), fy(-0.28)), module, ADSR::DECAY_INPUT));
	addInput(createInput<PJ301MPort>(prt(fx(0-0.28), fy(+0.28)), module, ADSR::SUSTAIN_INPUT));
	addInput(createInput<PJ301MPort>(prt(fx(1-0.28), fy(+0.28)), module, ADSR::RELEASE_INPUT));

	for (std::size_t i=0; i<GTX__N; ++i)
	{
		addInput(createInput<PJ301MPort> (prt(px(0, i), py(1, i)), module, ADSRBank::imap(ADSR::GATE_INPUT, i)));
		addInput(createInput<PJ301MPort> (prt(px(0, i), py(2, i)), module, ADSRBank::imap(ADSR::TRIG_INPUT, i)));

		addOutput(createOutput<PJ301MPort>(prt(px(1, i), py(1, i)), module, ADSRBank::omap(ADSR::ENVELOPE_OUTPUT, i)));
		addOutput(createOutput<PJ301MPort>(prt(px(1, i), py(2, i)), module, ADSRBank::omap(ADSR::INVERTED_OUTPUT, i)));
	}

	addInput(createInput<PJ301MPort> (prt(gx(0), gy(1)), module, ADSRBank::imap(ADSR::GATE_INPUT, GTX__N)));
	addInput(createInput<PJ301MPort> (prt(gx(0), gy(2)), module, ADSRBank::imap(ADSR::TRIG_INPUT, GTX__N)));
}
