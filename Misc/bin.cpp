#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

using namespace std;

//#define DB_MEAN

int main(int argc, char* argv[]) {
  auto bin_size = 1.0f;
  if (argc == 2) {
    bin_size = atof(argv[1]);
  }

  vector<float>  bin;
  vector<size_t> num;
  while (not cin.eof()) {
    float time, power;
    std::cin >> time >> power;
    const auto ndx = static_cast<size_t>(roundf(time / bin_size));
    if (ndx >= bin.size()) {
      bin.resize(ndx + 1);
      num.resize(ndx + 1);
    }
#ifdef DB_MEAN
    bin[ndx] += pow10(power / 10);
#else
    bin[ndx] += power;
#endif
    num[ndx]++;
  }
  for (size_t i = 0; i < bin.size(); i++) {
    if (num[i] > 0) {
      const auto myMean = bin[i] / num[i];
#ifdef DB_MEAN
      cout << bin_size * i << ' ' << 10 * log10(myMean) << '\n';
#else
      cout << bin_size * i << ' ' << myMean << '\n';
#endif
    }
  }

  return EXIT_SUCCESS;
}
