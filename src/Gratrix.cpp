#include "Gratrix.hpp"


Plugin *plugin;

void init(rack::Plugin *p) {
	plugin = p;
	p->slug = "Gratrix";
#ifdef VERSION
	p->version = TOSTRING(VERSION);
#endif
	p->addModel(createModel<GtxMidiWidget> ("Gratrix", "Gratrix", "MIDI-C1", "MIDI-C1"));
	p->addModel(createModel<VCO_F1_Widget> ("Gratrix", "Gratrix", "VCO-F1",  "VCO-F1"));
	p->addModel(createModel<VCO_F2_Widget> ("Gratrix", "Gratrix", "VCO-F2",  "VCO-F2"));
	p->addModel(createModel<VCF_F1_Widget> ("Gratrix", "Gratrix", "VCF-F1",  "VCF-F1"));
	p->addModel(createModel<VCAWidget>     ("Gratrix", "Gratrix", "VCA-F1",  "VCA-F1"));
	p->addModel(createModel<ADSRWidget>    ("Gratrix", "Gratrix", "Env-F1",  "Env-F1"));
	p->addModel(createModel<Chord12Widget> ("Gratrix", "Gratrix", "Chord12", "Chord 12"));
	p->addModel(createModel<Fade_G1_Widget>("Gratrix", "Gratrix", "Fade-G1", "Fade-G1"));
	p->addModel(createModel<Fade_G2_Widget>("Gratrix", "Gratrix", "Fade-G2", "Fade-G2"));
//	p->addModel(createModel<MuxWidget>     ("Gratrix", "Gratrix", "Mux",     "Mux"));
//	p->addModel(createModel<SplitWidget>   ("Gratrix", "Gratrix", "Split",   "Split"));
	p->addModel(createModel<OctaveWidget>  ("Gratrix", "Gratrix", "Octave",  "Octave"));
	p->addModel(createModel<KeysWidget>    ("Gratrix", "Gratrix", "Keys",    "Keys"));
	p->addModel(createModel<Blank3Widget>  ("Gratrix", "Gratrix", "Blank3",  "Blank 3"));
	p->addModel(createModel<Blank6Widget>  ("Gratrix", "Gratrix", "Blank6",  "Blank 6"));
	p->addModel(createModel<Blank9Widget>  ("Gratrix", "Gratrix", "Blank9",  "Blank 9"));
	p->addModel(createModel<Blank12Widget> ("Gratrix", "Gratrix", "Blank12", "Blank 12"));
}
