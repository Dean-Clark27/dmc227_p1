#include <cassert>
#include <cstring>
#include <iostream>
#include <libgen.h>
#include <string>
#include <unistd.h>
#include <vector>

#include "../common/constants.h"
#include "../common/contextmanager.h"
#include "../common/file.h"
#include "../common/net.h"

#include "requests.h"

using namespace std;

/// Use this instead of constructing empty vector args to send_cmd
vector<uint8_t> empty_vec;

/// Add the size of a value to a vector as a little-endian 4-byte value 
template <class T> void add_size(vector<uint8_t> &res, T t) {
  uint32_t size = t.size();
  res.push_back(static_cast<uint8_t>(size & 0xFF));
  res.push_back(static_cast<uint8_t>((size >> 8) & 0xFF));
  res.push_back(static_cast<uint8_t>((size >> 16) & 0xFF));
  res.push_back(static_cast<uint8_t>((size >> 24) & 0xFF));
}

/// Helper to extract a 4-byte little-endian length from a buffer
uint32_t extract_length(const vector<uint8_t> &buf, size_t start) {
  uint32_t len = 0;
  for (int i = 0; i < 4; ++i) {
    len |= (static_cast<uint32_t>(buf[start + i]) << (8 * i));
  }
  return len;
}

/// If a buffer consists of RES_OK.bbbb.d+, where `.` means concatenation, bbbb
/// is an 4-byte binary integer and d+ is a string of characters, write the
/// bytes (d+) to a file
void send_result_to_file(const vector<uint8_t> &buf, const string &filename) {
  // Check if buffer starts with "OK__" and has space for length
  if (buf.size() < 8 || memcmp(buf.data(), RES_OK.c_str(), 4) != 0) {
    cerr << "Error: Response does not start with OK__" << endl;
    return;
  }

  uint32_t data_len = extract_length(buf, 4);
  
  if (buf.size() < 8 + data_len) {
    cerr << "Error: Buffer too small for claimed data length" << endl;
    return;
  }

  // Extract the data starting at position 8
  vector<uint8_t> data(buf.begin() + 8, buf.begin() + 8 + data_len);
  
  // write_file signature: (filename, vector<uint8_t>, skip_count)
  if (!write_file(filename, data, 0)) {
    cerr << "Error: Failed to write to file " << filename << endl;
  }
}

/// Send a message to the server, using the common format for messages,
/// then take the response from the server and return it.
vector<uint8_t> send_cmd(int sd, const string &cmd,
                         const string &user, const string &password,
                         const vector<uint8_t> &msg) {
  vector<uint8_t> packet;
  
  // 1. Assemble Packet: CMD(4) | ULen(4) | PLen(4) | DLen(4) | User | Pass | Data
  packet.insert(packet.end(), cmd.begin(), cmd.end());
  add_size(packet, user);
  add_size(packet, password);
  add_size(packet, msg);
  packet.insert(packet.end(), user.begin(), user.end());
  packet.insert(packet.end(), password.begin(), password.end());
  packet.insert(packet.end(), msg.begin(), msg.end());
  
  if (!reliable_send(sd, packet.data(), packet.size())) {
    return {};
  }
  
  // 2. Read first 4 bytes (Response Code)
  vector<uint8_t> response(4);
  if (!reliable_get_to_eof_or_n(sd, response.begin(), 4)) {
    return {};
  }
  
  // 3. Handle Errors (starts with "ERR_")
  if (memcmp(response.data(), "ERR_", 4) == 0) {
    vector<uint8_t> error_rest(12);
    if (reliable_get_to_eof_or_n(sd, error_rest.begin(), 12)) {
        response.insert(response.end(), error_rest.begin(), error_rest.end());
    }
    return response;
  }
  
  // 4. Handle Success ("OK__")
  if (memcmp(response.data(), RES_OK.c_str(), 4) == 0) {
    vector<uint8_t> len_bytes(4);
    if (!reliable_get_to_eof_or_n(sd, len_bytes.begin(), 4)) {
      return response; 
    }
    
    uint32_t data_len = extract_length(len_bytes, 0);
    response.insert(response.end(), len_bytes.begin(), len_bytes.end());
    
    if (data_len > 0) {
      vector<uint8_t> data(data_len);
      if (!reliable_get_to_eof_or_n(sd, data.begin(), data_len)) {
        return {};
      }
      response.insert(response.end(), data.begin(), data.end());
    }
    return response;
  }
  
  return {};
}

// req_reg, req_bye, req_sav use the same basic pattern:
void req_reg(int sd, const string &user, const string &pass, const string &) {
  if (user.length() == 0 || user.length() > LEN_UNAME || pass.length() == 0 || pass.length() > LEN_PASSWORD) {
    cerr << RES_ERR_REQ_FMT << endl;
    return;
  }
  vector<uint8_t> response = send_cmd(sd, REQ_REG, user, pass, empty_vec);
  if (!response.empty()) cout << string(response.begin(), response.end()) << endl;
}

void req_bye(int sd, const string &user, const string &pass, const string &) {
  vector<uint8_t> response = send_cmd(sd, REQ_BYE, user, pass, empty_vec);
  if (!response.empty()) cout << string(response.begin(), response.end()) << endl;
}

void req_sav(int sd, const string &user, const string &pass, const string &) {
  vector<uint8_t> response = send_cmd(sd, REQ_SAV, user, pass, empty_vec);
  if (!response.empty()) cout << string(response.begin(), response.end()) << endl;
}

void req_set(int sd, const string &user, const string &pass, const string &setfile) {
  vector<uint8_t> file_contents = load_entire_file(setfile);
  if (file_contents.empty()) return;

  // Msg format: len(file).file
  vector<uint8_t> msg;
  add_size(msg, file_contents);
  msg.insert(msg.end(), file_contents.begin(), file_contents.end());
  
  vector<uint8_t> response = send_cmd(sd, REQ_SET, user, pass, msg);
  if (!response.empty()) cout << string(response.begin(), response.end()) << endl;
}

void req_get(int sd, const string &user, const string &pass, const string &getname) {
  // Msg format: len(getname).getname
  vector<uint8_t> msg;
  add_size(msg, getname);
  msg.insert(msg.end(), getname.begin(), getname.end());
  
  vector<uint8_t> response = send_cmd(sd, REQ_GET, user, pass, msg);
  if (!response.empty() && memcmp(response.data(), "OK__", 4) == 0) {
    send_result_to_file(response, getname + ".file.dat");
    cout << RES_OK << endl;
  } else if (!response.empty()) {
    cout << string(response.begin(), response.end()) << endl;
  }
}

void req_all(int sd, const string &user, const string &pass, const string &allfile) {
  vector<uint8_t> response = send_cmd(sd, REQ_ALL, user, pass, empty_vec);
  if (!response.empty() && memcmp(response.data(), "OK__", 4) == 0) {
    send_result_to_file(response, allfile);
    cout << RES_OK << endl;
  } else if (!response.empty()) {
    cout << string(response.begin(), response.end()) << endl;
  }
}