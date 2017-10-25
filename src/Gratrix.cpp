#include "Gratrix.hpp"


Plugin *plugin;

void init(rack::Plugin *p) {
	plugin = p;
	plugin->slug = "Gratrix";
	plugin->name = "Gratrix";
	plugin->homepageUrl = "http://gratrix.net/vcvrack";
	createModel<VCO_F1_Widget> (plugin, "VCO-F1", "VCO-F1");
	createModel<VCO_F2_Widget> (plugin, "VCO-F2", "VCO-F2");
	createModel<VCF_F1_Widget> (plugin, "VCF-F1", "VCF-F1");
	createModel<VCAWidget>     (plugin, "VCA-F1", "VCA-F1");
	createModel<ADSRWidget>    (plugin, "Env-F1", "Env-F1");
	createModel<Chord12Widget> (plugin, "Chord12", "Chord 12");
	createModel<Fade_G1_Widget>(plugin, "Fade-G1", "Fade-G1");
	createModel<Fade_G2_Widget>(plugin, "Fade-G2", "Fade-G2");
	createModel<MuxWidget>     (plugin, "Mux", "Mux");
	createModel<SplitWidget>   (plugin, "Split", "Split");
	createModel<OctaveWidget>  (plugin, "Octave", "Octave");
	createModel<Blank3Widget>  (plugin, "Blank3", "Blank 3");
	createModel<Blank6Widget>  (plugin, "Blank6", "Blank 6");
	createModel<Blank9Widget>  (plugin, "Blank9", "Blank 9");
	createModel<Blank12Widget> (plugin, "Blank12", "Blank 12");
}
