#include <string_view>
#include <bitset>
#include <queue>
#include <functional>
#include <unordered_map>
#include <optional>

#pragma once

class huffman_node {
public:
  // leaf
  huffman_node(size_t frequency, uint8_t character);

  // internal node
  huffman_node(size_t frequency, huffman_node *, huffman_node *);

  size_t frequency;
  std::optional<uint8_t> character;
  huffman_node *left;
  huffman_node *right;

  friend bool operator>(const huffman_node &a, const huffman_node &b) {
    return a.frequency > b.frequency;
  }
};

// std::priority_queue sorts by largest first, this needs to be by smallest for our use case
using internal_queue = std::priority_queue<huffman_node *, std::vector<huffman_node *>, std::greater<>>;

/**
 * A huffman encoded 8 bit array of data with a fixed bit length
 */
struct huffman_encoding {
  std::unique_ptr<uint8_t> data;
  size_t bit_length;
};

/**
 * Storing the mappings between characters and their bit representations
 * {
 *   "a": 010,
 *   "b": 001,
 *   "c": 100
 * }
 */
using bit_mapping = std::unordered_map<uint8_t, std::unique_ptr<huffman_encoding>>;

class huffman {
public:
  explicit huffman(const std::string_view &);

  [[nodiscard]]
  std::unique_ptr<huffman_encoding> encode(const std::string_view &) const;

  [[nodiscard]]
  std::string decode(const huffman_encoding &) const;

private:
  huffman_node *tree;

  [[nodiscard]]
  static huffman_node *build_tree(const std::string_view &);
};

void fill_mapping(
  const huffman_node *br,
  std::vector<bool> &bits,
  bit_mapping &mappings
);
