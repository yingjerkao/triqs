from cpp2py.wrap_generator import *

# The module
module = module_(full_name = "cpp2py.test_multivar", doc = "Doc of multivar GF")

module.add_include("multivar.hpp")

module.add_include("<triqs/cpp2py_converters.hpp>")

module.add_function(name = "make_vertex", signature = "gf<cartesian_product<imfreq,imfreq,imfreq>, tensor_valued<4>>(double a)")
module.add_function(name = "pass_vertex", signature = "void(gf_view<cartesian_product<imfreq,imfreq,imfreq>, tensor_valued<4>> gamma)")

module.add_function(name = "make_block_vertex", signature = "block2_gf<cartesian_product<imfreq,imfreq,imfreq>, tensor_valued<4>>(double a)")
module.add_function(name = "pass_block_vertex", signature = "void(block2_gf_view<cartesian_product<imfreq,imfreq,imfreq>, tensor_valued<4>> gamma)")

if __name__ == '__main__' :
   module.generate_code()
