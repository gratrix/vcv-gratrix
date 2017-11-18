//============================================================================================================
//!
//! \file Blank03.cpp
//!
//! \brief Blank 3 is a simple do nothing 3-hole high quality blank.
//!
//============================================================================================================


#include "Gratrix.hpp"


namespace GTX {
namespace Blank_03 {


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
	box.size = Vec(3*15, 380);

	#if GTX__SAVE_SVG
	{
		PanelGen pg(assetPlugin(plugin, "build/res/Blank03.svg"), box.size);
	}
	#endif

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Blank03.svg")));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
}


} // Blank_03
} // GTX
