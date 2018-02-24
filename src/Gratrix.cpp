#include "Gratrix.hpp"


Plugin *plugin;

void init(rack::Plugin *p)
{
	plugin     = p;
	p->slug    = TOSTRING(SLUG);
	p->version = TOSTRING(VERSION);

//	p->addModel(GTX::MIDI_C1  ::model);
//	p->addModel(GTX::MIDI_G1  ::model);
//	p->addModel(GTX::VCO_F1   ::model);
//	p->addModel(GTX::VCO_F2   ::model);
//	p->addModel(GTX::VCF_F1   ::model);
//	p->addModel(GTX::VCA_F1   ::model);
	p->addModel(GTX::ADSR_F1  ::model);
//	p->addModel(GTX::Chord_G1 ::model);
//	p->addModel(GTX::Octave_G1::model);
	p->addModel(GTX::Fade_G1  ::model);
	p->addModel(GTX::Fade_G2  ::model);
//	p->addModel(GTX::Binary_G1::model);
//	p->addModel(GTX::Seq_G1   ::model);
//	p->addModel(GTX::Seq_G2   ::model);
//	p->addModel(GTX::Keys_G1  ::model);
//	p->addModel(GTX::VU_G1    ::model);
//	p->addModel(GTX::Scope_G1 ::model);
	p->addModel(GTX::Blank_03 ::model);
	p->addModel(GTX::Blank_06 ::model);
	p->addModel(GTX::Blank_09 ::model);
	p->addModel(GTX::Blank_12 ::model);
}

/*
	p->addModel(createModel<GTX::MIDI_C1::Widget>  ("Gratrix", "MIDI-C1",   "MIDI-C1",   MIDI_TAG, EXTERNAL_TAG));
	p->addModel(createModel<GTX::MIDI_G1::Widget>  ("Gratrix", "MIDI-G1",   "MIDI-G1",   MIDI_TAG, EXTERNAL_TAG));
	p->addModel(createModel<GTX::VCO_F1::Widget>   ("Gratrix", "VCO-F1",    "VCO-F1",    OSCILLATOR_TAG));
	p->addModel(createModel<GTX::VCO_F2::Widget>   ("Gratrix", "VCO-F2",    "VCO-F2",    OSCILLATOR_TAG));
	p->addModel(createModel<GTX::VCF_F1::Widget>   ("Gratrix", "VCF-F1",    "VCF-F1",    FILTER_TAG));
	p->addModel(createModel<GTX::VCA_F1::Widget>   ("Gratrix", "VCA-F1",    "VCA-F1",    AMPLIFIER_TAG));
	p->addModel(createModel<GTX::Chord_G1::Widget> ("Gratrix", "Chord-G1",  "Chord-G1",  SYNTH_VOICE_TAG));  // right tag?
	p->addModel(createModel<GTX::Octave_G1::Widget>("Gratrix", "Octave-G1", "Octave-G1", SYNTH_VOICE_TAG));  // right tag?
	p->addModel(createModel<GTX::Binary_G1::Widget>("Gratrix", "Binary-G1", "Binary-G1", LOGIC_TAG));
	p->addModel(createModel<GTX::Seq_G1::Widget>   ("Gratrix", "Seq-G1",    "Seq-G1",    SEQUENCER_TAG));
	p->addModel(createModel<GTX::Seq_G2::Widget>   ("Gratrix", "Seq-G2",    "Seq-G2",    SEQUENCER_TAG));
	p->addModel(createModel<GTX::Keys_G1::Widget>  ("Gratrix", "Keys-G1",   "Keys-G1",   VISUAL_TAG));
	p->addModel(createModel<GTX::VU_G1::Widget>    ("Gratrix", "VU-G1",     "VU-G1",     VISUAL_TAG));
	p->addModel(createModel<GTX::Scope_G1::Widget> ("Gratrix", "Scope-G1",  "Scope-G1",  VISUAL_TAG));
*/
