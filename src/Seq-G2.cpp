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


#define RATIO     2
#define NOB_ROWS  2
#define NOB_COLS  16
#define BUT_ROWS  6
#define BUT_COLS  (NOB_COLS*RATIO)
#define OUT_LEFT  1
#define OUT_RIGHT 1

#define GATE_STATES 4


struct Impl : Module {
	enum ParamIds {
		CLOCK_PARAM,
		RUN_PARAM,
		RESET_PARAM,
		STEPS_PARAM,
		NOB_PARAM,
		BUT_PARAM  = NOB_PARAM + (NOB_COLS * NOB_ROWS),
		NUM_PARAMS = BUT_PARAM + (BUT_COLS * BUT_ROWS)
	};
	enum InputIds {
		CLOCK_INPUT,
		EXT_CLOCK_INPUT,
		RESET_INPUT,
		STEPS_INPUT,
		GATE_INPUT,      // N+1
		VOCT_INPUT,      // N+1
		NUM_INPUTS,
		OFF_INPUTS = GATE_INPUT
	};
	enum OutputIds {
		NOB_OUTPUT,
		BUT_OUTPUT  = NOB_OUTPUT + NOB_ROWS * (OUT_LEFT + OUT_RIGHT),
		GATE_OUTPUT = BUT_OUTPUT + BUT_ROWS * (OUT_LEFT + OUT_RIGHT),  // N
		VOCT_OUTPUT,                                                   // N
		NUM_OUTPUTS,
		OFF_OUTPUTS = GATE_OUTPUT
	};
	enum LightIds {
		RUNNING_LIGHT,
		RESET_LIGHT,
		BUT_LIGHT  = RESET_LIGHT + 3,
		NUM_LIGHTS = BUT_LIGHT   + (BUT_COLS * BUT_ROWS) * 3
	};

	static constexpr bool is_nob_snap(std::size_t row) { return true; }

	static constexpr std::size_t nob_val_map(std::size_t row, std::size_t col)     { return NOB_OUTPUT + (OUT_LEFT + OUT_RIGHT) * row + col; }
	static constexpr std::size_t but_val_map(std::size_t row, std::size_t col)     { return BUT_OUTPUT + (OUT_LEFT + OUT_RIGHT) * row + col; }

	static constexpr std::size_t nob_map(std::size_t row, std::size_t col)                  { return NOB_PARAM  +      NOB_COLS * row + col; }
	static constexpr std::size_t but_map(std::size_t row, std::size_t col)                  { return BUT_PARAM  +      BUT_COLS * row + col; }
	static constexpr std::size_t led_map(std::size_t row, std::size_t col, std::size_t idx) { return BUT_LIGHT  + 3 * (BUT_COLS * row + col) + idx; }

	static constexpr std::size_t imap(std::size_t port, std::size_t bank)
	{
		return (port < OFF_INPUTS)  ? port : port + bank * (NUM_INPUTS  - OFF_INPUTS);
	}

	static constexpr std::size_t omap(std::size_t port, std::size_t bank)
	{
		return (port < OFF_OUTPUTS) ? port : port + bank * (NUM_OUTPUTS - OFF_OUTPUTS);
	}

	bool running = true;
	SchmittTrigger clockTrigger; // for external clock
	// For buttons
	SchmittTrigger runningTrigger;
	SchmittTrigger resetTrigger;
	SchmittTrigger gateTriggers[BUT_ROWS][BUT_COLS];
	float phase = 0.0;
	int index = 0;
	int numSteps = 0;
	uint8_t gateState[BUT_ROWS][BUT_COLS] = {};
	float resetLight = 0.0;
	float stepLights[BUT_ROWS][BUT_COLS] = {};

	enum GateMode
	{
		GM_OFF,
		GM_CONTINUOUS,
		GM_RETRIGGER,
		GM_TRIGGER,
	};

	PulseGenerator gatePulse;

