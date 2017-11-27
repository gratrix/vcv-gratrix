//============================================================================================================
//!
//! \file Seq-G2.cpp
//!
//! \brief Seq-G2 is a thing.
//!
//============================================================================================================


#include "Gratrix.hpp"
#include "dsp/digital.hpp"


namespace GTX {
namespace Seq_G2 {


#define SEQ_COLS 16
#define NOB_ROWS 1
#define BUT_ROWS 6
#define PRT_ROWS 0


struct Impl : Module {
	enum ParamIds {
		CLOCK_PARAM,
		RUN_PARAM,
		RESET_PARAM,
		STEPS_PARAM,
		NOB_PARAM,
		BUT_PARAM  = NOB_PARAM + (SEQ_COLS * NOB_ROWS),
		NUM_PARAMS = BUT_PARAM + (SEQ_COLS * BUT_ROWS)
	};
	enum InputIds {
		CLOCK_INPUT,
		EXT_CLOCK_INPUT,
		RESET_INPUT,
		STEPS_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		NOB_OUTPUT,
		BUT_OUTPUT  = NOB_OUTPUT + NOB_ROWS,
		PRT_OUTPUT  = BUT_OUTPUT + BUT_ROWS,
		NUM_OUTPUTS = PRT_OUTPUT + (SEQ_COLS * PRT_ROWS)
	};
	enum LightIds {
		RUNNING_LIGHT,
		RESET_LIGHT,
		BUT_LIGHT  = RESET_LIGHT + 3,
		NUM_LIGHTS = BUT_LIGHT   + (SEQ_COLS * BUT_ROWS)
	};

	static std::size_t nob_val_map(std::size_t row) { return NOB_OUTPUT + row; }
	static std::size_t but_val_map(std::size_t row) { return BUT_OUTPUT + row; }

	static std::size_t nob_map(std::size_t row, std::size_t col) { return NOB_PARAM  + col * NOB_ROWS + row; }
	static std::size_t but_map(std::size_t row, std::size_t col) { return BUT_PARAM  + col * BUT_ROWS + row; }
	static std::size_t led_map(std::size_t row, std::size_t col) { return BUT_LIGHT  + col * BUT_ROWS + row; }
	static std::size_t prt_map(std::size_t row, std::size_t col) { return PRT_OUTPUT + col * PRT_ROWS + row; }

	bool running = true;
	SchmittTrigger clockTrigger; // for external clock
	// For buttons
	SchmittTrigger runningTrigger;
	SchmittTrigger resetTrigger;
	SchmittTrigger gateTriggers[BUT_ROWS][SEQ_COLS];
	float phase = 0.0;
	int index = 0;
	bool gateState[BUT_ROWS][SEQ_COLS] = {};
	float resetLight = 0.0;
	float stepLights[BUT_ROWS][SEQ_COLS] = {};

	enum GateMode {
		TRIGGER,
		RETRIGGER,
		CONTINUOUS,
	};
	GateMode gateMode = TRIGGER;
	PulseGenerator gatePulse;

	Impl() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		reset();
	}
	void step() override;

	json_t *toJson() override {
		json_t *rootJ = json_object();

		// running
		json_object_set_new(rootJ, "running", json_boolean(running));

		// gates
		json_t *gatesJ = json_array();
		for (int col = 0; col < SEQ_COLS; col++) {
			for (int row = 0; row < BUT_ROWS; row++) {
				json_t *gateJ = json_integer((int) gateState[row][col]);
				json_array_append_new(gatesJ, gateJ);
			}
		}
		json_object_set_new(rootJ, "gates", gatesJ);

		// gateMode
		json_t *gateModeJ = json_integer((int) gateMode);
		json_object_set_new(rootJ, "gateMode", gateModeJ);

		return rootJ;
	}

