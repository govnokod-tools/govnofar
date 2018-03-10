#include "json.h"
#include "Govnoclient.h"
#include <Wininet.h>
#include <assert.h>
#include <stdint.h>

# define GOVNO_URL L"http://b.gcode.cx/ngk/api/comments"
# define BUF_SIZE 4096


enum govno_message_type
{
	nick = 1,
	message = 2,
	id = 3
};

struct govno_messages
{
	struct govno_message_header* messages;
	uint32_t length;
	enum govno_message_type type;
	const uint32_t max_messages;
	BOOL success; //TODO: use int and -1 instead?
};

static void CopyString(LPWSTR dst, const char* src, const int dst_in_chars)
{
	MultiByteToWideChar(CP_UTF8, 0, src, -1, dst, dst_in_chars);
}

static int _json_callback(void* userdata, const int type, const char* data, uint32_t length)
{
	struct govno_messages* p_govno_messages = (struct govno_messages*)userdata;
	char* who_cares;
	// ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement
	switch (type)
	{
	case JSON_KEY:
		if (strcmp(data, "user_name") == 0)
		{
			p_govno_messages->type = nick;
		}
		else if (strcmp(data, "text") == 0)
		{
			p_govno_messages->type = message;
		}
		else if (strcmp(data, "post_id") == 0)
		{
			p_govno_messages->type = id;
		}
		else
		{
			p_govno_messages->type = 0;
		}
		break;
	case JSON_INT: //id is int
	case JSON_STRING:
		{
			struct govno_message_header* p_header = (p_govno_messages->messages + p_govno_messages->length);
			switch (p_govno_messages->type)
			{
			case nick:
				CopyString(p_header->nick, data, GOVNO_NICK_LEN_WCHARS);
				break;
			case message:
				CopyString(p_header->message, data, GOVNO_MESS_LEN_WCHARS);
				break;
			case id:
				p_header->id = strtol(data, &who_cares, 10);
				break;
			}
		}
		break;
	case JSON_OBJECT_END:
		p_govno_messages->length++;
		p_govno_messages->success = TRUE;
		if (p_govno_messages->length == p_govno_messages->max_messages)
		{
			return 0; // Stop processing
		}
		break;
	}
	return 0;
}

DWORD GovnoGet(const int max_messages, struct govno_message_header* p_dest, int* messages_fetched)
{
	assert(max_messages > 0);


	const HINTERNET inet = InternetOpen(L"Govnoclient", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (!inet)
	{
		return GetLastError();
	}


	const HINTERNET result = InternetOpenUrl(inet, GOVNO_URL, NULL, 0, INTERNET_FLAG_NO_UI,
	                                         (DWORD_PTR)NULL);
	if (!result)
	{
		const DWORD last_error = GetLastError();
		InternetCloseHandle(inet);		
		return last_error;
	}

	json_parser parser;

	struct govno_messages govno_messages = {
		.length = 0, .messages = p_dest, .max_messages = max_messages, .success = FALSE
	};
	const int json_error = json_parser_init(&parser, NULL, &_json_callback, &govno_messages);
	if (json_error != 0)
	{
		InternetCloseHandle(result);
		InternetCloseHandle(inet);
		return json_error;
	}

	char buffer[BUF_SIZE] = {0};
	DWORD bytes_read;
	do
	{
		if (!InternetReadFile(result, buffer, BUF_SIZE, &bytes_read))
		{
			const DWORD error = GetLastError();
			InternetCloseHandle(result);
			InternetCloseHandle(inet);
			return error;
		}
		uint32_t bytes_processed;
		const int parse_result = json_parser_string(&parser, buffer, bytes_read, &bytes_processed);
		if (parse_result != 0)
		{
			InternetCloseHandle(result);
			InternetCloseHandle(inet);
			return parse_result;
		}
	}
	while (bytes_read != 0);
	if (govno_messages.success)
	{
		*(messages_fetched) = govno_messages.length;
	}
	return 0;
}
