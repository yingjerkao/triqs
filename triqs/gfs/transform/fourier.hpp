/*******************************************************************************
 *
 * TRIQS: a Toolbox for Research in Interacting Quantum Systems
 *
 * Copyright (C) 2011-2017 by M. Ferrero, O. Parcollet
 * Copyright (C) 2018- by Simons Foundation
 *               authors : O. Parcollet, N. Wentzell
 *
 * TRIQS is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * TRIQS is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * TRIQS. If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/
#pragma once
#include "./../gf/flatten.hpp"
#include <triqs/utility/tuple_tools.hpp>

namespace triqs::gfs {

  using namespace triqs::arrays;

  /*------------------------------------------------------------------------------------------------------
  *                                  Mesh calculator
  *-----------------------------------------------------------------------------------------------------*/

  // general form. NB : options MUST have defaults (for traits later)
  //gf_mesh<Y> make_adjoint_mesh( gf_mesh<X> const &, options ...);

  // FIXME : DOC
  inline gf_mesh<imfreq> make_adjoint_mesh(gf_mesh<imtime> const &m, int n_iw = -1) {
    if (n_iw == -1) n_iw = (m.size() - 1) / 6;
    return {m.domain(), n_iw};
  }

  // FIXME : DOC
  inline gf_mesh<imtime> make_adjoint_mesh(gf_mesh<imfreq> const &m, int n_tau = -1) {
    if (n_tau == -1) n_tau = 6 * (m.last_index() + 1) + 1;
    return {m.domain(), n_tau};
  }

  // FIXME : DOC
  inline gf_mesh<refreq> make_adjoint_mesh(gf_mesh<retime> const &m, bool shift_half_bin = false) {
    int L       = m.size();
    double wmax = M_PI * (L - 1) / (L * m.delta());
    if (shift_half_bin) return {-wmax + M_PI / L / m.delta(), wmax + M_PI / L / m.delta(), L};
    return {-wmax, wmax, L};
  }

  inline gf_mesh<retime> make_adjoint_mesh(gf_mesh<refreq> const &m, bool shift_half_bin = false) {
    int L       = m.size();
    double tmax = M_PI * (L - 1) / (L * m.delta());
    if (shift_half_bin) return {-tmax + M_PI / L / m.delta(), tmax + M_PI / L / m.delta(), L};
    return {-tmax, tmax, L};
  }

  // FIXME : DOC
  inline gf_mesh<brillouin_zone> make_adjoint_mesh(gf_mesh<cyclic_lattice> const &m) { return {m.domain(), m.periodization_matrix}; }

  // FIXME : DOC
  inline gf_mesh<cyclic_lattice> make_adjoint_mesh(gf_mesh<brillouin_zone> const &m) { return {m.domain(), m.periodization_matrix}; }

  // trait for error messages later
  template <typename V> using _mesh_fourier_image = typename decltype(make_adjoint_mesh(gf_mesh<V>()))::var_t;

  /*------------------------------------------------------------------------------------------------------
            Implementation
  *-----------------------------------------------------------------------------------------------------*/

  template <typename V> using gf_vec_t   = gf<V, tensor_valued<1>>;
  template <typename V> using gf_vec_vt  = gf_view<V, tensor_valued<1>>;
  template <typename V> using gf_vec_cvt = gf_const_view<V, tensor_valued<1>>;

  // matsubara
  gf_vec_t<imfreq> _fourier_impl(gf_mesh<imfreq> const &iw_mesh, gf_vec_cvt<imtime> gt, array_const_view<dcomplex, 2> mom_123 = {});
  gf_vec_t<imtime> _fourier_impl(gf_mesh<imtime> const &tau_mesh, gf_vec_cvt<imfreq> gw, array_const_view<dcomplex, 2> mom_123 = {});

  // real
  gf_vec_t<refreq> _fourier_impl(gf_mesh<refreq> const &w_mesh, gf_vec_cvt<retime> gt, array_const_view<dcomplex, 2> mom_12 = {});
  gf_vec_t<retime> _fourier_impl(gf_mesh<retime> const &t_mesh, gf_vec_cvt<refreq> gw, array_const_view<dcomplex, 2> mom_12 = {});

  // lattice
  gf_vec_t<cyclic_lattice> _fourier_impl(gf_mesh<cyclic_lattice> const &r_mesh, gf_vec_cvt<brillouin_zone> gk);
  gf_vec_t<brillouin_zone> _fourier_impl(gf_mesh<brillouin_zone> const &k_mesh, gf_vec_cvt<cyclic_lattice> gr);

  /*------------------------------------------------------------------------------------------------------
   *
   * The general Fourier function
   * gin : input
   * gout : output
   * opt_args : e.g. moments. Must be flatten_2d
   *
   *-----------------------------------------------------------------------------------------------------*/

  // this function just regroups the green function data, and calls the vector_valued gf core implementation
  template <int N, typename V1, typename V2, typename T1, typename T2, typename... OptArgs>
  void _fourier(gf_const_view<V1, T1> gin, gf_view<V2, T2> gout, OptArgs const &... opt_args) {

    static_assert(std::is_same<typename T1::complex_t, T2>::value, "Incompatible target types for fourier transform");

    auto const &out_mesh = std::get<N>(gout.mesh());

    auto gout_flatten = _fourier_impl(out_mesh, flatten_gf_2d<N>(gin), flatten_2d(make_const_view(opt_args), 0)...);
    auto _            = ellipsis();
    if constexpr (gin.data_rank == 1)
      gout.data() = gout_flatten.data()(_, 0); // gout is scalar, gout_flatten vectorial
    else {
      // inverse operation as flatten_2d, exactly
      auto g_rot = rotate_index_view(gout.data(), N);
      auto a_0   = g_rot(0, _);
      for (auto const &mp : out_mesh) {
        auto g_rot_sl = g_rot(mp.linear_index(), _); // if the array is long, it is faster to precompute the view ...
        auto gout_col = gout_flatten.data()(mp.linear_index(), _);
        assign_foreach(g_rot_sl, [&gout_col, c = 0ll](auto &&... i) mutable { return gout_col(c++); });
      }
    }
  }

  /* *-----------------------------------------------------------------------------------------------------
   *
   * make_gf_from_fourier (g, mesh, options)  -> fourier_transform of g
   *
   * *-----------------------------------------------------------------------------------------------------*/

  // FIXME DOC
  // split : No N for 1 mesh ...
  template <int N = 0, typename V1, typename V2, typename T, typename... OptArgs>
  auto make_gf_from_fourier(gf_const_view<V1, T> gin, gf_mesh<V2> const &mesh, OptArgs const &... opt_args) {
    static_assert(N >= 0 && N < get_n_variables<V1>::value, "Mesh index exceeds Gf Mesh Rank");
    static_assert(get_n_variables<V2>::value == 1, "Cannot fourier transform on cartesian product mesh");

    if constexpr (get_n_variables<V1>::value == 1) { // === single mesh
      static_assert(get_n_variables<V2>::value == 1, "Incompatible mesh ranks");
      static_assert(N == 0, "Fourier transforming gf with mesh of rank 1 but fourier index N > 1");
      static_assert(std::is_same_v<V2, _mesh_fourier_image<V1>>, "There is no Fourier transform between these two meshes");
      auto gout = gf<V2, T>{mesh, gin.target_shape()};
      _fourier<N>(gin, gout(), opt_args...);
      return gout;
    } else { // === cartesian_product mesh
      static_assert(std::is_same_v<V2, _mesh_fourier_image<std::tuple_element_t<N, typename V1::type>>>,
                    "There is no Fourier transform between these two meshes");
      auto mesh_tpl = triqs::tuple::replace<N>(gin.mesh().components(), mesh);
      auto out_mesh = gf_mesh{mesh_tpl};
      using var_t   = typename std::decay_t<decltype(out_mesh)>::var_t;
      auto gout     = gf<var_t, T>{out_mesh, gin.target_shape()};
      _fourier<N>(gin, gout(), opt_args...);
      return gout;
    }
  }

  /* *-----------------------------------------------------------------------------------------------------
   *
   * make_gf_from_fourier : Specialized makers for different meshes
   *
   * *-----------------------------------------------------------------------------------------------------*/

  template <int N = 0, typename T> auto make_gf_from_fourier(gf_const_view<brillouin_zone, T> gin) {
    return make_gf_from_fourier(gin, make_adjoint_mesh(gin.mesh()));
  }

  template <int N = 0, typename T> auto make_gf_from_fourier(gf_const_view<cyclic_lattice, T> gin) {
    return make_gf_from_fourier(gin, make_adjoint_mesh(gin.mesh()));
  }

  template <int N = 0, typename T> gf<imtime, T> make_gf_from_fourier(gf_const_view<imfreq, T> gin, int n_tau = -1) {
    return make_gf_from_fourier(gin, make_adjoint_mesh(gin.mesh(), n_tau));
  }

  template <int N = 0, typename T> gf<imfreq, T> make_gf_from_fourier(gf_const_view<imtime, T> gin, int n_iw = -1) {
    return make_gf_from_fourier(gin, make_adjoint_mesh(gin.mesh(), n_iw));
  }

  template <int N = 0, typename T> gf<retime, T> make_gf_from_fourier(gf_const_view<refreq, T> gin, bool shift_half_bin = false) {
    return make_gf_from_fourier(gin, make_adjoint_mesh(gin.mesh(), shift_half_bin));
  }

  template <int N = 0, typename T> gf<refreq, T> make_gf_from_fourier(gf_const_view<retime, T> gin, bool shift_half_bin = false) {
    return make_gf_from_fourier(gin, make_adjoint_mesh(gin.mesh(), shift_half_bin));
  }

  /* *-----------------------------------------------------------------------------------------------------
   *
   * make_gf_from_fourier : Fourier transform multiple meshes
   *
   * *-----------------------------------------------------------------------------------------------------*/

  // FIXME : a generic implementation
  template <int N> struct _fou_wk {};
  template <int N, typename G> auto operator&(G &&gin, _fou_wk<N> const &) {
    return make_gf_from_fourier<N>(gin, make_adjoint_mesh(std::get<N>(gin.mesh())));
  }

  template <int... Ns, typename V, typename T> auto make_gf_from_multi_fourier(gf_const_view<V, T> gin) { return (gin() & ... & _fou_wk<Ns>{}); }

  template <int N1, int N2, typename M1, typename M2, typename... Vs, typename T>
  auto make_gf_from_fourier(gf_const_view<cartesian_product<Vs...>, T> gin, M1 &&m1, M2 &&m2) {
    static_assert(sizeof...(Vs) >= 2, "Green function mesh rank incompatible with mesh indices");

    auto g1 = make_gf_from_fourier<N1>(gin(), std::forward<M1>(m1));
    auto g2 = make_gf_from_fourier<N2>(g1(), std::forward<M2>(m2));

    return std::move(g2);
  }

  template <int N1, int N2, int N3, typename M1, typename M2, typename M3, typename... Vs, typename T>
  auto make_gf_from_fourier(gf_const_view<cartesian_product<Vs...>, T> gin, M1 &&m1, M2 &&m2, M3 &&m3) {
    static_assert(sizeof...(Vs) >= 3, "Green function mesh rank incompatible with mesh indices");

    auto g1 = make_gf_from_fourier<N1, N2>(gin(), std::forward<M1>(m1), std::forward<M2>(m2));
    auto g2 = make_gf_from_fourier<N3>(g1(), std::forward<M3>(m3));

    return std::move(g2);
  }

  template <int... Ns, typename... Vs, typename T> auto make_gf_from_fourier(gf_const_view<cartesian_product<Vs...>, T> gin) {
    return std::move(make_gf_from_fourier<Ns...>(gin(), make_adjoint_mesh(std::get<Ns>(gin.mesh()))...));
  }

  /* *-----------------------------------------------------------------------------------------------------
   *
   * make_gf_from_fourier : Block / Block2 Gf
   *
   * *-----------------------------------------------------------------------------------------------------*/

  template <int N = 0, typename G, typename M, int R>
  auto make_gf_from_fourier(G const &gin, M const &m, std::vector<array<dcomplex, R>> const &known_moments) REQUIRES(is_block_gf_or_view<G>::value) {

    using r_t = decltype(make_gf_from_fourier<N>(gin[0], m, known_moments[0]));
    std::vector<r_t> g_vec;

    TRIQS_ASSERT2(gin.size() == known_moments.size(), "Fourier: Require equal number of blocks in block_gf and known_moments vector");

    for (auto [gin_bl, km_bl] : triqs::utility::zip(gin, known_moments)) g_vec.push_back(make_gf_from_fourier<N>(gin_bl, m, km_bl));
    return make_block_gf(gin.block_names(), std::move(g_vec));
  }

  template <int N = 0, typename G, typename M, int R>
  auto make_gf_from_fourier(G const &gin, M const &m, std::vector<std::vector<array<dcomplex, R>>> const &known_moments)
     REQUIRES(is_block_gf_or_view<G>::value) {

    using r_t = decltype(make_gf_from_fourier<N>(gin(0, 0), m, known_moments[0][0]));
    std::vector<std::vector<r_t>> g_vecvec;

    TRIQS_ASSERT2(gin.size1() == known_moments.size(), "Fourier: Require matching block structure between gin and known_moments");

    for (int i : range(gin.size1())) {
      TRIQS_ASSERT2(gin.size2() == known_moments[i].size(), "Fourier: Require matching block structure between gin and known_moments");

      std::vector<r_t> g_vec;
      for (int j : range(gin.size2())) g_vec.push_back(make_gf_from_fourier<N>(gin(i, j), m, known_moments[i][j]));

      g_vecvec.push_back(std::move(g_vec));
    }
    return block2_gf{gin.block_names(), std::move(g_vecvec)};
  }

  template <int N = 0, int... Ns, typename G, typename... Args>
  auto make_gf_from_fourier(G const &gin, Args const &... args) REQUIRES(is_block_gf_or_view<G>::value) {
    auto l = [&](typename G::const_view_type g_bl) { return make_gf_from_fourier<N, Ns...>(make_const_view(g_bl), args...); };
    return map_block_gf(l, gin);
  }

  /* *-----------------------------------------------------------------------------------------------------
   *
   * make_gf_from_fourier : Give proper overloads for gf and gf_view
   *
   * *-----------------------------------------------------------------------------------------------------*/

  template <int N = 0, int... Ns, typename V, typename T, typename... Args> auto make_gf_from_fourier(gf_view<V, T> gin, Args &&... args) {
    return make_gf_from_fourier<N, Ns...>(make_const_view(gin), std::forward<Args>(args)...);
  }

  template <int N = 0, int... Ns, typename V, typename T, typename... Args> auto make_gf_from_fourier(gf<V, T> const &gin, Args &&... args) {
    return make_gf_from_fourier<N, Ns...>(make_const_view(gin), std::forward<Args>(args)...);
  }

  template <int N = 0, int... Ns, typename V, typename T, typename... Args> auto make_gf_from_fourier(block_gf_view<V, T> gin, Args &&... args) {
    return make_gf_from_fourier<N, Ns...>(make_const_view(gin), std::forward<Args>(args)...);
  }

  template <int N = 0, int... Ns, typename V, typename T, typename... Args> auto make_gf_from_fourier(block_gf<V, T> const &gin, Args &&... args) {
    return make_gf_from_fourier<N, Ns...>(make_const_view(gin), std::forward<Args>(args)...);
  }

  template <int N = 0, int... Ns, typename V, typename T, typename... Args> auto make_gf_from_fourier(block2_gf_view<V, T> gin, Args &&... args) {
    return make_gf_from_fourier<N, Ns...>(make_const_view(gin), std::forward<Args>(args)...);
  }

  template <int N = 0, int... Ns, typename V, typename T, typename... Args> auto make_gf_from_fourier(block2_gf<V, T> const &gin, Args &&... args) {
    return make_gf_from_fourier<N, Ns...>(make_const_view(gin), std::forward<Args>(args)...);
  }

  /*------------------------------------------------------------------------------------------------------
  *                                  Lazy transformation
  *-----------------------------------------------------------------------------------------------------*/

  // internal. Keep a view on a, and the argument of the call
  template <int N, typename GCV, typename... Args> struct _fourier_lazy {
    GCV g;
    std::tuple<Args...> args; // Args can be a ref.
  };

  template <int N = 0, typename G, typename... Args> _fourier_lazy<N, typename G::const_view_type, Args...> fourier(G const &g, Args &&... args) {
    return {g(), {std::forward<Args>(args)...}};
  }

  // FIXME: add gx = fourier(gy) for gx a gf (not a view), adding a triqs_gf_assign_delegation and using make_gf_from_fourier

  // realize the call for gx = fourier(gy);
  template <int N, typename V1, typename T1, typename V2, typename T2, typename... Args>
  void triqs_gf_view_assign_delegation(gf_view<V1, T1> lhs_g, _fourier_lazy<N, gf_const_view<V2, T2>, Args...> const &rhs) {
    static_assert(std::is_same_v<typename T1::real_t, typename T2::real_t>, "Error : in gx = fourier(gy), gx and gy must have the same target");

    if constexpr (get_n_variables<V1>::value == 1) // === single mesh
      static_assert(std::is_same_v<V2, _mesh_fourier_image<V1>>, "There is no Fourier transform between these two meshes");
    else { // === cartesian_product mesh
      using mesh_res_t = decltype(triqs::tuple::replace<N>(rhs.g.mesh().components(), make_adjoint_mesh(std::get<N>(rhs.g.mesh()))));
      static_assert(std::is_same_v<typename gf_mesh<V1>::m_tuple_t, mesh_res_t>, "Meshes in assignment don't match");
    }

    // check the size of the "inactive" dimensions

    std::apply([&](auto &&... u) { _fourier<N>(rhs.g, lhs_g, u...); }, rhs.args); // calls _fourier( rhs.g, lhs_g, rhs.args...)
  }

} // namespace triqs::gfs

// declares the function to accept the clef lazy expressions
namespace triqs::clef {
  TRIQS_CLEF_MAKE_FNT_LAZY(fourier);
} // namespace triqs::clef
