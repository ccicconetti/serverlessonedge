/**
 * Compile with:
 *
 * g++ -o time time.cc -O2 -std=c++11
 *
 * Example output:
 *
 * Elapsed: 1.09332 s, expected: 1 s
 */

#include <chrono>
#include <iostream>
#include <thread>

int main() {
  const auto myNumRuns = 1000;

  const auto myStart = std::chrono::high_resolution_clock::now();

  for (auto i = 0; i < myNumRuns; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  const auto myEnd = std::chrono::high_resolution_clock::now();

  const auto myElapsed =
      std::chrono::duration_cast<std::chrono::duration<double>>(myEnd -
                                                                myStart);

  std::cout << "Elapsed: " << myElapsed.count()
            << " s, expected: " << myNumRuns / 1000 << " s" << std::endl;
}
