#include <cassert>
#include <cstring>
#include <iostream>
#include <openssl/rand.h>
#include <vector>

#include "../common/constants.h"
#include "../common/contextmanager.h"
#include "../common/file.h"
#include "../common/net.h"

#include "requests.h"

using namespace std;

const string OK = "OK";
const string ERR = "ERR";

/// Use this instead of constructing empty vector args to send_cmd
vector<uint8_t> empty_vec;

/// Add the size of a value to a vector as a little-endian 4-byte value 
///
/// @param res  The vector to add to
/// @param t    The thing whose size should be added
///
/// @tparam T   The type of t
template <class T> void add_size(vector<uint8_t> &res, T t) {
  
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
  res.insert(res.end(), std::begin(t), std::end(t));
}

/// If a buffer consists of RES_OK.bbbb.d+, where `.` means concatenation, bbbb
/// is an 4-byte binary integer and d+ is a string of characters, write the
/// bytes (d+) to a file
///
/// @param buf      The buffer holding a response
/// @param filename The name of the file to write
void send_result_to_file(const vector<uint8_t> &buf, const string &filename) {
  // Check if buffer starts with "OK__"
  if (buf.size() < 8 || 
      buf[0] != 'O' || buf[1] != 'K' || buf[2] != '_' || buf[3] != '_') {
    cerr << "Error: Response does not start with OK__" << endl;
    return;
}
// Extract the length (4 bytes starting at position 4)
  uint32_t data_len = extract_length(buf, 4);
  
  // Check if we have enough data
  if (buf.size() < 8 + data_len) {
    cerr << "Error: Buffer too small for claimed data length" << endl;
    return;
  }

  // Extract the data starting at position 8
  vector<uint8_t> data(buf.begin() + 8, buf.begin() + 8 + data_len);
  
  // Write to file
  if (!write_file(filename, reinterpret_cast<const char*>(data.data()), data.size())) {
    cerr << "Error: Failed to write to file " << filename << endl;
  }
}


/// Send a message to the server, using the common format for messages,
/// then take the response from the server and return it.
///
/// @param sd       The open socket descriptor for communicating with the server
/// @param cmd      The command that is being sent
/// @param user     The username for the request
/// @param password The password for the request
/// @param msg      The contents
///
/// @return a vector with the result, or an empty vector on error
vector<uint8_t> send_cmd(int sd, const string &cmd,
                         const string &user, const string &password,
                         const vector<uint8_t> &msg) {
  // cout << "requests.cc::send_cmd() is not implemented\n";
  // NB: These asserts are to prevent compiler warnings
  // assert(sd);
  // assert(cmd.length() > 0);
  // assert(user.length() > 0);
  // assert(password.length() > 0);
  // assert(msg.size() > 0);
  // return {};

  vector<uint8_t> packet;
  
  // Add 4-byte command
  add_string(packet, cmd);
  
  // Add length of username
  add_length(packet, user.length());
  
  // Add length of password
  add_length(packet, password.length());
  
  // Add length of additional message data
  add_length(packet, msg.size());
  
  // Add username
  add_string(packet, user);
  
  // Add password
  add_string(packet, password);
  
  // Add additional message data
  if (!msg.empty()) {
    add_vec(packet, msg);
  }
  
  // Send the packet
  if (!send_reliably(sd, packet)) {
    cerr << "Error: Failed to send request to server" << endl;
    return {};
  }
  
  // Read response: first 4 bytes are the response code
  vector<uint8_t> response_code;
  if (!read_exactly(sd, response_code, 4)) {
    cerr << "Error: Failed to read response code" << endl;
    return {};
  }
  
  // Check if it's an error message (starts with "ERR_")
  if (response_code[0] == 'E' && response_code[1] == 'R' && 
      response_code[2] == 'R' && response_code[3] == '_') {
    // Read the rest of the error message (error codes are fixed length)
    vector<uint8_t> error_rest;
    if (!read_exactly(sd, error_rest, 12)) {
      return response_code; // Return partial error if we can't read more
    }
    response_code.insert(response_code.end(), error_rest.begin(), error_rest.end());
    return response_code;
  }
  
  // Check if it's "OK__"
  if (response_code[0] == 'O' && response_code[1] == 'K' && 
      response_code[2] == '_' && response_code[3] == '_') {
    // Read the next 4 bytes for data length
    vector<uint8_t> len_bytes;
    if (!read_exactly(sd, len_bytes, 4)) {
      return response_code; // Just return OK__ if no additional data
    }
    
    uint32_t data_len = extract_length(len_bytes, 0);
    
    // Build full response
    vector<uint8_t> full_response = response_code;
    full_response.insert(full_response.end(), len_bytes.begin(), len_bytes.end());
    
    // Read the actual data if length > 0
    if (data_len > 0) {
      vector<uint8_t> data;
      if (!read_exactly(sd, data, data_len)) {
        cerr << "Error: Failed to read response data" << endl;
        return {};
      }
      full_response.insert(full_response.end(), data.begin(), data.end());
    }
    
    return full_response;
  }
  
  // Unknown response format
  cerr << "Error: Unknown response format" << endl;
  return {};
}

