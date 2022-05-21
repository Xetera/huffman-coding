#include "huffman.hpp"
#include <iostream>

int main() {
  const std::string input_string = "testing!";
  const auto tree = huffman(input_string);

  const auto encoded = tree.encode(input_string);
  const auto decoded = tree.decode(*encoded);

  assert(decoded == input_string);

  std::cout << "Initial bytes: " << input_string.size() << "\n";
  std::cout << "Compressed bytes: " << ceil((float) encoded->bit_length / 8) << "\n";
  return 0;
}
