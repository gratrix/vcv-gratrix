#include "Gratrix.hpp"


// ===========================================================================================================

struct Fade01 : MicroModule {
	enum ParamIds {
		BLEND12_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		BLEND12_INPUT,
		IN1_INPUT,
		IN2_INPUT,
		NUM_INPUTS,
		OFF_INPUTS = IN1_INPUT
	};
	enum OutputIds {
		OUT1_OUTPUT,
		OUT2_OUTPUT,
		NUM_OUTPUTS,
		OFF_OUTPUTS = OUT1_OUTPUT
	};
	enum LightIds {
		IN1_LIGHT,
		IN2_LIGHT,
		OUT1_LIGHT,
		OUT2_LIGHT,
		NUM_LIGHTS
	};

	Fade01()
	:
		MicroModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS)
	{}

	void step(float blend12)
	{
	//	float blend12 = params[BLEND12_INPUT].value * clampf(inputs[BLEND12_INPUT].normalize(10.0) / 10.0, 0.0, 1.0);
		float delta12 = blend12 * (inputs[IN2_INPUT].value - inputs[IN1_INPUT].value);

		outputs[OUT1_OUTPUT].value = inputs[IN1_INPUT].value + delta12;
		outputs[OUT2_OUTPUT].value = inputs[IN2_INPUT].value - delta12;
	}
};


// ===========================================================================================================

struct Fade01Bank : Module
{
	std::array<Fade01, GTX__N> inst;

	Fade01Bank()
	:
		Module(Fade01::NUM_PARAMS,
		(GTX__N+1) * (Fade01::NUM_INPUTS  - Fade01::OFF_INPUTS ) + Fade01::OFF_INPUTS,
		(GTX__N  ) * (Fade01::NUM_OUTPUTS - Fade01::OFF_OUTPUTS) + Fade01::OFF_OUTPUTS,
		Fade01::NUM_LIGHTS)
	{
		lights[Fade01::IN1_LIGHT].value = -1.0f;
		lights[Fade01::IN2_LIGHT].value =  1.0f;
	}

	static constexpr std::size_t imap(std::size_t port, std::size_t bank)
	{
		return (port < Fade01::OFF_INPUTS)  ? port : port + bank * (Fade01::NUM_INPUTS  - Fade01::OFF_INPUTS);
	}

	static constexpr std::size_t omap(std::size_t port, std::size_t bank)
	{
	//	return (port < Fade01::OFF_OUTPUTS) ? port : port + bank * (Fade01::NUM_OUTPUTS - Fade01::OFF_OUTPUTS);
		return                                       port + bank *  Fade01::NUM_OUTPUTS;
	}

	void step() override
	{
		float blend12 = params[Fade01::BLEND12_INPUT].value * clampf(inputs[Fade01::BLEND12_INPUT].normalize(10.0) / 10.0, 0.0, 1.0);

		for (std::size_t i=0; i<GTX__N; ++i)
		{
		//	for (std::size_t p=0; p<Fade01::NUM_PARAMS;  ++p) inst[i].params[p]  = params[p];
			for (std::size_t p=0; p<Fade01::NUM_INPUTS;  ++p) inst[i].inputs[p]  = inputs[imap(p, i)].active ? inputs[imap(p, i)] : inputs[imap(p, GTX__N)];
			for (std::size_t p=0; p<Fade01::NUM_OUTPUTS; ++p) inst[i].outputs[p] = outputs[omap(p, i)];

			inst[i].step(blend12);

			for (std::size_t p=0; p<Fade01::NUM_OUTPUTS; ++p) outputs[omap(p, i)].value = inst[i].outputs[p].value;
		}

		lights[Fade01::OUT1_LIGHT].value = 2.0f * blend12 - 1.0f;
		lights[Fade01::OUT2_LIGHT].value = -lights[Fade01::OUT1_LIGHT].value;
	}
};


// ===========================================================================================================

Fade_G1_Widget::Fade_G1_Widget()
{
	GTX__WIDGET();

	Fade01Bank *module = new Fade01Bank();
	setModule(module);
	box.size = Vec(12*15, 380);

	#if GTX__SAVE_SVG
	{
		PanelGen pg(assetPlugin(plugin, "build/res/Fade-G1.svg"), box.size, "FADE-G1");

		pg.prt_in2(0, 0, "CV 1--2");   pg.nob_big(1, 0, "1--2");

		pg.bus_in(0, 1, "IN 1"); pg.bus_out(1, 1, "OUT 1");
		pg.bus_in(0, 2, "IN 2"); pg.bus_out(1, 2, "OUT 2");
	}
	#endif

	{
		#if GTX__LOAD_SVG
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Fade-G1.svg")));
		#else
		Panel *panel = new LightPanel();
		panel->box.size = box.size;
		panel->backgroundImage = Image::load(assetPlugin(plugin, "res/Fade-G1.png"));
		#endif
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

	addParam(createParam<RoundHugeBlackKnob>(n_b(fx(1), fy(0)), module, Fade01::BLEND12_PARAM, 0.0, 1.0, 0.0));

	addInput(createInput<PJ301MPort>(prt(fx(0), fy(0)), module, Fade01::BLEND12_INPUT));

	for (std::size_t i=0; i<GTX__N; ++i)
	{
		addInput (createInput<PJ301MPort> (prt(px(0, i), py(1, i)), module, Fade01Bank::imap(Fade01::IN1_INPUT,   i)));
		addInput (createInput<PJ301MPort> (prt(px(0, i), py(2, i)), module, Fade01Bank::imap(Fade01::IN2_INPUT,   i)));
		addOutput(createOutput<PJ301MPort>(prt(px(1, i), py(1, i)), module, Fade01Bank::omap(Fade01::OUT1_OUTPUT, i)));
		addOutput(createOutput<PJ301MPort>(prt(px(1, i), py(2, i)), module, Fade01Bank::omap(Fade01::OUT2_OUTPUT, i)));
	}

	addInput (createInput<PJ301MPort> (prt(gx(0), gy(1)), module, Fade01Bank::imap(Fade01::IN1_INPUT, GTX__N)));
	addInput (createInput<PJ301MPort> (prt(gx(0), gy(2)), module, Fade01Bank::imap(Fade01::IN2_INPUT, GTX__N)));

	for (std::size_t i=0, x=0; x<2; ++x)
	{
		for (std::size_t y=0; y<2; ++y)
		{
			addChild(createLight<SmallLight<GreenRedLight>>(led(gx(x)+28, gy(y+1)-46), module, i++));
		}
	}
}
