#include <string>
#include <iostream>
#include <cassert>
#include "../common/constants.h"
#include "../common/net.h"
#include "responses.h"

using namespace std;

/// Add the size of a value to a vector as a 4-byte value
///
/// @param res  The vector to add to
/// @param t    The thing whose size should be added
///
/// @tparam T   The type of t
template <class T> void add_size(vector<uint8_t> &res, T t) {
  // cout << "responses.cc::add_size() is not implemented\n";
  // NB: These asserts are to prevent compiler warnings
  // assert(res.size() > 0);
  // assert(t.size() > 0);

  uint32_t size = t.size();
  res.push_back(static_cast<uint8_t>(size & 0xFF));
  res.push_back(static_cast<uint8_t>((size >> 8) & 0xFF));
  res.push_back(static_cast<uint8_t>((size >> 16) & 0xFF));
  res.push_back(static_cast<uint8_t>((size >> 24) & 0xFF));
}

/// Add the contents of an iterable to a vector
///
/// @param res  The vector to add to
/// @param t    The thing to add
///
/// @tparam T   The type of t
template <class T> void add_it(vector<uint8_t> &res, T t) {
  // cout << "responses.cc::add_it() is not implemented\n";
  // NB: These asserts are to prevent compiler warnings
  // assert(res.size() > 0);
  // assert(t.size() > 0);
  res.insert(res.end(), std::begin(t), std::end(t));
}

/// Concatenate a string and a vector of content, in a format that can be sent
/// to the client as a single message.  Most often, this involves a message of
/// RES_OK and content that was returned from a hash table.
///
/// @param msg     A string message to send to the client
/// @param content A vector of content to send to the client
///
/// @return a vector with the correct concatenation of msg and content
vector<uint8_t> build_res(const string &msg, const vector<uint8_t> &content) {
  // cout << "responses.cc::build_res() is not implemented\n";
  // NB: These asserts are to prevent compiler warnings
  // assert(msg.length() > 0);
  // assert(content.size() > 0);
  
  vector<uint8_t> response;
  
  // 1. Add the 4-character message (e.g., "OK__")
  // Make sure add_it doesn't add a null terminator!
  add_it(response, msg);

  // 2. ALWAYS add the size, even if it's 0.
  // This ensures the client knows to expect 0 bytes of data.
  add_size(response, content);

  // 3. Add the actual content (if any)
  if (!content.empty()) {
    add_it(response, content);
  }
  
  return response;
}
/// Send a message format error
///
/// @param sd  The socket onto which the result should be written
///
/// @return false, to indicate that the server shouldn't stop
bool send_err_msg_format(int sd) {
    // RES_ERR_REQ_FMT is likely a constant for "ERR_" or similar
    return send_reliably(sd, RES_ERR_REQ_FMT); 
}

/// Extract a string from a vector
///
/// @param it     An iterator to the extraction point
/// @param count  The number of characters to extract
/// @return The extracted string
string extract_string(vector<uint8_t>::const_iterator &it,
                             size_t count) {
  // cout << "responses.cc::extract_string() is not implemented\n";
  // NB: These asserts are to prevent compiler warnings
  // assert(count != 0);
  std::string result(it, it + count); 

  std::advance(it,count);
  return result;
}

/// Extract a size (uint32_t) from a vector
/// @param it An iterator to the extraction point
/// @return The extracted uint32_t
uint32_t extract_size(vector<uint8_t>::const_iterator &it) {
  // cout << "responses.cc::extract_size() is not implemented\n";
  // NB: These asserts are to prevent compiler warnings
  uint32_t size;

  memcpy(&size, &(*it), sizeof(uint32_t));

  std::advance(it, sizeof(uint32_t));
  return size; 
}

/// Extract a vector from a vector
/// @param it     An iterator to the extraction point
/// @param count  The number of bytes to extract
/// @return The extracted vector
vector<uint8_t> extract_vec(vector<uint8_t>::const_iterator &it,
                                   size_t count) {
  // cout << "responses.cc::extract_vector() is not implemented\n";
  // NB: These asserts are to prevent compiler warnings
  // assert(count != 0);
  
  // Define the range as from [it,end)  
  auto end = next(it, count);

  // Construct new vector from range 
  vector<uint8_t> extracted(it, end);

  // Update iterator to new position in vector 
  it = end;
  return extracted; 
}