	Impl() : Module(
		NUM_PARAMS,
		(GTX__N+1) * (NUM_INPUTS  - OFF_INPUTS ) + OFF_INPUTS,
		(GTX__N  ) * (NUM_OUTPUTS - OFF_OUTPUTS) + OFF_OUTPUTS,
		NUM_LIGHTS)
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
			for (int row = 0; row < BUT_ROWS; ++row)
			{
				for (int col = 0; col < BUT_COLS; ++col)
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
			for (int row = 0, i = 0; row < BUT_ROWS; ++row)
			{
				for (int col = 0; col < BUT_COLS; ++col, ++i)
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
		for (int col = 0; col < BUT_COLS; col++)
		{
			for (int row = 0; row < BUT_ROWS; row++)
			{
				gateState[row][col] = GM_OFF;
			}
		}
	}

	void randomize() override
	{
		for (int col = 0; col < BUT_COLS; col++)
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
		index = BUT_COLS;
		nextStep = true;
		resetLight = 1.0;
	}

	numSteps = RATIO * clampi(roundf(params[STEPS_PARAM].value + inputs[STEPS_INPUT].value), 1, NOB_COLS);

	if (nextStep)
	{
		// Advance step
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

	for (int col = 0; col < BUT_COLS; ++col)
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

			stepLights[row][col] -= stepLights[row][col] / lightLambda / engineGetSampleRate();

			if (col < numSteps)
			{
				lights[led_map(row, col, 1)].value = gateState[row][col] == GM_CONTINUOUS ? 1.0 - stepLights[row][col] : stepLights[row][col];  // Green
				lights[led_map(row, col, 2)].value = gateState[row][col] == GM_RETRIGGER  ? 1.0 - stepLights[row][col] : stepLights[row][col];  // Blue
				lights[led_map(row, col, 0)].value = gateState[row][col] == GM_TRIGGER    ? 1.0 - stepLights[row][col] : stepLights[row][col];  // Red
			}
			else
			{
				lights[led_map(row, col, 1)].value = 0.01;  // Green
				lights[led_map(row, col, 2)].value = 0.01;  // Blue
				lights[led_map(row, col, 0)].value = 0.01;  // Red
			}
		}
	}

	// Rows

	int nob_index = index / RATIO;

	float nob_val[NOB_ROWS];
	for (std::size_t row = 0; row < NOB_ROWS; ++row)
	{
		nob_val[row] = params[nob_map(row, nob_index)].value;

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
		if (OUT_LEFT || OUT_RIGHT) outputs[nob_val_map(row, 0)].value = nob_val[row];
		if (OUT_LEFT && OUT_RIGHT) outputs[nob_val_map(row, 1)].value = nob_val[row];
	}

	for (std::size_t row = 0; row < BUT_ROWS; ++row)
	{
		if (OUT_LEFT || OUT_RIGHT) outputs[but_val_map(row, 0)].value = but_val[row] ? 10.0f : 0.0f;
		if (OUT_LEFT && OUT_RIGHT) outputs[but_val_map(row, 1)].value = but_val[row] ? 10.0f : 0.0f;
	}

	for (std::size_t i=0; i<GTX__N && i<BUT_ROWS; ++i)
	{
		// Pass V/OCT trough (for now)
		outputs[omap(VOCT_OUTPUT, i)].value = inputs[imap(VOCT_INPUT, i)].active ? inputs[imap(VOCT_INPUT, i)].value : inputs[imap(VOCT_INPUT, GTX__N)].value;

		// Generate gate out
		float gate_in  = inputs[imap(GATE_INPUT, i)].active ? inputs[imap(GATE_INPUT, i)].value : inputs[imap(GATE_INPUT, GTX__N)].value;

		outputs[omap(GATE_OUTPUT, i)].value = (but_val[i] && gate_in >= 1.0f) ? 10.0f : 0.0f;
	}

	lights[RESET_LIGHT].value = resetLight;
}


