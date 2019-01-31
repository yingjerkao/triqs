#include <cmath>
#define TRIQS_ARRAYS_ENFORCE_BOUNDCHECK
#include <triqs/gfs.hpp>
#include <triqs/test_tools/arrays.hpp>
#include <triqs/statistics/accumulator.hpp>
#include <triqs/statistics/jackknife.hpp>
#include <boost/version.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/normal_distribution.hpp>
using namespace triqs::stat;
using namespace triqs;
using namespace triqs::gfs;
using namespace boost;
using namespace triqs::utility;

int seed = 1567;
triqs::mpi::communicator world;

// FIXME : remove boost generator for std:: ones

// test the mecanics of the jackknife
TEST(Stat, Jack1) {

  int bin_size   = 1;
  int n_log_bins = 0;
  int Nbins      = 10;
  int Nsample    = 10; // should be N in this test

  accumulator<double> a(0.0, n_log_bins, Nbins, bin_size);
  accumulator<double> b(0.0, n_log_bins, Nbins, bin_size);

  {
    boost::variate_generator<boost::mt19937, boost::normal_distribution<>> generator((boost::mt19937(seed)), (boost::normal_distribution<>()));
    for (long i = 0; i < Nsample; ++i) {
      a << generator();
      b << generator();
    }
  }

  auto [x, xn, d] = jackknife(world, [](auto &&a, auto &&b) { return a / b; }, a, b);

  // redo the manual computation
  std::vector<double> A(Nsample, 0), B(Nsample, 0);
  {
    boost::variate_generator<boost::mt19937, boost::normal_distribution<>> generator((boost::mt19937(seed)), (boost::normal_distribution<>()));
    for (long i = 0; i < Nsample; ++i) {
      A[i] = generator();
      B[i] = generator();
    }
  }
  auto mj = [](auto &&v) {
    int N    = v.size();
    auto res = v;
    for (long i = 0; i < N; ++i) {
      double s = 0;
      for (long j = 0; j < N; ++j)
        if (i != j) s += v[j];
      res[i] = s / (N - 1);
    }
    return res;
  };
  auto jack_a = mj(A), jack_b = mj(B);

  auto Ba = a.linear_bins();
  auto Bb = b.linear_bins();
  triqs::stat::details::jackknifed_t<std::vector<double>> JJ{Ba, world};

  auto N = Nsample;

  double precision = 1.e-13;
  for (long i = 0; i < N; ++i) { EXPECT_NEAR(Ba[i], A[i], precision); }
  for (long i = 0; i < N; ++i) { EXPECT_NEAR(JJ[i], jack_a[i], precision); }

  //
  double X = 0, Xn = 0, D = 0;
  for (long i = 0; i < N; ++i) { X += jack_a[i] / jack_b[i]; }
  for (long i = 0; i < N; ++i) { Xn += A[i] / B[i]; }
  X /= N;
  Xn /= N;
  for (long i = 0; i < N; ++i) {
    double tmp = (X - jack_a[i] / jack_b[i]);
    D += tmp * tmp;
  }
  D /= N;
  D = sqrt((N - 1) * D);
  X = N * Xn - (N - 1) * X;

  EXPECT_NEAR(x, X, precision);
  EXPECT_NEAR(xn, Xn, precision);
  EXPECT_NEAR(d, D, precision);

  // for a linear function, the two average must be the same

  auto [av1, av2, avv] = jackknife(world, [](auto &&x) { return x; }, a);
  EXPECT_NEAR(av1, av2, precision);

}

//------------------------------------------------------

