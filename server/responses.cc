#include <string>
#include <iostream>
#include <cassert>
#include <cstring>
#include <vector>
#include <iterator>

#include "../common/constants.h"
#include "../common/net.h"
#include "responses.h"
#include "storage.h"

using namespace std;

// --- HELPER FUNCTIONS ---

template <class T> void add_size(vector<uint8_t> &res, T t) {
  uint32_t size = static_cast<uint32_t>(t.size());
  res.push_back(static_cast<uint8_t>(size & 0xFF));
  res.push_back(static_cast<uint8_t>((size >> 8) & 0xFF));
  res.push_back(static_cast<uint8_t>((size >> 16) & 0xFF));
  res.push_back(static_cast<uint8_t>((size >> 24) & 0xFF));
}

template <class T> void add_it(vector<uint8_t> &res, T t) {
  res.insert(res.end(), std::begin(t), std::end(t));
}

string extract_string(vector<uint8_t>::const_iterator &it, size_t count) {
  string result(it, it + count);
  advance(it, count);
  return result;
}

uint32_t extract_size(vector<uint8_t>::const_iterator &it) {
  uint32_t size;
  memcpy(&size, &(*it), sizeof(uint32_t));
  advance(it, sizeof(uint32_t));
  return size; 
}

vector<uint8_t> extract_vec(vector<uint8_t>::const_iterator &it, size_t count) {
  auto end = next(it, count);
  vector<uint8_t> extracted(it, end);
  it = end;
  return extracted; 
}

/// Send ONLY the string provided (used for REG, SET, SAV, BYE and ALL Errors)
bool send_status(int sd, const string &msg) {
    return reliable_send(sd, (const unsigned char*)msg.c_str(), msg.length());
}

/// Send 4-byte code + 4-byte length + payload (used for GET/ALL Success)
bool send_data(int sd, const string &msg, const vector<uint8_t> &content) {
    vector<uint8_t> response;
    // Protocol requires the 4-byte code (e.g., "OK__")
    string code = msg.substr(0, 4); 
    add_it(response, code);
    add_size(response, content);
    add_it(response, content);
    return reliable_send(sd, response.data(), response.size());
}

bool send_err_msg_format(int sd) {
    return send_status(sd, RES_ERR_REQ_FMT);
}

// --- COMMAND HANDLERS ---
// Return FALSE to keep the server running.
// Return TRUE to shut down the server (only used by BYE).

bool handle_all(int sd, Storage *storage, const string &u, const string &p, const vector<uint8_t> &) {
  auto result = storage->get_all_users(u, p);
  if (result.succeeded) {
    send_data(sd, RES_OK, result.data);
  } else {
    send_status(sd, result.msg);
  }
  return false; // Keep server alive
}

bool handle_set(int sd, Storage *storage, const string &u, const string &p, const vector<uint8_t> &req) {
  if (req.size() < 4) {
    send_err_msg_format(sd);
    return false;
  }
  auto it = req.begin();
  uint32_t content_len = extract_size(it);
  if (req.size() != 4 + content_len) {
    send_err_msg_format(sd);
    return false;
  }

  vector<uint8_t> content = extract_vec(it, content_len);
  auto result = storage->set_user_data(u, p, content);
  
  send_status(sd, result.succeeded ? RES_OK : result.msg);
  return false; // Keep server alive
}

bool handle_get(int sd, Storage *storage, const string &u, const string &p, const vector<uint8_t> &req) {
  if (req.size() < 4) {
    send_err_msg_format(sd);
    return false;
  }
  auto it = req.begin();
  uint32_t who_len = extract_size(it);
  if (req.size() != 4 + who_len) {
    send_err_msg_format(sd);
    return false;
  }

  string who = extract_string(it, who_len);
  auto result = storage->get_user_data(u, p, who);
  
  if (result.succeeded) {
    send_data(sd, RES_OK, result.data);
  } else {
    send_status(sd, result.msg);
  }
  return false; // Keep server alive
}

bool handle_reg(int sd, Storage *storage, const string &u, const string &p, const vector<uint8_t> &) {
  auto result = storage->add_user(u, p);
  send_status(sd, result.succeeded ? RES_OK : result.msg);
  return false; // Keep server alive
}

bool handle_sav(int sd, Storage *storage, const string &u, const string &p, const vector<uint8_t> &) {
  auto auth_result = storage->auth(u, p);
  if (!auth_result.succeeded) {
    send_status(sd, auth_result.msg);
    return false;
  }
  auto result = storage->save_file();
  send_status(sd, result.succeeded ? RES_OK : result.msg);
  return false; // Keep server alive
}

bool handle_bye(int sd, Storage *storage, const string &u, const string &p, const vector<uint8_t> &) {
  auto result = storage->auth(u, p);
  if (result.succeeded) {
    send_status(sd, RES_OK);
    return true; // Success! THIS command tells the server to shut down.
  }
  send_status(sd, result.msg);
  return false; // Auth failed, keep server alive
}