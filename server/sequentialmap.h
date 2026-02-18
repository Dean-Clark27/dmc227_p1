#include <functional>
#include <mutex>
#include <string>
#include <vector>
#include <iostream>

#include "map.h"

/// SequentialMap is a sequential implementation of the Map interface (a
/// Key/Value store).  This map has O(n) complexity.  It's just for p1.
///
/// @tparam K The type of the keys in this map
/// @tparam V The type of the values in this map
template <typename K, typename V> class SequentialMap : public Map<K, V> {
  /// The key/value pairs, as a vector
  ///
  /// NB: This is a very bad choice of data structure, but it's OK for p1
  std::vector<std::pair<K, V>> entries;

public:
  /// Construct by specifying the number of buckets it should have
  ///
  /// @param _buckets (unused) The number of buckets
  SequentialMap(size_t) {}

  /// Destruct the SequentialMap
  virtual ~SequentialMap() {}

  /// Clear the map
  virtual void clear() { 
    entries.clear();
  }

  /// Insert the provided key/value pair only if there is no mapping for the key
  /// yet.
  ///
  /// @param key        The key to insert
  /// @param val        The value to insert
  ///
  /// @return true if the key/value was inserted, false if the key already
  ///         existed in the table
  virtual bool insert(K key, V val) {
    // std::cout << "sequentialmap.h::insert() is not implemented\n";
    // return false;
    
    // Check if they key already exists
    for (const auto &entry: entries){
      if(entry.first == key){
        return false; // Key exists, don't insert it
      }
    }

    // Key doesn't exist, insert it
    entries.push_back({key, val});
    return true;
  }

  /// Insert the provided key/value pair if there is no mapping for the key yet.
  /// If there is a key, then update the mapping by replacing the old value with
  /// the provided value
  ///
  /// @param key    The key to upsert
  /// @param val    The value to upsert
  ///
  /// @return true if the key/value was inserted, false if the key already
  ///         existed in the table and was thus updated instead
  virtual bool upsert(K key, V val) {
    // std::cout << "sequentialmap.h::upsert() is not implemented\n";
    // return false;
  
    // Search for existing key in map
    for (auto &entry: entries){
      if(entry.first == key){
        // Key exists, update
        entry.second = val;
        return false; // False return indicates update, not insert
      }
    }

    // Key doesn't exist, therefore we can insert
    entries.push_back({key, val});
    return true: // True return indicates insert
  }

  /// Apply a function to the value associated with a given key.  The function
  /// is allowed to modify the value.
  ///
  /// @param key The key whose value will be modified
  /// @param f   The function to apply to the key's value
  ///
  /// @return true if the key existed and the function was applied, false
  ///         otherwise
  virtual bool do_with(K key, std::function<void(V &)> f) {
    // std::cout << "sequentialmap.h::do_with() is not implemented\n";
    // return false;
  
    // Linear search for key
    for (auto &entry: entries){
      if (entry.first == key){
        // Key found, apply function to value
        f(entry.second);
        return true;
      }
    }

    // Key not found
    return false;
  }

  /// Apply a function to the value associated with a given key. The function
  /// is not allowed to modify the value.
  ///
  /// @param key The key whose value will be modified
  /// @param f   The function to apply to the key's value
  ///
  /// @return true if the key existed and the function was applied, false
  ///         otherwise
  virtual bool do_with_readonly(K key, std::function<void(const V &)> f) {
    // std::cout << "sequentialmap.h::do_with_readonly() is not implemented\n";
    // return false;
  
    // Search for key
    for (auto &entry: entries){
      if (entry.first == key){
        // Key found, apply function but read-only
        f(entry.second);
        return true;
      }
    }

    // Key not found
    return false;
  }

  /// Remove the mapping from a key to its value
  ///
  /// @param key        The key whose mapping should be removed
  ///
  /// @return true if the key was found and the value unmapped, false otherwise
  virtual bool remove(K key) {
    // std::cout << "sequentialmap.h::remove() is not implemented\n";
    // return false;
  
    // Find first element that matches the key
    // Using std::remove_if with erase idiom  
    auto it = std::find_if(entries.begin(), entries.end(),
                          [&key](const std::pair<K, V> &entry) {
                            return entry.first == key;
                          });

    if (it != entries.end()){
      // Key found, remove it
      entries.erase(it);
    }

    // Key not found
    return false; 
  }

  /// Apply a function to every key/value pair in the map.  Note that the
  /// function is not allowed to modify keys or values.
  ///
  /// @param f    The function to apply to each key/value pair
  /// @param then A function to run when this is done, but before unlocking...
  ///             useful for 2pl
  virtual void do_all_readonly(std::function<void(const K, const V &)> f) {
    // std::cout << "sequentialmap.h::do_all_readonly() is not implemented\n";
    //(void)f;
  
    // Apply function to each entry
    for (const auto &entry : entries) {
      f(entry.first, entry.second);
    }
  }
};