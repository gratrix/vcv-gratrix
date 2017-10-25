#include "Gratrix.hpp"


struct Blank6 : Module
{
	Blank6() : Module(0, 0, 0) {}
};


Blank6Widget::Blank6Widget()
{
	Blank6 *module = new Blank6();
	setModule(module);
	box.size = Vec(6*15, 380);

	{
		PanelGen pg(assetPlugin(plugin, "res/Blank06.svg"), box.size);
	}

	{
/*		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Blank06.svg")));
		addChild(panel);
*/
		Panel *panel = new LightPanel();
		panel->box.size = box.size;
		panel->backgroundImage = Image::load(assetPlugin(plugin, "res/Blank06.png"));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));
}
