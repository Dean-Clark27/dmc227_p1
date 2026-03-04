#pragma once

#include <string>
#include <vector>
#include <cstdint>

/// AuthTableEntry represents one user stored in the authentication table
struct AuthTableEntry {
  std::string username;           // The name of the user
  std::string password;           // The password of the user
  std::vector<uint8_t> content;   // The user's content
};
