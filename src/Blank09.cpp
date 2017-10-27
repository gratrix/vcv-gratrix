#include "Gratrix.hpp"


struct Blank9 : Module
{
	Blank9() : Module(0, 0, 0) {}
};


Blank9Widget::Blank9Widget()
{
	GTX__WIDGET();

	Blank9 *module = new Blank9();
	setModule(module);
	box.size = Vec(9*15, 380);

	#if GTX__SAVE_SVG
	{
		PanelGen pg(assetPlugin(plugin, "res/Blank09.svg"), box.size);
	}
	#endif

	{
		#if GTX__LOAD_SVG
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Blank09.svg")));
		#else
		Panel *panel = new LightPanel();
		panel->box.size = box.size;
		panel->backgroundImage = Image::load(assetPlugin(plugin, "res/Blank09.png"));
		#endif
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));
}
