#include "stdafx.h"
#include "UI.h"
#include "../../../Minecraft.World/StringHelpers.h"
#include "../../../Minecraft.World/File.h"
#include "UITTFFont.h"

UITTFFont::UITTFFont(const string &name, const string &path, S32 fallbackCharacter, bool registerAsDefaultFonts)
	: m_strFontName(name),
	  pbData(nullptr),
	  m_loaded(false) // check if loaded
{
	app.DebugPrintf("UITTFFont opening %s\n",path.c_str());

#ifdef _UNICODE
	wstring wPath = convStringToWstring(path);
	HANDLE file = CreateFile(wPath.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
#else
	HANDLE file = CreateFile(path.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
#endif
	if( file == INVALID_HANDLE_VALUE )
	{
		DWORD error = GetLastError();
		app.DebugPrintf("Failed to open TTF file with error code %d (%x)\n", error, error);
		return; // Fireblade - replaced assert to avoid crashing if font fails to load
	}

	DWORD dwHigh=0;
	DWORD dwFileSize = GetFileSize(file,&dwHigh);

	if(dwFileSize!=0)
	{
		DWORD bytesRead;

		pbData =  (PBYTE) new BYTE[dwFileSize];
		BOOL bSuccess = ReadFile(file,pbData,dwFileSize,&bytesRead,nullptr);
		if(bSuccess==FALSE)
		{
			delete[] pbData; // Fireblade - avoid memory leaks (hopefully)
			pbData = nullptr;
			CloseHandle(file);
			app.FatalLoadError();
			return; // Fireblade - return early in case of error
		}
		CloseHandle(file);

		IggyFontInstallTruetypeUTF8 ( (void *)pbData, IGGY_TTC_INDEX_none, m_strFontName.c_str(), -1, IGGY_FONTFLAG_none );

		IggyFontInstallTruetypeFallbackCodepointUTF8( m_strFontName.c_str(), -1, IGGY_FONTFLAG_none, fallbackCharacter );

		if (registerAsDefaultFonts)
		{
			// 4J Stu - These are so we can use the default flash controls
			IggyFontInstallTruetypeUTF8 ( (void *)pbData, IGGY_TTC_INDEX_none, "Times New Roman", -1, IGGY_FONTFLAG_none );
			IggyFontInstallTruetypeUTF8 ( (void *)pbData, IGGY_TTC_INDEX_none, "Arial", -1, IGGY_FONTFLAG_none );
		}
		m_loaded = true;
	}
	else
	{
		CloseHandle(file);
	}
}

UITTFFont::~UITTFFont()
{
	delete[] pbData;
}


string UITTFFont::getFontName()
{
	return m_strFontName;
}

bool UITTFFont::isLoaded() const
{
	return m_loaded;
}