/// req_reg() sends the REG command to register a new user
///
/// @param sd      The open socket descriptor for communicating with the server
/// @param user    The name of the user doing the request
/// @param pass    The password of the user doing the request
void req_reg(int sd, const string &user, const string &pass,
             const string &) {
  // cout << "requests.cc::req_reg() is not implemented\n";
  // NB: These asserts are to prevent compiler warnings
  // assert(sd);
  // assert(user.length() > 0);
  // assert(pass.length() > 0);

  // Validate inputs
  if (user.length() == 0 || user.length() > 32 || pass.length() == 0 || pass.length() > 32) {
    cerr << RES_ERR_REQ_FMT << endl;
    return;
    }
  
  // Send REG_ command - no additional message data for registration
  vector<uint8_t> response = send_cmd(sd, REQ_REG, user, pass, empty_vec);

  // Handle response
  if (response.empty()) {
    cerr << "Error: No response from server" << endl;
    return;
  }

  // Check for OK__
  if (response.size() >= 4 && 
      response[0] == 'O' && response[1] == 'K' && 
      response[2] == '_' && response[3] == '_') {
    cout << RES_OK << endl;
    return;
  }

  // Else error and print the message 
  string error_msg(response.begin(), response.end());
  cout << error_msg << endl;
}

/// req_bye() writes a request for the server to exit.
///
/// @param sd      The open socket descriptor for communicating with the server
/// @param user    The name of the user doing the request
/// @param pass    The password of the user doing the request
void req_bye(int sd, const string &user, const string &pass,
             const string &) {
  //cout << "requests.cc::req_bye() is not implemented\n";
  // NB: These asserts are to prevent compiler warnings
  // assert(sd);
  // assert(user.length() > 0);
  // assert(pass.length() > 0);

  // Validate the input format 
  if (user.empty() || user.length() > 32 || pass.empty() || pass.length() > 32) {
    cerr << RES_ERR_REQ_FMT << endl;
    return;
  } 

  // Send EXIT command - no additional message data
  vector<uint8_t> response = send_cmd(sd, REQ_BYE, user, pass, empty_vec);

  // Handle response
  if (response.empty()) {
    cerr << "Error: No response from server" << endl;
    return;
  }

  // Check for OK__
  if (response.size() >= 4 && 
      response[0] == 'O' && response[1] == 'K' && 
      response[2] == '_' && response[3] == '_') {
    cout << RES_OK << endl;
    return;
  }

  // Else error and print the message 
  string error_msg(response.begin(), response.end());
  cout << error_msg << endl;
}

/// req_sav() writes a request for the server to save its contents
///
/// @param sd      The open socket descriptor for communicating with the server
/// @param user    The name of the user doing the request
/// @param pass    The password of the user doing the request
void req_sav(int sd, const string &user, const string &pass,
             const string &) {
  // cout << "requests.cc::req_sav() is not implemented\n";
  // NB: These asserts are to prevent compiler warnings
  // assert(sd);
  // assert(user.length() > 0);
  // assert(pass.length() > 0);

  // Validate the input format 
  if (user.empty() || user.length() > 32 || pass.empty() || pass.length() > 32) {
    cerr << RES_ERR_REQ_FMT << endl;
    return;
  } 

  // Send SAVE command - no additional message data
  vector<uint8_t> response = send_cmd(sd, REQ_SAV, user, pass, empty_vec);

  // Handle response
  if (response.empty()) {
    cerr << "Error: No response from server" << endl;
    return;
  }
  
  // Check for OK__
  if (response.size() >= 4 && 
      response[0] == 'O' && response[1] == 'K' && 
      response[2] == '_' && response[3] == '_') {
    cout << RES_OK << endl;
    return;
  }

  // Else error and print the message 
  string error_msg(response.begin(), response.end());
  cout << error_msg << endl;
}

