#include <plugin.hpp>
#include <Govnoclient.h>
#include <Windows.h>
#include <initguid.h>
#include <malloc.h>
#include <assert.h>
#include <Strsafe.h>


// {6B463403-4B00-45EB-822E-F5B17B8BE774}
DEFINE_GUID(govno_plugin_guid,
	0x6b463403, 0x4b00, 0x45eb, 0x82, 0x2e, 0xf5, 0xb1, 0x7b, 0x8b, 0xe7, 0x74);

// {1A0AF28E-6340-4248-9102-E183C338BC79}
DEFINE_GUID(govno_menu_item_guid,
	0x1a0af28e, 0x6340, 0x4248, 0x91, 0x2, 0xe1, 0x83, 0xc3, 0x38, 0xbc, 0x79);

#define MAX_PANELS 2
#define MESSAGES_TO_FETCH 50

static struct PluginStartupInfo g_info;
static BOOL broken;


struct govno_panel
{
	struct PluginPanelItem* file_items;
	struct govno_message_header* message_headers;
};


// Init plugin, FAR lanches it at the start
void WINAPI GetGlobalInfoW(struct GlobalInfo* info)
{
	info->StructSize = sizeof(struct GlobalInfo);
	info->MinFarVersion = MAKEFARVERSION(3, 0, 0, 5151, VS_RELEASE);
	info->Version = MAKEFARVERSION(0, 0, 0, 1, VS_ALPHA);
	info->Guid = govno_plugin_guid;
	info->Title = L"Govnokod";
	info->Description = L"Govnokod Far client for WinPerds";
	info->Author = L"Guest";
}

// Far provides struct we may save and use
void WINAPI SetStartupInfoW(const struct PluginStartupInfo* info)
{
	assert(info->StructSize >= sizeof(struct PluginStartupInfo));
	if (memcpy_s(&g_info, sizeof(struct PluginStartupInfo), info, info->StructSize) != 0)
	{
		broken = TRUE;
	}
}

// Creates Govnokod item in file menu
static struct PluginMenuItem GetPluginMenuItem()
{
	static BOOL initilaized = FALSE;
	static GUID guid;
	static struct PluginMenuItem result;
	static WCHAR* strings = L"Govnokod";
	if (! initilaized)
	{
		result.Count = 1;
		guid = govno_menu_item_guid;
		result.Guids = &guid;
		result.Strings = &strings;
		initilaized = TRUE;
	}
	return result;
}

// Introduce our plugin to far, say we wanna be file-like plugin
void WINAPI GetPluginInfoW(struct PluginInfo* info)
{
	info->StructSize = sizeof(struct PluginInfo);
	info->Flags = PF_NONE;
	info->DiskMenu = GetPluginMenuItem();
}


// Plugin launched
HANDLE WINAPI OpenW(const struct OpenInfo* info)
{
	const GUID menu_item_guid = govno_menu_item_guid;
	assert(IsEqualGUID(info->Guid, &menu_item_guid));
	struct govno_panel* result = calloc(1, sizeof(struct govno_panel));
	if (! result)
	{
		//TODO: LOG
		return 0;
	}
	return result;
}

void WINAPI ClosePanelW(const struct ClosePanelInfo* info)
{
	assert(info->StructSize == sizeof(struct ClosePanelInfo));

	if (info->hPanel)
	{
		free(info->hPanel);
	}
}


