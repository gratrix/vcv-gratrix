#include "Gratrix.hpp"


// ===========================================================================================================

struct VU_G1 : Module
{
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		IN1_INPUT,      // N+1
		NUM_INPUTS
	};
	enum OutputIds {
		NUM_OUTPUTS,
	};
	enum LightIds {
		NUM_LIGHTS = 10  // N
	};

	VU_G1()
	:
		Module(NUM_PARAMS, (GTX__N+1) * NUM_INPUTS, NUM_OUTPUTS, GTX__N * NUM_LIGHTS)
	{
	}

	static constexpr std::size_t imap(std::size_t port, std::size_t bank)
	{
		return port + bank * NUM_INPUTS;
	}

	void step() override
	{
		for (std::size_t i=0; i<GTX__N; ++i)
		{
			float input = inputs[imap(IN1_INPUT, i)].active ? inputs[imap(IN1_INPUT, i)].value : inputs[imap(IN1_INPUT, GTX__N)].value;
			float dB    = logf(fabsf(input * 0.1f)) * (10.0f / logf(20.0f));

			for (int j = 0; j < NUM_LIGHTS; j++)
			{
				float b = clampf(dB / 3.0f + 1.0f + j, 0.0f, 1.0f);

				lights[NUM_LIGHTS * i + j].setBrightnessSmooth(b * 0.9f);
			}
		}
	}
};


// ===========================================================================================================

VU_G1_Widget::VU_G1_Widget()
{
	GTX__WIDGET();

	VU_G1 *module = new VU_G1();
	setModule(module);
	box.size = Vec(6*15, 380);

	#if GTX__SAVE_SVG
	{
		PanelGen pg(assetPlugin(plugin, "build/res/VU-G1.svg"), box.size, "VU-G1");

		pg.nob_big(0, 0, "VOLUME");
		pg.bus_in (0, 2, "IN");
	}
	#endif

	{
		#if GTX__LOAD_SVG
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/VU-G1.svg")));
		#else
		Panel *panel = new LightPanel();
		panel->box.size = box.size;
		panel->backgroundImage = Image::load(assetPlugin(plugin, "res/VU-G1.png"));
		#endif
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

	for (std::size_t i=0; i<GTX__N; ++i)
	{
		addInput(createInput<PJ301MPort>(prt(px(0, i), py(2, i)), module, VU_G1::imap(VU_G1::IN1_INPUT, i)));
	}

	addInput(createInput<PJ301MPort>(prt(gx(0), gy(2)), module, VU_G1::imap(VU_G1::IN1_INPUT, GTX__N)));

	for (std::size_t i=0, k=0; i<GTX__N; ++i)
	{
		for (std::size_t j=0; j<VU_G1::NUM_LIGHTS; ++j, ++k)
		{
			switch (j)
			{
				case 0  : addChild(createLight<SmallLight<   RedLight>>(led(gx(0)+(i-2.5f)*13.0f, gy(0.5)+(j-4.5f)*11.0f), module, k)); break;
				case 1  :
				case 2  : addChild(createLight<SmallLight<YellowLight>>(led(gx(0)+(i-2.5f)*13.0f, gy(0.5)+(j-4.5f)*11.0f), module, k)); break;
				default : addChild(createLight<SmallLight< GreenLight>>(led(gx(0)+(i-2.5f)*13.0f, gy(0.5)+(j-4.5f)*11.0f), module, k)); break;
			}
		}
	}
}
