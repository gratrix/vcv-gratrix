#include <list>
#include <algorithm>
#include "rtmidi/RtMidi.h"
#include "core.hpp"
#include "MidiIO.hpp"
#include "dsp/digital.hpp"
#include "Gratrix.hpp"

struct GtxMidiKey {
	int pitch = 60;
	int at = 0; // aftertouch
	int vel = 0; // velocity
	int retriggerC = 0;
	bool gate = false;
};

struct GtxMidiInterface : MidiIO, Module {
	enum ParamIds {
		RESET_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		PITCH_OUTPUT,     // N
		GATE_OUTPUT,      // N
		VELOCITY_OUTPUT,  // N
		AT_OUTPUT,        // N
		NUM_OUTPUTS,
		OFF_OUTPUTS = PITCH_OUTPUT
	};
	enum LightIds {
		RESET_LIGHT,
		NUM_LIGHTS
	};

	enum Modes {
		ROTATE,
		RESET,
		REASSIGN
	};

	static constexpr std::size_t omap(std::size_t port, std::size_t bank)
	{
		return port + bank * NUM_OUTPUTS;
	}

	bool pedal = false;

	int mode = REASSIGN;

	int getMode() const;

	void setMode(int mode);

	GtxMidiKey activeKeys[GTX__N];
	std::list<int> open;

	SchmittTrigger resetTrigger;

	GtxMidiInterface() : MidiIO(), Module(NUM_PARAMS, NUM_INPUTS, GTX__N * (NUM_OUTPUTS - OFF_OUTPUTS) + OFF_OUTPUTS, NUM_LIGHTS) {
	}

	~GtxMidiInterface() {
	};

	void step();

	void processMidi(std::vector<unsigned char> msg);

	json_t *toJson() {
		json_t *rootJ = json_object();
		addBaseJson(rootJ);
		return rootJ;
	}

	void fromJson(json_t *rootJ) {
		baseFromJson(rootJ);
	}

	void reset() {
		resetMidi();
	}

	void resetMidi();

};

void GtxMidiInterface::resetMidi() {

	for (int i = 0; i < GTX__N; i++) {
		outputs[GATE_OUTPUT + i].value = 0.0;
		activeKeys[i].gate = false;
		activeKeys[i].vel = 0;
		activeKeys[i].at = 0;
	}

	open.clear();

	pedal = false;
	lights[RESET_LIGHT].value = 1.0;
}

void GtxMidiInterface::step() {
	static int msgsProcessed = 0;

	if (isPortOpen()) {
		std::vector<unsigned char> message;

		// midiIn->getMessage returns empty vector if there are no messages in the queue
		// NOTE: For the quadmidi we will process max GTX__N midi messages per step to avoid
		// problems with parallel input.
		getMessage(&message);
		while (msgsProcessed < GTX__N && message.size() > 0) {
			processMidi(message);
			getMessage(&message);
			msgsProcessed++;
		}
		msgsProcessed = 0;
	}


	for (int i = 0; i < GTX__N; i++) {
		outputs[omap(    GATE_OUTPUT, i)].value = activeKeys[i].gate ? 10.0 : 0;
		outputs[omap(   PITCH_OUTPUT, i)].value = (activeKeys[i].pitch - 60) / 12.0;
		outputs[omap(VELOCITY_OUTPUT, i)].value = activeKeys[i].vel / 127.0 * 10.0;
		outputs[omap(      AT_OUTPUT, i)].value = activeKeys[i].at / 127.0 * 10.0;
	}

	if (resetTrigger.process(params[RESET_PARAM].value)) {
		resetMidi();
		return;
	}

	lights[RESET_LIGHT].value -= lights[RESET_LIGHT].value / 0.55 / engineGetSampleRate(); // fade out light
}


void GtxMidiInterface::processMidi(std::vector<unsigned char> msg) {
	int channel = msg[0] & 0xf;
	int status = (msg[0] >> 4) & 0xf;
	int data1 = msg[1];
	int data2 = msg[2];

	static int gate;

	// Filter channels
	if (this->channel >= 0 && this->channel != channel)
		return;

	switch (status) {
		// note off
		case 0x8: {
			gate = false;
		}
			break;
		case 0x9: // note on
			if (data2 > 0) {
				gate = true;
			} else {
				// For some reason, some keyboards send a "note on" event with a velocity of 0 to signal that the key has been released.
				gate = false;
			}
			break;
		case 0xa: // channel aftertouch
			for (int i = 0; i < GTX__N; i++) {
				if (activeKeys[i].pitch == data1) {
					activeKeys[i].at = data2;
				}
			}
			return;
		case 0xb: // cc
			if (data1 == 0x40) { // pedal
				pedal = (data2 >= 64);
				if (!pedal) {
					for (int i = 0; i < GTX__N; i++) {
						activeKeys[i].gate = false;
						open.push_back(i);
					}
				}
			}
			return;
		default:
			return;
	}

	if (!pedal && !gate) {
		for (int i = 0; i < GTX__N; i++) {
			if (activeKeys[i].pitch == data1) {
				activeKeys[i].gate = false;
				activeKeys[i].vel = data2;
				if (std::find(open.begin(), open.end(), i) != open.end()) {
					open.remove(i);
				}
				open.push_front(i);
			}
		}
		return;
	}

	if (open.empty()) {
		open.clear();
		for (int i = 0; i < GTX__N; i++) {
			open.push_back(i);
		}
	}


	if (!activeKeys[0].gate && !activeKeys[1].gate &&
		!activeKeys[2].gate && !activeKeys[3].gate &&
		!activeKeys[4].gate && !activeKeys[5].gate) {
		open.sort();
	}

	switch (mode) {
		case RESET:
			if (open.size() == GTX__N ) {
				for (int i = 0; i < GTX__N; i++) {
					activeKeys[i].gate = false;
					open.push_back(i);
				}
			}
			break;
		case REASSIGN:
			open.push_back(open.front());
			break;
		case ROTATE:
			break;
		default:
			fprintf(stderr, "No mode selected?!\n");
	}

	activeKeys[open.front()].gate = true;
	activeKeys[open.front()].pitch = data1;
	activeKeys[open.front()].vel = data2;
	open.pop_front();
	return;


}

