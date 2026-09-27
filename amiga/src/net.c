#include <stdio.h>
#include <string.h>

#include <proto/dos.h>

#include "fujinet-nio.h"
#include "net.h"

#define READ_CHUNK 512
/* Delay() ticks are 1/50 s; allow a slow TLS fetch ~20 s before giving up. */
#define WAIT_TICKS 2
#define WAIT_LIMIT 500

static char body[BODY_MAX + 1];
static uint8_t chunk[READ_CHUNK];
static char error_text[64];

const char *net_error(void)
{
	return error_text;
}

static void set_error(const char *what, uint8_t code)
{
	snprintf(error_text, sizeof error_text, "%s: %s (%u)", what, fn_error_string(code),
	         (unsigned)code);
}

int net_init(void)
{
	uint8_t rc = fn_init();

	if (rc != FN_OK || !fn_is_ready()) {
		set_error("FujiNet", rc != FN_OK ? rc : FN_ERR_NOT_READY);
		return 1;
	}
	return 0;
}

void net_done(void)
{
	fn_shutdown();
}

const char *http_get(const char *url)
{
	fn_handle_t handle = FN_INVALID_HANDLE;
	uint32_t offset = 0;
	uint16_t stored = 0;
	uint16_t status = 0;
	uint32_t length = 0;
	uint8_t info_flags = 0;
	int waits = 0;
	uint8_t rc;

	if (strlen(url) >= FN_MAX_URL_LEN) {
		snprintf(error_text, sizeof error_text, "URL too long (%u > %u)", (unsigned)strlen(url),
		        (unsigned)FN_MAX_URL_LEN - 1);
		return NULL;
	}
	rc = fn_open(&handle, FN_METHOD_GET, url, FN_OPEN_FOLLOW_REDIR);
	if (rc != FN_OK) {
		set_error("open", rc);
		return NULL;
	}

	for (;;) {
		uint16_t got = 0;
		uint8_t flags = 0;

		rc = fn_read(handle, offset, chunk, READ_CHUNK, &got, &flags);
		if (rc == FN_ERR_NOT_READY || rc == FN_ERR_BUSY) {
			if (++waits > WAIT_LIMIT) {
				set_error("read", FN_ERR_TIMEOUT);
				fn_close(handle);
				return NULL;
			}
			Delay(WAIT_TICKS);
			continue;
		}
		if (rc != FN_OK) {
			set_error("read", rc);
			fn_close(handle);
			return NULL;
		}
		waits = 0;
		if (got && stored < BODY_MAX) {
			uint16_t take = got;

			if (take > BODY_MAX - stored)
				take = (uint16_t)(BODY_MAX - stored);
			memcpy(body + stored, chunk, take);
			stored = (uint16_t)(stored + take);
		}
		offset += got;
		if ((flags & FN_READ_EOF) || got == 0)
			break;
	}
	body[stored] = '\0';

	if (fn_info(handle, &status, &length, &info_flags) == FN_OK &&
	    (info_flags & FN_INFO_HAS_STATUS) && status != 200) {
		snprintf(error_text, sizeof error_text, "HTTP status %u", (unsigned)status);
		fn_close(handle);
		return NULL;
	}
	fn_close(handle);
	if (offset > BODY_MAX) {
		snprintf(error_text, sizeof error_text, "response too large (%lu bytes)",
		         (unsigned long)offset);
		return NULL;
	}
	return body;
}
