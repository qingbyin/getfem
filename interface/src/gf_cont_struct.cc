/*===========================================================================
 
 Copyright (C) 2012-2014 Tomas Ligursky, Yves Renard.
 
 This file is a part of GETFEM++
 
 Getfem++  is  free software;  you  can  redistribute  it  and/or modify it
 under  the  terms  of the  GNU  Lesser General Public License as published
 by  the  Free Software Foundation;  either version 3 of the License,  or
 (at your option) any later version along with the GCC Runtime Library
 Exception either version 3.1 or (at your option) any later version.
 This program  is  distributed  in  the  hope  that it will be useful,  but
 WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 or  FITNESS  FOR  A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 License and GCC Runtime Library Exception for more details.
 You  should  have received a copy of the GNU Lesser General Public License
 along  with  this program;  if not, write to the Free Software Foundation,
 Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
 
===========================================================================*/

#include <getfemint.h>
#include <getfemint_workspace.h>
#include <getfemint_models.h>
#include <getfemint_cont_struct.h>


using namespace getfemint;

/*@GFDOC
  This object serves for storing parameters and data used in numerical
  continuation of solution branches of models (for more details about
  continuation see the Getfem++ user documentation).
@*/

void gf_cont_struct(getfemint::mexargs_in& in, getfemint::mexargs_out& out) {
  getfemint_cont_struct *pgs = NULL;  
  if (check_cmd("ContStruct", "ContStruct", in, out, 3, 43, 0, 1)) {
    
    /*@INIT S = ('.init', @tmodel md, @str dataname_parameter[,@str dataname_init, @str dataname_final, @str dataname_current], @scalar sc_fac[, ...])
    The variable `dataname_parameter` should parametrise the model given by
    `md`. If the parametrisation is done via a vector datum, `dataname_init`
    and `dataname_final` should store two given values of this datum
    determining the parametrisation, and `dataname_current` serves for actual
    values of this datum. `sc_fac` is a scale factor involved in the weighted
    norm used in the continuation.
    
    Additional options:
    
    - 'lsolver', @str SOLVER_NAME
       name of the solver to be used for the incorporated linear systems
       (the default value is 'auto', which lets getfem choose itself);
       possible values are 'superlu', 'mumps' (if supported), 'cg/ildlt',
       'gmres/ilu' and 'gmres/ilut';
    - 'h_init', @scalar HIN
       initial step size (the default value is 1e-2);
    - 'h_max', @scalar HMAX
       maximum step size (the default value is 1e-1);
    - 'h_min', @scalar HMIN
       minimum step size (the default value is 1e-5);
    - 'h_inc', @scalar HINC
       factor for enlarging the step size (the default value is 1.3);
    - 'h_dec', @scalar HDEC
       factor for diminishing the step size (the default value is 0.5);
    - 'max_iter', @int MIT
       maximum number of iterations allowed in the correction (the default
       value is 10);
    - 'thr_iter', @int TIT
       threshold number of iterations of the correction for enlarging the
       step size (the default value is 4);
    - 'max_res', @scalar RES
       target residual value of a new point on the solution curve (the
       default value is 1e-6);
    - 'max_diff', @scalar DIFF
       determines a convergence criterion for two consecutive points (the
       default value is 1e-6);
    - 'min_cos', @scalar MCOS
       minimal value of the cosine of the angle between tangents to the
       solution curve at an old point and a new one (the default value is
       0.9);
    - 'max_res_solve', @scalar RES_SOLVE
       target residual value for the linear systems to be solved (the
       default value is 1e-8);
    - 'singularities', @int SING
       activates tools for detection and treatment of singular points (1 for
       limit points, 2 for bifurcation points);
    - 'non-smooth'
       determines that some special methods for non-smooth problems can be
       used;
    - 'delta_max', @scalar DMAX
       maximum size of division for evaluating the test function on the
       convex combination of two augmented Jacobians that belong to different
       smooth pieces (the default value is 0.005);
    - 'delta_min', @scalar DMIN
       minimum size of division for evaluating the test function on the
       convex combination (the default value is 0.00012);
    - 'thr_var', @scalar TVAR
       threshold variation for refining the division (the default value is
       0.02);
    - 'nb_dir', @int NDIR
       number of linear combinations of vectors in one subspace when
       searching for new tangent predictions during location of new one-sided
       branches (the default value is 40);
    - 'nb_comb', @int NCOMB
       maximum number of couples of reference vectors forming the linear
       combinations (the default value is 1);
    - 'noisy' or 'very_noisy'
       determines how detailed information has to be displayed during the
       continuation process (residual values etc.).@*/
    
       getfemint_model *md = in.pop().to_getfemint_model();
       std::string dataname_parameter = in.pop().to_string();
       bool with_parametrised_data = false;
       std::string dataname_init; std::string dataname_final;
       std::string dataname_current;
       if (in.front().is_string()) {
         with_parametrised_data = true;
         dataname_init = in.pop().to_string();
         dataname_final = in.pop().to_string();
         dataname_current = in.pop().to_string();
       } 
       scalar_type scfac = in.pop().to_scalar();

       std::string lsolver = "auto"; scalar_type h_init = 1.e-2;
       scalar_type h_max = 1.e-1; scalar_type h_min = 1.e-5;
       scalar_type h_inc = 1.3; scalar_type h_dec = 0.5;
       size_type maxit = 10; size_type thrit = 4; scalar_type maxres = 1.e-6;
       scalar_type maxdiff = 1.e-6; scalar_type mincos = 0.9;
       scalar_type maxres_solve = 1.e-8; scalar_type delta_max = 0.005;
       scalar_type delta_min = 0.00012;
       scalar_type thrvar = 0.02; size_type nbdir = 40; size_type nbcomb = 1;
       int noisy = 0; int singularities = 0; bool nonsmooth = false;

       while (in.remaining() && in.front().is_string()) {
         std::string opt = in.pop().to_string();
         if (cmd_strmatch(opt, "lsolver"))  {
           if (in.remaining()) lsolver = in.pop().to_string();
           else THROW_BADARG("missing name for " << opt);
         } else if (cmd_strmatch(opt, "h_init")) {
           if (in.remaining()) h_init = in.pop().to_scalar();
           else THROW_BADARG("missing value for " << opt);
         } else if (cmd_strmatch(opt, "h_max")) {
           if (in.remaining()) h_max = in.pop().to_scalar();
           else THROW_BADARG("missing value for " << opt);
         } else if (cmd_strmatch(opt, "h_min")) {
           if (in.remaining()) h_min = in.pop().to_scalar();
           else THROW_BADARG("missing value for " << opt);
         } else if (cmd_strmatch(opt, "h_inc")) {
           if (in.remaining()) h_inc = in.pop().to_scalar();
           else THROW_BADARG("missing value for " << opt);
         } else if (cmd_strmatch(opt, "h_dec")) {
           if (in.remaining()) h_dec = in.pop().to_scalar();
           else THROW_BADARG("missing value for " << opt);
         } else if (cmd_strmatch(opt, "max_iter")) {
           if (in.remaining()) maxit = in.pop().to_integer();
           else THROW_BADARG("missing value for " << opt);
         } else if (cmd_strmatch(opt, "thr_iter")) {
           if (in.remaining()) thrit = in.pop().to_integer();
           else THROW_BADARG("missing value for " << opt);
         } else if (cmd_strmatch(opt, "max_res")) {
           if (in.remaining()) maxres = in.pop().to_scalar();
           else THROW_BADARG("missing value for " << opt);
         } else if (cmd_strmatch(opt, "max_diff")) {
           if (in.remaining()) maxdiff = in.pop().to_scalar();
           else THROW_BADARG("missing value for " << opt);
         } else if (cmd_strmatch(opt, "min_cos")) {
           if (in.remaining()) mincos = in.pop().to_scalar();
           else THROW_BADARG("missing value for " << opt);
         } else if (cmd_strmatch(opt, "max_res_solve")) {
           if (in.remaining()) maxres_solve = in.pop().to_scalar();
           else THROW_BADARG("missing value for " << opt);
         } else if (cmd_strmatch(opt, "delta_max")) {
           if (in.remaining()) delta_max = in.pop().to_scalar();
           else THROW_BADARG("missing value for " << opt);
         } else if (cmd_strmatch(opt, "delta_min")) {
           if (in.remaining()) delta_min = in.pop().to_scalar();
           else THROW_BADARG("missing value for " << opt);
         } else if (cmd_strmatch(opt, "thr_var")) {
           if (in.remaining()) thrvar = in.pop().to_scalar();
           else THROW_BADARG("missing value for " << opt);
         } else if (cmd_strmatch(opt, "nb_dir")) {
           if (in.remaining()) nbdir = in.pop().to_integer();
           else THROW_BADARG("missing value for " << opt);
         } else if (cmd_strmatch(opt, "nb_comb")) {
           if (in.remaining()) nbcomb = in.pop().to_integer();
           else THROW_BADARG("missing value for " << opt);
         } else if (cmd_strmatch(opt, "singularities")) {
	   if (in.remaining()) singularities = in.pop().to_integer();
	   else THROW_BADARG("missing value for " << opt);
	 } else if (cmd_strmatch(opt, "non-smooth")) nonsmooth = true;
         else if (cmd_strmatch(opt, "noisy")) noisy = 1;
         else if (cmd_strmatch(opt, "very noisy") ||
                  cmd_strmatch(opt, "very_noisy")) noisy = 2;
         else THROW_BADARG("bad option: " << opt);
       }

       getfem::cont_struct_getfem_model *ps;
       if (!with_parametrised_data) {
	 getfem::cont_struct_getfem_model *ps1 =
	   new getfem::cont_struct_getfem_model
           (md->model(), dataname_parameter, scfac,
	    getfem::rselect_linear_solver(md->model(), lsolver), h_init,
	    h_max, h_min, h_inc, h_dec, maxit, thrit, maxres, maxdiff,
	    mincos, maxres_solve, noisy, singularities, nonsmooth,
	    delta_max, delta_min, thrvar, nbdir, nbcomb);
	 ps = ps1;
       }
       else {
	 getfem::cont_struct_getfem_model *ps1 =
	   new getfem::cont_struct_getfem_model
	   (md->model(), dataname_parameter, dataname_init, dataname_final,
	    dataname_current, scfac,
	    getfem::rselect_linear_solver(md->model(), lsolver), h_init,
	    h_max, h_min, h_inc, h_dec, maxit, thrit, maxres, maxdiff,
	    mincos, maxres_solve, noisy, singularities, nonsmooth,
	    delta_max, delta_min, thrvar, nbdir, nbcomb);
	 ps = ps1;
       }

       pgs = getfemint_cont_struct::get_from(ps);
       workspace().set_dependance(pgs, md);
  }
  out.pop().from_object_id(pgs->get_id(), CONT_STRUCT_CLASS_ID);
}
