#include "Govnoclient.h"
#include <stdio.h>
#include <Windows.h>

#define MAX_MESSAGES 50

static int fail(const char* error, const int result)
{
	//TODO: Use event logger instead?
	fprintf(stderr, "Error: %s (%d)", error, result);
	return 1;
}

//TODO: use macros instead?
static void _print(const WCHAR* message, const DWORD len)
{
	DWORD who_cares;
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), message, wcsnlen_s(message, len), &who_cares, NULL);
}

int main()
{
	struct govno_message_header* messages = malloc(sizeof(struct govno_message_header) * MAX_MESSAGES);
	if (!messages)
	{
		return fail("Can't get mem", 0);
	}

	int message_count = 0;

	const int getResult = GovnoGet(MAX_MESSAGES, messages, &message_count, L"1,2,3");
	if (getResult != 0)
	{
		return fail("Can't get data", getResult);
	}

	for (int i = 0; i < message_count; i++)
	{
		const struct govno_message_header message = messages[i];
		_print(message.nick, GOVNO_NICK_LEN_WCHARS);
		_print(L" : ", GOVNO_NICK_LEN_WCHARS); // Yes, its stupid, will optimize to sprintf later
		_print(message.message, GOVNO_MESS_LEN_WCHARS);
		_print(L"\n", GOVNO_NICK_LEN_WCHARS);
	}

	free(messages);
}