Widget::Widget()
{
	Impl *module = new Impl();
	setModule(module);
	box.size = Vec((OUT_LEFT+NOB_COLS+OUT_RIGHT)*3*15, 380);

	float grid_left  = 3*15*OUT_LEFT;
	float grid_right = 3*15*OUT_RIGHT;
	float grid_size  = box.size.x - grid_left - grid_right;

	float g_nobX[NOB_COLS] = {};
	for (std::size_t i = 0; i < NOB_COLS; i++)
	{
		float x  = grid_size / static_cast<double>(NOB_COLS);
		g_nobX[i] = grid_left + x * (i + 0.5);
	}

	float g_butX[BUT_COLS] = {};
	for (std::size_t i = 0; i < BUT_COLS; i++)
	{
		float x  = grid_size / static_cast<double>(BUT_COLS);
		g_butX[i] = grid_left + x * (i + 0.5);
	}

	float gridXl =              grid_left  / 2;
	float gridXr = box.size.x - grid_right / 2;

	float portX[4] = {};
	for (std::size_t i = 0; i < 4; i++)
	{
		float x = 4*6*15 / static_cast<double>(8);
		portX[i] = 3*7*15 + x * (i + 0.5);
	}

	float gridY[NOB_ROWS + BUT_ROWS] = {};
	{
		std::size_t j = 0;
		int pos = 35;

		for (std::size_t row = 0; row < NOB_ROWS; ++row, ++j)
		{
			pos += rad_n_s() + 4.5;
			gridY[j] = pos;
			pos += rad_n_s() + 4.5;
		}

		for (std::size_t row = 0; row < BUT_ROWS; ++row, ++j)
		{
			pos += rad_but() + 3.5;
			gridY[j] = pos;
			pos += rad_but() + 3.5;
		}
	}

	#if GTX__SAVE_SVG
	{
		PanelGen pg(assetPlugin(plugin, "build/res/Seq-G2.svg"), box.size, "SEQ-G2");

		for (std::size_t i=0; i<NOB_COLS-1; i++)
		{
			double x  = 0.5 * (g_nobX[i] + g_nobX[i+1]);
			double y0 = gridY[0];
			double y1 = gridY[NOB_ROWS + BUT_ROWS - 1];

			if (i % 4 == 3)
				pg.line(Vec(x, y0), Vec(x, y1), "fill:none;stroke:#7092BE;stroke-width:3");
			else
				pg.line(Vec(x, y0), Vec(x, y1), "fill:none;stroke:#7092BE;stroke-width:1");
		}

		pg.line(Vec(portX[0], gy(1.80)), Vec(portX[0], gy(2.2)), "fill:none;stroke:#7092BE;stroke-width:1");
		pg.line(Vec(portX[2], gy(1.75)), Vec(portX[2], gy(2.2)), "fill:none;stroke:#7092BE;stroke-width:1");
		pg.line(Vec(portX[3], gy(1.80)), Vec(portX[3], gy(2.2)), "fill:none;stroke:#7092BE;stroke-width:1");

		pg.nob_sml_raw(portX[0], gy(1.8), "CLOCK");
		pg.nob_sml_raw(portX[1], gy(1.8), "RUN");
		pg.nob_sml_raw(portX[2], gy(1.8), "RESET");
		pg.nob_sml_raw(portX[3], gy(1.8), "STEPS");

		pg.nob_sml_raw(portX[1], gy(2.2), "EXT CLK");

		pg.bus_in (0.5, 2, "GATE");
		pg.bus_in (1.5, 2, "V/OCT");
		pg.bus_out(7.5, 2, "GATE");
		pg.bus_out(6.5, 2, "V/OCT");
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

	addParam(createParam<RoundSmallBlackKnob>    (n_s(portX[0], gy(1.80)), module, Impl::CLOCK_PARAM, -2.0, 6.0, 2.0));
	addParam(createParam<LEDButton>              (but(portX[1], gy(1.75)), module, Impl::RUN_PARAM, 0.0, 1.0, 0.0));
	addChild(createLight<MediumLight<GreenLight>>(l_m(portX[1], gy(1.75)), module, Impl::RUNNING_LIGHT));
	addParam(createParam<LEDButton>              (but(portX[2], gy(1.75)), module, Impl::RESET_PARAM, 0.0, 1.0, 0.0));
	addChild(createLight<MediumLight<GreenLight>>(l_m(portX[2], gy(1.75)), module, Impl::RESET_LIGHT));
	addParam(createParam<RoundSmallBlackSnapKnob>(n_s(portX[3], gy(1.80)), module, Impl::STEPS_PARAM, 1.0, NOB_COLS, NOB_COLS));

	addInput(createInput<PJ301MPort>  (prt(portX[0], gy(2.2)), module, Impl::CLOCK_INPUT));
	addInput(createInput<PJ301MPort>  (prt(portX[1], gy(2.2)), module, Impl::EXT_CLOCK_INPUT));
	addInput(createInput<PJ301MPort>  (prt(portX[2], gy(2.2)), module, Impl::RESET_INPUT));
	addInput(createInput<PJ301MPort>  (prt(portX[3], gy(2.2)), module, Impl::STEPS_INPUT));

	{
		std::size_t j = 0;

		for (std::size_t row = 0; row < NOB_ROWS; ++row, ++j)
		{
			if (OUT_LEFT ) addOutput(createOutput<PJ301MPort>(prt(gridXl, gridY[j]), module, Impl::nob_val_map(row, 0)));
			if (OUT_RIGHT) addOutput(createOutput<PJ301MPort>(prt(gridXr, gridY[j]), module, Impl::nob_val_map(row, 1)));
		}

		for (std::size_t row = 0; row < BUT_ROWS; ++row, ++j)
		{
			if (OUT_LEFT ) addOutput(createOutput<PJ301MPort>(prt(gridXl, gridY[j]), module, Impl::but_val_map(row, 0)));
			if (OUT_RIGHT) addOutput(createOutput<PJ301MPort>(prt(gridXr, gridY[j]), module, Impl::but_val_map(row, 1)));
		}
	}

	{
		std::size_t j = 0;

		for (std::size_t row = 0; row < NOB_ROWS; ++row, ++j)
		{
			for (std::size_t col = 0; col < NOB_COLS; ++col)
			{
				if (Impl::is_nob_snap(row))
					addParam(createParam<RoundSmallBlackSnapKnob>(n_s(g_nobX[col], gridY[j]), module, Impl::nob_map(row, col), 0.0, 12.0, 0.0));
				else
					addParam(createParam<RoundSmallBlackKnob>    (n_s(g_nobX[col], gridY[j]), module, Impl::nob_map(row, col), 0.0, 10.0, 0.0));
			}
		}

		for (std::size_t row = 0; row < BUT_ROWS; ++row, ++j)
		{
			for (std::size_t col = 0; col < BUT_COLS; ++col)
			{
				addParam(createParam<LEDButton>                     (but(g_butX[col], gridY[j]), module, Impl::but_map(row, col), 0.0, 1.0, 0.0));
				addChild(createLight<MediumLight<RedGreenBlueLight>>(l_m(g_butX[col], gridY[j]), module, Impl::led_map(row, col, 0)));
			}
		}
	}

	for (std::size_t i=0; i<GTX__N; ++i)
	{
		addInput(createInput<PJ301MPort>  (prt(px(0.5, i), py(2, i)), module, Impl::imap(Impl::GATE_INPUT, i)));
		addInput(createInput<PJ301MPort>  (prt(px(1.5, i), py(2, i)), module, Impl::imap(Impl::VOCT_INPUT, i)));

		addOutput(createOutput<PJ301MPort>(prt(px(7.5, i), py(2, i)), module, Impl::omap(Impl::GATE_OUTPUT, i)));
		addOutput(createOutput<PJ301MPort>(prt(px(6.5, i), py(2, i)), module, Impl::omap(Impl::VOCT_OUTPUT, i)));
	}

	addInput(createInput<PJ301MPort>(prt(gx(0.5), gy(2)), module, Impl::imap(Impl::GATE_INPUT, GTX__N)));
	addInput(createInput<PJ301MPort>(prt(gx(1.5), gy(2)), module, Impl::imap(Impl::VOCT_INPUT, GTX__N)));
}


} // Seq_G2
} // GTX
