#include "stdafx.h"
#include "UI.h"
#include "UIScene_DLCOffersMenu.h"
#include "../../../Minecraft.World/StringHelpers.h"
#include <winhttp.h>
#include <thread>
#include <atomic>
#pragma comment(lib, "winhttp.lib")

#pragma pack(push,1)
struct LCEZipLocalHeader
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

struct LCEZipEntry
{
	std::string  fname;
	WORD         method;
	DWORD        compSize;
	DWORD        uncompSize;
	const BYTE*  fileData;
};

#include "miniz.h"
#include "miniz.c"

static const char* WORKSHOP_REGISTRY_URL     = "https://network-server-7kuc.onrender.com/workshop/registry.json";
static const char* WORKSHOP_RAW_BASE         = "https://network-server-7kuc.onrender.com/workshop";
static const char* COMMUNITY_REGISTRY_LOCAL  = "http://localhost:7601/registry.json";
static const char* COMMUNITY_RAW_BASE_LOCAL  = "http://localhost:7601";
static const char* COMMUNITY_REGISTRY_REMOTE = "https://raw.githubusercontent.com/LCE-Hub/LCE-Workshop/refs/heads/main/registry.json";
static const char* COMMUNITY_RAW_BASE_REMOTE = "https://github.com/LCE-Hub/LCE-Workshop/raw/refs/heads/main";

const char* UIScene_DLCOffersMenu::CategoryForIndex(int index)
{
	switch(index)
	{
	case 1: return "Skin";
	case 2: return "Texture";
	case 3: return "Mashup";
	case 4: return nullptr;
	case 5: return "Map";
	case 6: return "Bundle";
	default: return nullptr;
	}
}

