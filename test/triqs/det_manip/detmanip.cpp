#include <triqs/test_tools/arrays.hpp>
#include <triqs/det_manip/det_manip.hpp>
#include <random>
#include <triqs/arrays/linalg/det_and_inverse.hpp>
#include <triqs/arrays/asserts.hpp>
#include <iostream>

struct fun {

  using result_type   = double;
  using argument_type = double;

  double operator()(double x, double y) const {
    const double pi   = acos(-1);
    const double beta = 10.0;
    const double epsi = 0.1;
    double tau        = x - y;
    bool s            = (tau > 0);
    tau               = (s ? tau : beta + tau);
    double r          = epsi + tau / beta * (1 - 2 * epsi);
    return -2 * (pi / beta) / std::sin(pi * r);
  }
};

using d_t = triqs::det_manip::det_manip<fun>;

//=================================================================
// ---------- A simple "Fixture" for det manip test ---------

class DetTest : public ::testing::Test {

  const double precision = 1.e-6;

  protected:
  void SetUp() override {}

  // --------------------
  // Reset X, Y as random vectors of size N
  void reset_XY(int N) {

    const int seed = 23432;
    gen            = std::mt19937(seed);
    dis            = std::uniform_real_distribution<>(0.0, 10.0);

    X.resize(N, 0);
    Y.resize(N, 0);
    for (int n = 0; n < N; ++n) {
      X[n] = dis(gen);
      Y[n] = dis(gen);
    }
  }
  // void TearDown() override {}

  //---------------------
  // Check that d is consistent with rebuilding a det from X and Y.
  // throws if not ok.
  void check(bool do_complete, d_t &d, double detratio, std::vector<double> const &X, std::vector<double> const &Y) {

    auto d2   = d_t{fun{}, X, Y};
    auto det1 = d.determinant();
    if (std::abs(det1) < 1.e-5) {
      std::cerr << " Det Too small: not completed";
      return;
    }

    typename d_t::value_type det;
    if (do_complete) {
      d.complete_operation();
      det = d.determinant();
    } else
      det = det1 * detratio;

    auto det2 = d2.determinant();

    if (std::abs(detratio - det / det1) > precision) TRIQS_RUNTIME_ERROR << "detratio incorrect : " << detratio << "  " << det / det1;
    if (std::abs(det - det2) > precision) TRIQS_RUNTIME_ERROR << "Det != d2 : " << det << "  " << det2;

    if (do_complete) {
      auto det_check = 1 / determinant(d.inverse_matrix());
      if (std::abs(det - det_check) > precision) TRIQS_RUNTIME_ERROR << "Det != det_check : " << det << "  " << det_check;
      triqs::arrays::assert_all_close(make_matrix(inverse(d.matrix())), d.inverse_matrix(), precision, true);
    }
  }

  // -------- data ----------
  std::mt19937 gen; //
  std::uniform_real_distribution<> dis;
  std::vector<double> X, Y;
};

//=================================================================

// ------------ change_col_row -------------

TEST_F(DetTest, ChangeRowCol) {

  std::cerr << "N = ";
  for (int N = 1; N < 9; N++) {
    std::cerr << N << " ";
    for (int i0 = 0; i0 < N; i0++)
      for (int j0 = 0; j0 < N; j0++) {
        //std::cerr << "------------------------------\n i0 = " << i0 << "j0 = " << j0 << std::endl;

        // reset X, Y at proper dimensions
        reset_XY(N);

        // Make a det with X and Y
        auto d = d_t{fun{}, X, Y};

        // Pick up the x, and y , i0, j0  : position of the change
        auto x = dis(gen), y = dis(gen);
        X[i0] = x;
        Y[j0] = y;

        // the operation to check
        auto detratio = d.try_change_col_row(i0, j0, x, y);

        // check
        check(true, d, detratio, X, Y);
      }
  }
  std::cerr << std::endl;
}

// ------------ change_col_row -------------

TEST_F(DetTest, ChangeRowColOnly) {

  std::cerr << "N = ";
  for (int N = 1; N < 9; N++) {
    std::cerr << N << " ";
    for (int i0 = N - 1; i0 >= 0; i0--)
      for (int j0 = N - 1; j0 >= 0; j0--) {
        //std::cerr << "------------------------------\n i0 = " << i0 << "j0 = " << j0 << std::endl;

        // reset X, Y at proper dimensions
        reset_XY(N);

        // Make a det with X and Y
        auto d = d_t{fun{}, X, Y};

        // Pick up the x, and y , i0, j0  : position of the change
        auto x = dis(gen), y = dis(gen);
        X[i0] = x;
        Y[j0] = y;

        // the operation to check
        auto detratio = d.onlytry_change_col_row(i0, j0, x, y);

        // check
        check(false, d, detratio, X, Y);
      }
  }
  std::cerr << std::endl;
}

