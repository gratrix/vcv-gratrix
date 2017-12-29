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


#define PROGRAMS  12
#define RATIO     2
#define LCD_ROWS  2
#define LCD_COLS  16
#define LCD_TEXT  4
#define PRG_ROWS  1
#define PRG_COLS  8
#define NOB_ROWS  0
#define NOB_COLS  LCD_COLS
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
		PROG_PARAM,
		PLAY_PARAM,
		EDIT_PARAM,
		SPAN_R_PARAM,
		SPAN_C_PARAM,
		CLEAR_PARAM,
		RANDOM_PARAM,
		COPY_PARAM,
		PASTE_PARAM,
		PRG_ROW_PARAM,
		PRG_COL_PARAM,
		PRG_SPAN_PARAM,
		PRG_STRIDE_PARAM,
		PRG_NOTE_PARAM,
		PRG_OCTAVE_PARAM,
		PRG_VALUE_PARAM,
		PRG_GATE_PARAM,
		NOB_PARAM,
		BUT_PARAM  = NOB_PARAM + (NOB_COLS * NOB_ROWS),
		NUM_PARAMS = BUT_PARAM + (BUT_COLS * BUT_ROWS)
	};
	enum InputIds {
		CLOCK_INPUT,
		EXT_CLOCK_INPUT,
		RESET_INPUT,
		STEPS_INPUT,
		PROG_INPUT,
		GATE_INPUT,      // N+1
		VOCT_INPUT,      // N+1
		NUM_INPUTS,
		OFF_INPUTS = GATE_INPUT
	};
	enum OutputIds {
		LCD_OUTPUT,
		NOB_OUTPUT  = LCD_OUTPUT + LCD_ROWS * (OUT_LEFT + OUT_RIGHT),
		BUT_OUTPUT  = NOB_OUTPUT + NOB_ROWS * (OUT_LEFT + OUT_RIGHT),
		GATE_OUTPUT = BUT_OUTPUT + BUT_ROWS * (OUT_LEFT + OUT_RIGHT),  // N
		VOCT_OUTPUT,                                                   // N
		NUM_OUTPUTS,
		OFF_OUTPUTS = GATE_OUTPUT
	};
	enum LightIds {
		RUNNING_LIGHT,
		RESET_LIGHT,
		PROG_LIGHT,
		CLEAR_LIGHT = PROG_LIGHT + PROGRAMS * 2,
		RANDOM_LIGHT,
		COPY_LIGHT,
		PASTE_LIGHT,
		BUT_LIGHT,
		NUM_LIGHTS = BUT_LIGHT   + (BUT_COLS * BUT_ROWS) * 3
	};

	struct Decode
	{
		/*static constexpr*/ float e = static_cast<float>(PROGRAMS);  // Static constexpr gives
		/*static constexpr*/ float s = 1.0f / e;                      // link error on Mac build.

		float in    = 0;
		float out   = 0;
		int   note  = 0;
		int   key   = 0;
		int   oct   = 0;

		void step(float input)
		{
			int safe, fnote;

			in    = input;
			fnote = std::floor(in * PROGRAMS + 0.5f);
			out   = fnote * s;
			note  = static_cast<int>(fnote);
			safe  = note + (PROGRAMS * 1000);  // push away from negative numbers
			key   = safe % PROGRAMS;
			oct   = (safe / PROGRAMS) - 1000;
		}
	};

	static constexpr bool is_nob_snap(std::size_t row) { return false; }

	static constexpr std::size_t lcd_val_map(std::size_t row, std::size_t col)     { return LCD_OUTPUT + (OUT_LEFT + OUT_RIGHT) * row + col; }
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


	Decode prg_nob;
	Decode prg_cv;
	bool running = true;
	SchmittTrigger clockTrigger; // for external clock
	// For buttons
	SchmittTrigger runningTrigger;
	SchmittTrigger resetTrigger;
	SchmittTrigger clearTrigger;
	SchmittTrigger randomTrigger;
	SchmittTrigger copyTrigger;
	SchmittTrigger pasteTrigger;
	SchmittTrigger gateTriggers[BUT_ROWS][BUT_COLS];
	float phase  = 0;
	int index    = 0;
	int numSteps = 0;
	std::size_t play_prog = 0;
	std::size_t edit_prog = 0;

	struct LcdData
	{
		bool    active = 0;
		int8_t  mode   = 0;
		int8_t  note   = 0;   // C4
		int8_t  octave = 4;   // C4
		float   value  = 0;
	};

	#if LCD_ROWS
	LcdData lcd_state[PROGRAMS][LCD_ROWS][LCD_COLS] = {};
	LcdData lcd_cache          [LCD_ROWS][LCD_COLS] = {};
	#endif
	#if PRG_ROWS
	int8_t  prg_cache_row    = 0;
	int8_t  prg_cache_col    = 0;
	int8_t  prg_cache_note   = 0;
	int8_t  prg_cache_octave = 0;
	float   prg_cache_value  = 0;
	int8_t  prg_cache_gate   = 0;
	int8_t  prg_cache_rows   = 0;
	int8_t  prg_cache_cols   = 0;
	#endif
	#if BUT_ROWS
	uint8_t but_state[PROGRAMS][BUT_ROWS][BUT_COLS] = {};
	uint8_t but_cache          [BUT_ROWS][BUT_COLS] = {};
	float   but_lights         [BUT_ROWS][BUT_COLS] = {};
	#endif

	float resetLight  = 0;
	float clearLight  = 0;
	float randomLight = 0;
	float copyLight   = 0;
	float pasteLight  = 0;

	enum GateMode
	{
		GM_OFF,
		GM_CONTINUOUS,
		GM_RETRIGGER,
		GM_TRIGGER,
	};

	PulseGenerator gatePulse;

	//--------------------------------------------------------------------------------------------------------
	//! \brief Constructor.

	Impl()
	:
		Module(
			NUM_PARAMS,
			(GTX__N+1) * (NUM_INPUTS  - OFF_INPUTS ) + OFF_INPUTS,
			(GTX__N  ) * (NUM_OUTPUTS - OFF_OUTPUTS) + OFF_OUTPUTS,
			NUM_LIGHTS)
	{
		reset();
	}

	//--------------------------------------------------------------------------------------------------------
	//! \brief Step function.

	void step() override
	{
		const float lightLambda = 0.075;

		// Decode program info

		prg_nob.step(params[PROG_PARAM].value / 12.0f);
		prg_cv .step(inputs[PROG_INPUT].value);

		// Input leds

		float prog_leds[PROGRAMS * 2]  = {};
		prog_leds[prg_nob.key * 2    ] = 1.0f;  // Green
		prog_leds[prg_cv .key * 2 + 1] = 1.0f;  // Red

		// Determine what is playing and what is editing

		bool play_is_cv = (params[PLAY_PARAM].value < 0.5f);
		bool edit_is_cv = (params[EDIT_PARAM].value < 0.5f);

		play_prog = play_is_cv ? prg_cv.key : prg_nob.key;
		edit_prog = edit_is_cv ? prg_cv.key : prg_nob.key;

		// Run

		if (runningTrigger.process(params[RUN_PARAM].value))
		{
			running = !running;
		}

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

		// Update knobs

		knob_pull(edit_prog);

		// Trigger buttons

		{
			const float dim = 1.0f / lightLambda / engineGetSampleRate();

			// Reset
			if (resetTrigger.process(params[RESET_PARAM].value + inputs[RESET_INPUT].value))
			{
				phase = 0.0;
				index = BUT_COLS;
				nextStep = true;
				resetLight = 1.0;
			}
			resetLight -= resetLight * dim;

			// Clear current program
			if (clearTrigger.process(params[CLEAR_PARAM].value))
			{
				clear_prog(edit_prog);
				clearLight = 1.0;
			}
			clearLight -= clearLight * dim;

			// Randomise current program
			if (randomTrigger.process(params[RANDOM_PARAM].value))
			{
				randomize_prog(edit_prog);
				randomLight = 1.0;
			}
			randomLight -= randomLight * dim;

			// Copy current program
			if (copyTrigger.process(params[COPY_PARAM].value))
			{
				copy_prog(edit_prog);
				copyLight = 1.0;
			}
			copyLight -= copyLight * dim;

			// Paste current program
			if (pasteTrigger.process(params[PASTE_PARAM].value))
			{
				paste_prog(edit_prog);
				pasteLight = 1.0;
			}
			pasteLight -= pasteLight * dim;
		}

		numSteps = RATIO * clampi(roundf(params[STEPS_PARAM].value + inputs[STEPS_INPUT].value), 1, LCD_COLS);

		if (nextStep)
		{
			// Advance step
			index += 1;

			if (index >= numSteps)
			{
				index = 0;
			}

			#if BUT_ROWS
			for (int row = 0; row < BUT_ROWS; row++)
			{
				but_lights[row][index] = 1.0;
			}
			#endif

			gatePulse.trigger(1e-3);
		}

		bool pulse = gatePulse.process(1.0 / engineGetSampleRate());

		#if BUT_ROWS
		// Gate buttons

		for (int col = 0; col < BUT_COLS; ++col)
		{
			for (int row = 0; row < BUT_ROWS; ++row)
			{
				// User input to alter state of buttons

				if (gateTriggers[row][col].process(params[but_map(row, col)].value))
				{
					auto state = but_state[edit_prog][row][col];

					if (++state >= GATE_STATES)
					{
						state = GM_OFF;
					}

					std::size_t span_r = static_cast<std::size_t>(params[SPAN_R_PARAM].value + 0.5);
					std::size_t span_c = static_cast<std::size_t>(params[SPAN_C_PARAM].value + 0.5);

					for (std::size_t r = row; r < row + span_r && r < BUT_ROWS; ++r)
					{
						for (std::size_t c = col; c < col + span_c && c < BUT_COLS; ++c)
						{
							but_state[edit_prog][r][c] = state;
						}
					}
				}

				// Get state of buttons for lights

				{
					bool gateOn = (running && (col == index) && (but_state[edit_prog][row][col] > 0));

					switch (but_state[edit_prog][row][col])
					{
						case GM_CONTINUOUS :                            break;
						case GM_RETRIGGER  : gateOn = gateOn && !pulse; break;
						case GM_TRIGGER    : gateOn = gateOn &&  pulse; break;
						default            : break;
					}

					but_lights[row][col] -= but_lights[row][col] / lightLambda / engineGetSampleRate();

					if (col < numSteps)
					{
						float val = (play_prog == edit_prog) ? 1.0 : 0.1;

						lights[led_map(row, col, 1)].value = but_state[edit_prog][row][col] == GM_CONTINUOUS ? 1.0 - val * but_lights[row][col] : val * but_lights[row][col];  // Green
						lights[led_map(row, col, 2)].value = but_state[edit_prog][row][col] == GM_RETRIGGER  ? 1.0 - val * but_lights[row][col] : val * but_lights[row][col];  // Blue
						lights[led_map(row, col, 0)].value = but_state[edit_prog][row][col] == GM_TRIGGER    ? 1.0 - val * but_lights[row][col] : val * but_lights[row][col];  // Red
					}
					else
					{
						lights[led_map(row, col, 1)].value = 0.01;  // Green
						lights[led_map(row, col, 2)].value = 0.01;  // Blue
						lights[led_map(row, col, 0)].value = 0.01;  // Red
					}
				}
			}
		}
		#endif

		// Compute row outputs

		#if LCD_ROWS
		std::size_t lcd_index = index / RATIO;
		float       lcd_val[LCD_ROWS];
		for (std::size_t row = 0; row < LCD_ROWS; ++row)
		{
			auto &current = lcd_state[play_prog][row][lcd_index];

			switch (current.mode)
			{
				case 0 :
				{
					lcd_val[row] = (current.octave - 4.0f) + (current.note / 12.0f);
				}
				break;

				case 1 :
				{
					lcd_val[row] = current.value;
				}
				break;

				default :
				{
					lcd_val[row] = 0;
				}
				break;
			}
		}
		#endif

		#if NOB_ROWS
		std::size_t nob_index = index / RATIO;
		float       nob_val[NOB_ROWS];
		for (std::size_t row = 0; row < NOB_ROWS; ++row)
		{
			nob_val[row] = params[nob_map(row, nob_index)].value;

			if (is_nob_snap(row)) nob_val[row] /= 12.0f;
		}
		#endif

		#if BUT_ROWS
		bool but_val[BUT_ROWS];
		for (std::size_t row = 0; row < BUT_ROWS; ++row)
		{
			but_val[row] = running && (but_state[play_prog][row][index] > 0);

			switch (but_state[play_prog][row][index])
			{
				case GM_CONTINUOUS :                                        break;
				case GM_RETRIGGER  : but_val[row] = but_val[row] && !pulse; break;
				case GM_TRIGGER    : but_val[row] = but_val[row] &&  pulse; break;
				default            : break;
			}
		}
		#endif

		// Write row outputs

		#if LCD_ROWS
		for (std::size_t row = 0; row < LCD_ROWS; ++row)
		{
			if (OUT_LEFT || OUT_RIGHT) outputs[lcd_val_map(row, 0)].value = lcd_val[row];
			if (OUT_LEFT && OUT_RIGHT) outputs[lcd_val_map(row, 1)].value = lcd_val[row];
		}
		#endif

		#if NOB_ROWS
		for (std::size_t row = 0; row < NOB_ROWS; ++row)
		{
			if (OUT_LEFT || OUT_RIGHT) outputs[nob_val_map(row, 0)].value = nob_val[row];
			if (OUT_LEFT && OUT_RIGHT) outputs[nob_val_map(row, 1)].value = nob_val[row];
		}
		#endif

		#if BUT_ROWS
		for (std::size_t row = 0; row < BUT_ROWS; ++row)
		{
			if (OUT_LEFT || OUT_RIGHT) outputs[but_val_map(row, 0)].value = but_val[row] ? 10.0f : 0.0f;
			if (OUT_LEFT && OUT_RIGHT) outputs[but_val_map(row, 1)].value = but_val[row] ? 10.0f : 0.0f;
		}
		#endif

		// Detemine poly outputs

		#if BUT_ROWS
		for (std::size_t i=0; i<GTX__N && i<BUT_ROWS; ++i)
		{
			// Pass V/OCT trough (for now)
			outputs[omap(VOCT_OUTPUT, i)].value = inputs[imap(VOCT_INPUT, i)].active ? inputs[imap(VOCT_INPUT, i)].value : inputs[imap(VOCT_INPUT, GTX__N)].value;

			// Generate gate out
			float gate_in  = inputs[imap(GATE_INPUT, i)].active ? inputs[imap(GATE_INPUT, i)].value : inputs[imap(GATE_INPUT, GTX__N)].value;

			outputs[omap(GATE_OUTPUT, i)].value = (but_val[i] && gate_in >= 1.0f) ? 10.0f : 0.0f;
		}
		#endif

		// Update LEDs

		lights[RUNNING_LIGHT].value = running ? 1.0 : 0.0;
		lights[RESET_LIGHT]  .value = resetLight;
		lights[CLEAR_LIGHT]  .value = clearLight;
		lights[RANDOM_LIGHT] .value = randomLight;
		lights[COPY_LIGHT]   .value = copyLight;
		lights[PASTE_LIGHT]  .value = pasteLight;

		for (std::size_t i=0; i<PROGRAMS; ++i)
		{
			lights[PROG_LIGHT + i * 2    ].value = prog_leds[i * 2    ];
			lights[PROG_LIGHT + i * 2 + 1].value = prog_leds[i * 2 + 1];
		}
	}

	//--------------------------------------------------------------------------------------------------------
	//! \brief Save state.

	json_t *toJson() override
	{
		if (json_t *rootJ = json_object())
		{
			// Running
			json_object_set_new(rootJ, "running", json_boolean(running));

			#if LCD_ROWS
			if (json_t *ja = json_array())
			{
				for (std::size_t prog = 0; prog < PROGRAMS; ++prog)
				{
					for (std::size_t row = 0; row < LCD_ROWS; ++row)
					{
						for (std::size_t col = 0; col < LCD_COLS; ++col)
						{
							if (json_t *ji = json_integer(static_cast<int>(lcd_state[prog][row][col].note)))
							{
								json_array_append_new(ja, ji);
							}
						}
					}
				}

				json_object_set_new(rootJ, "lcd", ja);
			}
			#endif

			#if BUT_ROWS
			if (json_t *ja = json_array())
			{
				for (std::size_t prog = 0; prog < PROGRAMS; ++prog)
				{
					for (std::size_t row = 0; row < BUT_ROWS; ++row)
					{
						for (std::size_t col = 0; col < BUT_COLS; ++col)
						{
							if (json_t *ji = json_integer(static_cast<int>(but_state[prog][row][col])))
							{
								json_array_append_new(ja, ji);
							}
						}
					}
				}

				json_object_set_new(rootJ, "but", ja);
			}
			#endif

			return rootJ;
		}

		return nullptr;
	}

	//--------------------------------------------------------------------------------------------------------
	//! \brief Load state.

	void fromJson(json_t *rootJ) override
	{
		// Running
		if (json_t *jb = json_object_get(rootJ, "running"))
		{
			running = json_is_true(jb);
		}

		#if LCD_ROWS
		if (json_t *ja = json_object_get(rootJ, "lcd"))
		{
			for (std::size_t i = 0, prog = 0; prog < PROGRAMS; ++prog)
			{
				for (std::size_t row = 0; row < LCD_ROWS; ++row)
				{
					for (std::size_t col = 0; col < LCD_COLS; ++col, ++i)
					{
						if (json_t *ji = json_array_get(ja, i))
						{
							//! \todo Range check
							lcd_state[prog][row][col].note = json_integer_value(ji);
						}
					}
				}
			}
		}
		#endif

		#if BUT_ROWS
		if (json_t *ja = json_object_get(rootJ, "but"))
		{
			for (std::size_t i = 0, prog = 0; prog < PROGRAMS; ++prog)
			{
				for (std::size_t row = 0; row < BUT_ROWS; ++row)
				{
					for (std::size_t col = 0; col < BUT_COLS; ++col, ++i)
					{
						if (json_t *ji = json_array_get(ja, i))
						{
							int value = json_integer_value(ji);

							if (value < 0 || value >= GATE_STATES)
							{
								but_state[prog][row][col] = GM_OFF;
							}
							else
							{
								but_state[prog][row][col] = static_cast<uint8_t>(value);
							}
						}
					}
				}
			}
		}
		#endif
	}

	//--------------------------------------------------------------------------------------------------------
	//! \brief Reset state.

	void reset() override
	{
		for (std::size_t prog = 0; prog < PROGRAMS; ++prog)
		{
			clear_prog(prog);
		}
	}

	//--------------------------------------------------------------------------------------------------------
	//! \brief Random state.

	void randomize() override
	{
		for (std::size_t prog = 0; prog < PROGRAMS; ++prog)
		{
			randomize_prog(prog);
		}
	}

	//--------------------------------------------------------------------------------------------------------
	//! \brief Knob params to state.
	//!
	//! Only updates on knob value change, otheriwse current value always applied to current program.

	void knob_pull(std::size_t prog)
	{
		#if LCD_ROWS && PRG_ROWS
		for (std::size_t row = 0; row < LCD_ROWS; ++row)
		{
			for (std::size_t col = 0; col < LCD_COLS; ++col)
			{
				lcd_state[prog][row][col].active = false;
			}
		}

		{
			std::size_t prg_row = static_cast<std::size_t>(params[PRG_ROW_PARAM].value + 0.5f);

			if (prg_row < LCD_ROWS)
			{
				std::size_t prg_col = static_cast<std::size_t>(params[PRG_COL_PARAM].value + 0.5f);

				if (prg_col < LCD_COLS)
				{
					std::size_t prg_span   = static_cast<std::size_t>(params[PRG_SPAN_PARAM  ].value + 0.5f);
					std::size_t prg_stride = static_cast<std::size_t>(params[PRG_STRIDE_PARAM].value + 0.5f);

					int8_t prg_note   = static_cast<int8_t>(params[PRG_NOTE_PARAM  ].value + 0.5f);
					int8_t prg_octave = static_cast<int8_t>(params[PRG_OCTAVE_PARAM].value + 0.5f);
					float  prg_value  =                     params[PRG_VALUE_PARAM ].value;
		//			int8_t prg_gate   = static_cast<int8_t>(params[PRG_GATE_PARAM  ].value + 0.5f);

					std::size_t col_max = prg_col + prg_span * prg_stride;
					if (col_max > LCD_COLS)
						col_max = LCD_COLS;

					for (std::size_t col = prg_col; col < col_max; col += prg_stride)
					{
						auto &current = lcd_state[prog][prg_row][col];

						current.active = true;

						if (prg_cache_note != prg_note)
						{
							current.note = prg_note;
							current.mode = 0;
						}

						if (prg_cache_octave != prg_octave)
						{
							current.octave = prg_octave;
							current.mode   = 0;
						}

						if (prg_cache_value != prg_value)
						{
							current.value = prg_value;
							current.mode  = 1;
						}
					}

					prg_cache_note   = prg_note;
					prg_cache_octave = prg_octave;
					prg_cache_value  = prg_value;
				}
			}
		}
		#endif
	}

	//--------------------------------------------------------------------------------------------------------
	//! \brief Clear a program.

	void clear_prog(std::size_t prog)
	{
		#if LCD_ROWS
		for (std::size_t row = 0; row < LCD_ROWS; ++row)
		{
			for (std::size_t col = 0; col < LCD_COLS; col++)
			{
				lcd_state[prog][row][col] = LcdData();
			}
		}
		#endif

		#if BUT_ROWS
		for (std::size_t row = 0; row < BUT_ROWS; row++)
		{
			for (std::size_t col = 0; col < BUT_COLS; col++)
			{
				but_state[prog][row][col] = GM_OFF;
			}
		}
		#endif
	}

	//--------------------------------------------------------------------------------------------------------
	//! \brief Randomize a program.

	void randomize_prog(std::size_t prog)
	{
		#if BUT_ROWS
		for (std::size_t row = 0; row < BUT_ROWS; row++)
		{
			for (std::size_t col = 0; col < BUT_COLS; col++)
			{
				uint32_t r = randomu32() % (GATE_STATES + 1);

				if (r >= GATE_STATES) r = GM_CONTINUOUS;

				but_state[prog][row][col] = r;
			}
		}
		#endif
	}

	//--------------------------------------------------------------------------------------------------------
	//! \brief Copy a program.

	void copy_prog(std::size_t prog)
	{
		#if LCD_ROWS
		for (std::size_t row = 0; row < LCD_ROWS; ++row)
		{
			for (std::size_t col = 0; col < LCD_COLS; col++)
			{
				lcd_cache[row][col] = lcd_state[prog][row][col];
			}
		}
		#endif

		#if BUT_ROWS
		for (std::size_t row = 0; row < BUT_ROWS; row++)
		{
			for (std::size_t col = 0; col < BUT_COLS; col++)
			{
				but_cache[row][col] = but_state[prog][row][col];
			}
		}
		#endif
	}

	//--------------------------------------------------------------------------------------------------------
	//! \brief Paste a program.

	void paste_prog(std::size_t prog)
	{
		#if LCD_ROWS
		for (std::size_t row = 0; row < LCD_ROWS; ++row)
		{
			for (std::size_t col = 0; col < LCD_COLS; col++)
			{
				lcd_state[prog][row][col] = lcd_cache[row][col];
			}
		}
		#endif

		#if BUT_ROWS
		for (std::size_t row = 0; row < BUT_ROWS; row++)
		{
			for (std::size_t col = 0; col < BUT_COLS; col++)
			{
				but_state[prog][row][col] = but_cache[row][col];
			}
		}
		#endif
	}
};


