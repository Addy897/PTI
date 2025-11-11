#include <iostream>
#include <random>
#include <string>
int main() {
  std::cout << "Hello World" << "\n";
  std::string id;
  std::random_device rd;
  std::mt19937 g(rd());
  std::uniform_int_distribution<int> dist(100000, 999999);
  id = std::to_string(dist(g));
  std::cout << id << "\n";
}