// Panel opened, desribe panel
void WINAPI GetOpenPanelInfoW(struct OpenPanelInfo* info)
{
	info->StructSize = sizeof(struct OpenPanelInfo);
	info->Flags = OPIF_DISABLEHIGHLIGHTING;
	info->PanelTitle = L"Roosters are chatting..";
	info->CurDir = NULL;
	info->Format = L"Govnopanel"; // TODO: wat?!

	info->InfoLinesNumber = 1;
	info->DescrFilesNumber = 0; //TODO: FILL

	info->PanelModesNumber = 1;
	static WCHAR* panel_mode_titles[2] = {L"ѕитушок", L"„о л€пнул"};
	static struct PanelMode panel_mode = {
		.ColumnTypes = L"NON,Z",
		.ColumnWidths = L"10,0",
	};
	panel_mode.Flags = PMFLAGS_DETAILEDSTATUS;
	panel_mode.ColumnTitles = panel_mode_titles;
	//TODO: Support other modes aswell			
	info->PanelModesArray = &panel_mode;
	info->StartPanelMode = 1;

	//TODO: Set keybar and other stuff

	//TODO: Close panel info
}

//ClosePanelW


// Get list of files
intptr_t WINAPI GetFindDataW(struct GetFindDataInfo* info)
{
	assert(info->StructSize == sizeof(struct GetFindDataInfo));
	int fetched;

	struct govno_panel* p_panel = info->hPanel;
	assert(p_panel);

	p_panel->file_items = calloc(MESSAGES_TO_FETCH, sizeof(struct PluginPanelItem));
	p_panel->message_headers = calloc(MESSAGES_TO_FETCH, sizeof(struct govno_message_header));

	if (GovnoGet(MESSAGES_TO_FETCH, p_panel->message_headers, &fetched) != 0)
	{
		//TODO: LOG
		return 0;
	}

	assert(fetched <= MESSAGES_TO_FETCH);
	for (int i = 0; i < fetched; i++)
	{
		const long id = (p_panel->message_headers + i)->id;
		// TODO: Use userdata instead, do not forget to free it
		(p_panel->file_items + i)->AllocationSize = id;
		(p_panel->file_items + i)->FileName = (p_panel->message_headers + i)->nick;
		(p_panel->file_items + i)->Description = (p_panel->message_headers + i)->message;
	}

	info->ItemsNumber = fetched;
	info->PanelItem = p_panel->file_items;

	return 1;
}

void WINAPI FreeFindDataW(const struct FreeFindDataInfo* info)
{
	if (info->hPanel)
	{
		if (((struct govno_panel*)info->hPanel)->file_items)
		{
			free(((struct govno_panel*)info->hPanel)->file_items);
		}
		if (((struct govno_panel*)info->hPanel)->message_headers)
		{
			free(((struct govno_panel*)info->hPanel)->message_headers);
		}
	}
}


// Create and return list of files
intptr_t WINAPI GetFilesW(struct GetFilesInfo* info)
{
	const wchar_t* dest_path = info->DestPath;
	const size_t max_dest_path_len_chars = (GOVNO_NICK_LEN_WCHARS + wcslen(dest_path) + 11) + 5; //1 for .txtNULL
	// 11 is magic govnonumber here for slash and id (which is 2^32 unsigned max, I hope)
	const wchar_t* dest_file_name = _malloca(max_dest_path_len_chars * sizeof(WCHAR));
	if (!dest_file_name)
	{
		// todo: log
		return 0;
	}
	// Screw ms for not having vla in c
	for (size_t i = 0; i < info->ItemsNumber; i++)
	{
		const struct PluginPanelItem item = info->PanelItem[i];
		const wchar_t* file_name = item.FileName;
		swprintf_s(dest_file_name, max_dest_path_len_chars, L"%s\\%s%llu.txt", dest_path, file_name, item.AllocationSize);
		FILE* file;
		_wfopen_s(&file, dest_file_name, L"w,ccs=UNICODE");
		if (!file)
		{
			//TODO: LOG
			_freea(dest_file_name);
			return 0;
		}

		const static WCHAR* separator = L" : ";

		fwrite(item.FileName, sizeof(WCHAR), wcslen(item.FileName), file);
		fwrite(separator, sizeof(WCHAR), wcslen(separator), file);
		fwrite(item.Description, sizeof(WCHAR), wcslen(item.Description), file);

		fclose(file);
	}
	_freea(dest_file_name);
	return 1;
}
