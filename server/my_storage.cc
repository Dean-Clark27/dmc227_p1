#include <cassert>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

#include "../common/constants.h"
#include "../common/contextmanager.h"
#include "../common/err.h"

#include "authtableentry.h"
#include "map.h"
#include "map_factories.h"
#include "storage.h"

using namespace std;

/// MyStorage is the student implementation of the Storage class
class MyStorage : public Storage {
  /// The map of authentication information, indexed by username
  Map<string, AuthTableEntry> *auth_table;

  /// The name of the file from which the Storage object was loaded, and to
  /// which we persist the Storage object when save() is invoked
  const string filename;

public:
  /// Construct an empty object and specify the file from which it should be
  /// loaded.  To avoid exceptions and errors in the constructor, the act of
  /// loading data is separate from construction.
  ///
  /// @param fname   The name of the file to use for persistence
  /// @param buckets The number of buckets in the hash table
  MyStorage(const std::string &fname, size_t buckets)
      : auth_table(authtable_factory(buckets)), filename(fname) {}

  /// Destructor for the storage object.
  virtual ~MyStorage() { 
    // For the destructor, we need to clean auth_table
    delete auth_table;
  }

  /// Authenticate a user
  ///
  /// @param user The name of the user who made the request
  /// @param pass The password for the user, used to authenticate
  ///
  /// @return A result tuple, as described in storage.h
  result_t auth(const string &user, const string &pass) {
    // cout << "my_storage.cc::auth() is not implemented\n";
    // NB: These asserts are to prevent compiler warnings
    // assert(user.length() > 0);
    // assert(pass.length() > 0);
    // return {false, RES_ERR_UNIMPLEMENTED, {}};
  
    // Validate input lengths
    if (user.empty() || user.length() > LEN_UNAME || 
        pass.empty() || pass.length() > LEN_PASSWORD) {
      return {false, RES_ERR_REQ_FMT, {}};
        }
    // Attempt to find user, and if so, check password
    bool authenticated = false;
    auth_table->do_with_readonly(user, [&](const AuthTableEntry &entry) {
      if (entry.password == pass) {
        authenticated = true;
      }
    });

    if (authenticated) {
      return {true, RES_OK, {}};
    }

    // Either user doesn't exist or password is the same
    // (Returns same err)
    return {false, RES_ERR_LOGIN, {}};
  }

  /// Create a new entry in the Auth table.  If the user already exists, return
  /// an error.  Otherwise, save an entry with the username, password, and a zero-byte content.
  ///
  /// @param user The user name to register
  /// @param pass The password to associate with that user name
  ///
  /// @return A result tuple, as described in storage.h
  virtual result_t add_user(const string &user, const string &pass) {
    // cout << "my_storage.cc::add_user() is not implemented\n";
    // NB: These asserts are to prevent compiler warnings
    // assert(user.length() > 0);
    // assert(pass.length() > 0);
    // return {false, RES_ERR_UNIMPLEMENTED, {}};
    
    // Validate input lengths
    if (user.empty() || user.length() > LEN_UNAME || 
        pass.empty() || pass.length() > LEN_PASSWORD) {
      return {false, RES_ERR_REQ_FMT, {}};
    }

    // Create new entry with empty content
    AuthTableEntry new_entry;
    new_entry.username = user;
    new_entry.password = pass;
    new_entry.content = vector<uint8_t>(); // Empty vector 

    // Try to insert to auth_table
    // True if successful, false if key exists
    if (auth_table->insert(user, new_entry)) {
      return {true, RES_OK, {}};
    } else {
      return {false, RES_ERR_USER_EXISTS, {}};
    }
  }

  /// Set the data bytes for a user, but do so if and only if the password
  /// matches
  ///
  /// @param user    The name of the user whose content is being set
  /// @param pass    The password for the user, used to authenticate
  /// @param content The data to set for this user
  ///
  /// @return A result tuple, as described in storage.h
  virtual result_t set_user_data(const string &user, const string &pass,
                                 const vector<uint8_t> &content) {
    // cout << "my_storage.cc::set_user_data() is not implemented\n";
    // NB: These asserts are to prevent compiler warnings
    // assert(user.length() > 0);
    // assert(pass.length() > 0);
    // assert(content.size() > 0);
    // return {false, RES_ERR_UNIMPLEMENTED, {}};
  
    // Validate input lengths
    if (user.empty() || user.length() > LEN_UNAME || 
        pass.empty() || pass.length() > LEN_PASSWORD) {
      return {false, RES_ERR_REQ_FMT, {}};
    }

    // Check content size (max 1MB)
    if (content.size() > LEN_PROFILE_FILE) {
      return {false, RES_ERR_REQ_FMT, {}};
    }

    // Try to find user and update content if password matches
    bool success = false;
    bool user_found = auth_table->do_with(user, [&](AuthTableEntry &entry) {
      if (entry.password == pass) {
        entry.content = content;
        success = true;
      }
    });

    if (!user_found) {
      return {false, RES_ERR_LOGIN, {}};
    }

    if (!success) {
      return {false, RES_ERR_LOGIN, {}};
    }

    return {true, RES_OK, {}};
  }

