//============================================================================================================
//!
//! \file Fade-G2.cpp
//!
//! \brief Fade-G2 is a four input six voice two-dimensional fader.
//!
//============================================================================================================


#include "Gratrix.hpp"


namespace GTX {
namespace Fade_G2 {


//============================================================================================================
//! \brief The implementation.

struct Impl : Module
{
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
		NUM_INPUTS,
		OFF_INPUTS = IN1A_INPUT
	};
	enum OutputIds {
		OUT1A_OUTPUT,
		OUT2B_OUTPUT,
		NUM_OUTPUTS,
		OFF_OUTPUTS = OUT1A_OUTPUT
	};
	enum LightIds {
		IN_1AP_GREEN,  IN_1AP_RED,
		IN_1AQ_GREEN,  IN_1AQ_RED,
		IN_1BP_GREEN,  IN_1BP_RED,
		IN_1BQ_GREEN,  IN_1BQ_RED,
		IN_2AP_GREEN,  IN_2AP_RED,
		IN_2AQ_GREEN,  IN_2AQ_RED,
		IN_2BP_GREEN,  IN_2BP_RED,
		IN_2BQ_GREEN,  IN_2BQ_RED,
		OUT_1AP_GREEN, OUT_1AP_RED,
		OUT_1AQ_GREEN, OUT_1AQ_RED,
		OUT_2BP_GREEN, OUT_2BP_RED,
		OUT_2BQ_GREEN, OUT_2BQ_RED,
		NUM_LIGHTS
	};

