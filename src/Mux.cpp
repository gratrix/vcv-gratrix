#include "Gratrix.hpp"


struct Mux : Module {
	enum ParamIds {
		CH0_PARAM,
		CH1_PARAM,
		CH2_PARAM,
		CH3_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CH0_INPUT0,
		CH0_INPUT1,
		CH0_CV_INPUT,
		CH1_INPUT0,
		CH1_INPUT1,
		CH1_CV_INPUT,
		CH2_INPUT0,
		CH2_INPUT1,
		CH2_CV_INPUT,
		CH3_INPUT0,
		CH3_INPUT1,
		CH3_CV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		CH0_OUTPUT,
		CH1_OUTPUT,
		CH2_OUTPUT,
		CH3_OUTPUT,
		NUM_OUTPUTS
	};

	Mux()
	:
		Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS)
	{}

	void step() override
	{
		float mux0 = params[CH0_PARAM].value * clampf(inputs[CH0_CV_INPUT].normalize(10.0) / 10.0, 0.0, 1.0);
		float mux1 = params[CH1_PARAM].value * clampf(inputs[CH1_CV_INPUT].normalize(10.0) / 10.0, 0.0, 1.0);
		float mux2 = params[CH2_PARAM].value * clampf(inputs[CH2_CV_INPUT].normalize(10.0) / 10.0, 0.0, 1.0);
		float mux3 = params[CH3_PARAM].value * clampf(inputs[CH3_CV_INPUT].normalize(10.0) / 10.0, 0.0, 1.0);

		outputs[CH0_OUTPUT].value = inputs[CH0_INPUT0].value * (1.0f - mux0) + inputs[CH0_INPUT1].value * mux0;
		outputs[CH1_OUTPUT].value = inputs[CH1_INPUT0].value * (1.0f - mux1) + inputs[CH1_INPUT1].value * mux1;
		outputs[CH2_OUTPUT].value = inputs[CH2_INPUT0].value * (1.0f - mux2) + inputs[CH2_INPUT1].value * mux2;
		outputs[CH3_OUTPUT].value = inputs[CH3_INPUT0].value * (1.0f - mux3) + inputs[CH3_INPUT1].value * mux3;
	}
};


MuxWidget::MuxWidget()
{
	GTX__WIDGET();

	Mux *module = new Mux();
	setModule(module);
	box.size = Vec(9*15, 380);

	#if GTX__SAVE_SVG
	{
		PanelGen pg(assetPlugin(plugin, "res/Mux.svg"), box.size, "MUX");
	}
	#endif

	{
		#if GTX__LOAD_SVG
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Mux.svg")));
		#else
		Panel *panel = new LightPanel();
		panel->box.size = box.size;
		panel->backgroundImage = Image::load(assetPlugin(plugin, "res/Mux.png"));
		#endif
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

	addParam(createParam<Davies1900hBlackKnob>(Vec(65,  60), module, Mux::CH0_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<Davies1900hBlackKnob>(Vec(65, 140), module, Mux::CH1_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<Davies1900hBlackKnob>(Vec(65, 220), module, Mux::CH2_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<Davies1900hBlackKnob>(Vec(65, 300), module, Mux::CH3_PARAM, 0.0, 1.0, 0.0));

	addInput(createInput<PJ301MPort>(Vec( 6,  50), module, Mux::CH1_INPUT0));
	addInput(createInput<PJ301MPort>(Vec(34,  50), module, Mux::CH1_INPUT1));
	addInput(createInput<PJ301MPort>(Vec(20,  80), module, Mux::CH1_CV_INPUT));

	addInput(createInput<PJ301MPort>(Vec( 6, 130), module, Mux::CH1_INPUT0));
	addInput(createInput<PJ301MPort>(Vec(34, 130), module, Mux::CH1_INPUT1));
	addInput(createInput<PJ301MPort>(Vec(20, 160), module, Mux::CH1_CV_INPUT));

	addInput(createInput<PJ301MPort>(Vec( 6, 210), module, Mux::CH2_INPUT0));
	addInput(createInput<PJ301MPort>(Vec(34, 210), module, Mux::CH2_INPUT1));
	addInput(createInput<PJ301MPort>(Vec(20, 240), module, Mux::CH2_CV_INPUT));

	addInput(createInput<PJ301MPort>(Vec( 6, 290), module, Mux::CH3_INPUT0));
	addInput(createInput<PJ301MPort>(Vec(34, 290), module, Mux::CH3_INPUT1));
	addInput(createInput<PJ301MPort>(Vec(20, 320), module, Mux::CH3_CV_INPUT));

	addOutput(createOutput<PJ301MPort>(Vec(105,  65), module, Mux::CH0_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(105, 145), module, Mux::CH1_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(105, 225), module, Mux::CH2_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(105, 306), module, Mux::CH3_OUTPUT));
}
