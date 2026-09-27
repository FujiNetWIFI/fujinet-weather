#ifndef NET_H
#define NET_H

#define BODY_MAX 4096

/* Connect to FujiNet through fujinet-nio.device. 0 on success, else prints
 * why (usually: the resident driver is not loaded) and returns nonzero. */
int net_init(void);
void net_done(void);

/* HTTP(S) GET into a static buffer; returns the body or NULL, setting
 * net_error() to a printable reason. */
const char *http_get(const char *url);
const char *net_error(void);

#endif
