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
#define GATE_STATES 4


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
		NUM_LIGHTS = BUT_LIGHT   + (SEQ_COLS * BUT_ROWS) * 3
	};

	static std::size_t nob_val_map(std::size_t row) { return NOB_OUTPUT + row; }
	static std::size_t but_val_map(std::size_t row) { return BUT_OUTPUT + row; }

	static std::size_t nob_map(std::size_t row, std::size_t col)                  { return NOB_PARAM  +      col * NOB_ROWS + row; }
	static std::size_t but_map(std::size_t row, std::size_t col)                  { return BUT_PARAM  +      col * BUT_ROWS + row; }
	static std::size_t led_map(std::size_t row, std::size_t col, std::size_t idx) { return BUT_LIGHT  + 3 * (col * BUT_ROWS + row) + idx; }
	static std::size_t prt_map(std::size_t row, std::size_t col)                  { return PRT_OUTPUT +      col * PRT_ROWS + row; }

	static bool is_nob_snap(std::size_t row) { return row == 1; }

	bool running = true;
	SchmittTrigger clockTrigger; // for external clock
	// For buttons
	SchmittTrigger runningTrigger;
	SchmittTrigger resetTrigger;
	SchmittTrigger gateTriggers[BUT_ROWS][SEQ_COLS];
	float phase = 0.0;
	int index = 0;
	uint8_t gateState[BUT_ROWS][SEQ_COLS] = {};
	float resetLight = 0.0;
	float stepLights[BUT_ROWS][SEQ_COLS] = {};

	enum GateMode
	{
		GM_OFF,
		GM_CONTINUOUS,
		GM_RETRIGGER,
		GM_TRIGGER,
	};

	PulseGenerator gatePulse;

	Impl() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS)
	{
		reset();
	}

	void step() override;

	json_t *toJson() override
	{
		json_t *rootJ = json_object();

		// Running
		json_object_set_new(rootJ, "running", json_boolean(running));

		// Gates
		if (json_t *gatesJ = json_array())
		{
			for (int col = 0; col < SEQ_COLS; col++)
			{
				for (int row = 0; row < BUT_ROWS; row++)
				{
					if (json_t *gateJ = json_integer((int) gateState[row][col]))
					{
						json_array_append_new(gatesJ, gateJ);
					}
				}
			}

			json_object_set_new(rootJ, "gates", gatesJ);
		}

		return rootJ;
	}

	void fromJson(json_t *rootJ) override
	{
		// Running
		if (json_t *runningJ = json_object_get(rootJ, "running"))
		{
			running = json_is_true(runningJ);
		}

		// Gates
		if (json_t *gatesJ = json_object_get(rootJ, "gates"))
		{
			for (int col = 0, i = 0; col < SEQ_COLS; col++)
			{
				for (int row = 0; row < BUT_ROWS; row++, i++)
				{
					if (json_t *gateJ = json_array_get(gatesJ, i))
					{
						int value = json_integer_value(gateJ);

						if (value < 0 || value >= GATE_STATES)
						{
							gateState[row][col] = 0;
						}
						else
						{
							gateState[row][col] = static_cast<uint8_t>(value);
						}
					}
				}
			}
		}
	}

	void reset() override
	{
		for (int col = 0; col < SEQ_COLS; col++)
		{
			for (int row = 0; row < BUT_ROWS; row++)
			{
				gateState[row][col] = GM_OFF;
			}
		}
	}

	void randomize() override
	{
		for (int col = 0; col < SEQ_COLS; col++)
		{
			for (int row = 0; row < BUT_ROWS; row++)
			{
				uint32_t r = randomu32() % (GATE_STATES + 1);
				if (r >= GATE_STATES) r = GM_CONTINUOUS;

				gateState[row][col] = r;
			}
		}
	}
};