// ------------ change_2col_2row -------------

TEST_F(DetTest, Change2Row2Col) {

  std::cerr << "N = ";
  for (int N = 2; N < 9; N++) {
    std::cerr << N << " ";
    for (int i0 = 0; i0 < N; i0++)
      for (int j0 = 0; j0 < N; j0++)
        for (int i1 = 0; i1 < N; i1++)
          for (int j1 = 0; j1 < N; j1++) {
            //TRIQS_PRINT (i0); TRIQS_PRINT (i1); TRIQS_PRINT (j0); TRIQS_PRINT (j1);
            // we don't want i(j)1 to be equal to i(j)0
            if ((i1 == i0) || (j1 == j0)) continue;

            // reset X, Y at proper dimensions
            reset_XY(N);

            // Make a det with X and Y
            auto d = d_t{fun{}, X, Y};

            // Pick up the x, and y , i0, j0  : position of the change
            auto x0 = dis(gen), y0 = dis(gen);
            X[i0] = x0;
            Y[j0] = y0;
            // Same for i1 and j1
            auto x1 = dis(gen), y1 = dis(gen);
            X[i1] = x1;
            Y[j1] = y1;

            // the operation to check
            auto detratio = d.onlytry_change_2cols_2rows(std::vector<int>{i0, i1}, std::vector<int>{j0, j1}, std::vector<double>{x0, x1},
                                                         std::vector<double>{y0, y1});

            // check
            check(false, d, detratio, X, Y);
          }
  }
  std::cerr << std::endl;
}

// ------------ change_col_row_insert -------------

TEST_F(DetTest, ChangeRowColInsert) {

  std::cerr << "N = ";
  for (int N = 1; N < 9; N++) {
    std::cerr << N << " ";
    for (int i0 = 0; i0 < N; i0++)
      for (int j0 = 0; j0 < N; j0++) {
        //std::cerr << "------------------------------\n i0 = " << i0 << "j0 = " << j0 << std::endl;

        // reset X, Y at proper dimensions
        reset_XY(N);

        // Make a det with X and Y
        auto d = d_t{fun{}, X, Y};

        // Pick up the x, and y , i0, j0  : position of the change
        auto x0 = dis(gen), y0 = dis(gen);
        X[i0] = x0;
        Y[j0] = y0;

        // Same for i1 and j1
        auto x1 = dis(gen), y1 = dis(gen);
        X.push_back(x1);
        Y.push_back(y1);

        // the operation to check
        auto detratio = d.onlytry_change_col_row_insert(i0, j0, x0, y0, x1, y1);

        // check
        check(false, d, detratio, X, Y);
      }
  }
  std::cerr << std::endl;
}

// ------------ change_col_row_remove -------------

TEST_F(DetTest, ChangeRowColRemove) {

  std::cerr << "N = ";
  for (int N = 2; N < 9; N++) {
    std::cerr << N << " ";
    for (int i0 = 0; i0 < N; i0++)
      for (int j0 = 0; j0 < N; j0++)
        for (int i1 = 0; i1 < N; i1++)
          for (int j1 = 0; j1 < N; j1++) {
            //std::cerr << "------------------------------\n i0 = " << i0 << "j0 = " << j0 << std::endl;

            if ((i1 == i0) || (j1 == j0)) continue;

            // reset X, Y at proper dimensions
            reset_XY(N);

            // Make a det with X and Y
            auto d = d_t{fun{}, X, Y};

            // Pick up the x, and y , i0, j0  : position of the change
            auto x = dis(gen), y = dis(gen);
            X[i0] = x;
            Y[j0] = y;

            X.erase(begin(X) + i1);
            Y.erase(begin(Y) + j1);

            // the operation to check
            auto detratio = d.onlytry_change_col_row_remove(i0, j0, x, y, i1, j1);

            // check
            check(false, d, detratio, X, Y);
          }
  }
  std::cerr << std::endl;
}
MAKE_MAIN;
