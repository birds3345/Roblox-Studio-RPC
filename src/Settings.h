#pragma once

#include <string>

#include "Jsonify.h"

namespace Settings
{
	std::string getName();

	bool fileExists();

	void makeDefault();
	Jsonify::JsonValue getSettings();
}