  /// Return a copy of the user data for a user, but do so only if the password
  /// matches
  ///
  /// @param user The name of the user who made the request
  /// @param pass The password for the user, used to authenticate
  /// @param who  The name of the user whose content is being fetched
  ///
  /// @return A result tuple, as described in storage.h.  Note that "no data" is
  ///         an error
  virtual result_t get_user_data(const string &user, const string &pass,
                                 const string &who) {
    // cout << "my_storage.cc::get_user_data() is not implemented\n";
    // NB: These asserts are to prevent compiler warnings
    // assert(user.length() > 0);
    // assert(pass.length() > 0);
    // assert(who.length() > 0);
    // return {false, RES_ERR_UNIMPLEMENTED, {}};
  
    // Validate input lengths
    if (user.empty() || user.length() > LEN_UNAME || 
        pass.empty() || pass.length() > LEN_PASSWORD ||
        who.empty() || who.length() > LEN_UNAME) {
      return {false, RES_ERR_REQ_FMT, {}};
    }
    
    // Authenticate the requesting user
    result_t auth_result = auth(user, pass);
    if (!auth_result.succeeded) {
      return auth_result;
    }

    // Get requested user's content
    vector<uint8_t> content_copy;
    bool found = false;
    bool has_data = false;

    // Use read-only to retrieve content
    auth_table->do_with_readonly(who, [&](const AuthTableEntry &entry) {
      found = true;
      if (!entry.content.empty()) {
        content_copy = entry.content;
        has_data = true;
      }
    });

    if (!found) { // No user found 
      return {false, RES_ERR_NO_USER, {}};
    }

    if (!has_data) { // No data found
      return {false, RES_ERR_NO_DATA, {}};
    }

    // Returns data as intended
    return {true, RES_OK, content_copy};
  }


  /// Return a newline-delimited string containing all of the usernames in the
  /// auth table
  ///
  /// @param user The name of the user who made the request
  /// @param pass The password for the user, used to authenticate
  ///
  /// @return A result tuple, as described in storage.h
  virtual result_t get_all_users(const string &user, const string &pass) {
    // cout << "my_storage.cc::get_all_users() is not implemented\n";
    // NB: These asserts are to prevent compiler warnings
    // assert(user.length() > 0);
    // assert(pass.length() > 0);
    // return {false, RES_ERR_UNIMPLEMENTED, {}};
  
    // Validate input lengths
    if (user.empty() || user.length() > LEN_UNAME || 
        pass.empty() || pass.length() > LEN_PASSWORD) {
      return {false, RES_ERR_REQ_FMT, {}};
    }

    // Authenticate requesting user
    result_t auth_result = auth(user, pass);
    if (!auth_result.succeeded) {
      return auth_result;
    }

    // Build Newline-delimited list of usernames
    string usernames = "";
    bool first = true;

    // Call read-only because we don't want to be able to change
    // Other users' information, as a singular user shouldn't have 
    // Elevated privelages
    auth_table->do_all_readonly([&](const string &username, const AuthTableEntry &entry) {
      if (!first) {
        usernames += "\n";
      }
      usernames += username;
      first = false;
    });

    // Convert string to vector<uint8_t>
    vector<uint8_t> result(usernames.begin(), usernames.end());

    return {true, RES_OK, result};
  }