#if LCD_ROWS
//============================================================================================================
//! \brief Display.

struct Display : TransparentWidget
{
	Impl *module;
	int frame = 0;
	std::shared_ptr<Font> font;

	float tx[LCD_COLS] = {};
	float ty[LCD_ROWS] = {};

	char text[LCD_ROWS][LCD_COLS][LCD_TEXT + 1] = {};

	//--------------------------------------------------------------------------------------------------------
	//! \brief Constructor.

	Display(Impl *module_, const Rect &box_)
	:
		module(module_)
	{
		box  = box_;
		font = Font::load(assetPlugin(plugin, "res/fonts/lcd-solid/LCD_Solid.ttf"));

		for (std::size_t col = 0; col < LCD_COLS; col++)
		{
			tx[col] = 4.0f + col * box.size.x / static_cast<double>(LCD_COLS);
		}

		for (std::size_t row = 0; row < LCD_ROWS; row++)
		{
			ty[row] = (row + 1) * 18.0f;
		}

		for (std::size_t col = 0; col < LCD_COLS; ++col)
		{
			for (std::size_t row = 0; row < LCD_ROWS; ++row)
			{
				std::strncpy(text[row][col], "01234567012345670123456701234567", LCD_TEXT);
			}
		}
	}

	//--------------------------------------------------------------------------------------------------------
	//! \brief ...

