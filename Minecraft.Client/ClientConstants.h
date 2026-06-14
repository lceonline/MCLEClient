#pragma once
using namespace std;

class ClientConstants
{

	// This file holds global constants used by the client.
	// The file should be replaced at compile-time with the
	// proper settings for the given compilation. For example,
	// release builds should replace this file with no-cheat
	// settings.

	// INTERNAL DEVELOPMENT SETTINGS
public:
	static const wstring VERSION_STRING;
	static const wstring LCEN_STRING;
	static const wchar_t* LCEN_HOST;
	static std::wstring GetLCENString();

	static const bool DEADMAU5_CAMERA_CHEATS = false;
};