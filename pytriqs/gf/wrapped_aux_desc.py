from cpp2py.wrap_generator import *
import re

# This modules contains utilities that will only be called internally, not by users.
m = module_(full_name = "pytriqs.gf.wrapped_aux", doc = "C++ wrapping of functions on Green functions ...", app_name="triqs")

m.add_include("<triqs/gfs.hpp>")
m.add_include("<triqs/gfs/transform/pade.hpp>")

m.add_include("<cpp2py/converters/vector.hpp>")
m.add_include("<cpp2py/converters/std_array.hpp>")
m.add_include("<triqs/cpp2py_converters.hpp>")

m.add_include("<triqs/gfs/legacy_for_python_api.hpp>")
m.add_using("namespace triqs::arrays")
m.add_using("namespace triqs::gfs")
m.add_using("triqs::utility::mini_vector")
m.add_preamble("""
""")

#------------------------------------------------------------
# Call operators
#------------------------------------------------------------
def all_calls():
    for M in ['imfreq']:
        yield M, ["dcomplex"], 0, 'scalar_valued', ['int'] # R =2
        yield M, ["matrix<dcomplex>"], 2, 'matrix_valued', ['int'] # R =2
        for R in [3,4]:
            yield M, ["array<dcomplex,%s>"%R], R, 'tensor_valued<%s>'%R, ['int']
    
    for M in ['imtime', 'refreq', 'retime']:
        yield M, ["dcomplex"], 0, 'scalar_valued', ['double'] # R =1
        yield M, ["matrix<dcomplex>"], 2, 'matrix_valued', ['double'] # R =2
        for R in [3,4]:
            yield M, ["array<dcomplex,%s>"%R], R, 'tensor_valued<%s>'%R, ['double']
    
    for M in ['brillouin_zone']:
        yield M, ["dcomplex"], 0, 'scalar_valued', ['std::array<double, 3>'] # R =1
        yield M, ["matrix<dcomplex>"], 2, 'matrix_valued', ['std::array<double,3>'] # R =2
        for R in [3,4]:
            yield M, ["array<dcomplex,%s>"%R], R, 'tensor_valued<%s>'%R, ['std::array<double, 3>'] 

    for M in ['cyclic_lattice']:
        yield M, ["dcomplex"], 0, 'scalar_valued', ['triqs::utility::mini_vector<int, 3>'] # R =1
        yield M,["matrix<dcomplex>"], 2, 'matrix_valued', ['triqs::utility::mini_vector<int, 3>'] # R =2
        for R in [3,4]:
            yield M, ["array<dcomplex,%s>"%R], R, 'tensor_valued<%s>'%R, ['triqs::utility::mini_vector<int, 3>'] 

    for M1 in ['brillouin_zone', 'cyclic_lattice']:
      k_x = {'brillouin_zone' : 'std::array<double, 3>', 'cyclic_lattice' : 'triqs::utility::mini_vector<int, 3>'}[M1]

      for M2 in ['imfreq']:
        yield 'cartesian_product<%s,%s>'%(M1,M2), ["dcomplex", "gf<imfreq, scalar_valued>"], 0, 'scalar_valued',[ (k_x, 'long'), (k_x, 'all_t')]

      for M2 in ['imtime', 'refreq', 'retime']:
        yield 'cartesian_product<%s,%s>'%(M1,M2),["dcomplex"], 0, 'scalar_valued', [(k_x, 'double')]

# Fixme
C_py_transcript = {'imfreq' : 'ImFreq', 
                   'refreq' : 'ReFreq', 
                   'imtime' : 'ImTime', 
                   'retime' : 'ReTime',
                   'brillouin_zone' : 'BrillouinZone',
                   'cyclic_lattice' : 'CyclicLattice', 
                   'cartesian_product<brillouin_zone,imfreq>': 'BrillouinZone_x_ImFreq',
                   'cartesian_product<brillouin_zone,imtime>': 'BrillouinZone_x_ImTime',
                   'cartesian_product<brillouin_zone,refreq>': 'BrillouinZone_x_ReFreq',
                   'cartesian_product<brillouin_zone,retime>': 'BrillouinZone_x_ReTime',
                   'cartesian_product<cyclic_lattice,imfreq>': 'CyclicLattice_x_ImFreq',
                   'cartesian_product<cyclic_lattice,imtime>': 'CyclicLattice_x_ImTime',
                   'cartesian_product<cyclic_lattice,refreq>': 'CyclicLattice_x_ReFreq',
                   'cartesian_product<cyclic_lattice,retime>': 'CyclicLattice_x_ReTime',
                   }
 
m.add_preamble("""
namespace triqs {
 namespace gfs {
  template<typename Gv>
  struct gf_proxy { 
   Gv gv;
   gf_proxy(Gv gv) : gv(gv){}
   template<typename ... U> auto call(U&& ... x) { return gv(std::forward<U>(x)...);}
  };
 }
}
""")

