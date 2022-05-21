#include "huffman.hpp"

int main() {
  const std::string input_string = "testing!";
  const auto tree = huffman(input_string);

  const auto encoded = tree.encode(input_string);
  const auto decoded = tree.decode(*encoded);

  assert(decoded == input_string);
  return 0;
}
