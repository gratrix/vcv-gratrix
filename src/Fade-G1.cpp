//============================================================================================================
//!
//! \file Fade-G1.cpp
//!
//! \brief Fade-G1 is a two input six voice one-dimensional fader.
//!
//============================================================================================================


#include "Gratrix.hpp"


namespace GTX {
namespace Fade_G1 {


// ===========================================================================================================
//! \brief The implementation.

struct Impl : Module
{
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
		IN_1_GREEN,  IN_1_RED,
		IN_2_GREEN,  IN_2_RED,
		OUT_1_GREEN, OUT_1_RED,
		OUT_2_GREEN, OUT_2_RED,
		NUM_LIGHTS
	};

	Impl()
	:
		Module(NUM_PARAMS,
		(GTX__N+1) * (NUM_INPUTS  - OFF_INPUTS ) + OFF_INPUTS,
		(GTX__N  ) * (NUM_OUTPUTS - OFF_OUTPUTS) + OFF_OUTPUTS,
		NUM_LIGHTS)
	{
		lights[IN_1_GREEN].value = 0.0f;  lights[IN_1_RED].value = 1.0f;
		lights[IN_2_GREEN].value = 1.0f;  lights[IN_2_RED].value = 0.0f;
	}

	static constexpr std::size_t imap(std::size_t port, std::size_t bank)
	{
		return (port < OFF_INPUTS) ? port : port + bank * (NUM_INPUTS - OFF_INPUTS);
	}

	static constexpr std::size_t omap(std::size_t port, std::size_t bank)
	{
		return port + bank * NUM_OUTPUTS;
	}

	void step() override
	{
		float blend12 = params[BLEND12_PARAM].value;

		if (inputs[BLEND12_INPUT].active) blend12 *= clampf(inputs[BLEND12_INPUT].normalize(10.0) / 10.0, 0.0, 1.0);

		for (std::size_t i=0; i<GTX__N; ++i)
		{
			float input1 = inputs[imap(IN1_INPUT, i)].active ? inputs[imap(IN1_INPUT, i)].value : inputs[imap(IN1_INPUT, GTX__N)].value;
			float input2 = inputs[imap(IN2_INPUT, i)].active ? inputs[imap(IN2_INPUT, i)].value : inputs[imap(IN2_INPUT, GTX__N)].value;

			float delta12 = blend12 * (input2 - input1);

			outputs[omap(OUT1_OUTPUT, i)].value = input1 + delta12;
			outputs[omap(OUT2_OUTPUT, i)].value = input2 - delta12;
		}

		lights[OUT_1_GREEN].value = lights[OUT_2_RED].value =       blend12;
		lights[OUT_2_GREEN].value = lights[OUT_1_RED].value = 1.0 - blend12;
	}
};


// ===========================================================================================================
//! \brief The widget.

Widget::Widget()
{
	GTX__WIDGET();

	Impl *module = new Impl();
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

	addParam(createParam<RoundHugeBlackKnob>(n_b(fx(1), fy(0)), module, Impl::BLEND12_PARAM, 0.0, 1.0, 0.0));

	addInput(createInput<PJ301MPort>(prt(fx(0), fy(0)), module, Impl::BLEND12_INPUT));

	for (std::size_t i=0; i<GTX__N; ++i)
	{
		addInput (createInput<PJ301MPort> (prt(px(0, i), py(1, i)), module, Impl::imap(Impl::IN1_INPUT,   i)));
		addInput (createInput<PJ301MPort> (prt(px(0, i), py(2, i)), module, Impl::imap(Impl::IN2_INPUT,   i)));
		addOutput(createOutput<PJ301MPort>(prt(px(1, i), py(1, i)), module, Impl::omap(Impl::OUT1_OUTPUT, i)));
		addOutput(createOutput<PJ301MPort>(prt(px(1, i), py(2, i)), module, Impl::omap(Impl::OUT2_OUTPUT, i)));
	}

	addInput(createInput<PJ301MPort>(prt(gx(0), gy(1)), module, Impl::imap(Impl::IN1_INPUT, GTX__N)));
	addInput(createInput<PJ301MPort>(prt(gx(0), gy(2)), module, Impl::imap(Impl::IN2_INPUT, GTX__N)));

	for (std::size_t i=0, x=0; x<2; ++x)
	{
		for (std::size_t y=0; y<2; ++y)
		{
			addChild(createLight<SmallLight<GreenRedLight>>(led(gx(x)+28, gy(y+1)-46), module, i)); i+=2;
		}
	}
}


} // Fade_G1
} // GTX
