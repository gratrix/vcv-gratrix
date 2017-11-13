#include "Gratrix.hpp"


Plugin *plugin;

void init(rack::Plugin *p) {
	plugin = p;
	p->slug = "Gratrix";
#ifdef VERSION
	p->version = TOSTRING(VERSION);
#endif
	p->website = "http://gratrix.net/vcvrack";

	p->addModel(createModel<GTX::MIDI_C1::Widget> ("Gratrix", "MIDI-C1",  "MIDI-C1",  MIDI_TAG, EXTERNAL_TAG));
	p->addModel(createModel<GTX::MIDI_G1::Widget> ("Gratrix", "MIDI-G1",  "MIDI-G1",  MIDI_TAG, EXTERNAL_TAG));
	p->addModel(createModel<VCO_F1_Widget>        ("Gratrix", "VCO-F1",   "VCO-F1",   OSCILLATOR_TAG));
	p->addModel(createModel<VCO_F2_Widget>        ("Gratrix", "VCO-F2",   "VCO-F2",   OSCILLATOR_TAG));
	p->addModel(createModel<VCF_F1_Widget>        ("Gratrix", "VCF-F1",   "VCF-F1",   FILTER_TAG));
	p->addModel(createModel<VCAWidget>            ("Gratrix", "VCA-F1",   "VCA-F1",   AMPLIFIER_TAG));
	p->addModel(createModel<ADSRWidget>           ("Gratrix", "Env-F1",   "Env-F1",   ENVELOPE_GENERATOR_TAG));
	p->addModel(createModel<Chord_G1_Widget>      ("Gratrix", "Chord-G1", "Chord-G1", SYNTH_VOICE_TAG));  // right tag?
	p->addModel(createModel<GTX::Fade_G1::Widget> ("Gratrix", "Fade-G1",  "Fade-G1",  MIXER_TAG));        // right tag?
	p->addModel(createModel<GTX::Fade_G2::Widget> ("Gratrix", "Fade-G2",  "Fade-G2",  MIXER_TAG));        // right tag?
	p->addModel(createModel<VU_G1_Widget>         ("Gratrix", "VU-G1",    "VU-G1",    VISUAL_TAG));
//	p->addModel(createModel<MuxWidget>            ("Gratrix", "Mux",      "Mux",      MIXER_TAG));
//	p->addModel(createModel<SplitWidget>          ("Gratrix", "Split",    "Split",    MULTIPLE_TAG));
	p->addModel(createModel<OctaveWidget>         ("Gratrix", "Octave",   "Octave",   SYNTH_VOICE_TAG));  // right tag?
	p->addModel(createModel<KeysWidget>           ("Gratrix", "Keys-G1",  "Keys-G1",  VISUAL_TAG));
	p->addModel(createModel<GTX::Blank_03::Widget>("Gratrix", "Blank3",   "Blank 3",  BLANK_TAG));
	p->addModel(createModel<GTX::Blank_06::Widget>("Gratrix", "Blank6",   "Blank 6",  BLANK_TAG));
	p->addModel(createModel<GTX::Blank_09::Widget>("Gratrix", "Blank9",   "Blank 9",  BLANK_TAG));
	p->addModel(createModel<GTX::Blank_12::Widget>("Gratrix", "Blank12",  "Blank 12", BLANK_TAG));
}
