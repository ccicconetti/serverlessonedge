#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

using Matrix = std::vector<std::vector<double>>;

std::ostream& operator<<(std::ostream&              aStream,
                         const std::vector<double>& aVector) {
  for (auto i = 0u; i < aVector.size(); i++) {
    aStream << aVector[i] << ' ';
  }
  return aStream;
}

std::ostream& operator<<(std::ostream& aStream, const Matrix& aMatrix) {
  for (auto i = 0u; i < aMatrix.size(); i++) {
    aStream << aMatrix[i] << '\n';
  }
  return aStream;
}

double latency(const double lambda, const double D) {
  // assert(lambda * D > 0.0);
  // assert(lambda * D < 1.0);
  return 0.5 * D * (2.0 - lambda * D) / (1.0 - lambda * D);
}

void normalize(Matrix& aMatrix) {
  for (auto i = 0u; i < aMatrix.size(); i++) {
    double mySum = 0;
    for (auto j = 0u; j < aMatrix[i].size(); j++) {
      mySum += aMatrix[i][j];
    }
    assert(mySum != 0);
    for (auto j = 0u; j < aMatrix[i].size(); j++) {
      aMatrix[i][j] /= mySum;
    }
  }
}

void fillLatencies(const std::vector<double>& aLoads,
                   const std::vector<double>& aCapacities,
                   Matrix&                    aMatrix,
                   std::vector<double>&       aLatencies) {
  for (auto j = 0u; j < aLatencies.size(); j++) {
    double myLoad = 0;
    for (auto i = 0u; i < aLoads.size(); i++) {
      myLoad += aMatrix[i][j] * aLoads[i];
    }
    assert(myLoad > 0.0);
    aLatencies[j] = latency(myLoad, aCapacities[j]);
  }
}

