#include "Gratrix.hpp"




// ===========================================================================================================

struct VCA : MicroModule {
	enum ParamIds {
		LEVEL_PARAM,
		MONO_PARAM,
		STEREO_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		EXP_INPUT,     // N+1
		LIN_INPUT,     // N+1
		IN_INPUT,      // N+1
		NUM_INPUTS,
		OFF_INPUTS = EXP_INPUT
	};
	enum OutputIds {
		MONO_OUTPUT,   // 1
		LEFT_OUTPUT,   // 1
		RIGHT_OUTPUT,  // 1
		OUT_OUTPUT,    // N
		NUM_OUTPUTS,
		OFF_OUTPUTS = OUT_OUTPUT
	};

	VCA() : MicroModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS) {}
	void step();
};


// ===========================================================================================================

void VCA::step() {
	float v = inputs[IN_INPUT].value * params[LEVEL_PARAM].value;
	if (inputs[LIN_INPUT].active)
		v *= clampf(inputs[LIN_INPUT].value / 10.0, 0.0, 1.0);
	const float expBase = 50.0;
	if (inputs[EXP_INPUT].active)
		v *= rescalef(powf(expBase, clampf(inputs[EXP_INPUT].value / 10.0, 0.0, 1.0)), 1.0, expBase, 0.0, 1.0);
	outputs[OUT_OUTPUT].value = v;
}


// ===========================================================================================================

struct VCABank : Module
{
	std::array<VCA, GTX__N> inst;

	VCABank()
	:
		Module(VCA::NUM_PARAMS,
			(GTX__N+1) * (VCA::NUM_INPUTS  - VCA::OFF_INPUTS ) + VCA::OFF_INPUTS,
			(GTX__N  ) * (VCA::NUM_OUTPUTS - VCA::OFF_OUTPUTS) + VCA::OFF_OUTPUTS)
	{}

	static constexpr std::size_t imap(std::size_t port, std::size_t bank)
	{
	//	return (port < VCA::OFF_INPUTS)  ? port : port + bank * (VCA::NUM_INPUTS  - VCA::OFF_INPUTS);
		return                                    port + bank *  VCA::NUM_INPUTS;
	}

	static constexpr std::size_t omap(std::size_t port, std::size_t bank)
	{
		return (port < VCA::OFF_OUTPUTS) ? port : port + bank * (VCA::NUM_OUTPUTS - VCA::OFF_OUTPUTS);
	}

	void step() override
	{
		for (std::size_t i=0; i<GTX__N; ++i)
		{
			for (std::size_t p=0; p<VCA::NUM_PARAMS;  ++p) inst[i].params[p]  = params[p];
			for (std::size_t p=0; p<VCA::NUM_INPUTS;  ++p) inst[i].inputs[p]  = inputs[imap(p, i)].active ? inputs[imap(p, i)] : inputs[imap(p, GTX__N)];
			for (std::size_t p=0; p<VCA::NUM_OUTPUTS; ++p) inst[i].outputs[p] = outputs[omap(p, i)];

			inst[i].step();

			for (std::size_t p=0; p<VCA::NUM_OUTPUTS; ++p) outputs[omap(p, i)].value = inst[i].outputs[p].value;
		}

		float mono  = 0.0f;
		float left  = 0.0f;
		float right = 0.0f;

		for (std::size_t i=0; i<GTX__N; ++i)
		{
			mono  += inst[i].outputs[VCA::OUT_OUTPUT].value;
			left  += inst[i].outputs[VCA::OUT_OUTPUT].value;  // TODO add pan
			right += inst[i].outputs[VCA::OUT_OUTPUT].value;  // TODO add pan
		}

		outputs[VCA::MONO_OUTPUT] .value =  mono  * params[VCA::MONO_PARAM]  .value;
		outputs[VCA::LEFT_OUTPUT] .value =  left  * params[VCA::STEREO_PARAM].value;
		outputs[VCA::RIGHT_OUTPUT].value =  right * params[VCA::STEREO_PARAM].value;
	}
};


// ===========================================================================================================

VCAWidget::VCAWidget()
{
	VCABank *module = new VCABank();
	setModule(module);
	box.size = Vec(12*15, 380);

	#if GTX__SAVE_SVG
	{
		PanelGen pg(assetPlugin(plugin, "res/VCA-F1.svg"), box.size, "VCA-F1");

		pg.nob_big(0, 0, "LEVEL");

		pg.bus_in (0, 1, "EXP"); pg.bus_in (1, 1, "LIN");
		pg.bus_in (0, 2, "IN");  pg.bus_out(1, 2, "OUT");
	}
	#endif

	{
		#if GTX__LOAD_SVG
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/VCA-F1.svg")));
		#else
		Panel *panel = new LightPanel();
		panel->box.size = box.size;
		panel->backgroundImage = Image::load(assetPlugin(plugin, "res/VCA-F1.png"));
		#endif
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

	int r7 = 103;

	int d3, d4;
	{
		int size = 36;
		int gap  = (box.size.x - 4 * size) / 6;
		int span = gap + size;
		d3 = box.size.x - 2 * span;
		d4 = box.size.x - 1 * span;
	}

	addParam(createParam<RoundHugeBlackKnob>(n_b(fx(0), fy(0)), module, VCA::LEVEL_PARAM,  0.0, 1.0, 0.5));

	// Mixer
	addParam(createParam<RoundBlackKnob>(Vec(d3, r7 - 42), module, VCA::MONO_PARAM,   0.0, 1.0, 0.5));
	addParam(createParam<RoundBlackKnob>(Vec(d4, r7 - 42), module, VCA::STEREO_PARAM, 0.0, 1.0, 0.5));

	addOutput(createOutput<PJ301MPort>(prt(px(1, 4)-3, py(0, 2)), module, VCA::MONO_OUTPUT));
	addOutput(createOutput<PJ301MPort>(prt(px(1, 0)  , py(0, 2)), module, VCA::LEFT_OUTPUT));
	addOutput(createOutput<PJ301MPort>(prt(px(1, 1)+3, py(0, 2)), module, VCA::RIGHT_OUTPUT));

	for (std::size_t i=0; i<GTX__N; ++i)
	{
		addInput (createInput <PJ301MPort>(prt(px(1, i), py(1, i)), module, VCABank::imap(VCA::LIN_INPUT,  i)));
		addInput (createInput <PJ301MPort>(prt(px(0, i), py(1, i)), module, VCABank::imap(VCA::EXP_INPUT,  i)));
		addInput (createInput <PJ301MPort>(prt(px(0, i), py(2, i)), module, VCABank::imap(VCA::IN_INPUT,   i)));

		addOutput(createOutput<PJ301MPort>(prt(px(1, i), py(2, i)), module, VCABank::omap(VCA::OUT_OUTPUT, i)));
	}

	addInput (createInput <PJ301MPort>(prt(gx(1), gy(1)), module, VCABank::imap(VCA::LIN_INPUT, GTX__N)));
	addInput (createInput <PJ301MPort>(prt(gx(0), gy(1)), module, VCABank::imap(VCA::EXP_INPUT, GTX__N)));
	addInput (createInput <PJ301MPort>(prt(gx(0), gy(2)), module, VCABank::imap(VCA::IN_INPUT,  GTX__N)));
}