  /// Write the entire Storage object to the file specified by this.filename. To
  /// ensure durability, Storage must be persisted in two steps.  First, it must
  /// be written to a temporary file (this.filename.tmp).  Then the temporary
  /// file can be renamed to replace the older version of the Storage object.
  ///
  /// @return A result tuple, as described in storage.h
  virtual result_t save_file() {
  // Create temporary filename
  string temp_filename = filename + ".tmp";

  // Open temporary file for writing (binary mode)
  FILE *file = fopen(temp_filename.c_str(), "wb");
  if (file == nullptr) {
    return {false, RES_ERR_SERVER, {}};
  }

  // Write all entries to the file
  // File format for each entry:
  // - Header: "AUTH" (4 bytes)
  // - Lengths: len(username), len(password), len(content) (3 x 4 bytes)
  // - Contents: username, password, content
  // - Padding: up to 3 zeros to make entry length divisible by 4
  auth_table->do_all_readonly([&](const string &username, const AuthTableEntry &entry) {
    // Write header "AUTH"
    fwrite(AUTHENTRY.c_str(), 1, AUTHENTRY.length(), file);

    // Write lengths (little-endian 32-bit integers)
    uint32_t uname_len = username.length();
    uint32_t pass_len = entry.password.length();
    uint32_t content_len = entry.content.size();

    fwrite(&uname_len, sizeof(uint32_t), 1, file);
    fwrite(&pass_len, sizeof(uint32_t), 1, file);
    fwrite(&content_len, sizeof(uint32_t), 1, file);

    // Write contents
    fwrite(username.c_str(), 1, username.length(), file);
    fwrite(entry.password.c_str(), 1, entry.password.length(), file);
    if (content_len > 0) {
      fwrite(entry.content.data(), 1, entry.content.size(), file);
    }

    // Calculate padding needed (to make total length divisible by 4)
    size_t total_len = AUTHENTRY.length() + 3 * sizeof(uint32_t) + 
                       username.length() + entry.password.length() + 
                       entry.content.size();
    size_t padding = (4 - (total_len % 4)) % 4;

    // Write padding zeros
    for (size_t i = 0; i < padding; i++) {
      uint8_t zero = 0;
      fwrite(&zero, 1, 1, file);
    }
  });

  // Close the file
  fclose(file);

  // Rename temp file to actual file (atomic operation on most systems)
  if (rename(temp_filename.c_str(), filename.c_str()) != 0) {
    return {false, RES_ERR_SERVER, {}};
  }

  return {true, RES_OK, {}};
  }

  /// Populate the Storage object by loading this.filename.  Note that load()
  /// begins by clearing the maps, so that when the call is complete, exactly
  /// and only the contents of the file are in the Storage object.
  ///
  /// @return A result tuple, as described in storage.h.  Note that a
  ///         non-existent file is not an error.
  virtual result_t load_file() {
    // Clear the auth table first
    auth_table->clear();

    // Try to open the file
    FILE *file = fopen(filename.c_str(), "rb");
    if (file == nullptr) {
      // Non-existent file is not an error
      return {true, "File not found: " + filename, {}};
    }

    // Read entries until end of file
    while (!feof(file)) {
      // Read header (4 bytes: "AUTH")
      char header[5] = {0};
      size_t read_count = fread(header, 1, 4, file);
      if (read_count == 0) {
        break; // End of file
      }
      if (read_count != 4 || string(header) != AUTHENTRY) {
        fclose(file);
        return {false, RES_ERR_SERVER, {}};
      }

      // Read lengths (3 x 4 bytes)
      uint32_t uname_len, pass_len, content_len;
      if (fread(&uname_len, sizeof(uint32_t), 1, file) != 1 ||
          fread(&pass_len, sizeof(uint32_t), 1, file) != 1 ||
          fread(&content_len, sizeof(uint32_t), 1, file) != 1) {
        fclose(file);
        return {false, RES_ERR_SERVER, {}};
      }

      // Validate lengths
      if (uname_len > LEN_UNAME || pass_len > LEN_PASSWORD || 
          content_len > LEN_PROFILE_FILE) {
        fclose(file);
        return {false, RES_ERR_SERVER, {}};
      }

      // Read username
      vector<char> username_buf(uname_len + 1, 0);
      if (fread(username_buf.data(), 1, uname_len, file) != uname_len) {
        fclose(file);
        return {false, RES_ERR_SERVER, {}};
      }
      string username(username_buf.data(), uname_len);

      // Read password
      vector<char> password_buf(pass_len + 1, 0);
      if (fread(password_buf.data(), 1, pass_len, file) != pass_len) {
        fclose(file);
        return {false, RES_ERR_SERVER, {}};
      }
      string password(password_buf.data(), pass_len);

      // Read content
      vector<uint8_t> content;
      if (content_len > 0) {
        content.resize(content_len);
        if (fread(content.data(), 1, content_len, file) != content_len) {
          fclose(file);
          return {false, RES_ERR_SERVER, {}};
        }
      }

      // Calculate and skip padding
      size_t total_len = AUTHENTRY.length() + 3 * sizeof(uint32_t) + 
                         uname_len + pass_len + content_len;
      size_t padding = (4 - (total_len % 4)) % 4;
      if (padding > 0) {
        fseek(file, padding, SEEK_CUR);
      }

      // Create entry and insert into auth_table
      AuthTableEntry entry;
      entry.username = username;
      entry.password = password;
      entry.content = content;

      auth_table->insert(username, entry);
    }

    fclose(file);
    return {true, RES_OK, {}};
  }
};

/// Create an empty Storage object and specify the file from which it should be
/// loaded.  To avoid exceptions and errors in the constructor, the act of
/// loading data is separate from construction.
///
/// @param fname   The name of the file to use for persistence
/// @param buckets The number of buckets in the hash table
Storage *storage_factory(const std::string &fname, size_t buckets) {
  return new MyStorage(fname, buckets);
}