#include "Gratrix.hpp"


struct Blank3 : Module
{
	Blank3() : Module(0, 0, 0) {}
};


Blank3Widget::Blank3Widget()
{
	Blank3 *module = new Blank3();
	setModule(module);
	box.size = Vec(3*15, 380);

	{
		PanelGen pg(assetPlugin(plugin, "res/Blank03.svg"), box.size);
	}

	{
/*		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Blank03.svg")));
		addChild(panel);
*/
		Panel *panel = new LightPanel();
		panel->box.size = box.size;
		panel->backgroundImage = Image::load(assetPlugin(plugin, "res/Blank03.png"));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
}
