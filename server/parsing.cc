#include <cstring>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "../common/constants.h"
#include "../common/contextmanager.h"
#include "../common/err.h"
#include "../common/net.h"

#include "parsing.h"
#include "responses.h"

using namespace std;

/// Extract a string from a vector
///
/// @param it     An iterator to the extraction point
/// @param count  The number of characters to extract
/// @return The string
string extract_string(vector<uint8_t>::iterator &it, size_t count) {
  cout << "parsing.cc::extract_string() is not implemented\n";
  // NB: These assertions are only here to prevent compiler warnings
  assert(count != 0);
  return "";
}


/// Extract a size (uint32_t) from a vector
/// @param it An iterator to the extraction point
/// @return The uint32_t extracted
uint32_t extract_size(vector<uint8_t>::iterator &it) {
  cout << "parsing.cc::extract_size() is not implemented\n";
  return 0;
}

/// When a new client connection is accepted, this code will run to figure out
/// what the client is requesting, and to dispatch to the right function for
/// satisfying the request.
///
/// @param sd      The socket on which communication with the client takes place
/// @param storage The Storage object with which clients interact
///
/// @return true if the server should halt immediately, false otherwise
bool parse_request(int sd, Storage *storage) {
  cout << "parsing.cc::parse_request() is not implemented\n";
  // NB: These assertions are only here to prevent compiler warnings
  assert(sd);
  assert(storage);
  return false;
}