int GtxMidiInterface::getMode() const {
	return mode;
}

void GtxMidiInterface::setMode(int mode) {
	resetMidi();
	GtxMidiInterface::mode = mode;
}

struct ModeItem : MenuItem {
	int mode;
	GtxMidiInterface *module;

	void onAction() {
		module->setMode(mode);
	}
};

struct ModeChoice : ChoiceButton {
	GtxMidiInterface *module;
	const std::vector<std::string> modeNames = {"ROTATE MODE", "RESET MODE", "REASSIGN MODE"};


	void onAction() {
		Menu *menu = gScene->createMenu();
		menu->box.pos = getAbsoluteOffset(Vec(0, box.size.y)).round();
		menu->box.size.x = box.size.x;

		for (unsigned long i = 0; i < modeNames.size(); i++) {
			ModeItem *modeItem = new ModeItem();
			modeItem->mode = i;
			modeItem->module = module;
			modeItem->text = modeNames[i];
			menu->pushChild(modeItem);
		}
	}

	void step() {
		text = modeNames[module->getMode()];
	}
};


GtxMidiWidget::GtxMidiWidget()
{
	GTX__WIDGET();

	GtxMidiInterface *module = new GtxMidiInterface();
	setModule(module);
	box.size = Vec(12*15, 380);

	#if GTX__SAVE_SVG
	{
		PanelGen pg(assetPlugin(plugin, "build/res/MIDI-C1.svg"), box.size, "MIDI-C1");

		pg.but2(0.5, 0.4, "RESET");

		pg.bus_out(0, 1, "VEL"); pg.bus_out(1, 1, "GATE");
		pg.bus_out(0, 2, "AFT"); pg.bus_out(1, 2, "V/OCT");
	}
	#endif

	{
		#if GTX__LOAD_SVG
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/MIDI-C1.svg")));
		#else
		Panel *panel = new LightPanel();
		panel->box.size = box.size;
		panel->backgroundImage = Image::load(assetPlugin(plugin, "res/MIDI-C1.png"));
		#endif
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 365)));

	addParam(createParam<LEDButton>(           but(fx(0.5), fy(0.4)), module, GtxMidiInterface::RESET_PARAM, 0.0, 1.0, 0.0));
	addChild(createLight<SmallLight<RedLight>>(led(fx(0.5), fy(0.4)), module, GtxMidiInterface::RESET_LIGHT));

	{
		float margin =  8;
		float yPos   = 42;

		{
			MidiChoice *midiChoice = new MidiChoice();
			midiChoice->midiModule = dynamic_cast<MidiIO *>(module);
			midiChoice->box.pos = Vec(margin, yPos);
			midiChoice->box.size.x = box.size.x - margin * 2;
			addChild(midiChoice);
			yPos += midiChoice->box.size.y + margin;
		}

		{
			ChannelChoice *channelChoice = new ChannelChoice();
			channelChoice->midiModule = dynamic_cast<MidiIO *>(module);
			channelChoice->box.pos = Vec(margin, yPos);
			channelChoice->box.size.x = box.size.x - margin * 2;
			addChild(channelChoice);
			yPos += channelChoice->box.size.y + margin;
		}

		{
			ModeChoice *modeChoice = new ModeChoice();
			modeChoice->module = module;
			modeChoice->box.pos = Vec(margin, yPos);
			modeChoice->box.size.x = box.size.x - margin * 2;
			addChild(modeChoice);
		}
	}

	for (std::size_t i=0; i<GTX__N; ++i)
	{
		addOutput(createOutput<PJ301MPort>(prt(px(1, i), py(2, i)), module, GtxMidiInterface::omap(GtxMidiInterface::   PITCH_OUTPUT, i)));
		addOutput(createOutput<PJ301MPort>(prt(px(1, i), py(1, i)), module, GtxMidiInterface::omap(GtxMidiInterface::    GATE_OUTPUT, i)));
		addOutput(createOutput<PJ301MPort>(prt(px(0, i), py(1, i)), module, GtxMidiInterface::omap(GtxMidiInterface::VELOCITY_OUTPUT, i)));
		addOutput(createOutput<PJ301MPort>(prt(px(0, i), py(2, i)), module, GtxMidiInterface::omap(GtxMidiInterface::      AT_OUTPUT, i)));
	}
}

void GtxMidiWidget::step()
{
	ModuleWidget::step();
}
