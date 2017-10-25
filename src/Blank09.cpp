#include "Gratrix.hpp"


struct Blank9 : Module
{
	Blank9() : Module(0, 0, 0) {}
};


Blank9Widget::Blank9Widget()
{
	Blank9 *module = new Blank9();
	setModule(module);
	box.size = Vec(9*15, 380);

	{
		PanelGen pg(assetPlugin(plugin, "res/Blank09.svg"), box.size);
	}

	{
/*		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Blank09.svg")));
		addChild(panel);
*/
		Panel *panel = new LightPanel();
		panel->box.size = box.size;
		panel->backgroundImage = Image::load(assetPlugin(plugin, "res/Blank09.png"));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));
}