	void fromJson(json_t *rootJ) override {
		// running
		json_t *runningJ = json_object_get(rootJ, "running");
		if (runningJ)
			running = json_is_true(runningJ);

		// gates
		json_t *gatesJ = json_object_get(rootJ, "gates");
		if (gatesJ) {
			for (int col = 0, i = 0; col < SEQ_COLS; col++) {
				for (int row = 0; row < BUT_ROWS; row++, i++) {
					json_t *gateJ = json_array_get(gatesJ, i);
					if (gateJ)
						gateState[row][col] = !!json_integer_value(gateJ);
				}
			}
		}

		// gateMode
		json_t *gateModeJ = json_object_get(rootJ, "gateMode");
		if (gateModeJ)
			gateMode = (GateMode)json_integer_value(gateModeJ);
	}

	void reset() override {
		for (int col = 0; col < SEQ_COLS; col++) {
			for (int row = 0; row < BUT_ROWS; row++) {
				gateState[row][col] = true;
			}
		}
	}

	void randomize() override {
		for (int col = 0; col < SEQ_COLS; col++) {
			for (int row = 0; row < BUT_ROWS; row++) {
				gateState[row][col] = (randomf() > 0.5);
			}
		}
	}
};


void Impl::step()
{
	const float lightLambda = 0.075;
	// Run
	if (runningTrigger.process(params[RUN_PARAM].value)) {
		running = !running;
	}
	lights[RUNNING_LIGHT].value = running ? 1.0 : 0.0;

	bool nextStep = false;

	if (running) {
		if (inputs[EXT_CLOCK_INPUT].active) {
			// External clock
			if (clockTrigger.process(inputs[EXT_CLOCK_INPUT].value)) {
				phase = 0.0;
				nextStep = true;
			}
		}
		else {
			// Internal clock
			float clockTime = powf(2.0, params[CLOCK_PARAM].value + inputs[CLOCK_INPUT].value);
			phase += clockTime / engineGetSampleRate();
			if (phase >= 1.0) {
				phase -= 1.0;
				nextStep = true;
			}
		}
	}

	// Reset
	if (resetTrigger.process(params[RESET_PARAM].value + inputs[RESET_INPUT].value)) {
		phase = 0.0;
		index = SEQ_COLS;
		nextStep = true;
		resetLight = 1.0;
	}

	if (nextStep) {
		// Advance step
		int numSteps = clampi(roundf(params[STEPS_PARAM].value + inputs[STEPS_INPUT].value), 1, SEQ_COLS);
		index += 1;
		if (index >= numSteps) {
			index = 0;
		}
		for (int row = 0; row < BUT_ROWS; row++) {
			stepLights[row][index] = 1.0;
		}
		gatePulse.trigger(1e-3);
	}

	resetLight -= resetLight / lightLambda / engineGetSampleRate();

	bool pulse = gatePulse.process(1.0 / engineGetSampleRate());

	// Gate buttons
	for (int col = 0; col < SEQ_COLS; ++col)
	{
		for (int row = 0; row < BUT_ROWS; ++row)
		{
			if (gateTriggers[row][col].process(params[but_map(row, col)].value)) {
				gateState[row][col] = !gateState[row][col];
			}
			bool gateOn = (running && col == index && gateState[row][col]);
			if (gateMode == TRIGGER)
				gateOn = gateOn && pulse;
			else if (gateMode == RETRIGGER)
				gateOn = gateOn && !pulse;

			if (row < PRT_ROWS)
			{
				outputs[prt_map(row, col)].value = gateOn ? 10.0 : 0.0;
			}

			stepLights[row][col] -= stepLights[row][col] / lightLambda / engineGetSampleRate();
			lights[led_map(row, col)].value = gateState[row][col] ? 1.0 - stepLights[row][col] : stepLights[row][col];
		}
	}

	// Rows
	float nob_val[NOB_ROWS];
	for (std::size_t row = 0; row < NOB_ROWS; ++row)
	{
		nob_val[row] = params[nob_map(row, index)].value;
	}
	bool but_val[BUT_ROWS];
	for (std::size_t row = 0; row < BUT_ROWS; ++row)
	{
		but_val[row] = (running && gateState[row][index]);
		if (gateMode == TRIGGER)
			but_val[row] = but_val[row] && pulse;
		else if (gateMode == RETRIGGER)
			but_val[row] = but_val[row] && !pulse;
	}

	// Outputs
	for (std::size_t row = 0; row < NOB_ROWS; ++row)
	{
		outputs[nob_val_map(row)].value = nob_val[row];
	}
	for (std::size_t row = 0; row < BUT_ROWS; ++row)
	{
		outputs[but_val_map(row)].value = but_val[row] ? 10.0 : 0.0;
	}

	lights[RESET_LIGHT].value = resetLight;

//	lights[GATES_LIGHT].value = gatesOn ? 1.0 : 0.0;
//	lights[ROW_LIGHTS + 0].value = row1 / 10.0;
//	lights[ROW_LIGHTS + 1].value = row2 / 10.0;
//	lights[ROW_LIGHTS + 2].value = row3 / 10.0;
}


Widget::Widget()
{
	Impl *module = new Impl();
	setModule(module);
	box.size = Vec(36*15, 380);

	float gridX[SEQ_COLS] = {};
	float gridXr = 0;
	for (std::size_t i = 0; i < SEQ_COLS; i++)
	{
		float x  = (box.size.x - 10) / (1.5 + static_cast<double>(SEQ_COLS));
		gridX[i] = 5+x*(i+0.5);
	}
	{
		float x = (box.size.x - 10) / (static_cast<double>(SEQ_COLS));
		gridXr = 5+x*(SEQ_COLS-0.5);
	}

	float portX[8] = {};
	for (std::size_t i = 0; i < 8; i++)
	{
		float x = (3*6*15 - 10) / static_cast<double>(8);
		portX[i] = 2*6*15 + 5+x*(i+0.5);
	}

	#if GTX__SAVE_SVG
	{
		PanelGen pg(assetPlugin(plugin, "build/res/Seq-G2.svg"), box.size, "SEQ-G2");

		for (std::size_t i=0; i<SEQ_COLS; i++)
		{
			pg.line(Vec(gridX[i], 50), Vec(gridX[i], 250), "fill:none;stroke:#666666;stroke-width:1");
		}

	//	pg.bus_in(0, 2, "GATE IN");
	//	pg.bus_in(1, 2, "V/OCT IN");
	//	pg.bus_in(5, 2, "GATE OUT");
	}
	#endif

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Seq-G2.svg")));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

	addParam(createParam<RoundSmallBlackKnob>    (n_s(portX[0], gy(1.8)), module, Impl::CLOCK_PARAM, -2.0, 6.0, 2.0));
	addParam(createParam<LEDButton>              (but(portX[1], gy(1.8)), module, Impl::RUN_PARAM, 0.0, 1.0, 0.0));
	addChild(createLight<MediumLight<GreenLight>>(l_m(portX[1], gy(1.8)), module, Impl::RUNNING_LIGHT));
	addParam(createParam<LEDButton>              (but(portX[2], gy(1.8)), module, Impl::RESET_PARAM, 0.0, 1.0, 0.0));
	addChild(createLight<MediumLight<GreenLight>>(l_m(portX[2], gy(1.8)), module, Impl::RESET_LIGHT));
	addParam(createParam<RoundSmallBlackSnapKnob>(n_s(portX[3], gy(1.8)), module, Impl::STEPS_PARAM, 1.0, SEQ_COLS, SEQ_COLS));
//	addChild(createLight<MediumLight<GreenLight>>(l_m(portX[4], gy(1.8)), module, Impl::GATES_LIGHT));
//	addChild(createLight<MediumLight<GreenLight>>(l_m(portX[5], gy(1.8)), module, Impl::ROW_LIGHTS));
//	addChild(createLight<MediumLight<GreenLight>>(l_m(portX[6], gy(1.8)), module, Impl::ROW_LIGHTS + 1));
//	addChild(createLight<MediumLight<GreenLight>>(l_m(portX[7], gy(1.8)), module, Impl::ROW_LIGHTS + 2));

	addInput(createInput<PJ301MPort>  (prt(portX[0], gy(2.2)), module, Impl::CLOCK_INPUT));
	addInput(createInput<PJ301MPort>  (prt(portX[1], gy(2.2)), module, Impl::EXT_CLOCK_INPUT));
	addInput(createInput<PJ301MPort>  (prt(portX[2], gy(2.2)), module, Impl::RESET_INPUT));
	addInput(createInput<PJ301MPort>  (prt(portX[3], gy(2.2)), module, Impl::STEPS_INPUT));

	{
		int pos = 35, pad = 5;

		for (std::size_t row = 0; row < NOB_ROWS; ++row)
		{
			pos += rad_n_s() + pad;
			addOutput(createOutput<PJ301MPort>(prt(gridXr, pos), module, Impl::nob_val_map(row)));
			pos += rad_n_s() + pad;
		}
		for (std::size_t row = 0; row < BUT_ROWS; ++row)
		{
			pos += rad_but() + pad;
			addOutput(createOutput<PJ301MPort>(prt(gridXr, pos), module, Impl::but_val_map(row)));
			pos += rad_but() + pad;
		}
	}

	for (std::size_t col = 0; col < SEQ_COLS; ++col)
	{
		int pos = 35, pad = 5;

		for (std::size_t row = 0; row < NOB_ROWS; ++row)
		{
			pos += rad_n_s() + pad;
			addParam(createParam<RoundSmallBlackKnob>    (n_s(gridX[col], pos), module, Impl::nob_map(row, col), 0.0, 10.0, 0.0));
			pos += rad_n_s() + pad;
		}

		for (std::size_t row = 0; row < BUT_ROWS; ++row)
		{
			pos += rad_but() + pad;
			addParam(createParam<LEDButton>              (but(gridX[col], pos), module, Impl::but_map(row, col), 0.0, 1.0, 0.0));
			addChild(createLight<MediumLight<GreenLight>>(l_m(gridX[col], pos), module, Impl::led_map(row, col)));
			pos += rad_but() + pad;
		}

		for (std::size_t row = 0; row < PRT_ROWS; ++row)
		{
			pos += rad_prt() + pad;
			addOutput(createOutput<PJ301MPort>           (prt(gridX[col], pos), module, Impl::prt_map(row, col)));
			pos += rad_prt() + pad;
		}
	}
}

struct ImplGateModeItem : MenuItem {
	Impl *seq3;
	Impl::GateMode gateMode;
	void onAction(EventAction &e) override {
		seq3->gateMode = gateMode;
	}
	void step() override {
		rightText = (seq3->gateMode == gateMode) ? "✔" : "";
	}
};

Menu *Widget::createContextMenu() {
	Menu *menu = ModuleWidget::createContextMenu();

	MenuLabel *spacerLabel = new MenuLabel();
	menu->pushChild(spacerLabel);

	Impl *seq3 = dynamic_cast<Impl*>(module);
	assert(seq3);

	MenuLabel *modeLabel = new MenuLabel();
	modeLabel->text = "Gate Mode";
	menu->pushChild(modeLabel);

	ImplGateModeItem *triggerItem = new ImplGateModeItem();
	triggerItem->text = "Trigger";
	triggerItem->seq3 = seq3;
	triggerItem->gateMode = Impl::TRIGGER;
	menu->pushChild(triggerItem);

	ImplGateModeItem *retriggerItem = new ImplGateModeItem();
	retriggerItem->text = "Retrigger";
	retriggerItem->seq3 = seq3;
	retriggerItem->gateMode = Impl::RETRIGGER;
	menu->pushChild(retriggerItem);

	ImplGateModeItem *continuousItem = new ImplGateModeItem();
	continuousItem->text = "Continuous";
	continuousItem->seq3 = seq3;
	continuousItem->gateMode = Impl::CONTINUOUS;
	menu->pushChild(continuousItem);

	return menu;
}


} // Seq_G2
} // GTX
