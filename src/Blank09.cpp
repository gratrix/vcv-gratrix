//============================================================================================================
//!
//! \file Blank09.cpp
//!
//! \brief Blank 9 is a simple do nothing 9-hole high quality blank.
//!
//============================================================================================================


#include "Gratrix.hpp"


namespace GTX {
namespace Blank_09 {


//============================================================================================================
//! \brief The implementation.

struct Impl : Module
{
	Impl() : Module(0, 0, 0) {}
};


//============================================================================================================
//! \brief The widget.

Widget::Widget()
{
	GTX__WIDGET();

	Impl *module = new Impl();
	setModule(module);
	box.size = Vec(9*15, 380);

	#if GTX__SAVE_SVG
	{
		PanelGen pg(assetPlugin(plugin, "build/res/Blank09.svg"), box.size);
	}
	#endif

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Blank09.svg")));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));
}


} // Blank_09
} // GTX
