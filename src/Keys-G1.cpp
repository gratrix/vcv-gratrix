//============================================================================================================
//!
//! \file Keys-G1.cpp
//!
//! \brief Keys-G1 is a six input times six voice note monitoring module.
//!
//============================================================================================================


#include "Gratrix.hpp"


namespace GTX {
namespace Keys_G1 {


//============================================================================================================
//! \brief The implementation.

struct Impl : Module
{
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		GATE_1R_INPUT,  // N+1
		GATE_1G_INPUT,  // N+1
		GATE_1B_INPUT,  // N+1
		GATE_2R_INPUT,  // N+1
		GATE_2G_INPUT,  // N+1
		GATE_2B_INPUT,  // N+1
		VOCT_1R_INPUT,  // N
		VOCT_1G_INPUT,  // N
		VOCT_1B_INPUT,  // N
		VOCT_2R_INPUT,  // N
		VOCT_2G_INPUT,  // N
		VOCT_2B_INPUT,  // N
		NUM_INPUTS,
	};
	enum OutputIds {
		NUM_OUTPUTS
	};
	enum LightIds {
		KEY_LIGHT_1 = 0,
		KEY_LIGHT_2 = KEY_LIGHT_1 + 6 * 12 * 3,
		NUM_LIGHTS  = KEY_LIGHT_2 + 6 * 12 * 3
	};

	static constexpr std::size_t imap(std::size_t port, std::size_t bank)
	{
		return port + bank * NUM_INPUTS;
	}

	static void decode(float *lights, int offset, const Input &in_gate, const Input &in_voct)
	{
		bool enable = ((in_gate.active && in_gate.value >= 0.5f) || !in_gate.active) && in_voct.active;

		if (enable)
		{
			int note = static_cast<int>(std::floor(in_voct.value * 12.0f + 0.5f)) + 3*12;

			if (note >= 0 && note < 6*12)
			{
				lights[note * 3 + offset] = 1.0f;
			}
		}
	}

	Impl()
	:
		Module(NUM_PARAMS, ((GTX__N+1) * NUM_INPUTS/2) + (GTX__N * NUM_INPUTS/2), NUM_OUTPUTS, NUM_LIGHTS)
	{}

