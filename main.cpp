#include <cstdio>
#include <iostream>
#include <openssl/sha.h>
#include <random>
#include <string>
int main() {
  std::cout << "Hello World" << "\n";
  std::string id = "TEST";
  uint8_t hash[32];
  SHA256((unsigned char *)id.data(), id.size(), hash);
  printf("Hash: ");
  for (int i = 0; i < 32; i++) {
    printf("%x", hash[i]);
  }
  printf("\n");
}