TEST(Binned, array) {

  int bin_size   = 1;
  int Nbins      = 10;
  int n_log_bins = 0;

  using A = array<double, 2>;

  A a0{{1, 2}, {2, 1}};
  auto zero = a0;
  zero      = 0;

  accumulator<double> b1{0.0, n_log_bins, Nbins, bin_size};
  accumulator<double> b2{0.0, n_log_bins, Nbins, bin_size};
  auto c1 = b1, c2 = b2;

  accumulator<A> b{zero, n_log_bins, Nbins, bin_size};
  auto c = b;

  {
    boost::variate_generator<boost::mt19937, boost::normal_distribution<>> generator((boost::mt19937(seed)), (boost::normal_distribution<>()));
    for (long i = 0; i < 1000; ++i) {
      b1 << generator();
      b2 << generator();
      c1 << generator();
      c2 << generator();
    }
  }

  {
    boost::variate_generator<boost::mt19937, boost::normal_distribution<>> generator((boost::mt19937(seed)), (boost::normal_distribution<>()));
    for (long i = 0; i < 1000; ++i) {
      auto x1 = generator();
      auto y1 = generator();
      auto x2 = generator();
      auto y2 = generator();
      b << A{{x1, y1}, {y1, x1}};
      c << A{{x2, y2}, {y2, x2}};
    }
  }
  // checks the bins are ok.
  for (auto [x1, x2, a] : triqs::utility::zip(b1.linear_bins(), b2.linear_bins(), b.linear_bins())) {
    EXPECT_NEAR(x1, a(0, 0), 1.e-15);
    EXPECT_NEAR(x2, a(0, 1), 1.e-15);
    EXPECT_NEAR(x1, a(1, 1), 1.e-15);
    EXPECT_NEAR(x2, a(1, 0), 1.e-15);
  }

  auto a_b = [](auto &&a, auto &&b) { return a / b; };
  //
  auto [x1, xn1, d1] = jackknife(world, a_b, b1, c1);
  auto [x2, xn2, d2] = jackknife(world, a_b, b2, c2);

  auto [x, xn, xd] = jackknife(world, a_b, b, c);

  std::cerr << x1 << std::endl;
  std::cerr << xn1 << std::endl;
  std::cerr << d1 << std::endl;

  std::cerr << x << std::endl;
  std::cerr << xn << std::endl;
  std::cerr << xd << std::endl;

  EXPECT_NEAR(x1, x(0, 0), 1.e-15);
  EXPECT_NEAR(x2, x(0, 1), 1.e-15);
  EXPECT_NEAR(xn1, xn(0, 0), 1.e-15);
  EXPECT_NEAR(xn2, xn(0, 1), 1.e-15);
  EXPECT_NEAR(d1, xd(0, 0), 1.e-15);
  EXPECT_NEAR(d2, xd(1, 0), 1.e-15);
}

//------------------------------------------------------

TEST(Binned, Gf) {

  int bin_size   = 1;
  int Nbins      = 10;
  int n_log_bins = 0;

  using g_t = gf<imfreq, tensor_valued<2>>;

  auto zero = g_t{{1, Fermion, 5}, {2, 2}};
  auto work = zero;
  zero      = 0;

  accumulator<std::complex<double>> b{0, n_log_bins, Nbins, bin_size};
  accumulator<double> Z{0, n_log_bins, Nbins, bin_size};
  accumulator<g_t> g{zero, n_log_bins, Nbins, bin_size};

  {
    boost::variate_generator<boost::mt19937, boost::normal_distribution<>> generator((boost::mt19937(seed)), (boost::normal_distribution<>()));
    for (long i = 0; i < Nbins; ++i) {
      auto tmp      = generator();
      work[0]       = array<double, 2>{{generator(), generator()}, {generator(), generator()}}; // put garbage to test array product
      work[0](0, 0) = tmp;
      g << work;
      work = 0;
      b << tmp;
      Z << generator();
    }
  }

  auto gg = g.linear_bins();
  auto bb = b.linear_bins();

  // checks the bins are ok.
  for (auto [x, y] : triqs::utility::zip(b.linear_bins(), g.linear_bins())) { EXPECT_COMPLEX_NEAR(x, y[0](0, 0), 1.e-15); }

  auto a_b = [](auto &&a, auto &&b) { return a / b; };
  //
  auto [x1, xn1, d1]  = jackknife(world, a_b, b, Z);
  auto [g1, gn1, gd1] = jackknife(world, a_b, g, Z);

  EXPECT_COMPLEX_NEAR(x1, g1[0](0, 0), 1.e-15);
  EXPECT_COMPLEX_NEAR(xn1, gn1[0](0, 0), 1.e-15);
  EXPECT_COMPLEX_NEAR(d1, gd1[0](0, 0), 1.e-15);
}
MAKE_MAIN;
