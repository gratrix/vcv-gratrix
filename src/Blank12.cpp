#include "Gratrix.hpp"


struct Blank12 : Module
{
	Blank12() : Module(0, 0, 0) {}
};


Blank12Widget::Blank12Widget()
{
	Blank12 *module = new Blank12();
	setModule(module);
	box.size = Vec(12*15, 380);

	{
		PanelGen pg(assetPlugin(plugin, "res/Blank12.svg"), box.size);
	}

	{
/*		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Blank12.svg")));
		addChild(panel);
*/
		Panel *panel = new LightPanel();
		panel->box.size = box.size;
		panel->backgroundImage = Image::load(assetPlugin(plugin, "res/Blank12.png"));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));
}