	void step() override
	{
		float leds[NUM_LIGHTS] = {};

		for (std::size_t i=0; i<GTX__N; ++i)
		{
			if (inputs[imap(GATE_1R_INPUT, i)].active) decode(&leds[KEY_LIGHT_1], 0, inputs[imap(GATE_1R_INPUT,      i)], inputs[imap(VOCT_1R_INPUT, i)]);
			else                                       decode(&leds[KEY_LIGHT_1], 0, inputs[imap(GATE_1R_INPUT, GTX__N)], inputs[imap(VOCT_1R_INPUT, i)]);
			if (inputs[imap(GATE_1G_INPUT, i)].active) decode(&leds[KEY_LIGHT_1], 1, inputs[imap(GATE_1G_INPUT,      i)], inputs[imap(VOCT_1G_INPUT, i)]);
			else                                       decode(&leds[KEY_LIGHT_1], 1, inputs[imap(GATE_1G_INPUT, GTX__N)], inputs[imap(VOCT_1G_INPUT, i)]);
			if (inputs[imap(GATE_1B_INPUT, i)].active) decode(&leds[KEY_LIGHT_1], 2, inputs[imap(GATE_1B_INPUT,      i)], inputs[imap(VOCT_1B_INPUT, i)]);
			else                                       decode(&leds[KEY_LIGHT_1], 2, inputs[imap(GATE_1B_INPUT, GTX__N)], inputs[imap(VOCT_1B_INPUT, i)]);
			if (inputs[imap(GATE_2R_INPUT, i)].active) decode(&leds[KEY_LIGHT_2], 0, inputs[imap(GATE_2R_INPUT,      i)], inputs[imap(VOCT_2R_INPUT, i)]);
			else                                       decode(&leds[KEY_LIGHT_2], 0, inputs[imap(GATE_2R_INPUT, GTX__N)], inputs[imap(VOCT_2R_INPUT, i)]);
			if (inputs[imap(GATE_2G_INPUT, i)].active) decode(&leds[KEY_LIGHT_2], 1, inputs[imap(GATE_2G_INPUT,      i)], inputs[imap(VOCT_2G_INPUT, i)]);
			else                                       decode(&leds[KEY_LIGHT_2], 1, inputs[imap(GATE_2G_INPUT, GTX__N)], inputs[imap(VOCT_2G_INPUT, i)]);
			if (inputs[imap(GATE_2B_INPUT, i)].active) decode(&leds[KEY_LIGHT_2], 2, inputs[imap(GATE_2B_INPUT,      i)], inputs[imap(VOCT_2B_INPUT, i)]);
			else                                       decode(&leds[KEY_LIGHT_2], 2, inputs[imap(GATE_2B_INPUT, GTX__N)], inputs[imap(VOCT_2B_INPUT, i)]);
		}

		// Write output in one go, seems to prevent flicker

		for (std::size_t i=0; i<NUM_LIGHTS; ++i)
		{
			lights[i].value = leds[i];
		}
	}
};


//============================================================================================================
//! \brief The widget.

Widget::Widget()
{
	GTX__WIDGET();

	Impl *module = new Impl();
	setModule(module);
	box.size = Vec(36*15, 380);

	#if GTX__SAVE_SVG
	{
		PanelGen pg(assetPlugin(plugin, "build/res/Keys-G1.svg"), box.size, "KEYS-G1");

		pg.line(Vec(fx(0-.4), fy(0.36)), Vec(fx(2+.4), fy(0.36)), "fill:none;stroke:#7092BE;stroke-width:1");
		pg.line(Vec(fx(3-.4), fy(0.36)), Vec(fx(5+.4), fy(0.36)), "fill:none;stroke:#7092BE;stroke-width:1");

		                              pg.nob_med(0, 0.7, "RED"  ); pg.nob_med(0, -0.28, "C1-B1");
		pg.nob_med(1, 0.55, "UPPER"); pg.nob_med(1, 0.7, "GREEN"); pg.nob_med(1, -0.28, "C2-B2");
		                              pg.nob_med(2, 0.7, "BLUE" ); pg.nob_med(2, -0.28, "C3-B3");
		                              pg.nob_med(3, 0.7, "RED"  ); pg.nob_med(3, -0.28, "C4-B4");
		pg.nob_med(4, 0.55, "LOWER"); pg.nob_med(4, 0.7, "GREEN"); pg.nob_med(4, -0.28, "C5-B5");
		                              pg.nob_med(5, 0.7, "BLUE" ); pg.nob_med(5, -0.28, "C6-B6");

		pg.bus_in(0, 1, "VOCT"); pg.bus_in(0, 2, "V/OCT");
		pg.bus_in(1, 1, "VOCT"); pg.bus_in(1, 2, "V/OCT");
		pg.bus_in(2, 1, "VOCT"); pg.bus_in(2, 2, "V/OCT");
		pg.bus_in(3, 1, "VOCT"); pg.bus_in(3, 2, "V/OCT");
		pg.bus_in(4, 1, "VOCT"); pg.bus_in(4, 2, "V/OCT");
		pg.bus_in(5, 1, "VOCT"); pg.bus_in(5, 2, "V/OCT");
	}
	#endif

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Keys-G1.svg")));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

	for (std::size_t i=0; i<GTX__N; ++i)
	{
		addInput(createInput<PJ301MPort>(prt(px(0, i), py(1, i)), module, Impl::imap(Impl::GATE_1R_INPUT, i)));
		addInput(createInput<PJ301MPort>(prt(px(1, i), py(1, i)), module, Impl::imap(Impl::GATE_1G_INPUT, i)));
		addInput(createInput<PJ301MPort>(prt(px(2, i), py(1, i)), module, Impl::imap(Impl::GATE_1B_INPUT, i)));
		addInput(createInput<PJ301MPort>(prt(px(3, i), py(1, i)), module, Impl::imap(Impl::GATE_2R_INPUT, i)));
		addInput(createInput<PJ301MPort>(prt(px(4, i), py(1, i)), module, Impl::imap(Impl::GATE_2G_INPUT, i)));
		addInput(createInput<PJ301MPort>(prt(px(5, i), py(1, i)), module, Impl::imap(Impl::GATE_2B_INPUT, i)));

		addInput(createInput<PJ301MPort>(prt(px(0, i), py(2, i)), module, Impl::imap(Impl::VOCT_1R_INPUT, i)));
		addInput(createInput<PJ301MPort>(prt(px(1, i), py(2, i)), module, Impl::imap(Impl::VOCT_1G_INPUT, i)));
		addInput(createInput<PJ301MPort>(prt(px(2, i), py(2, i)), module, Impl::imap(Impl::VOCT_1B_INPUT, i)));
		addInput(createInput<PJ301MPort>(prt(px(3, i), py(2, i)), module, Impl::imap(Impl::VOCT_2R_INPUT, i)));
		addInput(createInput<PJ301MPort>(prt(px(4, i), py(2, i)), module, Impl::imap(Impl::VOCT_2G_INPUT, i)));
		addInput(createInput<PJ301MPort>(prt(px(5, i), py(2, i)), module, Impl::imap(Impl::VOCT_2B_INPUT, i)));
	}

	addInput(createInput<PJ301MPort>(prt(gx(0), gy(1)), module, Impl::imap(Impl::GATE_1R_INPUT, GTX__N)));
	addInput(createInput<PJ301MPort>(prt(gx(1), gy(1)), module, Impl::imap(Impl::GATE_1G_INPUT, GTX__N)));
	addInput(createInput<PJ301MPort>(prt(gx(2), gy(1)), module, Impl::imap(Impl::GATE_1B_INPUT, GTX__N)));
	addInput(createInput<PJ301MPort>(prt(gx(3), gy(1)), module, Impl::imap(Impl::GATE_2R_INPUT, GTX__N)));
	addInput(createInput<PJ301MPort>(prt(gx(4), gy(1)), module, Impl::imap(Impl::GATE_2G_INPUT, GTX__N)));
	addInput(createInput<PJ301MPort>(prt(gx(5), gy(1)), module, Impl::imap(Impl::GATE_2B_INPUT, GTX__N)));

	for (std::size_t i=0; i<6; ++i)
	{
		addChild(createLight<SmallLight<RedGreenBlueLight>>(led(gx(i) - 30, fy(0+0.08) + 5), module, Impl::KEY_LIGHT_2 + 3 * (i * 12 +  0)));  // C
		addChild(createLight<SmallLight<RedGreenBlueLight>>(led(gx(i) - 25, fy(0+0.08) - 5), module, Impl::KEY_LIGHT_2 + 3 * (i * 12 +  1)));  // C#
		addChild(createLight<SmallLight<RedGreenBlueLight>>(led(gx(i) - 20, fy(0+0.08) + 5), module, Impl::KEY_LIGHT_2 + 3 * (i * 12 +  2)));  // D
		addChild(createLight<SmallLight<RedGreenBlueLight>>(led(gx(i) - 15, fy(0+0.08) - 5), module, Impl::KEY_LIGHT_2 + 3 * (i * 12 +  3)));  // Eb
		addChild(createLight<SmallLight<RedGreenBlueLight>>(led(gx(i) - 10, fy(0+0.08) + 5), module, Impl::KEY_LIGHT_2 + 3 * (i * 12 +  4)));  // E
		addChild(createLight<SmallLight<RedGreenBlueLight>>(led(gx(i)     , fy(0+0.08) + 5), module, Impl::KEY_LIGHT_2 + 3 * (i * 12 +  5)));  // F
		addChild(createLight<SmallLight<RedGreenBlueLight>>(led(gx(i) +  5, fy(0+0.08) - 5), module, Impl::KEY_LIGHT_2 + 3 * (i * 12 +  6)));  // Fs
		addChild(createLight<SmallLight<RedGreenBlueLight>>(led(gx(i) + 10, fy(0+0.08) + 5), module, Impl::KEY_LIGHT_2 + 3 * (i * 12 +  7)));  // G
		addChild(createLight<SmallLight<RedGreenBlueLight>>(led(gx(i) + 15, fy(0+0.08) - 5), module, Impl::KEY_LIGHT_2 + 3 * (i * 12 +  8)));  // Ab
		addChild(createLight<SmallLight<RedGreenBlueLight>>(led(gx(i) + 20, fy(0+0.08) + 5), module, Impl::KEY_LIGHT_2 + 3 * (i * 12 +  9)));  // A
		addChild(createLight<SmallLight<RedGreenBlueLight>>(led(gx(i) + 25, fy(0+0.08) - 5), module, Impl::KEY_LIGHT_2 + 3 * (i * 12 + 10)));  // Bb
		addChild(createLight<SmallLight<RedGreenBlueLight>>(led(gx(i) + 30, fy(0+0.08) + 5), module, Impl::KEY_LIGHT_2 + 3 * (i * 12 + 11)));  // B
	}

	for (std::size_t i=0; i<6; ++i)
	{
		addChild(createLight<SmallLight<RedGreenBlueLight>>(led(gx(i) - 30, fy(0-0.28) + 5), module, Impl::KEY_LIGHT_1 + 3 * (i * 12 +  0)));  // C
		addChild(createLight<SmallLight<RedGreenBlueLight>>(led(gx(i) - 25, fy(0-0.28) - 5), module, Impl::KEY_LIGHT_1 + 3 * (i * 12 +  1)));  // C#
		addChild(createLight<SmallLight<RedGreenBlueLight>>(led(gx(i) - 20, fy(0-0.28) + 5), module, Impl::KEY_LIGHT_1 + 3 * (i * 12 +  2)));  // D
		addChild(createLight<SmallLight<RedGreenBlueLight>>(led(gx(i) - 15, fy(0-0.28) - 5), module, Impl::KEY_LIGHT_1 + 3 * (i * 12 +  3)));  // Eb
		addChild(createLight<SmallLight<RedGreenBlueLight>>(led(gx(i) - 10, fy(0-0.28) + 5), module, Impl::KEY_LIGHT_1 + 3 * (i * 12 +  4)));  // E
		addChild(createLight<SmallLight<RedGreenBlueLight>>(led(gx(i)     , fy(0-0.28) + 5), module, Impl::KEY_LIGHT_1 + 3 * (i * 12 +  5)));  // F
		addChild(createLight<SmallLight<RedGreenBlueLight>>(led(gx(i) +  5, fy(0-0.28) - 5), module, Impl::KEY_LIGHT_1 + 3 * (i * 12 +  6)));  // Fs
		addChild(createLight<SmallLight<RedGreenBlueLight>>(led(gx(i) + 10, fy(0-0.28) + 5), module, Impl::KEY_LIGHT_1 + 3 * (i * 12 +  7)));  // G
		addChild(createLight<SmallLight<RedGreenBlueLight>>(led(gx(i) + 15, fy(0-0.28) - 5), module, Impl::KEY_LIGHT_1 + 3 * (i * 12 +  8)));  // Ab
		addChild(createLight<SmallLight<RedGreenBlueLight>>(led(gx(i) + 20, fy(0-0.28) + 5), module, Impl::KEY_LIGHT_1 + 3 * (i * 12 +  9)));  // A
		addChild(createLight<SmallLight<RedGreenBlueLight>>(led(gx(i) + 25, fy(0-0.28) - 5), module, Impl::KEY_LIGHT_1 + 3 * (i * 12 + 10)));  // Bb
		addChild(createLight<SmallLight<RedGreenBlueLight>>(led(gx(i) + 30, fy(0-0.28) + 5), module, Impl::KEY_LIGHT_1 + 3 * (i * 12 + 11)));  // B
	}
}


} // Keys_G1
} // GTX
