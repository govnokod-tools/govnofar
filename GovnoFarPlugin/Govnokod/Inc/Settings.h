#pragma once
#include <Windows.h>
#include "Govnoclient.h"

typedef HANDLE govnosettings;

// Called on first OpenW hence AFTER plugin is loaded
govnosettings LoadGovnoSettings(const struct PluginStartupInfo* p_methods, const GUID plugin_guid);

//Called when all panles closed, so plugin is almost unloaded
void FreeGovnoSettings(const govnosettings settings);

// Make sure to free handle when you do not need id. Should I create separate "free" function?

// Returns comma-separated string of users to ignore. May be NULL if noone to ignore.
// Please, do not free it nor modify in any way. It will be freed automatically on FreeGovnoSettings
const WCHAR* GetListOfIgnoredGovnoUsers(const govnosettings settings);


void IgnoreGovnoUser(const govnosettings settings, const long user_id);
