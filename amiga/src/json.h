#ifndef JSON_H
#define JSON_H

/*
 * Minimal read-only JSON lookup.  Other fnweather ports ask FujiNet to parse
 * JSON (network_json_query); NIO on the Amiga has no such service, so the
 * body is parsed here.  Paths use the same "/key/key/index" form, e.g.
 * "/daily/temperature_2m_max/3".
 *
 * json_get copies the value into out: strings without their quotes (escapes
 * other than \" and \\ are passed through), numbers/true/false as written,
 * null as "".  Returns 1 if the path exists, 0 otherwise (out is then "").
 */
int json_get(const char *json, const char *path, char *out, int out_len);

#endif