bool UIScene_DLCOffersMenu::FetchURL(const std::string& url, std::vector<BYTE>& outData)
{
	outData.clear();

	bool isHttps = (url.size() >= 8 && _strnicmp(url.c_str(), "https://", 8) == 0);
	const char* hostStart = url.c_str() + (isHttps ? 8 : 7);
	const char* pathStart = strchr(hostStart, '/');

	std::string host = pathStart ? std::string(hostStart, pathStart - hostStart) : std::string(hostStart);
	std::string path = pathStart ? std::string(pathStart) : "/";

	int port = isHttps ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;

	std::wstring wHost(host.begin(), host.end());
	std::wstring wPath(path.begin(), path.end());

	HINTERNET hSession = WinHttpOpen(L"LCEWorkshop/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if(!hSession) return false;

	HINTERNET hConnect = WinHttpConnect(hSession, wHost.c_str(), static_cast<INTERNET_PORT>(port), 0);
	if(!hConnect) { WinHttpCloseHandle(hSession); return false; }

	DWORD flags = isHttps ? WINHTTP_FLAG_SECURE : 0;
	HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", wPath.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
	if(!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

	DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
	WinHttpSetOption(hRequest, WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy));

	bool sent = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) != FALSE;
	if(sent) sent = WinHttpReceiveResponse(hRequest, nullptr) != FALSE;

	if(!sent) { WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

	DWORD statusCode = 0;
	DWORD statusSize = sizeof(statusCode);
	WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);

	if(statusCode != 200) { WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

	BYTE buf[65536];
	DWORD dwRead = 0;
	while(WinHttpReadData(hRequest, buf, sizeof(buf), &dwRead) && dwRead > 0)
		outData.insert(outData.end(), buf, buf + dwRead);

	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);
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

static bool InflateBuffer(const BYTE* in, DWORD inSize, std::vector<BYTE>& out, DWORD uncompSize)
{
	out.resize(uncompSize);
	mz_stream strm = {};
	strm.next_in   = reinterpret_cast<const unsigned char*>(in);
	strm.avail_in  = static_cast<mz_uint32>(inSize);
	strm.next_out  = reinterpret_cast<unsigned char*>(out.data());
	strm.avail_out = static_cast<mz_uint32>(uncompSize);
	if(mz_inflateInit2(&strm, -15) != MZ_OK) return false;
	int ret = mz_inflate(&strm, MZ_FINISH);
	mz_inflateEnd(&strm);
	out.resize(strm.total_out);
	return ret == MZ_STREAM_END;
}

static std::vector<LCEZipEntry> CollectEntries(const std::vector<BYTE>& zipData)
{
	std::vector<LCEZipEntry> entries;
	const BYTE* base = zipData.data();
	const BYTE* p    = base;
	const BYTE* end  = base + zipData.size();

	while(p + sizeof(LCEZipLocalHeader) <= end)
	{
		const LCEZipLocalHeader* hdr = reinterpret_cast<const LCEZipLocalHeader*>(p);
		if(hdr->sig == 0x02014b50 || hdr->sig == 0x06054b50) break;
		if(hdr->sig != 0x04034b50) break;

		WORD   flags      = hdr->flags;
		WORD   method     = hdr->method;
		DWORD  compSize   = hdr->compSize;
		DWORD  uncompSize = hdr->uncompSize;
		WORD   fnLen      = hdr->fileNameLen;
		WORD   exLen      = hdr->extraLen;

		p += sizeof(LCEZipLocalHeader);
		if(p + fnLen + exLen > end) break;

		LCEZipEntry e;
		e.fname.assign(reinterpret_cast<const char*>(p), fnLen);
		p += fnLen + exLen;

		e.method     = method;
		e.compSize   = compSize;
		e.uncompSize = uncompSize;
		e.fileData   = p;

		if(flags & 0x0008)
		{
			if(method == 0)
			{
				compSize   = uncompSize;
				e.compSize = compSize;
			}
			else
			{
				const BYTE* scan = p;
				while(scan + 4 <= end)
				{
					DWORD sig = *reinterpret_cast<const DWORD*>(scan);
					if(sig == 0x08074b50 || sig == 0x04034b50 || sig == 0x02014b50)
					{
						compSize   = static_cast<DWORD>(scan - p);
						e.compSize = compSize;
						break;
					}
					++scan;
				}
			}
		}

		if(p + e.compSize > end) break;
		p += e.compSize;

		if(flags & 0x0008)
		{
			if(p + 4 <= end && *reinterpret_cast<const DWORD*>(p) == 0x08074b50)
				p += 16;
			else if(p + 12 <= end)
				p += 12;
		}

		for(size_t i = 0; i < e.fname.size(); i++)
			if(e.fname[i] == '/') e.fname[i] = '\\';

		if(!e.fname.empty())
			entries.push_back(e);
	}
	return entries;
}

bool UIScene_DLCOffersMenu::InstallPack(const std::vector<BYTE>& zipData, const std::string& packId, const std::string& packName, const std::string& destToken)
{
	if(zipData.empty()) return false;

	std::string outDir;
	if(destToken == "{GameHDDDir}")
		outDir = GetGameDir() + "Windows64\\GameHDD\\";
	else
		outDir = GetGameDir() + "Windows64Media\\DLC\\" + packName + "\\";

	EnsureDirExists(outDir);

	std::vector<LCEZipEntry> entries = CollectEntries(zipData);
	if(entries.empty()) return false;

	int extracted = 0;

	for(size_t i = 0; i < entries.size(); i++)
	{
		const LCEZipEntry& e = entries[i];
		if(e.fname.empty() || e.fname[e.fname.size() - 1] == '\\') continue;

		std::string outPath = outDir + e.fname;

		size_t lastSlash = outPath.rfind('\\');
		if(lastSlash != std::string::npos)
			EnsureDirExists(outPath.substr(0, lastSlash + 1));

		std::vector<BYTE> outBuf;
		bool ok = false;

		if(e.method == 0)
		{
			outBuf.assign(e.fileData, e.fileData + e.compSize);
			ok = true;
		}
		else if(e.method == 8)
		{
			ok = InflateBuffer(e.fileData, e.compSize, outBuf, e.uncompSize);
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

bool UIScene_DLCOffersMenu::IsPackInstalled(const std::string& packId, const std::string& packName)
{
	std::string dirByName = GetGameDir() + "Windows64Media\\DLC\\" + packName;
	DWORD attr = GetFileAttributesA(dirByName.c_str());
	if(attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
		return true;

	std::string dirByNameHDD = GetGameDir() + "Windows64\\GameHDD\\" + packName;
	attr = GetFileAttributesA(dirByNameHDD.c_str());
	if(attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
		return true;

	std::string dirById = GetGameDir() + "Windows64Media\\DLC\\" + packId;
	attr = GetFileAttributesA(dirById.c_str());
	return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
}

bool UIScene_DLCOffersMenu::IsBundleInstalled(const WorkshopPack* pk)
{
	for(int b = 0; b < (int)pk->bundledPackIds.size(); b++)
	{
		const std::string& bid = pk->bundledPackIds[b];
		for(int i = 0; i < (int)m_packs.size(); i++)
		{
			if(m_packs[i].id == bid)
			{
				std::string bName(m_packs[i].name.begin(), m_packs[i].name.end());
				if(!IsPackInstalled(bid, bName))
					return false;
				break;
			}
		}
	}
	return !pk->bundledPackIds.empty();
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

bool UIScene_DLCOffersMenu::ParseRegistry(const std::string& json, const std::string& rawBase)
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
			else if(key == "bundledPacks")
			{
				if(*p == '[') ++p;
				while(true)
				{
					SkipWhitespace(p);
					if(*p == ']' || *p == '\0') { if(*p == ']') ++p; break; }
					if(*p == ',') { ++p; continue; }
					if(*p == '"') { std::string pid; ReadString(p, pid); pack.bundledPackIds.push_back(pid); }
					else ++p;
				}
			}
			else SkipValue(p);
		}
		if(*p == '}') ++p;

		bool hasZips = !pack.zips.empty();
		bool hasBundle = !pack.bundledPackIds.empty();
		if(!hasZips && !hasBundle) continue;

		bool validDests = true;
		for(int i = 0; i < (int)pack.zips.size(); i++)
		{
			const std::string& dest = pack.zips[i].destToken;
			if(dest != "{DLCDir}" && dest != "{GameHDDDir}") { validDests = false; break; }
		}
		if(!validDests) continue;

		if(!pack.id.empty() && !thumbFile.empty())
			pack.thumbnailUrl = rawBase + "/" + pack.id + "/" + thumbFile;

		if(!pack.id.empty())
			m_packs.push_back(pack);
	}

	return !m_packs.empty();
}

UIScene_DLCOffersMenu::UIScene_DLCOffersMenu(int iPad, void* initData, UILayer* parentLayer)
	: UIScene(iPad, parentLayer)
	, m_eFetchState(eFetch_Idle)
	, m_bIsCommunity(false)
	, m_iCurrentPack(0)
	, m_iProductInfoIndex(0)
	, m_bIsSD(false)
	, m_bHasPurchased(false)
	, m_bIsSelected(false)
	, m_bThumbnailDirty(false)
	, m_bAlive(true)
	, m_activeInstalls(0)
{
	DLCOffersParam* param = static_cast<DLCOffersParam*>(initData);
	m_iProductInfoIndex   = param ? param->iType : 0;
	m_bIsCommunity        = (m_iProductInfoIndex == 4);

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
	m_bAlive.store(false);
	app.SetLiveLinkRequired(false);
}

void UIScene_DLCOffersMenu::PreloadAllThumbnails()
{
	for(size_t i = 0; i < m_packs.size(); i++)
	{
		WorkshopPack* pk = &m_packs[i];
		if(pk->thumbnailUrl.empty() || pk->thumbnailLoaded.load() || pk->thumbnailFetching.load())
			continue;

		pk->thumbnailFetching.store(true);

		std::string url             = pk->thumbnailUrl;
		std::atomic<bool>* pLoaded  = &pk->thumbnailLoaded;
		std::atomic<bool>* pFetching = &pk->thumbnailFetching;
		std::vector<BYTE>* pData    = &pk->thumbnailData;
		std::atomic<bool>* pAlive   = &m_bAlive;
		std::atomic<bool>* pDirty   = &m_bThumbnailDirty;

		std::thread([url, pLoaded, pFetching, pData, pAlive, pDirty]()
		{
			std::vector<BYTE> imgData;
			if(UIScene_DLCOffersMenu::FetchURL(url, imgData) && !imgData.empty())
			{
				if(pAlive->load())
				{
					*pData = std::move(imgData);
					pLoaded->store(true);
					pDirty->store(true);
				}
			}
			pFetching->store(false);
		}).detach();
	}
}

void UIScene_DLCOffersMenu::tick()
{
	UIScene::tick();

	if(m_bThumbnailDirty.load())
	{
		m_bThumbnailDirty.store(false);
		if(m_iCurrentPack >= 0 && m_iCurrentPack < (int)m_filteredPacks.size())
		{
			WorkshopPack* pk = m_filteredPacks[m_iCurrentPack];
			if(pk->thumbnailLoaded.load() && !pk->thumbnailData.empty())
				ApplyThumbnail(pk);
		}
	}

	{
		std::lock_guard<std::mutex> lock(m_pendingInstallMutex);
		for(size_t i = 0; i < m_pendingInstallResults.size(); i++)
		{
			PendingInstallResult& r = m_pendingInstallResults[i];
			if(r.success && r.listIndex >= 0 && r.listIndex < (int)m_filteredPacks.size())
			{
				m_buttonListOffers.showTick(r.listIndex, true);
				if(m_iCurrentPack == r.listIndex)
					m_labelPriceTag.setLabel(L"Installed");
			}
		}
		m_pendingInstallResults.clear();
	}

	if(m_activeInstalls.load() == 0)
		m_Timer.setVisible(false);

	if(m_eFetchState != eFetch_Idle)
		return;

	m_eFetchState = eFetch_Error;

	std::string jsonStr;

	if(m_bIsCommunity)
	{
		if(FetchURLString(COMMUNITY_REGISTRY_LOCAL, jsonStr))
		{
			m_rawBase = COMMUNITY_RAW_BASE_LOCAL;
		}
		else
		{
			if(!FetchURLString(COMMUNITY_REGISTRY_REMOTE, jsonStr))
			{
				m_labelOffers.setLabel(app.GetString(IDS_NO_DLCCATEGORIES));
				m_Timer.setVisible(false);
				return;
			}
			m_rawBase = COMMUNITY_RAW_BASE_REMOTE;
		}
	}
	else
	{
		char urlWithBust[512];
		snprintf(urlWithBust, sizeof(urlWithBust), "%s?_=%llu",
			WORKSHOP_REGISTRY_URL,
			static_cast<unsigned long long>(GetTickCount64()));

		if(!FetchURLString(urlWithBust, jsonStr))
		{
			m_labelOffers.setLabel(app.GetString(IDS_NO_DLCCATEGORIES));
			m_Timer.setVisible(false);
			return;
		}
		m_rawBase = WORKSHOP_RAW_BASE;
	}

	if(!ParseRegistry(jsonStr, m_rawBase))
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
	PreloadAllThumbnails();
	PopulateList();
}

void UIScene_DLCOffersMenu::PopulateList()
{
	m_buttonListOffers.clearList();

	for(int i = 0; i < (int)m_filteredPacks.size(); i++)
	{
		WorkshopPack* pk = m_filteredPacks[i];
		std::string packName(pk->name.begin(), pk->name.end());
		bool installed = pk->bundledPackIds.empty()
			? IsPackInstalled(pk->id, packName)
			: IsBundleInstalled(pk);
		m_buttonListOffers.addItem(pk->name, installed, i);
	}

	m_Timer.setVisible(false);

	if(!m_filteredPacks.empty())
	{
		m_iCurrentPack = 0;
		m_buttonListOffers.setFocus(true);
		UpdateDetailPanel();
	}
}

void UIScene_DLCOffersMenu::ApplyThumbnail(WorkshopPack* pk)
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

void UIScene_DLCOffersMenu::UpdateDetailPanel()
{
	if(m_iCurrentPack < 0 || m_iCurrentPack >= (int)m_filteredPacks.size())
		return;

	WorkshopPack* pk = m_filteredPacks[m_iCurrentPack];

	std::wstring desc = L"All DLC Content from LCE Online's 'Skin Packs', 'Texture Packs', 'Mash-up Packs', 'Map Packs' and 'Bundle Packs' are property of 4J Studios, Mojang AB and Microsoft. ";
	desc += pk->description;
	m_labelHTMLSellText.setLabel(desc.c_str());

	std::string packName(pk->name.begin(), pk->name.end());
	bool installed = pk->bundledPackIds.empty()
		? IsPackInstalled(pk->id, packName)
		: IsBundleInstalled(pk);
	m_labelPriceTag.setLabel(installed ? L"Installed" : L"Free");

	if(pk->thumbnailLoaded.load() && !pk->thumbnailData.empty())
		ApplyThumbnail(pk);
	else
		m_bitmapIconOfferImage.setTextureName(L"");
}

void UIScene_DLCOffersMenu::handlePress(F64 controlId, F64 childId)
{
	if(static_cast<int>(controlId) != eControl_OffersList) return;
	if(m_eFetchState != eFetch_Ready) return;

	int iIndex = static_cast<int>(childId);
	if(iIndex < 0 || iIndex >= (int)m_filteredPacks.size()) return;

	WorkshopPack* pk = m_filteredPacks[iIndex];
	std::string packName(pk->name.begin(), pk->name.end());

	if(!pk->bundledPackIds.empty())
	{
		if(IsBundleInstalled(pk))
		{
			m_buttonListOffers.showTick(iIndex, true);
			return;
		}

		struct SubPackJob
		{
			std::string id;
			std::string name;
			std::vector<WorkshopZip> zips;
		};

		std::vector<SubPackJob> jobs;
		for(int b = 0; b < (int)pk->bundledPackIds.size(); b++)
		{
			const std::string& bid = pk->bundledPackIds[b];
			for(int i = 0; i < (int)m_packs.size(); i++)
			{
				if(m_packs[i].id == bid)
				{
					std::string bName(m_packs[i].name.begin(), m_packs[i].name.end());
					if(!IsPackInstalled(bid, bName) && !m_packs[i].zips.empty())
					{
						SubPackJob j;
						j.id   = m_packs[i].id;
						j.name = bName;
						j.zips = m_packs[i].zips;
						jobs.push_back(j);
					}
					break;
				}
			}
		}

		if(jobs.empty())
		{
			m_buttonListOffers.showTick(iIndex, true);
			return;
		}

		m_activeInstalls.fetch_add(1);
		m_Timer.setVisible(true);

		std::string rawBase                             = m_rawBase;
		std::vector<PendingInstallResult>* pResults     = &m_pendingInstallResults;
		std::mutex* pMutex                              = &m_pendingInstallMutex;
		std::atomic<bool>* pAlive                       = &m_bAlive;
		std::atomic<int>* pActive                       = &m_activeInstalls;

		std::thread([jobs, rawBase, iIndex, pResults, pMutex, pAlive, pActive]()
		{
			bool anyInstalled = false;
			for(size_t b = 0; b < jobs.size(); b++)
			{
				const SubPackJob& job = jobs[b];
				for(size_t z = 0; z < job.zips.size(); z++)
				{
					std::string zipUrl = rawBase + "/" + job.id + "/" + job.zips[z].filename;
					std::vector<BYTE> zipData;
					if(!UIScene_DLCOffersMenu::FetchURL(zipUrl, zipData) || zipData.empty()) continue;
					if(UIScene_DLCOffersMenu::InstallPack(zipData, job.id, job.name, job.zips[z].destToken))
						anyInstalled = true;
				}
			}
			if(pAlive->load())
			{
				std::lock_guard<std::mutex> lock(*pMutex);
				PendingInstallResult r;
				r.listIndex = iIndex;
				r.success   = anyInstalled;
				pResults->push_back(r);
			}
			pActive->fetch_sub(1);
		}).detach();

		return;
	}

	if(IsPackInstalled(pk->id, packName))
	{
		m_buttonListOffers.showTick(iIndex, true);
		return;
	}

	if(pk->zips.empty()) return;

	struct SingleJob
	{
		std::string id;
		std::string name;
		std::vector<WorkshopZip> zips;
	};

	SingleJob job;
	job.id   = pk->id;
	job.name = packName;
	job.zips = pk->zips;

	m_activeInstalls.fetch_add(1);
	m_Timer.setVisible(true);

	std::string rawBase                             = m_rawBase;
	std::vector<PendingInstallResult>* pResults     = &m_pendingInstallResults;
	std::mutex* pMutex                              = &m_pendingInstallMutex;
	std::atomic<bool>* pAlive                       = &m_bAlive;
	std::atomic<int>* pActive                       = &m_activeInstalls;

	std::thread([job, rawBase, iIndex, pResults, pMutex, pAlive, pActive]()
	{
		bool installed = false;
		for(size_t z = 0; z < job.zips.size(); z++)
		{
			std::string zipUrl = rawBase + "/" + job.id + "/" + job.zips[z].filename;
			std::vector<BYTE> zipData;
			if(!UIScene_DLCOffersMenu::FetchURL(zipUrl, zipData) || zipData.empty()) continue;
			if(UIScene_DLCOffersMenu::InstallPack(zipData, job.id, job.name, job.zips[z].destToken))
				installed = true;
		}
		if(pAlive->load())
		{
			std::lock_guard<std::mutex> lock(*pMutex);
			PendingInstallResult r;
			r.listIndex = iIndex;
			r.success   = installed;
			pResults->push_back(r);
		}
		pActive->fetch_sub(1);
	}).detach();
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