#include <unordered_map>
#include <vector>
#include <optional>
#include <string>
#include "huffman.hpp"

huffman::huffman(const std::string_view &input) :
  tree(huffman::build_tree(input)) {
}

huffman_node::huffman_node(size_t frequency, uint8_t character)
  : frequency(frequency), character(character),
    left(nullptr), right(nullptr) {}

huffman_node::huffman_node(size_t frequency, huffman_node *left, huffman_node *right)
  : frequency(frequency), character(std::nullopt),
    left(left), right(right) {}

huffman_node *huffman::build_tree(const std::string_view &input) {
  std::unordered_map<char, size_t> frequencies;

  for (const auto &ch: input) {
    frequencies[ch]++;
  }

  std::vector<huffman_node *> items;
  items.reserve(frequencies.size());

  for (const auto &[ch, freq]: frequencies) {
    items.emplace_back(new huffman_node{freq, (uint8_t) ch});
  }

  internal_queue queue(std::greater<>(), std::move(items));

  if (queue.empty()) {
    throw std::runtime_error("Queue cannot be empty");
  }

  while (queue.size() > 1) {
    auto left = queue.top();
    queue.pop();

    auto right = queue.top();
    queue.pop();

    queue.emplace(new huffman_node{
      // should be safe to access here
      left->frequency + right->frequency,
      left,
      right
    });
  }
  return queue.top();
}

std::string huffman::decode(const huffman_encoding &encoding) const {
  std::string out{};
  auto root = tree;
  auto branch = root;

  for (size_t i = 0; i < encoding.bit_length; i++) {
    const size_t chunk = floor(i / 8);
    const auto byte = encoding.data.get()[chunk];
    const auto bit = (byte >> (7 - (i % 8))) & 0b1;
    const auto next_branch = bit == 1 ? branch->right : branch->left;

    if (next_branch->character.has_value()) {
      out += next_branch->character.value();
      branch = root;
    } else {
      branch = next_branch;
    }
  }
  return out;
}

inline int read_bit(uint8_t const *data, size_t offset) {
  const size_t chunk = floor(offset / 8);
  const auto byte = data[chunk];
  return (byte >> (7 - (offset % 8))) & 0b1;
}

inline void write_bit(uint8_t *data, size_t offset) {
  size_t chunk = floor(offset / 8);
  data[chunk] |= (1UL << (7 - (offset % 8)));
}

inline void clear_bit(uint8_t *data, size_t offset) {
  size_t chunk = floor(offset / 8);
  data[chunk] &= ~(1UL << (7 - (offset % 8)));
}

/**
 * Turns a {true, false, true} vector into a uint8_t* with bits that are padded to fill 8 bits with 0s.
 */
std::unique_ptr<huffman_encoding> to_padded_bits(const std::vector<bool> &input) {
  const auto bit_size = input.size();
  const size_t byte_size = ceil((float) bit_size / 8);
  auto output = std::make_unique<uint8_t[]>(byte_size);

  for (size_t i = 0; i < bit_size; i++) {
    auto set = input[i];
    if (set) {
      write_bit(output.get(), i);
    } else {
      clear_bit(output.get(), i);
    }
  }

  return std::make_unique<huffman_encoding>(huffman_encoding{std::move(output), bit_size});
}

void explore_branch(
  const huffman_node *hn,
  std::vector<bool> &bits,
  bit_mapping &mappings,
  bool bit_to_set
) {
  if (hn->character.has_value()) {
    const auto key = hn->character.value();
    bits.emplace_back(bit_to_set);
    mappings[key] = to_padded_bits(bits);
    bits.pop_back();
  } else {
    const auto next_branch = hn;
    bits.emplace_back(bit_to_set);
    fill_mapping(next_branch, bits, mappings);
    bits.pop_back();
  }
}

void fill_mapping(const huffman_node *br, std::vector<bool> &bits, bit_mapping &mappings) {
  explore_branch(br->left, bits, mappings, false);
  explore_branch(br->right, bits, mappings, true);
}

//void bitcpy(uint8_t *destination, uint8_t const *src, size_t dest_offset, size_t bit_length) {
//  const size_t remaining_bits = bit_length % 8;
////  const size_t src_byte_length = floor((float) bit_length / 8);
////  if (src_byte_length > 0) {
////    memcpy(destination + dest_offset, src, src_byte_length);
////  }
//
//  const size_t dest_byte_offset = floor((float) dest_offset / 8);
//
//  const uint8_t mask = ((0xFFu << remaining_bits) - 1);
//  const uint8_t cleared = *(destination + dest_byte_offset) & (~mask);
//  const uint8_t snippet = *src & mask;
//  *(destination + dest_byte_offset) = cleared | snippet;
//}


bit_mapping infer_bits(huffman_node *hn) {
  std::vector<bool> bits{};
  bit_mapping mappings;
  fill_mapping(hn, bits, mappings);
  return mappings;
}

std::unique_ptr<huffman_encoding> huffman::encode(const std::string_view &input) const {
  const auto mappings = infer_bits(tree);
  size_t bit_size = 0;

  for (const auto &ch: input) {
    const auto &encoding = mappings.at(ch);
    bit_size += encoding->bit_length;
  }

  const size_t byte_size = ceil((float) bit_size / 8);
  auto bits = std::make_unique<uint8_t[]>(byte_size);

  const size_t string_size = input.length();
  size_t offset = 0;

  for (size_t i = 0; i < string_size; i++) {
    const uint8_t character = input[i];
    const auto &encoding = mappings.at(character);
    // there's a better way to copy these bits, but it's quite complex
    // bitcpy(bits, encoding->data, offset, encoding->bit_length);
    // offset += encoding->bit_length;
    for (size_t k = 0; k < encoding->bit_length; k++) {
      const auto bit = read_bit(encoding->data.get(), k);
      if (bit) {
        write_bit(bits.get(), offset++);
      } else {
        clear_bit(bits.get(), offset++);
      }
    }
  }

  return std::make_unique<huffman_encoding>(huffman_encoding{std::move(bits), bit_size});
}
