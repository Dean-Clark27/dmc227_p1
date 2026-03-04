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
  // cout << "parsing.cc::extract_string() is not implemented\n";
  // NB: These assertions are only here to prevent compiler warnings
  // assert(count != 0);
  string result(it, it + count); 

  advance(it,count);
  return result;
}


/// Extract a size (uint32_t) from a vector
/// @param it An iterator to the extraction point
/// @return The uint32_t extracted
uint32_t extract_size(vector<uint8_t>::iterator &it) {
  // cout << "parsing.cc::extract_size() is not implemented\n";
  // return 0;
  uint32_t size;

  memcpy(&size, &(*it), sizeof(uint32_t));
  advance(it, sizeof(uint32_t));
  return size; 
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
  // cout << "parsing.cc::parse_request() is not implemented\n";
  // NB: These assertions are only here to prevent compiler warnings
  // assert(sd);
  // assert(storage);
  
  vector<uint8_t> header(16);

  if (!reliable_get_to_eof_or_n(sd, header.begin(), 16)) {
    return false; 
  }

  auto it = header.begin();
  string cmd = extract_string(it, 4);
  uint32_t ulen = extract_size(it);
  uint32_t plen = extract_size(it);
  uint32_t mlen = extract_size(it);

  // 2. Security Check: Validate lengths against constants.h limits
  if (ulen > LEN_UNAME || plen > LEN_PASSWORD || mlen > LEN_PROFILE_FILE) {
    return false; // Or send RES_ERR_REQ_FMT
  }

  // 3. Read the variable data (Username, Password, and Message/Profile)
  vector<uint8_t> body(ulen + plen + mlen);
  if (!reliable_get_to_eof_or_n(sd, body.begin(), body.size())) {
    return false;
  }

  auto body_it = body.begin();
  string user = extract_string(body_it, ulen);
  string pass = extract_string(body_it, plen);
  vector<uint8_t> msg(body_it, body_it + mlen);

  // 4. Dispatch to the appropriate handler
  if (cmd == REQ_REG) return handle_reg(sd, storage, user, pass, msg);
  if (cmd == REQ_BYE) return handle_bye(sd, storage, user, pass, msg);
  if (cmd == REQ_SAV) return handle_sav(sd, storage, user, pass, msg);
  if (cmd == REQ_SET) return handle_set(sd, storage, user, pass, msg);
  if (cmd == REQ_GET) return handle_get(sd, storage, user, pass, msg);
  if (cmd == REQ_ALL) return handle_all(sd, storage, user, pass, msg);

  return false;
}