#include <string.h>

#include "json.h"

static const char *skip_ws(const char *p)
{
	while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
		++p;
	return p;
}

/* p at an opening quote; returns just past the closing quote, or NULL. */
static const char *skip_string(const char *p)
{
	for (++p; *p; ++p) {
		if (*p == '\\') {
			if (!*++p)
				return NULL;
		} else if (*p == '"') {
			return p + 1;
		}
	}
	return NULL;
}

/* Returns just past the value at p, or NULL on malformed input. Iterative,
 * so deep nesting cannot exhaust a 4 KB Shell stack. */
static const char *skip_value(const char *p)
{
	int depth = 0;

	p = skip_ws(p);
	do {
		switch (*p) {
		case '\0':
			return NULL;
		case '"':
			p = skip_string(p);
			if (!p)
				return NULL;
			break;
		case '{':
		case '[':
			++depth;
			++p;
			break;
		case '}':
		case ']':
			if (--depth < 0)
				return NULL;
			++p;
			break;
		default:
			if (depth == 0) {
				/* scalar at top level: number, true, false, null */
				while (*p && *p != ',' && *p != '}' && *p != ']' &&
				       *p != ' ' && *p != '\r' && *p != '\n' && *p != '\t')
					++p;
				return p;
			}
			++p;
		}
	} while (depth > 0);
	return p;
}

/* In the object at p, find key (length len); return its value or NULL. */
static const char *find_key(const char *p, const char *key, int len)
{
	p = skip_ws(p);
	if (*p != '{')
		return NULL;
	p = skip_ws(p + 1);
	while (*p == '"') {
		const char *name = p + 1;
		const char *end = skip_string(p);

		if (!end)
			return NULL;
		p = skip_ws(end);
		if (*p != ':')
			return NULL;
		p = skip_ws(p + 1);
		if (end - 1 - name == len && strncmp(name, key, len) == 0)
			return p;
		p = skip_value(p);
		if (!p)
			return NULL;
		p = skip_ws(p);
		if (*p == ',')
			p = skip_ws(p + 1);
	}
	return NULL;
}

/* In the array at p, return element index or NULL. */
static const char *find_index(const char *p, int index)
{
	p = skip_ws(p);
	if (*p != '[')
		return NULL;
	p = skip_ws(p + 1);
	while (*p && *p != ']') {
		if (index-- == 0)
			return p;
		p = skip_value(p);
		if (!p)
			return NULL;
		p = skip_ws(p);
		if (*p == ',')
			p = skip_ws(p + 1);
	}
	return NULL;
}

int json_get(const char *json, const char *path, char *out, int out_len)
{
	const char *p = json;
	int n = 0;

	out[0] = '\0';
	while (p && *path) {
		const char *seg;
		int len;

		if (*path == '/')
			++path;
		seg = path;
		while (*path && *path != '/')
			++path;
		len = (int)(path - seg);
		if (len == 0)
			continue;
		if (*skip_ws(p) == '[') {
			int index = 0;
			int i;

			for (i = 0; i < len; ++i) {
				if (seg[i] < '0' || seg[i] > '9')
					return 0;
				index = index * 10 + (seg[i] - '0');
			}
			p = find_index(p, index);
		} else {
			p = find_key(p, seg, len);
		}
	}
	if (!p)
		return 0;

	p = skip_ws(p);
	if (*p == '"') {
		for (++p; *p && *p != '"' && n < out_len - 1; ++p) {
			if (*p == '\\' && (p[1] == '"' || p[1] == '\\'))
				++p;
			out[n++] = *p;
		}
	} else if (strncmp(p, "null", 4) != 0) {
		while (*p && *p != ',' && *p != '}' && *p != ']' && *p != ' ' &&
		       *p != '\r' && *p != '\n' && n < out_len - 1)
			out[n++] = *p++;
	}
	out[n] = '\0';
	return 1;
}