/// Respond to an ALL command by generating a list of all the usernames in the
/// Auth table and returning them, one per line.
///
/// @param sd      The socket onto which the result should be written
/// @param storage The Storage object, which contains the auth table
/// @param u       The user name associated with the request
/// @param p       The password associated with the request
/// @param req     The contents of the request
///
/// @return false, to indicate that the server shouldn't stop
bool handle_all(int sd, Storage *storage,
                const std::string &u, const std::string &p,
                const vector<uint8_t> &req) {
  // cout << "responses.cc::handle_all() is not implemented\n";
  // NB: These asserts are to prevent compiler warnings
  // assert(sd);
  // assert(storage);
  // assert(u.length() > 0);
  // assert(p.length() > 0);
  // assert(req.size() > 0);
  
  // ALL_ command should have no additional data (req should be empty)
  // Protocol: "ALL_".len(user).len(pass).@0.user.pass
  // The req parameter here represents the additional message data, which is empty
  // Call storage to get all users
  auto result = storage->get_all_users(u, p);
  
  if (result.succeeded) {
    // Build response: OK__.len(data).data
    vector<uint8_t> response = build_res(RES_OK, result.data);
    send_reliably(sd, response);
  } else {
    // Send error message
    send_reliably(sd, result.msg);
  }
  
  return false;
}

/// Respond to a SET command by putting the provided data into the Auth table
///
/// @param sd      The socket onto which the result should be written
/// @param storage The Storage object, which contains the auth table
/// @param u       The user name associated with the request
/// @param p       The password associated with the request
/// @param req     The contents of the request
///
/// @return false, to indicate that the server shouldn't stop
bool handle_set(int sd, Storage *storage,
                const std::string &u, const std::string &p,
                const vector<uint8_t> &req) {
  // cout << "responses.cc::handle_set() is not implemented\n";
  // NB: These asserts are to prevent compiler warnings
  // assert(sd);
  // assert(storage);
  // assert(u.length() > 0);
  // assert(p.length() > 0);
  // assert(req.size() > 0);
  // return false;

  // SETP protocol: "SETP".len(user).len(pass).len(msg).user.pass.msg
  // Where msg = len(content).content
  // The req parameter contains: len(content).content
  
  // Parse the request: first 4 bytes are the content length
  auto it = req.begin();
  
  if (req.size() < 4) {
    return send_err_msg_format(sd);
  }
  
  uint32_t content_len = extract_size(it);
  
  // Check if we have enough data
  if (req.size() != 4 + content_len) {
    return send_err_msg_format(sd);
  }
  
  // Extract the content
  vector<uint8_t> content = extract_vec(it, content_len);
  
  // Call storage to set user data
  auto result = storage->set_user_data(u, p, content);
  
  if (result.succeeded) {
    send_reliably(sd, RES_OK);
  } else {
    send_reliably(sd, result.msg);
  }
  
  return false;
}

/// Respond to a GET command by getting the data for a user
///
/// @param sd      The socket onto which the result should be written
/// @param storage The Storage object, which contains the auth table
/// @param u       The user name associated with the request
/// @param p       The password associated with the request
/// @param req     The contents of the request
///
/// @return false, to indicate that the server shouldn't stop
bool handle_get(int sd, Storage *storage,
                const std::string &u, const std::string &p,
                const vector<uint8_t> &req) {
  // cout << "responses.cc::handle_get() is not implemented\n";
  // NB: These asserts are to prevent compiler warnings
  // assert(sd);
  // assert(storage);
  // assert(u.length() > 0);
  // assert(p.length() > 0);
  // assert(req.size() > 0);

  // GETP protocol: "GETP".len(user).len(pass).len(msg).user.pass.msg
  // Where msg = len(who).who
  // The req parameter contains: len(who).who
  
  // Parse the request: first 4 bytes are the length of 'who'
  auto it = req.begin();
  
  if (req.size() < 4) {
    return send_err_msg_format(sd);
  }
  
  uint32_t who_len = extract_size(it);
  
  // Check if we have enough data
  if (req.size() != 4 + who_len) {
    return send_err_msg_format(sd);
  }
  
  // Extract the username we're getting data for
  string who = extract_string(it, who_len);
  
  // Call storage to get user data
  auto result = storage->get_user_data(u, p, who);
  
  if (result.succeeded) {
    // Build response: OK__.len(data).data
    vector<uint8_t> response = build_res(RES_OK, result.data);
    send_reliably(sd, response);
  } else {
    // Send error message
    send_reliably(sd, result.msg);
  }
  
  return false;
}