/// req_set() sends the SET command to set the content for a user
///
/// @param sd      The open socket descriptor for communicating with the server
/// @param user    The name of the user doing the request
/// @param pass    The password of the user doing the request
/// @param setfile The file whose contents should be sent
void req_set(int sd, const string &user, const string &pass, const string &setfile) {
  // cout << "requests.cc::req_set() is not implemented\n";
  // NB: These asserts are to prevent compiler warnings
  // assert(sd);
  // assert(user.length() > 0);
  // assert(pass.length() > 0);
  // assert(setfile.length() > 0);

  // Validate inputs
  if (user.empty() || user.length() > 32 || pass.empty() || pass.length() > 32) {
    cerr << RES_ERR_REQ_FMT << endl;
    return;
  }
  
  // Read the file contents
  vector<uint8_t> file_contents = load_entire_file(setfile);
  if (file_contents.empty()) {
    cerr << "Error: Failed to read file " << setfile << endl;
    return;
  }
  
  // Check file size (max 1MB)
  if (file_contents.size() > 1048576) {
    cerr << "Error: File too large (max 1MB)" << endl;
    return;
  }
  
  // Build the message: len(@f).@f
  vector<uint8_t> msg;
  add_length(msg, file_contents.size());
  add_vec(msg, file_contents);
  
  // Send SETP command
  vector<uint8_t> response = send_cmd(sd, REQ_SET, user, pass, msg);
  
  // Handle response
  if (response.empty()) {
    cerr << "Error: No response from server" << endl;
    return;
  }
  
  // Check for OK__
  if (response.size() >= 4 && 
      response[0] == 'O' && response[1] == 'K' && 
      response[2] == '_' && response[3] == '_') {
    cout << RES_OK << endl;
    return;
  }
  
  // Else error and print the message 
  string error_msg(response.begin(), response.end());
  cout << error_msg << endl;
}

/// req_get() requests the content associated with a user, and saves it to a
/// file called <user>.file.dat.
///
/// @param sd      The open socket descriptor for communicating with the server
/// @param user    The name of the user doing the request
/// @param pass    The password of the user doing the request
/// @param getname The name of the user whose content should be fetched
void req_get(int sd, const string &user, const string &pass,
             const string &getname) {
  // cout << "requests.cc::req_get() is not implemented\n";
  // NB: These asserts are to prevent compiler warnings
  // assert(sd);
  // assert(user.length() > 0);
  // assert(pass.length() > 0);
  // assert(getname.length() > 0);

   if (user.empty() || user.length() > 32 || pass.empty() || pass.length() > 32 ||
      getname.empty() || getname.length() > 32) {
    cerr << RES_ERR_REQ_FMT << endl;
    return;
  }
  
  // Build the message: len(@w).@w
  vector<uint8_t> msg;
  add_length(msg, getname.length());
  add_string(msg, getname);
  
  // Send GETP command
  vector<uint8_t> response = send_cmd(sd, REQ_GET, user, pass, msg);
  
  // Handle response
  if (response.empty()) {
    cerr << "Error: No response from server" << endl;
    return;
  }
  
  // Check for OK__
  if (response.size() >= 4 && 
      response[0] == 'O' && response[1] == 'K' && 
      response[2] == '_' && response[3] == '_') {
    // Save the result to file
    string filename = getname + ".file.dat";
    send_result_to_file(response, filename);
    cout << RES_OK << endl;
    return;
  }
  
  // Else error and print the message 
  string error_msg(response.begin(), response.end());
  cout << error_msg << endl;
}

/// req_all() sends the ALL command to get a listing of all users, formatted
/// as text with one entry per line.
///
/// @param sd      The open socket descriptor for communicating with the server
/// @param user    The name of the user doing the request
/// @param pass    The password of the user doing the request
/// @param allfile The file where the result should go
void req_all(int sd, const string &user, const string &pass,
             const string &allfile) {
  // cout << "requests.cc::req_all() is not implemented\n";
  // NB: These asserts are to prevent compiler warnings
  // assert(sd);
  // assert(user.length() > 0);
  // assert(pass.length() > 0);
  // assert(allfile.length() > 0);

  // Validate inputs
  if (user.empty() || user.length() > 32 || pass.empty() || pass.length() > 32) {
    cerr << RES_ERR_REQ_FMT << endl;
    return;
  }
  
  // Send ALL_ command - no additional message data
  vector<uint8_t> response = send_cmd(sd, REQ_ALL, user, pass, empty_vec);
  
  // Handle response
  if (response.empty()) {
    cerr << "Error: No response from server" << endl;
    return;
  }
  
  // Check for OK__
  if (response.size() >= 4 && 
      response[0] == 'O' && response[1] == 'K' && 
      response[2] == '_' && response[3] == '_') {
    // Save the result to file
    send_result_to_file(response, allfile);
    cout << RES_OK << endl;
    return;
  }
  
  // Else error and print the message
  string error_msg(response.begin(), response.end());
  cout << error_msg << endl;
}