//============================================================================================================
//!
//! \file Octave-G1.cpp
//!
//! \brief Octave-G1 quantises the input to 12-ET and provides an octaves-worth of output.
//!
//============================================================================================================


#include "Gratrix.hpp"


namespace GTX {
namespace Octave_G1 {


//============================================================================================================
//! \brief Some settings.

enum Spec
{
	LO_BEGIN = -5,    // C-1
	LO_END   =  5,    // C+9
	LO_SIZE  = LO_END - LO_BEGIN + 1,
	E        = 12,    // ET
	N        = 12,    // Number of note outputs
	T        = 2,
	M        = 2*T+1  // Number of octave outputs
};


//============================================================================================================
//! \brief The implementation.

struct Impl : Module
{
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		VOCT_INPUT   = 0,
		NUM_INPUTS = VOCT_INPUT + 1
	};
	enum OutputIds {
		NOTE_OUTPUT = 0,
		OCT_OUTPUT  = NOTE_OUTPUT + N,
		NUM_OUTPUTS = OCT_OUTPUT  + M
	};
	enum LightIds {
		KEY_LIGHT  = 0,
		OCT_LIGHT  = KEY_LIGHT + E,
		NUM_LIGHTS = OCT_LIGHT + LO_SIZE
	};

	struct Decode
	{
		static constexpr float e = static_cast<float>(E);
		static constexpr float s = 1.0f / e;

		float in    = 0;
		float out   = 0;
		int   note  = 0;
		int   key   = 0;
		int   oct   = 0;

		void step(float input)
		{
			int safe, fnote;

			in    = input;
			fnote = std::floor(in * e + 0.5f);
			out   = fnote * s;
			note  = static_cast<int>(fnote);
			safe  = note + (E * 1000);  // push away from negative numbers
			key   = safe % E;
			oct   = (safe / E) - 1000;
		}
	};

	Decode input;

	//--------------------------------------------------------------------------------------------------------
	//! \brief Constructor.

	Impl()
	:
		Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS)
	{}

	//--------------------------------------------------------------------------------------------------------
	//! \brief Step function.

	void step() override
	{
		// Clear all lights

		float leds[NUM_LIGHTS] = {};

		// Decode inputs and params
		input.step(inputs[VOCT_INPUT].value);

		for (std::size_t i=0; i<N; ++i)
		{
			outputs[i + NOTE_OUTPUT].value = input.out + i * input.s;
		}

		for (std::size_t i=0; i<M; ++i)
		{
			outputs[i + OCT_OUTPUT].value = (input.out - T) + i;
		}

		// Lights

		leds[KEY_LIGHT + input.key] = 1.0f;

		if (LO_BEGIN <= input.oct && input.oct <= LO_END)
		{
			leds[OCT_LIGHT + input.oct - LO_BEGIN] = 1.0f;
		}

		// Write output in one go, seems to prevent flicker

		for (std::size_t i=0; i<NUM_LIGHTS; ++i)
		{
			lights[i].value = leds[i];
		}
	}
};


int x(std::size_t i, double radius) { return static_cast<int>(6*15     + 0.5 + radius * dx(i, E)); }
int y(std::size_t i, double radius) { return static_cast<int>(-20+206  + 0.5 + radius * dy(i, E)); }


//============================================================================================================
//! \brief The widget.