/// Respond to a REG command by trying to add a new user
///
/// @param sd      The socket onto which the result should be written
/// @param storage The Storage object, which contains the auth table
/// @param u       The user name associated with the request
/// @param p       The password associated with the request
/// @param req     The contents of the request
///
/// @return false, to indicate that the server shouldn't stop
bool handle_reg(int sd, Storage *storage,
                const std::string &u, const std::string &p,
                const vector<uint8_t> &req) {
  // cout << "responses.cc::handle_reg() is not implemented\n";
  // NB: These asserts are to prevent compiler warnings
  // assert(sd);
  // assert(storage);
  // assert(u.length() > 0);
  // assert(p.length() > 0);
  // assert(req.size() > 0);
  // REG_ protocol: "REG_".len(user).len(pass).@0.user.pass
  // The req parameter should be empty (the third length field is 0)
  
  // For REG, there should be no additional data
  // The username and password are already extracted (u and p parameters)
  // Call storage to add user
  auto result = storage->add_user(u, p);
  
  if (result.succeeded) {
    send_reliably(sd, RES_OK);
  } else {
    send_reliably(sd, result.msg);
  }
  
  return false;
}

/// Respond to a BYE command by returning false, but only if the user
/// authenticates
///
/// @param sd      The socket onto which the result should be written
/// @param storage The Storage object, which contains the auth table
/// @param u       The user name associated with the request
/// @param p       The password associated with the request
/// @param req     The contents of the request
///
/// @return true, to indicate that the server should stop, or false on an error
bool handle_bye(int sd, Storage *storage,
                const std::string &u, const std::string &p,
                const vector<uint8_t> &req) {
  // cout << "responses.cc::handle_bye() is not implemented\n";
  // NB: These asserts are to prevent compiler warnings
  // assert(sd);
  // assert(storage);
  // assert(u.length() > 0);
  // assert(p.length() > 0);
  // assert(req.size() > 0);
  
  // EXIT protocol: "EXIT".len(user).len(pass).@0.user.pass
  // The req parameter should be empty
  // Authenticate the user
  auto result = storage->auth(u, p);
  
  if (result.succeeded) {
    // Send OK and then return true to stop the server
    send_reliably(sd, RES_OK);
    return true;  // Signal server to stop
  } else {
    // Send error and don't stop the server
    send_reliably(sd, result.msg);
    return false;
  }
}

/// Respond to a SAV command by persisting the file, but only if the user
/// authenticates
///
/// @param sd      The socket onto which the result should be written
/// @param storage The Storage object, which contains the auth table
/// @param u       The user name associated with the request
/// @param p       The password associated with the request
/// @param req     The contents of the request
///
/// @return false, to indicate that the server shouldn't stop
bool handle_sav(int sd, Storage *storage,
                const std::string &u, const std::string &p,
                const vector<uint8_t> &req) {
  // cout << "responses.cc::handle_sav() is not implemented\n";
  // NB: These asserts are to prevent compiler warnings
  // assert(sd);
  // assert(storage);
  // assert(u.length() > 0);
  // assert(p.length() > 0);
  // assert(req.size() > 0);
  
  // SAVE protocol: "SAVE".len(user).len(pass).@0.user.pass
  // The req parameter should be empty
  // First authenticate the user
  auto auth_result = storage->auth(u, p);
  
  if (!auth_result.succeeded) {
    send_reliably(sd, auth_result.msg);
    return false;
  }
  
  // User authenticated, now save the file
  auto result = storage->save_file();
  
  if (result.succeeded) {
    send_reliably(sd, RES_OK);
  } else {
    send_reliably(sd, result.msg);
  }
  
  return false;
}
