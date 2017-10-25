#include "Gratrix.hpp"


// ===========================================================================================================

struct Fade02 : MicroModule {
	enum ParamIds {
		BLEND12_PARAM,
		BLENDAB_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		BLEND12_INPUT,
		BLENDAB_INPUT,
		IN1A_INPUT,
		IN1B_INPUT,
		IN2A_INPUT,
		IN2B_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT1A_OUTPUT,
//		OUT1B_OUTPUT,
//		OUT2A_OUTPUT,
		OUT2B_OUTPUT,
		NUM_OUTPUTS
	};

	Fade02()
	:
		MicroModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS)
	{}

	void step(float blend12, float blendAB)
	{
	//	float blend12 = params[BLEND12_INPUT].value * clampf(inputs[BLEND12_INPUT].normalize(10.0) / 10.0, 0.0, 1.0);
	//	float blendAB = params[BLENDAB_INPUT].value * clampf(inputs[BLENDAB_INPUT].normalize(10.0) / 10.0, 0.0, 1.0);

		float temp_1A = inputs[IN1A_INPUT].value * (1.0f - blendAB) + inputs[IN1B_INPUT].value *         blendAB;
		float temp_1B = inputs[IN1A_INPUT].value *         blendAB  + inputs[IN1B_INPUT].value * (1.0f - blendAB);
		float temp_2A = inputs[IN2A_INPUT].value * (1.0f - blendAB) + inputs[IN2B_INPUT].value *         blendAB;
		float temp_2B = inputs[IN2A_INPUT].value *         blendAB  + inputs[IN2B_INPUT].value * (1.0f - blendAB);

		outputs[OUT1A_OUTPUT].value = temp_1A * (1.0f - blend12) + temp_2A *         blend12;
//		outputs[OUT2A_OUTPUT].value = temp_1A *         blend12  + temp_2A * (1.0f - blend12);
//		outputs[OUT1B_OUTPUT].value = temp_1B * (1.0f - blend12) + temp_2B *         blend12;
		outputs[OUT2B_OUTPUT].value = temp_1B *         blend12  + temp_2B * (1.0f - blend12);
	}
};


// ===========================================================================================================

struct Fade02Bank : Module
{
	std::array<Fade02, GTX__N> inst;
	float lights[3][2][2] = {{{-1.0f, -1.0f}, {1.0f, -1.0f}}, {{-1.0f, 1.0f}, {1.0f, 1.0f}}, {{0.0f, 0.0f}, {0.0f, 0.0f}}};

	Fade02Bank()
	:
		Module(Fade02::NUM_PARAMS, Fade02::IN1A_INPUT + (GTX__N+1) * (Fade02::NUM_INPUTS - Fade02::IN1A_INPUT), GTX__N * Fade02::NUM_OUTPUTS)
	{}

	static std::size_t imap(std::size_t port, std::size_t bank)
	{
		if (port < Fade02::IN1A_INPUT)
		{
			return port;
		}

		return port + bank * (Fade02::NUM_INPUTS - Fade02::IN1A_INPUT);
	}

	static std::size_t omap(std::size_t port, std::size_t bank)
	{
		return port + bank * Fade02::NUM_OUTPUTS;
	}

	void step() override
	{
		float blend12 = params[Fade02::BLEND12_INPUT].value * clampf(inputs[Fade02::BLEND12_INPUT].normalize(10.0) / 10.0, 0.0, 1.0);
		float blendAB = params[Fade02::BLENDAB_INPUT].value * clampf(inputs[Fade02::BLENDAB_INPUT].normalize(10.0) / 10.0, 0.0, 1.0);

		for (std::size_t i=0; i<GTX__N; ++i)
		{
		//	for (std::size_t p=0; p<Fade02::NUM_PARAMS;  ++p) inst[i].params[p]  = params[p];
			for (std::size_t p=0; p<Fade02::NUM_INPUTS;  ++p) inst[i].inputs[p]  = inputs[imap(p, i)].active ? inputs[imap(p, i)] : inputs[imap(p, GTX__N)];
			for (std::size_t p=0; p<Fade02::NUM_OUTPUTS; ++p) inst[i].outputs[p] = outputs[omap(p, i)];

			inst[i].step(blend12, blendAB);

			for (std::size_t p=0; p<Fade02::NUM_OUTPUTS; ++p) outputs[omap(p, i)].value = inst[i].outputs[p].value;
		}

		lights[2][0][0] = 2.0f * blendAB - 1.0f;
		lights[2][0][1] = 2.0f * blend12 - 1.0f;
		lights[2][1][0] = -lights[2][0][0];
		lights[2][1][1] = -lights[2][0][1];
	}
};