	void draw_main(NVGcontext *vg)
	{
		nvgFontSize(vg, 16);
		nvgFontFaceId(vg, font->handle);
		nvgTextLetterSpacing(vg, -2);

		char old = 'x';

		for (std::size_t col = 0; col < LCD_COLS; ++col)
		{
			for (std::size_t row = 0; row < LCD_ROWS; ++row)
			{
				if (old != text[row][col][0])
				{
					switch (text[row][col][0])
					{
						case 'p' : nvgFillColor(vg, nvgRGBA(0xe1, 0x02, 0x78, 0xc0)); break;
						default  : nvgFillColor(vg, nvgRGBA(0x28, 0xb0, 0xf3, 0xc0)); break;
					}

					old = text[row][col][0];
				}

				nvgText(vg, tx[col], ty[row], text[row][col] + 1, NULL);
			}
		}
	}

	//--------------------------------------------------------------------------------------------------------
	//! \brief ...

	void draw(NVGcontext *vg) override
	{
		// Calculate
		if (++frame >= 4)
		{
			frame = 0;

			static const char   *note_names[13] = {"C-", "C#", "D-", "Eb", "E-", "F-", "F#", "G-", "Ab", "A-", "Bb", "B-", "??"};
			static const char *octave_names[10] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "?"};

			for (std::size_t col = 0; col < LCD_COLS; ++col)
			{
				for (std::size_t row = 0; row < LCD_ROWS; ++row)
				{
					bool active = module->lcd_state[module->edit_prog][row][col].active;
					int  mode   = module->lcd_state[module->edit_prog][row][col].mode;

					text[row][col][0] = active ? 'p' : 'b';

					switch (mode)
					{
						case 0 :
						{
							int  note   = module->lcd_state[module->edit_prog][row][col].note;
							int  octave = module->lcd_state[module->edit_prog][row][col].octave;

							if (note   < 0 || note   > 12) note   = 12;
							if (octave < 0 || octave >  9) octave =  9;

							text[row][col][1] =   note_names[  note][0];
							text[row][col][2] =   note_names[  note][1];
							text[row][col][3] = octave_names[octave][0];
							text[row][col][4] = '\0';
						}
						break;

						case 1 :
						{
							float value = module->lcd_state[module->edit_prog][row][col].value;

							snprintf(&text[row][col][1], 4, "%4.2f", value);
						}
						break;

						default :
						{
							text[row][col][1] = '?';
							text[row][col][2] = '\0';
						}
						break;
					}
				}
			}
		}

