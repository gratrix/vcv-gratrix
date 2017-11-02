#include "Gratrix.hpp"

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


// ===========================================================================================================

struct Keys : Module
{
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		VOCT_INPUT = 0,
		NUM_INPUTS = VOCT_INPUT + 1
	};
	enum OutputIds {
		NUM_OUTPUTS
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
		float led   = 0.0f;

		void step(float input)
		{
			int safe, fnote;

			in    = input;
			fnote = std::floor(in * e + 0.5f);
			out   = fnote * s;
			note  = static_cast<int>(fnote);
			safe  = note + (E * 1000);  // push away from negative numbers
			key   = safe % E;
			oct   = safe / E;
			led   = 1.0f; // (oct & 1) ? -1.0f : 1.0f;
			oct  -= 1000;
		}
	};


	Decode input;

	Keys()
	:
		Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS)
	{}

	void step() override
	{
		input.step(inputs[VOCT_INPUT].value);

		// Lights

		for (auto &light : lights) light.value = 0.0f;

		lights[KEY_LIGHT + input.key].value = input.led;
	}
};


KeysWidget::KeysWidget()
{
	GTX__WIDGET();

	Keys *module = new Keys();
	setModule(module);
	box.size = Vec(36*15, 380);

	#if GTX__SAVE_SVG
	{
		PanelGen pg(assetPlugin(plugin, "build/res/Keys.svg"), box.size, "KEYS");
	}
	#endif

	{
		#if GTX__LOAD_SVG
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Keys.svg")));
		#else
		Panel *panel = new LightPanel();
		panel->box.size = box.size;
		panel->backgroundImage = Image::load(assetPlugin(plugin, "res/Keys.png"));
		#endif
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

	addInput(createInput<PJ301MPort>(prt(gx(0), gy(1)), module, Keys::VOCT_INPUT));

	addChild(createLight<SmallLight<RedLight>>(led(gx(0.5) - 30, fy(0-0.28) + 5), module, Keys::KEY_LIGHT +  0));  // C
	addChild(createLight<SmallLight<RedLight>>(led(gx(0.5) - 25, fy(0-0.28) - 5), module, Keys::KEY_LIGHT +  1));  // C#
	addChild(createLight<SmallLight<RedLight>>(led(gx(0.5) - 20, fy(0-0.28) + 5), module, Keys::KEY_LIGHT +  2));  // D
	addChild(createLight<SmallLight<RedLight>>(led(gx(0.5) - 15, fy(0-0.28) - 5), module, Keys::KEY_LIGHT +  3));  // Eb
	addChild(createLight<SmallLight<RedLight>>(led(gx(0.5) - 10, fy(0-0.28) + 5), module, Keys::KEY_LIGHT +  4));  // E
	addChild(createLight<SmallLight<RedLight>>(led(gx(0.5)     , fy(0-0.28) + 5), module, Keys::KEY_LIGHT +  5));  // F
	addChild(createLight<SmallLight<RedLight>>(led(gx(0.5) +  5, fy(0-0.28) - 5), module, Keys::KEY_LIGHT +  6));  // Fs
	addChild(createLight<SmallLight<RedLight>>(led(gx(0.5) + 10, fy(0-0.28) + 5), module, Keys::KEY_LIGHT +  7));  // G
	addChild(createLight<SmallLight<RedLight>>(led(gx(0.5) + 15, fy(0-0.28) - 5), module, Keys::KEY_LIGHT +  8));  // Ab
	addChild(createLight<SmallLight<RedLight>>(led(gx(0.5) + 20, fy(0-0.28) + 5), module, Keys::KEY_LIGHT +  9));  // A
	addChild(createLight<SmallLight<RedLight>>(led(gx(0.5) + 25, fy(0-0.28) - 5), module, Keys::KEY_LIGHT + 10));  // Bb
	addChild(createLight<SmallLight<RedLight>>(led(gx(0.5) + 30, fy(0-0.28) + 5), module, Keys::KEY_LIGHT + 11));  // B
}