	Impl()
	:
		Module(NUM_PARAMS,
		(GTX__N+1) * (NUM_INPUTS  - OFF_INPUTS ) + OFF_INPUTS,
		(GTX__N  ) * (NUM_OUTPUTS - OFF_OUTPUTS) + OFF_OUTPUTS,
		NUM_LIGHTS)
	{
		lights[IN_1AP_GREEN].value = 0.0f;  lights[IN_1AP_RED].value = 1.0f;
		lights[IN_1AQ_GREEN].value = 0.0f;  lights[IN_1AQ_RED].value = 1.0f;
		lights[IN_1BP_GREEN].value = 1.0f;  lights[IN_1BP_RED].value = 0.0f;
		lights[IN_1BQ_GREEN].value = 0.0f;  lights[IN_1BQ_RED].value = 1.0f;
		lights[IN_2AP_GREEN].value = 0.0f;  lights[IN_2AP_RED].value = 1.0f;
		lights[IN_2AQ_GREEN].value = 1.0f;  lights[IN_2AQ_RED].value = 0.0f;
		lights[IN_2BP_GREEN].value = 1.0f;  lights[IN_2BP_RED].value = 0.0f;
		lights[IN_2BQ_GREEN].value = 1.0f;  lights[IN_2BQ_RED].value = 0.0f;
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
		float blendAB = params[BLENDAB_PARAM].value;

		if (inputs[BLEND12_INPUT].active) blend12 *= clampf(inputs[BLEND12_INPUT].normalize(10.0) / 10.0, 0.0, 1.0);
		if (inputs[BLENDAB_INPUT].active) blendAB *= clampf(inputs[BLENDAB_INPUT].normalize(10.0) / 10.0, 0.0, 1.0);

		for (std::size_t i=0; i<GTX__N; ++i)
		{
			float input1A  = inputs[imap(IN1A_INPUT, i)].active ? inputs[imap(IN1A_INPUT, i)].value : inputs[imap(IN1A_INPUT, GTX__N)].value;
			float input1B  = inputs[imap(IN1B_INPUT, i)].active ? inputs[imap(IN1B_INPUT, i)].value : inputs[imap(IN1B_INPUT, GTX__N)].value;
			float input2A  = inputs[imap(IN2A_INPUT, i)].active ? inputs[imap(IN2A_INPUT, i)].value : inputs[imap(IN2A_INPUT, GTX__N)].value;
			float input2B  = inputs[imap(IN2B_INPUT, i)].active ? inputs[imap(IN2B_INPUT, i)].value : inputs[imap(IN2B_INPUT, GTX__N)].value;

			float delta1AB = blendAB * (input1B - input1A);
			float delta2AB = blendAB * (input2B - input2A);

			float temp_1A  = input1A + delta1AB;
			float temp_1B  = input1B - delta1AB;
			float temp_2A  = input2A + delta2AB;
			float temp_2B  = input2B - delta2AB;

			float delta12A = blend12 * (temp_2A - temp_1A);
			float delta12B = blend12 * (temp_2B - temp_1B);

			outputs[omap(OUT1A_OUTPUT, i)].value = temp_1A + delta12A;
			outputs[omap(OUT2B_OUTPUT, i)].value = temp_2B - delta12B;
		}

		lights[OUT_1AP_GREEN].value = lights[OUT_2BP_RED].value =        blendAB;
		lights[OUT_1AQ_GREEN].value = lights[OUT_2BQ_RED].value =        blend12;
		lights[OUT_2BP_GREEN].value = lights[OUT_1AP_RED].value = 1.0f - blendAB;
		lights[OUT_2BQ_GREEN].value = lights[OUT_1AQ_RED].value = 1.0f - blend12;
	}
};


//============================================================================================================
//! \brief The widget.

Widget::Widget()
{
	GTX__WIDGET();

	Impl *module = new Impl();
	setModule(module);
	box.size = Vec(18*15, 380);

	#if GTX__SAVE_SVG
	{
		PanelGen pg(assetPlugin(plugin, "build/res/Fade-G2.svg"), box.size, "FADE-G2");

		// ― is a horizontal bar see https://en.wikipedia.org/wiki/Dash#Horizontal_bar
		pg.prt_in2(0, -0.28, "CV 1―2");   pg.nob_big(1, 0, "1―2");
		pg.prt_in2(0, +0.28, "CV A―B");   pg.nob_big(2, 0, "A―B");

		pg.bus_in(0, 1, "IN 1A"); pg.bus_out(2, 1, "OUT");
		pg.bus_in(1, 1, "IN 1B");
		pg.bus_in(0, 2, "IN 2A");
		pg.bus_in(1, 2, "IN 2B"); pg.bus_out(2, 2, "INV OUT");
	}
	#endif

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Fade-G2.svg")));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

	addParam(createParamGTX<KnobFreeHug>(Vec(fx(1), fy(0)), module, Impl::BLENDAB_PARAM, 0.0, 1.0, 0.0));
	addParam(createParamGTX<KnobFreeHug>(Vec(fx(2), fy(0)), module, Impl::BLEND12_PARAM, 0.0, 1.0, 0.0));

	addInput(createInput<PJ301MPort>(prt(fx(0), fy(-0.28)), module, Impl::BLENDAB_INPUT));
	addInput(createInput<PJ301MPort>(prt(fx(0), fy(+0.28)), module, Impl::BLEND12_INPUT));

	for (std::size_t i=0; i<GTX__N; ++i)
	{
		addInput (createInput<PJ301MPort> (prt(px(0, i), py(1, i)), module, Impl::imap(Impl::IN1A_INPUT,   i)));
		addInput (createInput<PJ301MPort> (prt(px(0, i), py(2, i)), module, Impl::imap(Impl::IN1B_INPUT,   i)));
		addInput (createInput<PJ301MPort> (prt(px(1, i), py(1, i)), module, Impl::imap(Impl::IN2A_INPUT,   i)));
		addInput (createInput<PJ301MPort> (prt(px(1, i), py(2, i)), module, Impl::imap(Impl::IN2B_INPUT,   i)));
		addOutput(createOutput<PJ301MPort>(prt(px(2, i), py(1, i)), module, Impl::omap(Impl::OUT1A_OUTPUT, i)));
		addOutput(createOutput<PJ301MPort>(prt(px(2, i), py(2, i)), module, Impl::omap(Impl::OUT2B_OUTPUT, i)));
	}

	addInput(createInput<PJ301MPort>(prt(gx(0), gy(1)), module, Impl::imap(Impl::IN1A_INPUT, GTX__N)));
	addInput(createInput<PJ301MPort>(prt(gx(0), gy(2)), module, Impl::imap(Impl::IN1B_INPUT, GTX__N)));
	addInput(createInput<PJ301MPort>(prt(gx(1), gy(1)), module, Impl::imap(Impl::IN2A_INPUT, GTX__N)));
	addInput(createInput<PJ301MPort>(prt(gx(1), gy(2)), module, Impl::imap(Impl::IN2B_INPUT, GTX__N)));

	for (std::size_t i=0, x=0; x<3; ++x)
	{
		for (std::size_t y=0; y<2; ++y)
		{
			addChild(createLight<SmallLight<GreenRedLight>>(l_s(gx(x)+rad_l_s()/2+28, gy(y+1)-47.5-(1+rad_l_s())), module, i)); i+=2;
			addChild(createLight<SmallLight<GreenRedLight>>(l_s(gx(x)+rad_l_s()/2+28, gy(y+1)-47.5+(1+rad_l_s())), module, i)); i+=2;
		}
	}
}


} // Fade_G2
} // GTX