void solve(const std::vector<double>& aLoads,
           const std::vector<double>& aCapacities,
           Matrix&                    aMatrix) {
  assert(not aMatrix.empty());
  assert(aLoads.size() == aMatrix.size());
  for (auto i = 0u; i < aMatrix.size(); i++) {
    assert(aCapacities.size() == aMatrix[i].size());
  }

  std::vector<double> myLatencies(aCapacities.size());

  //
  // make all latencies finite
  //

  for (auto myStep = 0u;; myStep++) {
    fillLatencies(aLoads, aCapacities, aMatrix, myLatencies);

    auto myMaxIt = std::max_element(myLatencies.begin(), myLatencies.end());
    assert(myMaxIt != myLatencies.end());
    if (std::isfinite(*myMaxIt)) {
      break;
    }
    auto myMaxNdx = std::distance(myLatencies.begin(), myMaxIt);

    auto myCandMax = 0.0;
    auto myCandNdx = 0u;
    for (auto i = 0u; i < aLoads.size(); i++) {
      if (aLoads[i] * aMatrix[i][myMaxNdx] > myCandMax) {
        myCandMax = aLoads[i] * aMatrix[i][myMaxNdx];
        myCandNdx = i;
      }
    }
    assert(myCandMax > 0);

    assert(aMatrix[myCandNdx][myMaxNdx] > 0);
    aMatrix[myCandNdx][myMaxNdx] *= 0.95;
    auto mySum = 0.0;
    for (auto j = 0u; j < myLatencies.size(); j++) {
      mySum += aMatrix[myCandNdx][j];
    }
    assert(mySum > 0);
    for (auto j = 0u; j < myLatencies.size(); j++) {
      aMatrix[myCandNdx][j] /= mySum;
    }
  }

  std::cout << "latencies:\n" << myLatencies << "\n";
  std::cout << "intermediate:\n" << aMatrix << "-----\n";

  //
  // solve the system iteratively
  //

  std::ofstream myWeightsFile("weights.dat");
  std::ofstream myLatenciesFile("latencies.dat");

  Matrix myRatios(aMatrix);

  for (auto myStep = 0u; myStep < 100000; myStep++) {
    fillLatencies(aLoads, aCapacities, aMatrix, myLatencies);

    for (auto i = 0u; i < aLoads.size(); i++) {
      for (auto j = 0u; j < myLatencies.size(); j++) {
        assert(myLatencies[j] > 0);
        assert(std::isfinite(myLatencies[j]));
        myRatios[i][j] = aMatrix[i][j] / myLatencies[j];
      }
    }

    // find the pair of flows from the same node that have maximum difference
    // between the ratios of their probabilities to the latencies
    auto myDelta    = 0.0;
    auto myCandLoad = -1;
    auto myCandMin  = -1;
    auto myCandMax  = -1;
    for (auto i = 0u; i < aLoads.size(); i++) {
      auto myMinNdx = -1;
      auto myMaxNdx = -1;
      auto myMinVal = 0.0;
      auto myMaxVal = 0.0;
      for (auto j = 0u; j < myLatencies.size(); j++) {
        if (myRatios[i][j] == 0) {
          continue;
        }
        if (myMinNdx < 0 or myRatios[i][j] < myMinVal) {
          myMinNdx = j;
          myMinVal = myRatios[i][j];
        }
        if (myMaxNdx < 0 or myRatios[i][j] > myMaxVal) {
          myMaxNdx = j;
          myMaxVal = myRatios[i][j];
        }
      }

      if (myMinNdx == myMaxNdx) {
        continue;
      }

      assert(myMaxVal >= myMinVal);

      auto myCurDelta = myMaxVal - myMinVal;

      if (myCandLoad < 0 or myCurDelta > myDelta) {
        myCandLoad = i;
        myCandMin  = myMinNdx;
        myCandMax  = myMaxNdx;
        myDelta    = myCurDelta;
      }
    }

    std::cout << myCandLoad << ' ' << myCandMin << ' ' << myCandMax << ' '
              << myDelta << std::endl;

    std::cout << "ratios[" << myStep << "]:\n" << myRatios;

    // transfer some probability from max to min

    assert(myCandLoad > -1);
    assert(myCandLoad < (int)aLoads.size());
    assert(myCandMin > -1);
    assert(myCandMin < (int)aMatrix[myCandLoad].size());
    assert(myCandMax > -1);
    assert(myCandMax < (int)aMatrix[myCandLoad].size());
    /*
    auto myTransfer =
        fabs(aMatrix[myCandLoad][myCandMin] - aMatrix[myCandLoad][myCandMax]) /
        2.0;
    myTransfer = std::min(std::min(aMatrix[myCandLoad][myCandMin],
                                   aMatrix[myCandLoad][myCandMax]) /
                              2.0,
                          myTransfer);
                          */
    auto myTransfer = std::min(aMatrix[myCandLoad][myCandMin],
                               aMatrix[myCandLoad][myCandMax]) /
                      100.0;
    if (myTransfer < 1e-6) {
      myTransfer = std::min(aMatrix[myCandLoad][myCandMin],
                            aMatrix[myCandLoad][myCandMax]);
    }
    aMatrix[myCandLoad][myCandMin] -= myTransfer;
    aMatrix[myCandLoad][myCandMax] += myTransfer;
    assert(aMatrix[myCandLoad][myCandMin] >= 0);
    assert(aMatrix[myCandLoad][myCandMin] <= 1);
    assert(aMatrix[myCandLoad][myCandMax] >= 0);
    assert(aMatrix[myCandLoad][myCandMax] <= 1);

    // print stuff out
    fillLatencies(aLoads, aCapacities, aMatrix, myLatencies);

    std::cout << "latencies[" << myStep << "]:\n" << myLatencies << "\n";
    std::cout << "intermediate[" << myStep << "]:\n" << aMatrix << "-----\n";

    for (auto i = 0u; i < aLoads.size(); i++) {
      for (auto j = 0u; j < myLatencies.size(); j++) {
        myWeightsFile << aMatrix[i][j] << ' ';
      }
    }
    myWeightsFile << '\n';

    for (auto j = 0u; j < myLatencies.size(); j++) {
      myLatenciesFile << myLatencies[j] << ' ';
    }
    myLatenciesFile << '\n';
  }
}

int main() {
  Matrix              myInput{{1, 1, 0}, {0, 1, 1}};
  std::vector<double> myLoads{1, 1};
  std::vector<double> myCapacities{1, 1, 0.5};

  std::cout << "input:\n" << myInput << "-----\n";

  normalize(myInput);

  std::cout << "normalized input:\n" << myInput << "-----\n";

  solve(myLoads, myCapacities, myInput);

  return EXIT_SUCCESS;
}
