#pragma once
#include "GlobalDeclarations.h"

class Plugin
{
public:
	Plugin(AEffect* effect);

	// lifecycle methods
	void open();
	void close();
	void suspend();
	void resume();

	// info methods
	void displayInfo();
	void displayCanDo();

private:
	AEffect* e;
	AEffectDispatcherProc dispatcher;
	AEffectDispatcherProc configurePlugin();
};


