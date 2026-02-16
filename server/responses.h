#pragma once

#include <openssl/pem.h>
#include <vector>

#include "../common/constants.h"

#include "storage.h"

/// Add the size of a value to a vector as a 4-byte value
///
/// @param res  The vector to add to
/// @param t    The thing whose size should be added
///
/// @tparam T   The type of t
template <class T> void add_size(vector<uint8_t> &res, T t);

/// Add the contents of an iterable to a vector
///
/// @param res  The vector to add to
/// @param t    The thing to add
///
/// @tparam T   The type of t
template <class T> void add_it(vector<uint8_t> &res, T t);

/// Concatenate a string and a vector of content, in a format that can be sent
/// to the client as a single message.  Most often, this involves a message of
/// RES_OK and content that was returned from a hash table.
///
/// @param msg     A string message to send to the client
/// @param content A vector of content to send to the client
///
/// @return a vector with the correct concatenation of msg and content
vector<uint8_t> build_res(const string &msg, const vector<uint8_t> &content);

/// Send a message format error
///
/// @param sd  The socket onto which the result should be written
///
/// @return false, to indicate that the server shouldn't stop
bool send_err_msg_format(int sd);

/// Extract a string from a vector
///
/// @param it     An iterator to the extraction point
/// @param count  The number of characters to extract
/// @return The extracted string
string extract_string(vector<uint8_t>::const_iterator &it,
                             size_t count);

/// Extract a size (uint32_t) from a vector
/// @param it An iterator to the extraction point
/// @return The extracted uint32_t
uint32_t extract_size(vector<uint8_t>::const_iterator &it);

/// Extract a vector from a vector
/// @param it     An iterator to the extraction point
/// @param count  The number of bytes to extract
/// @return The extracted vector
vector<uint8_t> extract_vec(vector<uint8_t>::const_iterator &it,
                                   size_t count);

/// Respond to an ALL command by generating a list of all the usernames in the
/// Auth table and returning them, one per line.
///
/// @param sd      The socket onto which the result should be written
/// @param storage The Storage object, which contains the auth table
/// @param u       The user name associated with the request
/// @param p       The password associated with the request
/// @param req     The unencrypted contents of the request
///
/// @return false, to indicate that the server shouldn't stop
bool handle_all(int sd, Storage *storage,
                const std::string &u, const std::string &p,
                const std::vector<uint8_t> &req);

/// Respond to a SET command by putting the provided data into the Auth table
///
/// @param sd      The socket onto which the result should be written
/// @param storage The Storage object, which contains the auth table
/// @param u       The user name associated with the request
/// @param p       The password associated with the request
/// @param req     The unencrypted contents of the request
///
/// @return false, to indicate that the server shouldn't stop
bool handle_set(int sd, Storage *storage,
                const std::string &u, const std::string &p,
                const std::vector<uint8_t> &req);

/// Respond to a GET command by getting the data for a user
///
/// @param sd      The socket onto which the result should be written
/// @param storage The Storage object, which contains the auth table
/// @param u       The user name associated with the request
/// @param p       The password associated with the request
/// @param req     The unencrypted contents of the request
///
/// @return false, to indicate that the server shouldn't stop
bool handle_get(int sd, Storage *storage,
                const std::string &u, const std::string &p,
                const std::vector<uint8_t> &req);

/// Respond to a REG command by trying to add a new user
///
/// @param sd      The socket onto which the result should be written
/// @param storage The Storage object, which contains the auth table
/// @param u       The user name associated with the request
/// @param p       The password associated with the request
/// @param req     The unencrypted contents of the request
///
/// @return false, to indicate that the server shouldn't stop
bool handle_reg(int sd, Storage *storage,
                const std::string &u, const std::string &p,
                const std::vector<uint8_t> &req);

/// Respond to a BYE command by returning false, but only if the user
/// authenticates
///
/// @param sd      The socket onto which the result should be written
/// @param storage The Storage object, which contains the auth table
/// @param u       The user name associated with the request
/// @param p       The password associated with the request
/// @param req     The unencrypted contents of the request
///
/// @return true, to indicate that the server should stop, or false on an error
bool handle_bye(int sd, Storage *storage,
                const std::string &u, const std::string &p,
                const std::vector<uint8_t> &req);

/// Respond to a SAV command by persisting the file, but only if the user
/// authenticates
///
/// @param sd      The socket onto which the result should be written
/// @param storage The Storage object, which contains the auth table
/// @param u       The user name associated with the request
/// @param p       The password associated with the request
/// @param req     The unencrypted contents of the request
///
/// @return false, to indicate that the server shouldn't stop
bool handle_sav(int sd, Storage *storage,
                const std::string &u, const std::string &p,
                const std::vector<uint8_t> &req);