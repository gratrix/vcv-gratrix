#include "Gratrix.hpp"

enum Spec
{
	N = 6
};


struct Split : Module
{
	enum ParamIds
	{
		PARAMS_NUM
	};

	enum InputIds
	{
		INPUT_IN   = 0,
		INPUTS_NUM = INPUT_IN + N
	};

	enum OutputIds
	{
		OUTPUT_OUT1 = 0,
		OUTPUT_OUT2 = OUTPUT_OUT1 + N,
		OUTPUT_OUT3 = OUTPUT_OUT2 + N,
		OUTPUT_OUT4 = OUTPUT_OUT3 + N,
		OUTPUT_OUT5 = OUTPUT_OUT4 + N,
		OUTPUTS_NUM = OUTPUT_OUT5 + N
	};

	Split() : Module(PARAMS_NUM, INPUTS_NUM, OUTPUTS_NUM) {}
	void step() override;
};


void Split::step()
{
	for (std::size_t i=0; i<N; ++i)
	{
		float in = inputs[i + INPUT_IN].value;

		outputs[i + OUTPUT_OUT1].value = in;
		outputs[i + OUTPUT_OUT2].value = in;
		outputs[i + OUTPUT_OUT3].value = in;
		outputs[i + OUTPUT_OUT4].value = in;
		outputs[i + OUTPUT_OUT5].value = in;
	}
}


SplitWidget::SplitWidget()
{
	Split *module = new Split();
	setModule(module);
	box.size = Vec(12*15, 380);

	#if GTX__SAVE_SVG
	{
		PanelGen pg(assetPlugin(plugin, "res/Split.svg"), box.size, "SPLIT");

		pg.bus_out(0, 0, "OUT 1"); pg.bus_out(1, 0, "OUT 2");
		pg.bus_in (0, 1, "IN");    pg.bus_out(1, 1, "OUT 3");
		pg.bus_out(0, 2, "OUT 5"); pg.bus_out(1, 2, "OUT 4");
	}
	#endif

	{
		#if GTX__LOAD_SVG
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Split.svg")));
		#else
		Panel *panel = new LightPanel();
		panel->box.size = box.size;
		panel->backgroundImage = Image::load(assetPlugin(plugin, "res/Split.png"));
		#endif
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

	for (std::size_t i=0; i<N; ++i)
	{
		addInput (createInput <PJ301MPort>(prt(px(0, i), py(1, i)), module, i + Split::INPUT_IN));

		addOutput(createOutput<PJ301MPort>(prt(px(0, i), py(0, i)), module, i + Split::OUTPUT_OUT1));
		addOutput(createOutput<PJ301MPort>(prt(px(1, i), py(0, i)), module, i + Split::OUTPUT_OUT2));
		addOutput(createOutput<PJ301MPort>(prt(px(1, i), py(1, i)), module, i + Split::OUTPUT_OUT3));
		addOutput(createOutput<PJ301MPort>(prt(px(0, i), py(2, i)), module, i + Split::OUTPUT_OUT4));
		addOutput(createOutput<PJ301MPort>(prt(px(1, i), py(2, i)), module, i + Split::OUTPUT_OUT5));
	}
}