Widget::Widget()
{
	GTX__WIDGET();

	Impl *module = new Impl();
	setModule(module);
	box.size = Vec(12*15, 380);

//	double r1 = 30;
	double r2 = 55;

	#if GTX__SAVE_SVG
	{
		PanelGen pg(assetPlugin(plugin, "build/res/Octave-G1.svg"), box.size, "OCTAVE-G1");

		pg.circle(Vec(x(0, 0), y(0, 0)), r2+16, "fill:#7092BE;stroke:none");
		pg.circle(Vec(x(0, 0), y(0, 0)), r2-16, "fill:#CEE1FD;stroke:none");

/*		// Wires
		for (std::size_t i=0; i<N; ++i)
		{
					 pg.line(Vec(x(i,   r1), y(i,   r1)), Vec(x(i, r2), y(i, r2)), "stroke:#440022;stroke-width:1");
			if (i) { pg.line(Vec(x(i-1, r1), y(i-1, r1)), Vec(x(i, r1), y(i, r1)), "stroke:#440022;stroke-width:1"); }
		}
*/
		// Ports
		pg.circle(Vec(x(0, 0), y(0, 0)), 10, "stroke:#440022;stroke-width:1");
		for (std::size_t i=0; i<N; ++i)
		{
			 pg.circle(Vec(x(i, r2), y(i,   r2)), 10, "stroke:#440022;stroke-width:1");
		}

		pg.prt_out(-0.20, 2,          "", "-2");
		pg.prt_out( 0.15, 2,          "", "-1");
		pg.prt_out( 0.50, 2, "TRANSPOSE",  "0");
		pg.prt_out( 0.85, 2,          "", "+1");
		pg.prt_out( 1.20, 2,          "", "+2");
	}
	#endif

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Octave-G1.svg")));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

	addInput(createInput<PJ301MPort>(prt(x(0, 0), y(0, 0)), module, Impl::VOCT_INPUT));
	for (std::size_t i=0; i<N; ++i)
	{
		addOutput(createOutput<PJ301MPort>(prt(x(i, r2), y(i, r2)), module, i + Impl::NOTE_OUTPUT));
	}

	addOutput(createOutput<PJ301MPort>(prt(gx(-0.20), gy(2)), module, 0 + Impl::OCT_OUTPUT));
	addOutput(createOutput<PJ301MPort>(prt(gx( 0.15), gy(2)), module, 1 + Impl::OCT_OUTPUT));
	addOutput(createOutput<PJ301MPort>(prt(gx( 0.50), gy(2)), module, 2 + Impl::OCT_OUTPUT));
	addOutput(createOutput<PJ301MPort>(prt(gx( 0.85), gy(2)), module, 3 + Impl::OCT_OUTPUT));
	addOutput(createOutput<PJ301MPort>(prt(gx( 1.20), gy(2)), module, 4 + Impl::OCT_OUTPUT));

	addChild(createLight<SmallLight<RedLight>>(led(gx(0.5) - 30, fy(0-0.28) + 5), module, Impl::KEY_LIGHT +  0));  // C
	addChild(createLight<SmallLight<RedLight>>(led(gx(0.5) - 25, fy(0-0.28) - 5), module, Impl::KEY_LIGHT +  1));  // C#
	addChild(createLight<SmallLight<RedLight>>(led(gx(0.5) - 20, fy(0-0.28) + 5), module, Impl::KEY_LIGHT +  2));  // D
	addChild(createLight<SmallLight<RedLight>>(led(gx(0.5) - 15, fy(0-0.28) - 5), module, Impl::KEY_LIGHT +  3));  // Eb
	addChild(createLight<SmallLight<RedLight>>(led(gx(0.5) - 10, fy(0-0.28) + 5), module, Impl::KEY_LIGHT +  4));  // E
	addChild(createLight<SmallLight<RedLight>>(led(gx(0.5)     , fy(0-0.28) + 5), module, Impl::KEY_LIGHT +  5));  // F
	addChild(createLight<SmallLight<RedLight>>(led(gx(0.5) +  5, fy(0-0.28) - 5), module, Impl::KEY_LIGHT +  6));  // Fs
	addChild(createLight<SmallLight<RedLight>>(led(gx(0.5) + 10, fy(0-0.28) + 5), module, Impl::KEY_LIGHT +  7));  // G
	addChild(createLight<SmallLight<RedLight>>(led(gx(0.5) + 15, fy(0-0.28) - 5), module, Impl::KEY_LIGHT +  8));  // Ab
	addChild(createLight<SmallLight<RedLight>>(led(gx(0.5) + 20, fy(0-0.28) + 5), module, Impl::KEY_LIGHT +  9));  // A
	addChild(createLight<SmallLight<RedLight>>(led(gx(0.5) + 25, fy(0-0.28) - 5), module, Impl::KEY_LIGHT + 10));  // Bb
	addChild(createLight<SmallLight<RedLight>>(led(gx(0.5) + 30, fy(0-0.28) + 5), module, Impl::KEY_LIGHT + 11));  // B

	for (std::size_t i=0; i<LO_SIZE; ++i)
	{
		addChild(createLight<SmallLight<RedLight>>(led(gx(0.5) + (i - LO_SIZE/2) * 10, fy(0-0.28) + 20), module, Impl::OCT_LIGHT + i));
	}
}


} // Octave_G1
} // GTX
