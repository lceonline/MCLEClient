#pragma once
#include "UIScene.h"
#include <atomic>
#include <mutex>
#include <vector>

struct WorkshopZip
{
	std::string filename;
	std::string destToken;
};

struct WorkshopPack
{
	std::string              id;
	std::wstring             name;
	std::wstring             author;
	std::wstring             description;
	std::vector<std::string> categories;
	std::string              thumbnailUrl;
	std::vector<WorkshopZip> zips;
	std::string              version;
	std::vector<BYTE>        thumbnailData;
	std::atomic<bool>        thumbnailLoaded;
	std::atomic<bool>        thumbnailFetching;
	std::vector<std::string> bundledPackIds;

	WorkshopPack() : thumbnailLoaded(false), thumbnailFetching(false) {}

	WorkshopPack(const WorkshopPack& o)
		: id(o.id), name(o.name), author(o.author), description(o.description)
		, categories(o.categories), thumbnailUrl(o.thumbnailUrl), zips(o.zips)
		, version(o.version), thumbnailData(o.thumbnailData)
		, thumbnailLoaded(o.thumbnailLoaded.load())
		, thumbnailFetching(o.thumbnailFetching.load())
		, bundledPackIds(o.bundledPackIds)
	{}

	WorkshopPack& operator=(const WorkshopPack& o)
	{
		id = o.id; name = o.name; author = o.author; description = o.description;
		categories = o.categories; thumbnailUrl = o.thumbnailUrl; zips = o.zips;
		version = o.version; thumbnailData = o.thumbnailData;
		thumbnailLoaded.store(o.thumbnailLoaded.load());
		thumbnailFetching.store(o.thumbnailFetching.load());
		bundledPackIds = o.bundledPackIds;
		return *this;
	}
};

struct PendingInstallResult
{
	int  listIndex;
	bool success;
};

class UIScene_DLCOffersMenu : public UIScene
{
private:
	enum EControls   { eControl_OffersList };
	enum EFetchState { eFetch_Idle, eFetch_Ready, eFetch_Error };

	bool                       m_bIsSD;
	bool                       m_bHasPurchased;
	bool                       m_bIsSelected;
	bool                       m_bIsCommunity;
	std::atomic<bool>          m_bThumbnailDirty;
	std::atomic<bool>          m_bAlive;
	std::atomic<int>           m_activeInstalls;

	std::vector<PendingInstallResult> m_pendingInstallResults;
	std::mutex                        m_pendingInstallMutex;

	UIControl_DLCList          m_buttonListOffers;
	UIControl_Label            m_labelOffers, m_labelPriceTag, m_labelXboxStore;
	UIControl_HTMLLabel        m_labelHTMLSellText;
	UIControl_BitmapIcon       m_bitmapIconOfferImage;
	UIControl                  m_Timer;

	UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
		UI_MAP_ELEMENT(m_buttonListOffers,     "OffersList")
		UI_MAP_ELEMENT(m_labelOffers,          "OffersList_Title")
		UI_MAP_ELEMENT(m_labelPriceTag,        "PriceTag")
		UI_MAP_ELEMENT(m_labelHTMLSellText,    "HTMLSellText")
		UI_MAP_ELEMENT(m_bitmapIconOfferImage, "DLCIcon")
		UI_MAP_ELEMENT(m_Timer,                "Timer")
		if(m_loadedResolution == eSceneResolution_1080)
		{
			UI_MAP_ELEMENT(m_labelXboxStore, "XboxLabel")
		}
	UI_END_MAP_ELEMENTS_AND_NAMES()

	EFetchState                m_eFetchState;
	std::string                m_rawBase;
	std::vector<WorkshopPack>  m_packs;
	std::vector<WorkshopPack*> m_filteredPacks;
	int                        m_iCurrentPack;
	int                        m_iProductInfoIndex;

	static const char* CategoryForIndex(int index);
	static bool        FetchURL(const std::string& url, std::vector<BYTE>& outData);
	static bool        FetchURLString(const std::string& url, std::string& outStr);
	bool               ParseRegistry(const std::string& json, const std::string& rawBase);
	void               PopulateList();
	void               UpdateDetailPanel();
	void               ApplyThumbnail(WorkshopPack* pk);
	void               PreloadAllThumbnails();
	static bool        InstallPack(const std::vector<BYTE>& zipData, const std::string& packId, const std::string& packName, const std::string& destToken);
	static bool        IsPackInstalled(const std::string& packId, const std::string& packName);
	bool               IsBundleInstalled(const WorkshopPack* pk);

public:
	UIScene_DLCOffersMenu(int iPad, void* initData, UILayer* parentLayer);
	~UIScene_DLCOffersMenu();

	static int       ExitDLCOffersMenu(void* pParam, int iPad, C4JStorage::EMessageResult result);
	virtual EUIScene getSceneType() { return eUIScene_DLCOffersMenu; }
	virtual void     tick();
	virtual void     updateTooltips();

protected:
	virtual wstring getMoviePath();

public:
	virtual void handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool& handled);
	virtual void handlePress(F64 controlId, F64 childId);
	virtual void handleSelectionChanged(F64 selectedId);
	virtual void handleFocusChange(F64 controlId, F64 childId);
	virtual void handleTimerComplete(int id);
};