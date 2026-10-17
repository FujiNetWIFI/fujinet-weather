#ifndef JSONQUERY_H
#define JSONQUERY_H

#include "fujinet-network.h"

// JSON_QUERY(devicespec, query, s, maxlen)
//
// Query a JSON value into s, same arguments as network_json_query_n.
//
// When fujinet-lib provides network_json_query_n (announced by the
// FN_HAS_NETWORK_JSON_QUERY_N macro in fujinet-network.h) at most maxlen
// bytes are written to s. Older libs fall back to network_json_query:
// maxlen is ignored there and a long value can overrun s.
#ifdef FN_HAS_NETWORK_JSON_QUERY_N
#define JSON_QUERY(devicespec, query, s, maxlen) \
	network_json_query_n((devicespec), (query), (s), (maxlen))
#else
#define JSON_QUERY(devicespec, query, s, maxlen) \
	network_json_query((devicespec), (query), (s))
#endif

#endif // JSONQUERY_H