// ===========================================================================================================

Fade_G2_Widget::Fade_G2_Widget()
{
	Fade02Bank *module = new Fade02Bank();
	setModule(module);
	box.size = Vec(18*15, 380);

	{
		PanelGen pg(assetPlugin(plugin, "res/Fade-G2.svg"), box.size, "FADE-G2");

		pg.prt_in2(0, -0.28, "CV 1--2");   pg.nob_big(1, 0, "1--2");
		pg.prt_in2(0, +0.28, "CV A--B");   pg.nob_big(2, 0, "A--B");

		pg.bus_in(0, 1, "IN 1A"); pg.bus_out(2, 1, "OUT");
		pg.bus_in(1, 1, "IN 1B");
		pg.bus_in(0, 2, "IN 2A");
		pg.bus_in(1, 2, "IN 2B"); pg.bus_out(2, 2, "INV OUT");
	}

	{
	/*	SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Fade-G2.svg")));
		addChild(panel);
	*/
		Panel *panel = new LightPanel();
		panel->box.size = box.size;
		panel->backgroundImage = Image::load(assetPlugin(plugin, "res/Fade-G2.png"));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

	addParam(createParam<RoundHugeBlackKnob>(n_b(fx(1), fy(0)), module, Fade02::BLENDAB_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<RoundHugeBlackKnob>(n_b(fx(2), fy(0)), module, Fade02::BLEND12_PARAM, 0.0, 1.0, 0.0));

	addInput(createInput<PJ301MPort>(prt(fx(0), fy(-0.28)), module, Fade02::BLENDAB_INPUT));
	addInput(createInput<PJ301MPort>(prt(fx(0), fy(+0.28)), module, Fade02::BLEND12_INPUT));

	for (std::size_t i=0; i<GTX__N; ++i)
	{
		addInput (createInput<PJ301MPort> (prt(px(0, i), py(1, i)), module, Fade02Bank::imap(Fade02::IN1A_INPUT,   i)));
		addInput (createInput<PJ301MPort> (prt(px(0, i), py(2, i)), module, Fade02Bank::imap(Fade02::IN1B_INPUT,   i)));
		addInput (createInput<PJ301MPort> (prt(px(1, i), py(1, i)), module, Fade02Bank::imap(Fade02::IN2A_INPUT,   i)));
		addInput (createInput<PJ301MPort> (prt(px(1, i), py(2, i)), module, Fade02Bank::imap(Fade02::IN2B_INPUT,   i)));
		addOutput(createOutput<PJ301MPort>(prt(px(2, i), py(1, i)), module, Fade02Bank::omap(Fade02::OUT1A_OUTPUT, i)));
	//	addOutput(createOutput<PJ301MPort>(prt(px(2, i), py(2, i)), module, Fade02Bank::omap(Fade02::OUT1B_OUTPUT, i)));
	//	addOutput(createOutput<PJ301MPort>(prt(px(3, i), py(1, i)), module, Fade02Bank::omap(Fade02::OUT2A_OUTPUT, i)));
		addOutput(createOutput<PJ301MPort>(prt(px(2, i), py(2, i)), module, Fade02Bank::omap(Fade02::OUT2B_OUTPUT, i)));
	}

	addInput (createInput<PJ301MPort> (prt(gx(0), gy(1)), module, Fade02Bank::imap(Fade02::IN1A_INPUT, GTX__N)));
	addInput (createInput<PJ301MPort> (prt(gx(0), gy(2)), module, Fade02Bank::imap(Fade02::IN1B_INPUT, GTX__N)));
	addInput (createInput<PJ301MPort> (prt(gx(1), gy(1)), module, Fade02Bank::imap(Fade02::IN2A_INPUT, GTX__N)));
	addInput (createInput<PJ301MPort> (prt(gx(1), gy(2)), module, Fade02Bank::imap(Fade02::IN2B_INPUT, GTX__N)));

	for (std::size_t x=0; x<3; ++x)
	{
		for (std::size_t y=0; y<2; ++y)
		{
			addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(led(gx(x)+rad_led()/2+28, gy(y+1)-46-rad_led()), &module->lights[x][y][0]));
			addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(led(gx(x)+rad_led()/2+28, gy(y+1)-46+rad_led()), &module->lights[x][y][1]));
		}
	}
}
