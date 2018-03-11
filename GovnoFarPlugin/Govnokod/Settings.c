#include <plugin.hpp>
#include "Settings.h"
#include <assert.h>
#include <Strsafe.h>
#include <malloc.h>

#define GOVNOKEY L"PetuhsToIgnore"

struct gonvostruct
{
	HANDLE settings_handle;
	const struct PluginStartupInfo* p_methods;
};

govnosettings LoadGovnoSettings(const struct PluginStartupInfo* p_methods, const GUID plugin_guid)
{
	struct FarSettingsCreate create = {.Guid = plugin_guid, .StructSize = sizeof(struct FarSettingsCreate)};
	//"%LOCALAPPDATA%\Far manager\Profile\PluginsData"
	if (!p_methods->SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, PSL_LOCAL, &create))
	{
		//TODO: LOG
		return 0;
	}
	struct gonvostruct* p_settings = calloc(1, sizeof(struct gonvostruct));
	p_settings->p_methods = p_methods;
	p_settings->settings_handle = create.Handle;
	return (govnosettings)p_settings;
}

//Called when all panles closed, so plugin is almost unloaded
void FreeGovnoSettings(const govnosettings settings)
{
	assert(settings);
	HANDLE handle = ((struct gonvostruct*)settings)->settings_handle;
	assert(handle);
	((struct gonvostruct*)settings)->p_methods->SettingsControl(handle, SCTL_FREE, 0, NULL);
}

const WCHAR* GetListOfIgnoredGovnoUsers(const govnosettings settings)
{
	const struct gonvostruct* gonvostruct = ((struct gonvostruct*)settings);
	HANDLE settings_handle = gonvostruct->settings_handle;

	struct FarSettingsItem item = {
		.StructSize = sizeof(struct FarSettingsItem), .Root = 0, .Type = FST_STRING, .Name = GOVNOKEY,
	};

	if (!gonvostruct->p_methods->SettingsControl(settings_handle, SCTL_GET, 0, &item))
	{
		// No current settings
		return NULL;
	}
	return item.Value.String;
}

void IgnoreGovnoUser(const govnosettings settings, const long user)
{
	assert(user);
	const WCHAR* current_value = GetListOfIgnoredGovnoUsers(settings);
	const size_t current_len_ch = (current_value ? wcslen(current_value) : 0);

	// Should be enough for long in string format plus comma plus 0
	const size_t new_value_ch = current_len_ch + 16;
	WCHAR* new_value = malloc(new_value_ch * sizeof(WCHAR));
	if (!new_value)
	{
		//TODO: LOG
		return;
	}

	StringCchPrintf(new_value, new_value_ch, L"%s%c%lu", (current_value ? current_value : L""),
	                (current_value ? L',' : ' '), user);

	const struct gonvostruct* gonvostruct = ((struct gonvostruct*)settings);
	struct FarSettingsItem item = {
		.StructSize = sizeof(struct FarSettingsItem), .Root = 0, .Type = FST_STRING, .Name = GOVNOKEY,
		.Value.Data.Size = ((wcslen(new_value) + 1) * sizeof(WCHAR)),
		.Value.String = new_value
	};

	if (!gonvostruct->p_methods->SettingsControl(gonvostruct->settings_handle, SCTL_SET, 0, &item))
	{
		//TODO: LOG;
	}
	free(new_value);
}
