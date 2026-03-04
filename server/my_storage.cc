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
  MyStorage(const std::string &fname, size_t buckets)
      : auth_table(authtable_factory(buckets)), filename(fname) {}

  virtual ~MyStorage() { 
    delete auth_table;
  }

  result_t auth(const string &user, const string &pass) {
    if (user.empty() || user.length() > LEN_UNAME || 
        pass.empty() || pass.length() > LEN_PASSWORD) {
      // FIX: Append newline to error
      return {false, RES_ERR_REQ_FMT + "\n", {}};
    }
    
    bool authenticated = false;
    auth_table->do_with_readonly(user, [&](const AuthTableEntry &entry) {
      if (entry.password == pass) {
        authenticated = true;
      }
    });

    if (authenticated) {
      return {true, RES_OK, {}};
    }

    // FIX: Append newline to error
    return {false, RES_ERR_LOGIN + "\n", {}};
  }

  virtual result_t add_user(const string &user, const string &pass) {
    if (user.empty() || user.length() > LEN_UNAME || 
        pass.empty() || pass.length() > LEN_PASSWORD) {
      // FIX: Append newline to error
      return {false, RES_ERR_REQ_FMT + "\n", {}};
    }

    AuthTableEntry new_entry;
    new_entry.username = user;
    new_entry.password = pass;
    new_entry.content = vector<uint8_t>(); 

    if (auth_table->insert(user, new_entry)) {
      return {true, RES_OK, {}};
    } else {
      // FIX: Append newline to error
      return {false, RES_ERR_USER_EXISTS + "\n", {}};
    }
  }

  virtual result_t set_user_data(const string &user, const string &pass,
                                 const vector<uint8_t> &content) {
    if (user.empty() || user.length() > LEN_UNAME || 
        pass.empty() || pass.length() > LEN_PASSWORD) {
      // FIX: Append newline to error
      return {false, RES_ERR_REQ_FMT + "\n", {}};
    }

    if (content.size() > LEN_PROFILE_FILE) {
      // FIX: Append newline to error
      return {false, RES_ERR_REQ_FMT + "\n", {}};
    }

    bool success = false;
    bool user_found = auth_table->do_with(user, [&](AuthTableEntry &entry) {
      if (entry.password == pass) {
        entry.content = content;
        success = true;
      }
    });

    if (!user_found || !success) {
      // FIX: Append newline to error
      return {false, RES_ERR_LOGIN + "\n", {}};
    }

    return {true, RES_OK, {}};
  }

  virtual result_t get_user_data(const string &user, const string &pass,
                                 const string &who) {
    if (user.empty() || user.length() > LEN_UNAME || 
        pass.empty() || pass.length() > LEN_PASSWORD ||
        who.empty() || who.length() > LEN_UNAME) {
      // FIX: Append newline to error
      return {false, RES_ERR_REQ_FMT + "\n", {}};
    }
    
    result_t auth_result = auth(user, pass);
    if (!auth_result.succeeded) {
      return auth_result;
    }

    vector<uint8_t> content_copy;
    bool found = false;
    bool has_data = false;

    auth_table->do_with_readonly(who, [&](const AuthTableEntry &entry) {
      found = true;
      if (!entry.content.empty()) {
        content_copy = entry.content;
        has_data = true;
      }
    });

    if (!found) { 
      // FIX: Append newline to error
      return {false, RES_ERR_NO_USER + "\n", {}};
    }

    if (!has_data) { 
      // FIX: Append newline to error
      return {false, RES_ERR_NO_DATA + "\n", {}};
    }

    return {true, RES_OK, content_copy};
  }

  virtual result_t get_all_users(const string &user, const string &pass) {
    if (user.empty() || user.length() > LEN_UNAME || 
        pass.empty() || pass.length() > LEN_PASSWORD) {
      // FIX: Append newline to error
      return {false, RES_ERR_REQ_FMT + "\n", {}};
    }

    result_t auth_result = auth(user, pass);
    if (!auth_result.succeeded) {
      return auth_result;
    }

    string usernames = "";
    auth_table->do_all_readonly([&](const string &username, const AuthTableEntry &entry) {
      usernames += username + "\n";
    });

    vector<uint8_t> result(usernames.begin(), usernames.end());
    return {true, RES_OK, result};
  }

  virtual result_t save_file() {
    string temp_filename = filename + ".tmp";
    FILE *file = fopen(temp_filename.c_str(), "wb");
    if (file == nullptr) {
      return {false, RES_ERR_SERVER + "\n", {}};
    }

    auth_table->do_all_readonly([&](const string &username, const AuthTableEntry &entry) {
      fwrite(AUTHENTRY.c_str(), 1, AUTHENTRY.length(), file);

      uint32_t uname_len = username.length();
      uint32_t pass_len = entry.password.length();
      uint32_t content_len = entry.content.size();

      fwrite(&uname_len, sizeof(uint32_t), 1, file);
      fwrite(&pass_len, sizeof(uint32_t), 1, file);
      fwrite(&content_len, sizeof(uint32_t), 1, file);

      fwrite(username.c_str(), 1, username.length(), file);
      fwrite(entry.password.c_str(), 1, entry.password.length(), file);
      if (content_len > 0) {
        fwrite(entry.content.data(), 1, entry.content.size(), file);
      }

      size_t total_len = AUTHENTRY.length() + 3 * sizeof(uint32_t) + 
                         username.length() + entry.password.length() + 
                         entry.content.size();
      size_t padding = (4 - (total_len % 4)) % 4;

      for (size_t i = 0; i < padding; i++) {
        uint8_t zero = 0;
        fwrite(&zero, 1, 1, file);
      }
    });

    fclose(file);

    if (rename(temp_filename.c_str(), filename.c_str()) != 0) {
      // FIX: Append newline to error
      return {false, RES_ERR_SERVER + "\n", {}};
    }

    return {true, RES_OK, {}};
  }

  virtual result_t load_file() {
    auth_table->clear();
    FILE *file = fopen(filename.c_str(), "rb");
    if (file == nullptr) {
      return {true, "File not found: " + filename, {}};
    }

    while (true) {
      char header[4]; 
      size_t read_count = fread(header, 1, 4, file);
      
      if (read_count == 0) break; 

      if (read_count != 4 || memcmp(header, AUTHENTRY.c_str(), 4) != 0) {
        fclose(file);
        // FIX: Append newline to error
        return {false, RES_ERR_SERVER + "\n", {}}; 
      }

      uint32_t uname_len, pass_len, content_len;
      if (fread(&uname_len, sizeof(uint32_t), 1, file) != 1 ||
          fread(&pass_len, sizeof(uint32_t), 1, file) != 1 ||
          fread(&content_len, sizeof(uint32_t), 1, file) != 1) {
        fclose(file);
        // FIX: Append newline to error
        return {false, RES_ERR_SERVER + "\n", {}};
      }

      if (uname_len > LEN_UNAME || pass_len > LEN_PASSWORD || 
          content_len > LEN_PROFILE_FILE) {
        fclose(file);
        // FIX: Append newline to error
        return {false, RES_ERR_SERVER + "\n", {}};
      }

      vector<char> uname_vec(uname_len);
      vector<char> pass_vec(pass_len);
      vector<uint8_t> content_vec(content_len);

      if (fread(uname_vec.data(), 1, uname_len, file) != uname_len ||
          fread(pass_vec.data(), 1, pass_len, file) != pass_len ||
          (content_len > 0 && fread(content_vec.data(), 1, content_len, file) != content_len)) {
        fclose(file);
        // FIX: Append newline to error
        return {false, RES_ERR_SERVER + "\n", {}};
      }

      string username(uname_vec.begin(), uname_vec.end());
      string password(pass_vec.begin(), pass_vec.end());

      size_t total_read = 4 + (3 * sizeof(uint32_t)) + uname_len + pass_len + content_len;
      size_t padding = (4 - (total_read % 4)) % 4;
      if (padding > 0) {
        if (fseek(file, padding, SEEK_CUR) != 0) {
          fclose(file);
          // FIX: Append newline to error
          return {false, RES_ERR_SERVER + "\n", {}};
        }
      }

      AuthTableEntry entry;
      entry.username = username;
      entry.password = password;
      entry.content = content_vec;

      if (!auth_table->insert(username, entry)) {
        fclose(file);
        // FIX: Append newline to error
        return {false, RES_ERR_SERVER + "\n", {}};
      }
    }

    fclose(file);
    return {true, "Loaded: " + filename, {}};
  }
};

Storage *storage_factory(const std::string &fname, size_t buckets) {
  return new MyStorage(fname, buckets);
}