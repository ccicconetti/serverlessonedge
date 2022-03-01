/*
              __ __ __
             |__|__|  | __
             |  |  |  ||__|
  ___ ___ __ |  |  |  |
 |   |   |  ||  |  |  |    Ubiquitous Internet @ IIT-CNR
 |   |   |  ||  |  |  |    C++ edge computing libraries and tools
 |_______|__||__|__|__|    https://github.com/ccicconetti/serverlessonedge

Licensed under the MIT License <http://opensource.org/licenses/MIT>
Copyright (c) 2022 C. Cicconetti <https://ccicconetti.github.io/>

Permission is hereby  granted, free of charge, to any  person obtaining a copy
of this software and associated  documentation files (the "Software"), to deal
in the Software  without restriction, including without  limitation the rights
to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "gtest/gtest.h"

// #include "Support/parallelbatch.h"

#include <ortools/base/logging.h>
#include <ortools/linear_solver/linear_solver.h>

#include <vector>

namespace uiiit {
namespace lambdamusim {

struct TestOrTools : public ::testing::Test {};

TEST_F(TestOrTools, test_assignment_example) {
  // Data
  const std::vector<std::vector<double>> costs{
      {90, 80, 75, 70},
      {35, 85, 55, 65},
      {125, 95, 90, 95},
      {45, 110, 95, 115},
      {50, 100, 90, 100},
  };
  const int num_workers = costs.size();
  const int num_tasks   = costs[0].size();

  // Solver
  // Create the mip solver with the SCIP backend.
  std::unique_ptr<operations_research::MPSolver> solver(
      operations_research::MPSolver::CreateSolver("SCIP"));
  if (!solver) {
    LOG(WARNING) << "SCIP solver unavailable.";
    return;
  }

  // Variables
  // x[i][j] is an array of 0-1 variables, which will be 1
  // if worker i is assigned to task j.
  std::vector<std::vector<const operations_research::MPVariable*>> x(
      num_workers,
      std::vector<const operations_research::MPVariable*>(num_tasks));
  for (int i = 0; i < num_workers; ++i) {
    for (int j = 0; j < num_tasks; ++j) {
      x[i][j] = solver->MakeIntVar(0, 1, "");
    }
  }

  // Constraints
  // Each worker is assigned to at most one task.
  for (int i = 0; i < num_workers; ++i) {
    operations_research::LinearExpr worker_sum;
    for (int j = 0; j < num_tasks; ++j) {
      worker_sum += x[i][j];
    }
    solver->MakeRowConstraint(worker_sum <= 1.0);
  }
  // Each task is assigned to exactly one worker.
  for (int j = 0; j < num_tasks; ++j) {
    operations_research::LinearExpr task_sum;
    for (int i = 0; i < num_workers; ++i) {
      task_sum += x[i][j];
    }
    solver->MakeRowConstraint(task_sum == 1.0);
  }

  // Objective.
  operations_research::MPObjective* const objective =
      solver->MutableObjective();
  for (int i = 0; i < num_workers; ++i) {
    for (int j = 0; j < num_tasks; ++j) {
      objective->SetCoefficient(x[i][j], costs[i][j]);
    }
  }
  objective->SetMinimization();

  // Solve
  const operations_research::MPSolver::ResultStatus result_status =
      solver->Solve();

  // Print solution.
  // Check that the problem has a feasible solution.
  if (result_status != operations_research::MPSolver::OPTIMAL &&
      result_status != operations_research::MPSolver::FEASIBLE) {
    LOG(FATAL) << "No solution found.";
  }

  LOG(INFO) << "Total cost = " << objective->Value() << "\n\n";

  for (int i = 0; i < num_workers; ++i) {
    for (int j = 0; j < num_tasks; ++j) {
      // Test if x[i][j] is 0 or 1 (with tolerance for floating point
      // arithmetic).
      if (x[i][j]->solution_value() > 0.5) {
        LOG(INFO) << "Worker " << i << " assigned to task " << j
                  << ".  Cost = " << costs[i][j];
      }
    }
  }
}

} // namespace lambdamusim
} // namespace uiiit