		draw_main(vg);
	}
};
#endif


//============================================================================================================
//! \brief The Widget.

Widget::Widget()
{
	Impl *module = new Impl();
	setModule(module);
	box.size = Vec((OUT_LEFT+LCD_COLS+OUT_RIGHT)*3*15, 380);

	float grid_left  = 3*15*OUT_LEFT;
	float grid_right = 3*15*OUT_RIGHT;
	float grid_size  = box.size.x - grid_left - grid_right;

	auto display_rect = Rect(Vec(grid_left, 35), Vec(grid_size, (rad_but()+1.5) * 2 * LCD_ROWS));

	#if LCD_ROWS
	float g_lcdX[LCD_COLS] = {};
	for (std::size_t i = 0; i < LCD_COLS; i++)
	{
		float x  = grid_size / static_cast<double>(LCD_COLS);
		g_lcdX[i] = grid_left + x * (i + 0.5);
	}
	#endif

	#if NOB_ROWS
	float g_nobX[NOB_COLS] = {};
	for (std::size_t i = 0; i < NOB_COLS; i++)
	{
		float x  = grid_size / static_cast<double>(NOB_COLS);
		g_nobX[i] = grid_left + x * (i + 0.5);
	}
	#endif

	#if BUT_ROWS
	float g_butX[BUT_COLS] = {};
	for (std::size_t i = 0; i < BUT_COLS; i++)
	{
		float x  = grid_size / static_cast<double>(BUT_COLS);
		g_butX[i] = grid_left + x * (i + 0.5);
	}
	#endif

	float gridXl =              grid_left  / 2;
	float gridXr = box.size.x - grid_right / 2;

	float portX[10] = {};
	for (std::size_t i = 0; i < 10; i++)
	{
		float x = 5*6*15 / static_cast<double>(10);
		portX[i] = 2*6*15 + x * (i + 0.5);
	}
	float dX = 0.5*(portX[1]-portX[0]);

	float portY[4] = {};
	portY[0] = gy(2-0.24);
	portY[1] = gy(2+0.22);
	float dY = 0.5*(portY[1]-portY[0]);
	portY[2] = portY[0] + 0.45 * dY;
	portY[3] = portY[0] +        dY;

	float gridY[LCD_ROWS + PRG_ROWS + NOB_ROWS + BUT_ROWS] = {};
	{
		std::size_t j = 0;
		float pos = 35;

		#if LCD_ROWS
		for (std::size_t row = 0; row < LCD_ROWS; ++row, ++j)
		{
			pos += rad_but() + 1.5;
			gridY[j] = pos;
			pos += rad_but() + 1.5;
		}
		#endif

		pos += 13;

		#if PRG_ROWS
		{
			pos += rad_n_s() + 4.5;
			gridY[j] = pos;
			pos += rad_n_s() + 4.5;
			++j;
		}
		#endif

		#if NOB_ROWS
		for (std::size_t row = 0; row < NOB_ROWS; ++row, ++j)
		{
			pos += rad_n_s() + 4.5;
			gridY[j] = pos;
			pos += rad_n_s() + 4.5;
		}
		#endif

		#if BUT_ROWS
		for (std::size_t row = 0; row < BUT_ROWS; ++row, ++j)
		{
			pos += rad_but() + 1.5;
			gridY[j] = pos;
			pos += rad_but() + 1.5;
		}
		#endif
	}

	#if GTX__SAVE_SVG
	{
		PanelGen pg(assetPlugin(plugin, "build/res/Seq-G2.svg"), box.size, "SEQ-G2");

		pg.rect(display_rect.pos, display_rect.size, "fill:#222222;stroke:none");

		{
			float y0 = display_rect.pos.y - 2;
			float y1 = display_rect.pos.y + display_rect.size.y + 3;

			pg.line(Vec(g_lcdX[0]-dX, y0), Vec(g_lcdX[0]-dX, y1), "fill:none;stroke:#CEE1FD;stroke-width:3");
			for (std::size_t i=3; i<LCD_COLS; i+=4)
			{
				pg.line(Vec(g_lcdX[i]+dX, y0), Vec(g_lcdX[i]+dX, y1), "fill:none;stroke:#CEE1FD;stroke-width:3");
			}
		}

		for (std::size_t i=0; i<LCD_COLS-1; i++)
		{
			double x  = 0.5 * (g_lcdX[i] + g_lcdX[i+1]);
			double y0 = gridY[LCD_ROWS + PRG_ROWS                          ] - rad_but();
			double y1 = gridY[LCD_ROWS + PRG_ROWS + NOB_ROWS + BUT_ROWS - 1] + rad_but();

			if (i % 4 == 3)
			{
				pg.line(Vec(x, y0), Vec(x, y1), "fill:none;stroke:#7092BE;stroke-width:3");
			}
			else
			{
				pg.line(Vec(x, y0), Vec(x, y1), "fill:none;stroke:#7092BE;stroke-width:1");
			}
		}

		pg.line(Vec(portX[0],    portY[0]), Vec(portX[0],    portY[1]), "fill:none;stroke:#7092BE;stroke-width:1");
		pg.line(Vec(portX[2],    portY[0]), Vec(portX[2],    portY[1]), "fill:none;stroke:#7092BE;stroke-width:1");
		pg.line(Vec(portX[3],    portY[0]), Vec(portX[3],    portY[1]), "fill:none;stroke:#7092BE;stroke-width:1");
		pg.line(Vec(portX[4],    portY[0]), Vec(portX[4],    portY[1]), "fill:none;stroke:#7092BE;stroke-width:1");
		pg.line(Vec(portX[8],    portY[0]), Vec(portX[9],    portY[0]), "fill:none;stroke:#7092BE;stroke-width:1");
		pg.line(Vec(portX[8],    portY[1]), Vec(portX[9],    portY[1]), "fill:none;stroke:#7092BE;stroke-width:1");
		pg.line(Vec(portX[4]+dX, portY[2]), Vec(portX[6],    portY[2]), "fill:none;stroke:#7092BE;stroke-width:1");
		pg.line(Vec(portX[4]+dX, portY[3]), Vec(portX[4]+dX, portY[2]), "fill:none;stroke:#7092BE;stroke-width:1");
		pg.line(Vec(portX[4],    portY[3]), Vec(portX[4]+dX, portY[3]), "fill:none;stroke:#7092BE;stroke-width:1");

		pg.line(Vec(portX[0]-dX, portY[0]-29), Vec(portX[0]-dX, portY[1]+16), "fill:none;stroke:#7092BE;stroke-width:2");
		pg.line(Vec(portX[3]+dX, portY[0]-29), Vec(portX[3]+dX, portY[1]+16), "fill:none;stroke:#7092BE;stroke-width:2");
		pg.line(Vec(portX[6]+dX, portY[0]-29), Vec(portX[6]+dX, portY[1]+16), "fill:none;stroke:#7092BE;stroke-width:2");
		pg.line(Vec(portX[9]+dX, portY[0]-29), Vec(portX[9]+dX, portY[1]+16), "fill:none;stroke:#7092BE;stroke-width:2");

		pg.nob_sml_raw(g_lcdX[ 4], gridY[LCD_ROWS], "ROW");
		pg.nob_sml_raw(g_lcdX[ 5], gridY[LCD_ROWS], "COL");
		pg.nob_sml_raw(g_lcdX[ 6], gridY[LCD_ROWS], "SPAN");
		pg.nob_sml_raw(g_lcdX[ 7], gridY[LCD_ROWS], "STRIDE");
		pg.nob_sml_raw(g_lcdX[ 8], gridY[LCD_ROWS], "NOTE");
		pg.nob_sml_raw(g_lcdX[ 9], gridY[LCD_ROWS], "OCT");
		pg.nob_sml_raw(g_lcdX[10], gridY[LCD_ROWS], "VALUE");
		pg.nob_sml_raw(g_lcdX[11], gridY[LCD_ROWS], "---");

		pg.nob_sml_raw(portX[0], portY[0], "CLOCK");
		pg.nob_sml_raw(portX[1], portY[0], "RUN");
		pg.nob_sml_raw(portX[2], portY[0], "RESET");
		pg.nob_sml_raw(portX[3], portY[0], "STEPS");
		pg.nob_sml_raw(portX[4], portY[0], "PROG");
		pg.nob_sml_raw(portX[5], portY[0], "PLAY");
		pg.nob_sml_raw(portX[6], portY[0], "EDIT");
		pg.nob_sml_raw(portX[7], portY[0], "ROWS");
		pg.nob_sml_raw(portX[8], portY[0], "CLEAR");
		pg.nob_sml_raw(portX[9], portY[0], "RAND");

		pg.nob_sml_raw(portX[1], portY[1], "EXT CLK");
		pg.nob_sml_raw(portX[7], portY[1], "COLS");
		pg.nob_sml_raw(portX[8], portY[1], "COPY");
		pg.nob_sml_raw(portX[9], portY[1], "PASTE");

		pg.tog_raw2   (portX[5], portY[2], "KNOB", "CV");
		pg.tog_raw2   (portX[6], portY[2], "KNOB", "CV");

		pg.bus_in (0, 2, "GATE");
		pg.bus_in (1, 2, "V/OCT");
		pg.bus_out(8, 2, "GATE");
		pg.bus_out(7, 2, "V/OCT");
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

	addChild(new Display(module, display_rect));

	addParam(createParam<RoundSmallBlackKnob>    (n_s(portX[0], portY[0]), module, Impl::CLOCK_PARAM, -2.0, 6.0, 2.0));
	addParam(createParam<LEDButton>              (but(portX[1], portY[0]), module, Impl::RUN_PARAM, 0.0, 1.0, 0.0));
	addChild(createLight<MediumLight<GreenLight>>(l_m(portX[1], portY[0]), module, Impl::RUNNING_LIGHT));
	addParam(createParam<LEDButton>              (but(portX[2], portY[0]), module, Impl::RESET_PARAM, 0.0, 1.0, 0.0));
	addChild(createLight<MediumLight<GreenLight>>(l_m(portX[2], portY[0]), module, Impl::RESET_LIGHT));
	addParam(createParam<RoundSmallBlackSnapKnob>(n_s(portX[3], portY[0]), module, Impl::STEPS_PARAM, 1.0, NOB_COLS, NOB_COLS));
	addParam(createParam<RoundSmallBlackSnapKnob>(n_s(portX[4], portY[0]), module, Impl::PROG_PARAM, 0.0, 11.0, 0.0));
	addParam(createParam<CKSS>                   (tog(portX[5], portY[2]), module, Impl::PLAY_PARAM, 0.0, 1.0, 1.0));
	addParam(createParam<CKSS>                   (tog(portX[6], portY[2]), module, Impl::EDIT_PARAM, 0.0, 1.0, 1.0));

	addChild(createLight<SmallLight<GreenRedLight>>(l_s(portX[5] + dX - 30, portY[1] + 5 + 1), module, Impl::PROG_LIGHT +  0*2));  // C
	addChild(createLight<SmallLight<GreenRedLight>>(l_s(portX[5] + dX - 25, portY[1] - 5 + 1), module, Impl::PROG_LIGHT +  1*2));  // C#
	addChild(createLight<SmallLight<GreenRedLight>>(l_s(portX[5] + dX - 20, portY[1] + 5 + 1), module, Impl::PROG_LIGHT +  2*2));  // D
	addChild(createLight<SmallLight<GreenRedLight>>(l_s(portX[5] + dX - 15, portY[1] - 5 + 1), module, Impl::PROG_LIGHT +  3*2));  // Eb
	addChild(createLight<SmallLight<GreenRedLight>>(l_s(portX[5] + dX - 10, portY[1] + 5 + 1), module, Impl::PROG_LIGHT +  4*2));  // E
	addChild(createLight<SmallLight<GreenRedLight>>(l_s(portX[5] + dX     , portY[1] + 5 + 1), module, Impl::PROG_LIGHT +  5*2));  // F
	addChild(createLight<SmallLight<GreenRedLight>>(l_s(portX[5] + dX +  5, portY[1] - 5 + 1), module, Impl::PROG_LIGHT +  6*2));  // Fs
	addChild(createLight<SmallLight<GreenRedLight>>(l_s(portX[5] + dX + 10, portY[1] + 5 + 1), module, Impl::PROG_LIGHT +  7*2));  // G
	addChild(createLight<SmallLight<GreenRedLight>>(l_s(portX[5] + dX + 15, portY[1] - 5 + 1), module, Impl::PROG_LIGHT +  8*2));  // Ab
	addChild(createLight<SmallLight<GreenRedLight>>(l_s(portX[5] + dX + 20, portY[1] + 5 + 1), module, Impl::PROG_LIGHT +  9*2));  // A
	addChild(createLight<SmallLight<GreenRedLight>>(l_s(portX[5] + dX + 25, portY[1] - 5 + 1), module, Impl::PROG_LIGHT + 10*2));  // Bb
	addChild(createLight<SmallLight<GreenRedLight>>(l_s(portX[5] + dX + 30, portY[1] + 5 + 1), module, Impl::PROG_LIGHT + 11*2));  // B

	addParam(createParam<RoundSmallBlackSnapKnob>(n_s(portX[7], portY[0]), module, Impl::SPAN_R_PARAM, 1.0, 8.0, 1.0));
	addParam(createParam<RoundSmallBlackSnapKnob>(n_s(portX[7], portY[1]), module, Impl::SPAN_C_PARAM, 1.0, 8.0, 1.0));
	addParam(createParam<LEDButton>              (but(portX[8], portY[0]), module, Impl::CLEAR_PARAM, 0.0, 1.0, 0.0));
	addChild(createLight<MediumLight<GreenLight>>(l_m(portX[8], portY[0]), module, Impl::CLEAR_LIGHT));
	addParam(createParam<LEDButton>              (but(portX[9], portY[0]), module, Impl::RANDOM_PARAM, 0.0, 1.0, 0.0));
	addChild(createLight<MediumLight<GreenLight>>(l_m(portX[9], portY[0]), module, Impl::RANDOM_LIGHT));
	addParam(createParam<LEDButton>              (but(portX[8], portY[1]), module, Impl::COPY_PARAM, 0.0, 1.0, 0.0));
	addChild(createLight<MediumLight<GreenLight>>(l_m(portX[8], portY[1]), module, Impl::COPY_LIGHT));
	addParam(createParam<LEDButton>              (but(portX[9], portY[1]), module, Impl::PASTE_PARAM, 0.0, 1.0, 0.0));
	addChild(createLight<MediumLight<GreenLight>>(l_m(portX[9], portY[1]), module, Impl::PASTE_LIGHT));

	addInput(createInputGTX<IPrtMed>(Vec(portX[0], portY[1]), module, Impl::CLOCK_INPUT));
	addInput(createInputGTX<IPrtMed>(Vec(portX[1], portY[1]), module, Impl::EXT_CLOCK_INPUT));
	addInput(createInputGTX<IPrtMed>(Vec(portX[2], portY[1]), module, Impl::RESET_INPUT));
	addInput(createInputGTX<IPrtMed>(Vec(portX[3], portY[1]), module, Impl::STEPS_INPUT));
	addInput(createInputGTX<IPrtMed>(Vec(portX[4], portY[1]), module, Impl::PROG_INPUT));

	{
		std::size_t j = 0;

		#if LCD_ROWS
		for (std::size_t row = 0; row < LCD_ROWS; ++row, ++j)
		{
			if (OUT_LEFT ) addOutput(createOutputGTX<OPrtSml>(Vec(gridXl, gridY[j]), module, Impl::lcd_val_map(row, 0)));
			if (OUT_RIGHT) addOutput(createOutputGTX<OPrtSml>(Vec(gridXr, gridY[j]), module, Impl::lcd_val_map(row, 1)));
		}
		#endif

		#if PRG_ROWS
		++j;
		#endif

		#if NOB_ROWS
		for (std::size_t row = 0; row < NOB_ROWS; ++row, ++j)
		{
			if (OUT_LEFT ) addOutput(createOutputGTX<OPrtSml>(Vec(gridXl, gridY[j]), module, Impl::nob_val_map(row, 0)));
			if (OUT_RIGHT) addOutput(createOutputGTX<OPrtSml>(Vec(gridXr, gridY[j]), module, Impl::nob_val_map(row, 1)));
		}
		#endif

		#if BUT_ROWS
		for (std::size_t row = 0; row < BUT_ROWS; ++row, ++j)
		{
			if (OUT_LEFT ) addOutput(createOutputGTX<OPrtSml>(Vec(gridXl, gridY[j]), module, Impl::but_val_map(row, 0)));
			if (OUT_RIGHT) addOutput(createOutputGTX<OPrtSml>(Vec(gridXr, gridY[j]), module, Impl::but_val_map(row, 1)));
		}
		#endif
	}

	{
		std::size_t j = 0;

		#if LCD_ROWS
		for (std::size_t row = 0; row < LCD_ROWS; ++row, ++j)
		{
			;
		}
		#endif

		#if PRG_ROWS
		{
			addParam(createParam<RoundSmallBlackSnapKnob>(n_s(g_lcdX[ 4], gridY[j]), module, Impl::PRG_ROW_PARAM,    0.0, LCD_ROWS - 1, 0.0));
			addParam(createParam<RoundSmallBlackSnapKnob>(n_s(g_lcdX[ 5], gridY[j]), module, Impl::PRG_COL_PARAM,    0.0, LCD_COLS - 1, 0.0));
			addParam(createParam<RoundSmallBlackSnapKnob>(n_s(g_lcdX[ 6], gridY[j]), module, Impl::PRG_SPAN_PARAM,   1.0, LCD_COLS    , 1.0));
			addParam(createParam<RoundSmallBlackSnapKnob>(n_s(g_lcdX[ 7], gridY[j]), module, Impl::PRG_STRIDE_PARAM, 1.0, LCD_COLS - 1, 1.0));
			addParam(createParam<RoundSmallBlackSnapKnob>(n_s(g_lcdX[ 8], gridY[j]), module, Impl::PRG_NOTE_PARAM,   0.0,         11.0, 0.0));
			addParam(createParam<RoundSmallBlackSnapKnob>(n_s(g_lcdX[ 9], gridY[j]), module, Impl::PRG_OCTAVE_PARAM, 0.0,          8.0, 4.0));
			addParam(createParam<RoundSmallBlackKnob>    (n_s(g_lcdX[10], gridY[j]), module, Impl::PRG_VALUE_PARAM,  0.0,         10.0, 0.0));
			addParam(createParam<RoundSmallBlackSnapKnob>(n_s(g_lcdX[11], gridY[j]), module, Impl::PRG_GATE_PARAM,   0.0,          3.0, 0.0));
			++j;
		}
		#endif

		#if NOB_ROWS
		for (std::size_t row = 0; row < NOB_ROWS; ++row, ++j)
		{
			for (std::size_t col = 0; col < NOB_COLS; ++col)
			{
				if (Impl::is_nob_snap(row))
				{
					addParam(createParam<RoundSmallBlackSnapKnob>(n_s(g_nobX[col], gridY[j]), module, Impl::nob_map(row, col), 0.0, 12.0, 0.0));
				}
				else
				{
					addParam(createParam<RoundSmallBlackKnob>    (n_s(g_nobX[col], gridY[j]), module, Impl::nob_map(row, col), 0.0, 10.0, 0.0));
				}
			}
		}
		#endif

		#if BUT_ROWS
		for (std::size_t row = 0; row < BUT_ROWS; ++row, ++j)
		{
			for (std::size_t col = 0; col < BUT_COLS; ++col)
			{
				addParam(createParam<LEDButton>                     (but(g_butX[col], gridY[j]), module, Impl::but_map(row, col), 0.0, 1.0, 0.0));
				addChild(createLight<MediumLight<RedGreenBlueLight>>(l_m(g_butX[col], gridY[j]), module, Impl::led_map(row, col, 0)));
			}
		}
		#endif
	}

	for (std::size_t i=0; i<GTX__N; ++i)
	{
		addInput(createInputGTX<IPrtMed>  (Vec(px(0, i), py(2, i)), module, Impl::imap(Impl::GATE_INPUT, i)));
		addInput(createInputGTX<IPrtMed>  (Vec(px(1, i), py(2, i)), module, Impl::imap(Impl::VOCT_INPUT, i)));

		addOutput(createOutputGTX<OPrtMed>(Vec(px(8, i), py(2, i)), module, Impl::omap(Impl::GATE_OUTPUT, i)));
		addOutput(createOutputGTX<OPrtMed>(Vec(px(7, i), py(2, i)), module, Impl::omap(Impl::VOCT_OUTPUT, i)));
	}

	addInput(createInputGTX<IPrtMed>(Vec(gx(0), gy(2)), module, Impl::imap(Impl::GATE_INPUT, GTX__N)));
	addInput(createInputGTX<IPrtMed>(Vec(gx(1), gy(2)), module, Impl::imap(Impl::VOCT_INPUT, GTX__N)));
}


} // Seq_G2
} // GTX
