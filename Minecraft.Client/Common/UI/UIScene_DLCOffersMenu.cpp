#include "stdafx.h"
#include "UI.h"
#include "UIScene_DLCOffersMenu.h"
#include "../../../Minecraft.World/StringHelpers.h"
#include <wininet.h>
#include "miniz.h"
#include "miniz.c"
#pragma comment(lib, "wininet.lib")

static const char* WORKSHOP_REGISTRY_URL = "https://network-server-7kuc.onrender.com/workshop/registry.json";
static const char* WORKSHOP_RAW_BASE     = "https://network-server-7kuc.onrender.com/workshop";

const char* UIScene_DLCOffersMenu::CategoryForIndex(int index)
{
	switch(index)
	{
	case 1: return "Skin";
	case 2: return "Texture";
	case 3: return "Mashup";
	case 4: return "Mod";
	default: return nullptr;
	}
}

bool UIScene_DLCOffersMenu::FetchURL(const std::string& url, std::vector<BYTE>& outData)
{
	outData.clear();
	HINTERNET hInet = InternetOpenA("LCEWorkshop/1.0", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
	if(!hInet) return false;
	HINTERNET hUrl = InternetOpenUrlA(hInet, url.c_str(), nullptr, 0,
		INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
	if(!hUrl) { InternetCloseHandle(hInet); return false; }
	BYTE buf[8192];
	DWORD dwRead = 0;
	while(InternetReadFile(hUrl, buf, sizeof(buf), &dwRead) && dwRead > 0)
		outData.insert(outData.end(), buf, buf + dwRead);
	InternetCloseHandle(hUrl);
	InternetCloseHandle(hInet);
	return !outData.empty();
}

bool UIScene_DLCOffersMenu::FetchURLString(const std::string& url, std::string& outStr)
{
	std::vector<BYTE> data;
	if(!FetchURL(url, data)) return false;
	outStr.assign(reinterpret_cast<const char*>(data.data()), data.size());
	return true;
}

static std::string GetGameDir()
{
	char szPath[MAX_PATH] = {};
	GetModuleFileNameA(nullptr, szPath, MAX_PATH);
	char* pLast = strrchr(szPath, '\\');
	if(pLast) *(pLast + 1) = '\0';
	return std::string(szPath);
}

static void EnsureDirExists(const std::string& path)
{
	std::string cur;
	for(size_t i = 0; i < path.size(); i++)
	{
		cur += path[i];
		if((path[i] == '\\' || path[i] == '/') && cur.size() > 3)
			CreateDirectoryA(cur.c_str(), nullptr);
	}
	CreateDirectoryA(path.c_str(), nullptr);
}

#pragma pack(push,1)
struct ZipLocalHeader
{
	DWORD sig;
	WORD  verNeeded;
	WORD  flags;
	WORD  method;
	WORD  modTime;
	WORD  modDate;
	DWORD crc32;
	DWORD compSize;
	DWORD uncompSize;
	WORD  fileNameLen;
	WORD  extraLen;
};
#pragma pack(pop)

static bool InflateBuffer(const BYTE* in, DWORD inSize, std::vector<BYTE>& out, DWORD uncompSize)
{
	out.resize(uncompSize);
	mz_stream strm  = {};
	strm.next_in    = in;
	strm.avail_in   = inSize;
	strm.next_out   = out.data();
	strm.avail_out  = uncompSize;
	if(mz_inflateInit2(&strm, -15) != MZ_OK) return false;
	int ret = mz_inflate(&strm, MZ_FINISH);
	mz_inflateEnd(&strm);
	out.resize(strm.total_out);
	return ret == MZ_STREAM_END;
}

bool UIScene_DLCOffersMenu::InstallPack(const std::vector<BYTE>& zipData, const std::string& packName)
{
	if(zipData.empty()) return false;

	std::string baseDir = GetGameDir() + "Windows64Media\\DLC\\" + packName + "\\";
	EnsureDirExists(baseDir);

	const BYTE* p   = zipData.data();
	const BYTE* end = p + zipData.size();
	int extracted   = 0;

	while(p + sizeof(ZipLocalHeader) <= end)
	{
		const ZipLocalHeader* hdr = reinterpret_cast<const ZipLocalHeader*>(p);

		if(hdr->sig == 0x02014b50 || hdr->sig == 0x06054b50)
			break;
		if(hdr->sig != 0x04034b50)
			break;

		p += sizeof(ZipLocalHeader);
		if(p + hdr->fileNameLen + hdr->extraLen > end) break;

		std::string fname(reinterpret_cast<const char*>(p), hdr->fileNameLen);
		p += hdr->fileNameLen + hdr->extraLen;

		if(p + hdr->compSize > end) break;
		const BYTE* fileData = p;
		p += hdr->compSize;

		if(fname.empty() || fname.back() == '/' || fname.back() == '\\')
			continue;

		for(char& c : fname) if(c == '/') c = '\\';

		size_t firstSlash = fname.find('\\');
		if(firstSlash != std::string::npos)
			fname = fname.substr(firstSlash + 1);

		if(fname.empty()) continue;

		std::string outPath = baseDir + fname;

		size_t lastSlash = outPath.rfind('\\');
		if(lastSlash != std::string::npos)
			EnsureDirExists(outPath.substr(0, lastSlash + 1));

		std::vector<BYTE> outBuf;
		bool ok = false;

		if(hdr->method == 0)
		{
			outBuf.assign(fileData, fileData + hdr->compSize);
			ok = true;
		}
		else if(hdr->method == 8)
		{
			ok = InflateBuffer(fileData, hdr->compSize, outBuf, hdr->uncompSize);
		}

		if(ok && !outBuf.empty())
		{
			FILE* f = fopen(outPath.c_str(), "wb");
			if(f)
			{
				fwrite(outBuf.data(), 1, outBuf.size(), f);
				fclose(f);
				extracted++;
			}
		}
	}

	return extracted > 0;
}

bool UIScene_DLCOffersMenu::IsPackInstalled(const std::string& packName)
{
	std::string dir = GetGameDir() + "Windows64Media\\DLC\\" + packName;
	DWORD attr = GetFileAttributesA(dir.c_str());
	return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
}

static void SkipWhitespace(const char*& p)
{
	while(*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) ++p;
}

static bool ReadString(const char*& p, std::string& out)
{
	if(*p != '"') return false;
	++p; out.clear();
	while(*p && *p != '"')
	{
		if(*p == '\\') { ++p; if(*p) out += *p; }
		else           { out += *p; }
		++p;
	}
	if(*p == '"') ++p;
	return true;
}

static bool ReadWString(const char*& p, std::wstring& out)
{
	std::string s;
	if(!ReadString(p, s)) return false;
	out.assign(s.begin(), s.end());
	return true;
}

static void SkipValue(const char*& p)
{
	SkipWhitespace(p);
	if(*p == '"') { std::string d; ReadString(p, d); return; }
	if(*p == '{' || *p == '[')
	{
		char open = *p, close = (*p == '{') ? '}' : ']';
		int depth = 1; ++p;
		while(*p && depth > 0)
		{
			if(*p == '"') { std::string d; ReadString(p, d); continue; }
			if(*p == open)  ++depth;
			if(*p == close) --depth;
			++p;
		}
		return;
	}
	while(*p && *p != ',' && *p != '}' && *p != ']') ++p;
}

bool UIScene_DLCOffersMenu::ParseRegistry(const std::string& json)
{
	m_packs.clear();
	const char* p = json.c_str();

	const char* arrStart = strstr(p, "\"packages\"");
	if(!arrStart) arrStart = strstr(p, "\"packs\"");
	if(!arrStart) { arrStart = strchr(p, '['); if(!arrStart) return false; --arrStart; }

	p = strchr(arrStart, '[');
	if(!p) return false;
	++p;

	while(true)
	{
		SkipWhitespace(p);
		if(*p == ']' || *p == '\0') break;
		if(*p != '{') { ++p; continue; }
		++p;

		WorkshopPack pack;
		std::string thumbFile;

		while(true)
		{
			SkipWhitespace(p);
			if(*p == '}' || *p == '\0') break;
			if(*p == ',') { ++p; continue; }
			if(*p != '"') { ++p; continue; }

			std::string key;
			ReadString(p, key);
			SkipWhitespace(p);
			if(*p == ':') ++p;
			SkipWhitespace(p);

			if(key == "id")               ReadString(p, pack.id);
			else if(key == "name")        ReadWString(p, pack.name);
			else if(key == "author")      ReadWString(p, pack.author);
			else if(key == "description") ReadWString(p, pack.description);
			else if(key == "thumbnail")   ReadString(p, thumbFile);
			else if(key == "version")     ReadString(p, pack.version);
			else if(key == "category")
			{
				if(*p == '[') ++p;
				while(true)
				{
					SkipWhitespace(p);
					if(*p == ']' || *p == '\0') { if(*p == ']') ++p; break; }
					if(*p == ',') { ++p; continue; }
					if(*p == '"') { std::string cat; ReadString(p, cat); pack.categories.push_back(cat); }
					else ++p;
				}
			}
			else if(key == "zips")
			{
				if(*p == '{') ++p;
				while(true)
				{
					SkipWhitespace(p);
					if(*p == '}' || *p == '\0') { if(*p == '}') ++p; break; }
					if(*p == ',') { ++p; continue; }
					if(*p == '"')
					{
						WorkshopZip z;
						ReadString(p, z.filename);
						SkipWhitespace(p);
						if(*p == ':') ++p;
						SkipWhitespace(p);
						ReadString(p, z.destToken);
						pack.zips.push_back(z);
					}
					else ++p;
				}
			}
			else SkipValue(p);
		}
		if(*p == '}') ++p;

		if(!pack.id.empty() && !thumbFile.empty())
			pack.thumbnailUrl = std::string(WORKSHOP_RAW_BASE) + "/" + pack.id + "/" + thumbFile;

		if(!pack.id.empty())
			m_packs.push_back(pack);
	}

	return !m_packs.empty();
}

UIScene_DLCOffersMenu::UIScene_DLCOffersMenu(int iPad, void* initData, UILayer* parentLayer)
	: UIScene(iPad, parentLayer)
	, m_eFetchState(eFetch_Idle)
	, m_iCurrentPack(0)
	, m_iProductInfoIndex(0)
	, m_bIsSD(false)
	, m_bHasPurchased(false)
	, m_bIsSelected(false)
{
	DLCOffersParam* param = static_cast<DLCOffersParam*>(initData);
	m_iProductInfoIndex   = param ? param->iType : 0;

	initialiseMovie();
	app.SetLiveLinkRequired(true);

	m_bIsSD = !RenderManager.IsHiDef() && !RenderManager.IsWidescreen();

	m_labelOffers.init(app.GetString(IDS_DOWNLOADABLE_CONTENT_OFFERS));
	m_buttonListOffers.init(eControl_OffersList);
	m_labelHTMLSellText.init(L" ");
	m_labelPriceTag.init(L" ");

	TelemetryManager->RecordMenuShown(m_iPad, eUIScene_DLCOffersMenu, 0);

	if(m_loadedResolution == eSceneResolution_1080)
		m_labelXboxStore.init(L"");

	m_Timer.setVisible(true);
}

UIScene_DLCOffersMenu::~UIScene_DLCOffersMenu()
{
	app.SetLiveLinkRequired(false);
}

void UIScene_DLCOffersMenu::tick()
{
	UIScene::tick();

	if(m_eFetchState != eFetch_Idle)
		return;

	m_eFetchState = eFetch_Error;

	char urlWithBust[512];
	snprintf(urlWithBust, sizeof(urlWithBust), "%s?_=%llu",
		WORKSHOP_REGISTRY_URL,
		static_cast<unsigned long long>(GetTickCount64()));

	std::string jsonStr;
	if(!FetchURLString(urlWithBust, jsonStr) || !ParseRegistry(jsonStr))
	{
		m_labelOffers.setLabel(app.GetString(IDS_NO_DLCCATEGORIES));
		m_Timer.setVisible(false);
		return;
	}

	m_filteredPacks.clear();
	const char* wantCat = CategoryForIndex(m_iProductInfoIndex);

	for(int i = 0; i < (int)m_packs.size(); i++)
	{
		WorkshopPack& pk = m_packs[i];
		if(wantCat == nullptr)
		{
			m_filteredPacks.push_back(&pk);
		}
		else
		{
			for(int j = 0; j < (int)pk.categories.size(); j++)
			{
				if(pk.categories[j] == wantCat)
				{
					m_filteredPacks.push_back(&pk);
					break;
				}
			}
		}
	}

	if(m_filteredPacks.empty())
	{
		m_labelOffers.setLabel(app.GetString(IDS_NO_DLCCATEGORIES));
		m_Timer.setVisible(false);
		return;
	}

	m_eFetchState = eFetch_Ready;
	PopulateList();
}

void UIScene_DLCOffersMenu::PopulateList()
{
	m_buttonListOffers.clearList();

	for(int i = 0; i < (int)m_filteredPacks.size(); i++)
		m_buttonListOffers.addItem(m_filteredPacks[i]->name, false, i);

	m_Timer.setVisible(false);

	if(!m_filteredPacks.empty())
	{
		m_iCurrentPack = 0;
		m_buttonListOffers.setFocus(true);
		UpdateDetailPanel();
	}
}

void UIScene_DLCOffersMenu::UpdateDetailPanel()
{
	if(m_iCurrentPack < 0 || m_iCurrentPack >= (int)m_filteredPacks.size())
		return;

	WorkshopPack* pk = m_filteredPacks[m_iCurrentPack];

	std::wstring desc = pk->description;
	if(!pk->version.empty())
	{
		desc += L"\n\nVersion: ";
		desc += std::wstring(pk->version.begin(), pk->version.end());
	}

	m_labelHTMLSellText.setLabel(desc.c_str());

	std::string packName(pk->name.begin(), pk->name.end());
	if(IsPackInstalled(packName))
		m_labelPriceTag.setLabel(L"Installed");
	else
		m_labelPriceTag.setLabel(L"Free");

	if(!pk->thumbnailLoaded && !pk->thumbnailUrl.empty())
	{
		std::vector<BYTE> imgData;
		if(FetchURL(pk->thumbnailUrl, imgData) && !imgData.empty())
		{
			pk->thumbnailData   = imgData;
			pk->thumbnailLoaded = true;
		}
	}

	if(pk->thumbnailLoaded && !pk->thumbnailData.empty())
	{
		std::wstring texName = L"ws_thumb_";
		texName.append(pk->id.begin(), pk->id.end());
		if(!hasRegisteredSubstitutionTexture(texName))
		{
			registerSubstitutionTexture(
				texName,
				const_cast<BYTE*>(pk->thumbnailData.data()),
				static_cast<int>(pk->thumbnailData.size()),
				false);
		}
		m_bitmapIconOfferImage.setTextureName(texName);
	}
	else
	{
		m_bitmapIconOfferImage.setTextureName(L"");
	}
}

void UIScene_DLCOffersMenu::handlePress(F64 controlId, F64 childId)
{
	if(static_cast<int>(controlId) != eControl_OffersList) return;
	if(m_eFetchState != eFetch_Ready) return;

	int iIndex = static_cast<int>(childId);
	if(iIndex < 0 || iIndex >= (int)m_filteredPacks.size()) return;

	WorkshopPack* pk = m_filteredPacks[iIndex];
	std::string packName(pk->name.begin(), pk->name.end());

	if(IsPackInstalled(packName))
	{
		m_buttonListOffers.showTick(iIndex, true);
		return;
	}

	if(pk->zips.empty()) return;

	m_Timer.setVisible(true);

	const WorkshopZip& zip = pk->zips[0];
	std::string zipUrl = std::string(WORKSHOP_RAW_BASE) + "/" + pk->id + "/" + zip.filename;

	std::vector<BYTE> zipData;
	if(!FetchURL(zipUrl, zipData) || zipData.empty())
	{
		m_Timer.setVisible(false);
		return;
	}

	bool installed = InstallPack(zipData, packName);
	m_Timer.setVisible(false);

	if(installed)
	{
		m_buttonListOffers.showTick(iIndex, true);
		m_labelPriceTag.setLabel(L"Installed");
	}
}

void UIScene_DLCOffersMenu::handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool& handled)
{
	ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

	switch(key)
	{
	case ACTION_MENU_CANCEL:
		if(pressed) navigateBack();
		break;
	case ACTION_MENU_OK:
#ifdef __ORBIS__
	case ACTION_MENU_TOUCHPAD_PRESS:
#endif
		sendInputToMovie(key, repeat, pressed, released);
		break;
	case ACTION_MENU_UP:
		if(pressed && m_eFetchState == eFetch_Ready)
			if(m_iCurrentPack > 0) { m_iCurrentPack--; UpdateDetailPanel(); }
		sendInputToMovie(key, repeat, pressed, released);
		break;
	case ACTION_MENU_DOWN:
		if(pressed && m_eFetchState == eFetch_Ready)
			if(m_iCurrentPack < (int)m_filteredPacks.size() - 1) { m_iCurrentPack++; UpdateDetailPanel(); }
		sendInputToMovie(key, repeat, pressed, released);
		break;
	case ACTION_MENU_LEFT:
	case ACTION_MENU_RIGHT:
	case ACTION_MENU_OTHER_STICK_DOWN:
	case ACTION_MENU_OTHER_STICK_UP:
		sendInputToMovie(key, repeat, pressed, released);
		break;
	}
}

void UIScene_DLCOffersMenu::handleFocusChange(F64 controlId, F64 childId)
{
	if(static_cast<int>(controlId) != eControl_OffersList) return;
	if(m_eFetchState != eFetch_Ready) return;

	int iIndex = static_cast<int>(childId);
	if(iIndex >= 0 && iIndex < (int)m_filteredPacks.size())
	{
		m_iCurrentPack  = iIndex;
		m_bIsSelected   = true;
		m_bHasPurchased = false;
		UpdateDetailPanel();
		updateTooltips();
	}
}

void UIScene_DLCOffersMenu::handleSelectionChanged(F64 selectedId) {}
void UIScene_DLCOffersMenu::handleTimerComplete(int id) {}

void UIScene_DLCOffersMenu::updateTooltips()
{
	int iA = -1;
	if(m_bIsSelected) iA = IDS_TOOLTIPS_INSTALL;
	ui.SetTooltips(m_iPad, iA, IDS_TOOLTIPS_BACK);
}

wstring UIScene_DLCOffersMenu::getMoviePath()
{
	return L"DLCOffersMenu";
}

int UIScene_DLCOffersMenu::ExitDLCOffersMenu(void* pParam, int iPad, C4JStorage::EMessageResult result)
{
	ui.NavigateToHomeMenu();
	return 0;
}