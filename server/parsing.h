#pragma once

#include <vector>

#include "storage.h"

/// Extract a string from a vector
///
/// @param it     An iterator to the extraction point
/// @param count  The number of characters to extract
/// @return The string
string extract_string(vector<uint8_t>::iterator &it, size_t count);

/// Extract a size (uint32_t) from a vector
/// @param it An iterator to the extraction point
/// @return The uint32_t extracted
uint32_t extract_size(vector<uint8_t>::iterator &it);

/// When a new client connection is accepted, this code will run to figure out
/// what the client is requesting, and to dispatch to the right function for
/// satisfying the request.
///
/// @param sd      The socket on which communication with the client takes place
/// @param storage The Storage object with which clients interact
///
/// @return true if the server should halt immediately, false otherwise
bool parse_request(int sd, Storage *storage);
