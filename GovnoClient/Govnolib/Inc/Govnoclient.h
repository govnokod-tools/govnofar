#pragma once
#include <Windows.h>
#define GOVNO_NICK_LEN_WCHARS  32
#define GOVNO_MESS_LEN_WCHARS  1024

struct govno_message_header {
	const WCHAR nick[GOVNO_NICK_LEN_WCHARS];
	const WCHAR message[GOVNO_MESS_LEN_WCHARS];
	long id;
};


extern DWORD GovnoGet(const int max_messages, struct govno_message_header* p_dest, int* messages_fetched);
