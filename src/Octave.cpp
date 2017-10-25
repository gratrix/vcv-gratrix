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


struct Octave : Module
{
	enum ParamIds
	{
		PARAMS_NUM
	};

	enum InputIds
	{
		INPUT_IN   = 0,
		INPUTS_NUM = INPUT_IN + 1
	};

	enum OutputIds
	{
		OUTPUT_NOTE = 0,
		OUTPUT_OCT  = OUTPUT_NOTE + N,
		OUTPUTS_NUM = OUTPUT_OCT  + M
	};


	float lights_key[E]       = {};
	float lights_oct[LO_SIZE] = {};

	Octave()
	:
		Module(PARAMS_NUM, INPUTS_NUM, OUTPUTS_NUM)
	{}

	void step() override
	{
		static constexpr float e = static_cast<float>(E);
		static constexpr float s = 1.0f / e;

		float in    = inputs[INPUT_IN].value;
		float fnote = std::floor(in * e + 0.5f);
		float qin   = fnote * s;

		int   note = static_cast<int>(fnote);
		int   safe = note + (E * 1000);  // push away from negative numbers
		int   key  = safe % E;
		int   oct  = safe / E;
		float led  = (oct & 1) ? -1.0f : 1.0f;
		      oct -= 1000;

		// quant
		in = qin;

		for (std::size_t i=0; i<N; ++i)
		{
			outputs[i + OUTPUT_NOTE].value = in + i * s;
		}

		for (std::size_t i=0; i<M; ++i)
		{
			outputs[i + OUTPUT_OCT].value = (in - T) + i;
		}

		// Lights

		for (std::size_t i=0; i<E;       ++i) lights_key[i] = 0.0f;
		for (std::size_t i=0; i<LO_SIZE; ++i) lights_oct[i] = 0.0f;

		lights_key[key] = led;

		if (LO_BEGIN <= oct && oct <= LO_END)
		{
			lights_oct[oct - LO_BEGIN] = led;
		}
	}
};


constexpr int x(std::size_t i, double radius) { return static_cast<int>(6*15 + 0.5 + radius * dx(i, E)); }
constexpr int y(std::size_t i, double radius) { return static_cast<int>(-20+206  + 0.5 + radius * dy(i, E)); }


OctaveWidget::OctaveWidget()
{
	Octave *module = new Octave();
	setModule(module);
	box.size = Vec(12*15, 380);

//	double r1 = 30;
	double r2 = 55;

	{
		PanelGen pg(assetPlugin(plugin, "res/Octave.svg"), box.size, "OCTAVE");

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

	{
/*		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load("plugins/Gratrix/res/Octave.svg"));
		addChild(panel);
*/
		Panel *panel = new LightPanel();
		panel->backgroundImage = Image::load("plugins/Gratrix/res/Octave.png");
		panel->box.size = box.size;
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

	addInput(createInput <PJ301MPort>(prt(x(0, 0), y(0, 0)), module, Octave::INPUT_IN));
	for (std::size_t i=0; i<N; ++i)
	{
		addOutput(createOutput<PJ301MPort>(prt(x(i, r2), y(i, r2)), module, i + Octave::OUTPUT_NOTE));
	}

	addOutput(createOutput<PJ301MPort>(prt(gx(-0.20), gy(2)), module, 0 + Octave::OUTPUT_OCT));
	addOutput(createOutput<PJ301MPort>(prt(gx( 0.15), gy(2)), module, 1 + Octave::OUTPUT_OCT));
	addOutput(createOutput<PJ301MPort>(prt(gx( 0.50), gy(2)), module, 2 + Octave::OUTPUT_OCT));
	addOutput(createOutput<PJ301MPort>(prt(gx( 0.85), gy(2)), module, 3 + Octave::OUTPUT_OCT));
	addOutput(createOutput<PJ301MPort>(prt(gx( 1.20), gy(2)), module, 4 + Octave::OUTPUT_OCT));

	addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(lx(0.5) - 30, ly(0) - 30 + 5), &module->lights_key[ 0]));  // C
	addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(lx(0.5) - 25, ly(0) - 30 - 5), &module->lights_key[ 1]));  // C#
	addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(lx(0.5) - 20, ly(0) - 30 + 5), &module->lights_key[ 2]));  // D
	addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(lx(0.5) - 15, ly(0) - 30 - 5), &module->lights_key[ 3]));  // Eb
	addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(lx(0.5) - 10, ly(0) - 30 + 5), &module->lights_key[ 4]));  // E
	addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(lx(0.5)     , ly(0) - 30 + 5), &module->lights_key[ 5]));  // F
	addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(lx(0.5) +  5, ly(0) - 30 - 5), &module->lights_key[ 6]));  // Fs
	addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(lx(0.5) + 10, ly(0) - 30 + 5), &module->lights_key[ 7]));  // G
	addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(lx(0.5) + 15, ly(0) - 30 - 5), &module->lights_key[ 8]));  // Ab
	addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(lx(0.5) + 20, ly(0) - 30 + 5), &module->lights_key[ 9]));  // A
	addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(lx(0.5) + 25, ly(0) - 30 - 5), &module->lights_key[10]));  // Bb
	addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(lx(0.5) + 30, ly(0) - 30 + 5), &module->lights_key[11]));  // B

	for (std::size_t i=0; i<LO_SIZE; ++i)
	{
		addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(lx(0.5) + (i - LO_SIZE/2) * 10, ly(0) -8), &module->lights_oct[i]));
	}
}
