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
  std::string result(it, it + count); 

  std::advance(it,count);
  return result;
}


/// Extract a size (uint32_t) from a vector
/// @param it An iterator to the extraction point
/// @return The uint32_t extracted
uint32_t extract_size(vector<uint8_t>::iterator &it) {
  // cout << "parsing.cc::extract_size() is not implemented\n";
  // return 0;
  uint32_t size;

  std::memcpy(&size, &(*it), sizeof(uint32_t));

  std::advance(it, sizeof(uint32_t));
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
  // Protocol format: CMD (4 bytes) | len1 (4) | len2 (4) | len3 (4) | data1 | data2 | data3
  // Where: CMD = command, len1 = username length, len2 = password length, len3 = additional msg length
  //        data1 = username, data2 = password, data3 = additional message data

  // 1. Read the command (4 bytes)
  vector<uint8_t> cmd_buf(4);
  if (!reliable_get_to_eof_or_n(sd, cmd_buf.begin(), 4)) {
    return false;
  }
  // Convert buffer to string for easy comparison
  string cmd(cmd_buf.begin(), cmd_buf.end());

  // 2. Read three length fields (12 bytes total: 3 x 4 bytes)
  vector<uint8_t> lengths_buf(12);
  if (!reliable_get_to_eof_or_n(sd, lengths_buf.begin(), 12)) {
    return false;
  }
  
  auto len_it = lengths_buf.begin();
  uint32_t user_len = extract_size(len_it);
  uint32_t pass_len = extract_size(len_it);
  uint32_t msg_len  = extract_size(len_it);

  // 3. Validate lengths (Sanity check to prevent OOM attacks)
  if (user_len > LEN_UNAME || pass_len > LEN_PASSWORD) {
    send_err_msg_format(sd);
    return false;
  }

  // 4. Read username
  string user = "";
  if (user_len > 0) {
    vector<uint8_t> user_buf(user_len);
    if (!reliable_get_to_eof_or_n(sd, user_buf.begin(), user_len)) return false;
    user.assign(user_buf.begin(), user_buf.end());
  }

  // 5. Read password
  string pass = "";
  if (pass_len > 0) {
    vector<uint8_t> pass_buf(pass_len);
    if (!reliable_get_to_eof_or_n(sd, pass_buf.begin(), pass_len)) return false;
    pass.assign(pass_buf.begin(), pass_buf.end());
  }

  // 6. Read additional message data
  vector<uint8_t> msg;
  if (msg_len > 0) {
    msg.resize(msg_len);
    if (!reliable_get_to_eof_or_n(sd, msg.begin(), msg_len)) {
      return false;
    }
  }

// 7. Dispatch
  vector<string> command_names = {REQ_REG, REQ_BYE, REQ_SAV, REQ_SET, REQ_GET, REQ_ALL};
  
  // Use decltype to automatically get the correct function pointer signature
  // handle_reg is used as the template for the type
  using handler_t = decltype(&handle_reg);

  handler_t handlers[] = {
      handle_reg, handle_bye, handle_sav, handle_set, handle_get, handle_all
  };

  for (size_t i = 0; i < command_names.size(); ++i) {
    if (cmd == command_names[i]) {
      // The 'msg' vector we populated in step 6 is passed here
      return handlers[i](sd, storage, user, pass, msg); 
    }
  }

  // Unknown command
  send_err_msg_format(sd);
  return false;
}