for var, return_t, R, target_t, point_t in all_calls():
    c_type = "gf_proxy<gf_view<%s,%s>>"%(var, target_t)
    c_py_trans = C_py_transcript[var]
    # print " Proxy : ", c_type, " : Py : ", "CallProxy%s_%s"%(c_py_trans,R)
    c = class_( 
            py_type = "CallProxy%s_%s"%(c_py_trans,R),
            c_type = c_type,
            c_type_absolute = "triqs::gfs::" + c_type,
            export = False
            )
    c.add_constructor("(gf_view<%s,%s> g)"%(var, target_t), doc = "")
    for P, Ret in zip(point_t, return_t) : 
        if not isinstance(P, tuple) : 
          c.add_call(signature = "%s call(%s x)"%(Ret,P), doc = "")
        else: 
            xs = ['%s x_%s'%(t,n) for (n,t) in enumerate(P)]
            sig =  "%s call(%s)"%(Ret, ','.join(xs))
            c.add_call(signature =sig,  doc = "")
    m.add_class (c)

    # FIX FIRST THE call and wrap of real_valued
#     target_t = target_t.replace("_value", "_real_value")
    # c_type = "gf_proxy<gf_view<%s,%s>>"%(var, target_t)
    # c = class_( 
            # py_type = "CallProxy%s_%s_R"%(C_py_transcript[var],R),
            # c_type = c_type,
            # c_type_absolute = "triqs::gfs::" + c_type
           # )
    # c.add_constructor("(gf_view<%s,%s> g)"%(var, target_t), doc = "")
    # c.add_call(signature = "%s call(%s x)"%(return_t, point_t), doc = "")
    # m.add_class (c)


# TESTING ONLY
# m.add_function("std::vector<dcomplex> call_vec(gf_view<imfreq, matrix_valued> g)", 
               # calling_pattern =""" auto result = std::vector<dcomplex> (100000); 
                                    # for (int i =0; i < 100000; ++i) result[i] = g(i)(0,0);
                                # """)   
# m.add_function("std::vector<dcomplex> call_s(gf_view<imfreq, matrix_valued> g)", 
               # calling_pattern =""" auto result = g(2)(0,0);
                                # """)   
# #------------------------------------------------------------

# matrix valued target
for TY in ['double', 'dcomplex'] : 
    for R in ['3'] : # To be generalized in C++
        m.add_function("void set_from_gf_data_mul_LR (array_view<{TY}, {R}> a, matrix<{TY}> l, array_view<{TY}, {R}> b,  matrix<{TY}> r)".format(TY=TY, R=R),
                        calling_pattern = "set_from_gf_data_mul_LR(a,l,b,r)")

# invert auxiliary tool
m.add_function("void _gf_invert_data_in_place(array_view <dcomplex, 3> a)", doc = "Aux function for inversion")

# For legacy Python code : authorize g + Matrix
for M in ['imfreq', 'refreq'] : 
    m.add_function("void _iadd_g_matrix_scalar (gf_view<%s, matrix_valued> x, matrix<std::complex<double>> y)"%M, calling_pattern = "x += y")
    m.add_function("void _iadd_g_matrix_scalar (gf_view<%s, matrix_valued> x, std::complex<double> y)"%M, calling_pattern = "x += y")
    #m.add_function("void _iadd_g_matrix_scalar (gf_view<%s, scalar_valued> x, std::complex<double> y)"%M, calling_pattern = "x += y")
    
    m.add_function("void _isub_g_matrix_scalar (gf_view<%s, matrix_valued> x, matrix<std::complex<double>> y)"%M, calling_pattern = "x -= y")
    m.add_function("void _isub_g_matrix_scalar (gf_view<%s, matrix_valued> x, std::complex<double> y)"%M, calling_pattern = "x -= y")
    #m.add_function("void _isub_g_matrix_scalar (gf_view<%s, scalar_valued> x, std::complex<double> y)"%M, calling_pattern = "x -= y")

    # is it useful ?
    #m.add_function("gf<imfreq> _imul_R_g_matrix (gf_view<{M}, matrix_valued> x, matrix<std::complex<double>> y)", calling_pattern = "x = x * y")
    #m.add_function("gf<imfreq> _imul_L_g_matrix (matrix<std::complex<double>> y, gf_view<{M}, matrix_valued> x)", calling_pattern = "x = y * x")

# invert auxiliary tool
m.add_function("gf<imfreq, matrix_valued> _make_gf_from_real_gf(gf_view<imfreq, matrix_valued> g)",
        calling_pattern = " auto result = make_gf_from_real_gf(make_const_view(g));", doc = "Backward Compat. Internal")
 
########################
##   Code generation
########################

m.generate_code()