void Impl::step()
{
	const float lightLambda = 0.075;

	// Run
	if (runningTrigger.process(params[RUN_PARAM].value))
	{
		running = !running;
	}
	lights[RUNNING_LIGHT].value = running ? 1.0 : 0.0;

	bool nextStep = false;

	if (running)
	{
		if (inputs[EXT_CLOCK_INPUT].active)
		{
			// External clock
			if (clockTrigger.process(inputs[EXT_CLOCK_INPUT].value))
			{
				phase = 0.0;
				nextStep = true;
			}
		}
		else
		{
			// Internal clock
			float clockTime = powf(2.0, params[CLOCK_PARAM].value + inputs[CLOCK_INPUT].value);
			phase += clockTime / engineGetSampleRate();

			if (phase >= 1.0)
			{
				phase -= 1.0;
				nextStep = true;
			}
		}
	}

	// Reset
	if (resetTrigger.process(params[RESET_PARAM].value + inputs[RESET_INPUT].value))
	{
		phase = 0.0;
		index = SEQ_COLS;
		nextStep = true;
		resetLight = 1.0;
	}

	if (nextStep)
	{
		// Advance step
		int numSteps = clampi(roundf(params[STEPS_PARAM].value + inputs[STEPS_INPUT].value), 1, SEQ_COLS);
		index += 1;

		if (index >= numSteps)
		{
			index = 0;
		}

		for (int row = 0; row < BUT_ROWS; row++)
		{
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
			if (gateTriggers[row][col].process(params[but_map(row, col)].value))
			{
				if (++gateState[row][col] >= GATE_STATES)
				{
					gateState[row][col] = 0;
				}
			}

			bool gateOn = (running && (col == index) && (gateState[row][col] > 0));

			switch (gateState[row][col])
			{
				case GM_CONTINUOUS :                            break;
				case GM_RETRIGGER  : gateOn = gateOn && !pulse; break;
				case GM_TRIGGER    : gateOn = gateOn &&  pulse; break;
				default            : break;
			}

			if (row < PRT_ROWS)
			{
				outputs[prt_map(row, col)].value = gateOn ? 10.0 : 0.0;
			}

			stepLights[row][col] -= stepLights[row][col] / lightLambda / engineGetSampleRate();

			lights[led_map(row, col, 1)].value = gateState[row][col] == GM_CONTINUOUS ? 1.0 - stepLights[row][col] : stepLights[row][col];  // Green
			lights[led_map(row, col, 2)].value = gateState[row][col] == GM_RETRIGGER  ? 1.0 - stepLights[row][col] : stepLights[row][col];  // Blue
			lights[led_map(row, col, 0)].value = gateState[row][col] == GM_TRIGGER    ? 1.0 - stepLights[row][col] : stepLights[row][col];  // Red
		}
	}

	// Rows
	float nob_val[NOB_ROWS];
	for (std::size_t row = 0; row < NOB_ROWS; ++row)
	{
		nob_val[row] = params[nob_map(row, index)].value;

		if (is_nob_snap(row)) nob_val[row] /= 12.0f;
	}
	bool but_val[BUT_ROWS];
	for (std::size_t row = 0; row < BUT_ROWS; ++row)
	{
		but_val[row] = running && (gateState[row][index] > 0);

		switch (gateState[row][index])
		{
			case GM_CONTINUOUS :                                        break;
			case GM_RETRIGGER  : but_val[row] = but_val[row] && !pulse; break;
			case GM_TRIGGER    : but_val[row] = but_val[row] &&  pulse; break;
			default            : break;
		}
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

		pg.bus_in(0, 2, "GATE IN");
		pg.bus_in(1, 2, "V/OCT IN");
		pg.bus_in(5, 2, "GATE OUT");
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
			if (Impl::is_nob_snap(row))
				addParam(createParam<RoundSmallBlackSnapKnob>(n_s(gridX[col], pos), module, Impl::nob_map(row, col), 0.0, 12.0, 12.0));
			else
				addParam(createParam<RoundSmallBlackKnob>    (n_s(gridX[col], pos), module, Impl::nob_map(row, col), 0.0, 10.0, 0.0));
			pos += rad_n_s() + pad;
		}

		for (std::size_t row = 0; row < BUT_ROWS; ++row)
		{
			pos += rad_but() + pad;
			addParam(createParam<LEDButton>                     (but(gridX[col], pos), module, Impl::but_map(row, col), 0.0, 1.0, 0.0));
			addChild(createLight<MediumLight<RedGreenBlueLight>>(l_m(gridX[col], pos), module, Impl::led_map(row, col, 0)));
			pos += rad_but() + pad;
		}

		for (std::size_t row = 0; row < PRT_ROWS; ++row)
		{
			pos += rad_prt() + pad;
			addOutput(createOutput<PJ301MPort>(prt(gridX[col], pos), module, Impl::prt_map(row, col)));
			pos += rad_prt() + pad;
		}
	}
}


} // Seq_G2
} // GTX
