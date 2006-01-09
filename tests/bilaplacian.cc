// -*- c++ -*- (enables emacs c++ mode)
/* *********************************************************************** */
/*                                                                         */
/* Copyright (C) 2006-2006 Yves Renard, Julien Pommier.                    */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU Lesser General Public License as          */
/* published by the Free Software Foundation; version 2.1 of the License.  */
/*                                                                         */
/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* GNU Lesser General Public License for more details.                     */
/*                                                                         */
/* You should have received a copy of the GNU Lesser General Public        */
/* License along with this program; if not, write to the Free Software     */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,  */
/* USA.                                                                    */
/*                                                                         */
/* *********************************************************************** */

/**
   @file bilaplacian.cc
   @brief Bilaplacian problem. A dummy
   bilaplacian problem is solved on a regular mesh, and is compared to
   the analytical solution.

   This program is used to check that getfem++ is working. This is also 
   a good example of use of Getfem++.

   @see laplacian.cc
*/

#include <getfem_config.h>
#include <getfem_assembling.h> /* import assembly methods (and norms comp.) */
#include <getfem_export.h>   /* export functions (save solution in a file)  */
#include <getfem_regular_meshes.h>
#include <getfem_fourth_order.h>
#include <getfem_model_solvers.h>
#include <gmm.h>
#include <getfem_superlu.h>
#include <getfem_derivatives.h>

/* some Getfem++ types that we will be using */
using bgeot::base_small_vector; /* special class for small (dim<16) vectors */
using bgeot::base_node;  /* geometrical nodes(derived from base_small_vector)*/
using bgeot::scalar_type; /* = double */
using bgeot::size_type;   /* = unsigned long */
using bgeot::base_matrix; /* small dense matrix. */

/* definition of some matrix/vector types. 
 * default types of getfem_model_solvers.h
 */
typedef getfem::modeling_standard_sparse_vector sparse_vector;
typedef getfem::modeling_standard_sparse_matrix sparse_matrix;
typedef getfem::modeling_standard_plain_vector  plain_vector;

/**************************************************************************/
/*  Exact solution.                                                       */
/**************************************************************************/

scalar_type sol_u(const base_node &x)
{ return sin(std::accumulate(x.begin(), x.end(), 0.0)); }

scalar_type sol_f(const base_node &x) { return sol_u(x)*gmm::sqr(x.size()); }

base_small_vector sol_du(const base_node &x) {
  base_small_vector res(x.size());
  std::fill(res.begin(), res.end(),
	    cos(std::accumulate(x.begin(), x.end(), 0.0)));
  return res;
}

base_small_vector neumann_val(const base_node &x)
{ return -sol_du(x) * scalar_type(x.size()); }

/**************************************************************************/
/*  Structure for the bilaplacian problem.                                */
/**************************************************************************/

struct bilaplacian_problem {

  enum { CLAMPED_BOUNDARY_NUM = 0, SIMPLE_SUPPORT_BOUNDARY_NUM = 1,
	 NEUMANN_BOUNDARY_NUM = 2};
  getfem::mesh mesh;        /* the mesh */
  getfem::mesh_im mim;      /* the integration methods.                     */
  getfem::mesh_fem mf_u;    /* main mesh_fem, for the bilaplacian solution  */
  getfem::mesh_fem mf_mult; /* mesh_fem for the Dirichlet condition.        */
  getfem::mesh_fem mf_rhs;  /* mesh_fem for the right hand side (f(x),..)   */

  scalar_type residual;     /* max residual for the iterative solvers       */
  getfem::constraints_type dirichlet_version;

  std::string datafilename;
  ftool::md_param PARAM;

  bool solve(plain_vector &U);
  void init(void);
  void compute_error(plain_vector &U);
  bilaplacian_problem(void) : mim(mesh),mf_u(mesh), mf_mult(mesh),
			      mf_rhs(mesh) {}
};

namespace getfem {

  template<typename VECT> // just for a test
  void asm_hess(VECT &V, const mesh_im &mim, const mesh_fem &mf,
	const VECT &A, const mesh_region &rg = mesh_region::all_convexes()) {
    generic_assembly assem("a=data(#1); t=comp(Hess(#1));"
			   "V$1()+=t(j,1,1).a(j); V$2()+=t(j,1,2).a(j);"
			   "V$3()+=t(j,2,1).a(j); V$4()+=t(j,2,2).a(j)"
			   );
    assem.push_mi(mim);
    assem.push_mf(mf);
    assem.push_data(A);
    std::vector<scalar_type> V1(1), V2(1), V3(1), V4(1);
    assem.push_vec(V1); assem.push_vec(V2);
    assem.push_vec(V3); assem.push_vec(V4);
    assem.assembly(rg);
    V[0] = V1[0]; V[1] = V2[0]; V[2] = V3[0]; V[3] = V4[0];
  }

}


/* Read parameters from the .param file, build the mesh, set finite element
 * and integration methods and selects the boundaries.
 */
void bilaplacian_problem::init(void) {
  std::string MESH_TYPE = PARAM.string_value("MESH_TYPE","Mesh type ");
  std::string FEM_TYPE  = PARAM.string_value("FEM_TYPE","FEM name");
  std::string INTEGRATION = PARAM.string_value("INTEGRATION",
					       "Name of integration method");
  cout << "MESH_TYPE=" << MESH_TYPE << "\n";
  cout << "FEM_TYPE="  << FEM_TYPE << "\n";
  cout << "INTEGRATION=" << INTEGRATION << "\n";

  /* First step : build the mesh */
  bgeot::pgeometric_trans pgt = 
    bgeot::geometric_trans_descriptor(MESH_TYPE);
  size_type N = pgt->dim();
  std::vector<size_type> nsubdiv(N);
  std::fill(nsubdiv.begin(),nsubdiv.end(),
	    PARAM.int_value("NX", "Nomber of space steps "));
  getfem::regular_unit_mesh(mesh, nsubdiv, pgt,
			    PARAM.int_value("MESH_NOISED") != 0);
  
  bgeot::base_matrix M(N,N);
  for (size_type i=0; i < N; ++i) {
    static const char *t[] = {"LX","LY","LZ"};
    M(i,i) = (i<3) ? PARAM.real_value(t[i],t[i]) : 1.0;
  }
  if (N>1) { M(0,1) = PARAM.real_value("INCLINE") * PARAM.real_value("LY"); }

  /* scale the unit mesh to [LX,LY,..] and incline it */
  mesh.transformation(M);

  int dv = PARAM.int_value("DIRICHLET_VERSION", "Dirichlet version");
  dirichlet_version = getfem::constraints_type(dv);
  datafilename=PARAM.string_value("ROOTFILENAME","Base name of data files.");
  residual=PARAM.real_value("RESIDUAL"); if (residual == 0.) residual = 1e-10;

  /* set the finite element on the mf_u */
  getfem::pfem pf_u = getfem::fem_descriptor(FEM_TYPE);
  getfem::pintegration_method ppi = 
    getfem::int_method_descriptor(INTEGRATION);

  mim.set_integration_method(mesh.convex_index(), ppi);
  mf_u.set_finite_element(mesh.convex_index(), pf_u);

  // assembly test
  if (1) {
  mf_rhs.set_finite_element(mesh.convex_index(), 
			    getfem::fem_descriptor("FEM_PK(2,4)"));
  std::vector<scalar_type> UU(mf_u.nb_dof()), VV(mf_u.nb_dof()),
    WW(mf_rhs.nb_dof());
  for (size_type k = 0; k < mf_rhs.nb_dof(); ++k) {
    base_node pt = mf_rhs.point_of_dof(k);
    WW[k] = pt[1]*pt[1]*pt[0]*pt[0];
  }
  cout << "WW = " << WW << endl;

  sparse_matrix MM(mf_u.nb_dof(), mf_u.nb_dof());
  getfem::asm_mass_matrix(MM, mim, mf_u);
  getfem::asm_source_term(VV, mim, mf_u, mf_rhs, WW);
  
  gmm::iteration iter(1E-10, 1, 2000);
  gmm::ilut_precond<sparse_matrix> P(MM, 90, 1E-9);
  // gmm::identity_matrix P;
  gmm::cg(MM, UU, VV, P, iter);
  gmm::clean(UU, 1E-10);
  cout << "UU = " << UU << endl;

   mf_rhs.set_finite_element
     (mesh.convex_index(), 
      getfem::fem_descriptor("FEM_PK_DISCONTINUOUS(2,1)"));
   std::vector<scalar_type> WG(2*mf_rhs.nb_dof());
   getfem::compute_gradient(mf_u, mf_rhs, UU, WG);
   cout << "WG = " << WG << endl;


  getfem::vtk_export exp(datafilename + "_test.vtk",
			 PARAM.int_value("VTK_EXPORT")==1);
  exp.exporting(mf_u); 
  exp.write_point_data(mf_u, UU, "bilaplacian_displacement");
  cout << "export done, you can view the data file with (for example)\n"
       << "mayavi -d " << datafilename
       << "_test.vtk -m BandedSurfaceMap -m Outline\n";
  

  std::vector<scalar_type> RR(4);
  getfem::asm_hess(RR, mim, mf_u, UU);
  cout << "RR = " << RR << endl;

  }
     
  // To "see" the base function of a 2D element  ... 
  if (0) {
    for (size_type ii = 0; ii < mf_u.nb_dof(); ++ii) {
      std::vector<scalar_type> VV(mf_u.nb_dof());
      
      mf_rhs.set_finite_element
	(mesh.convex_index(), 
	 getfem::fem_descriptor("FEM_PK_DISCONTINUOUS(2,4)"));
      
      std::vector<scalar_type> WW(2*mf_rhs.nb_dof());
      
      VV[ii] = 1.0;
      getfem::compute_gradient(mf_u, mf_rhs, VV, WW);
      
      std::vector<scalar_type> G1(mf_rhs.nb_dof()), G2(mf_rhs.nb_dof());
      gmm::copy(gmm::sub_vector(WW, gmm::sub_slice(0, mf_rhs.nb_dof(),2)), G1);
      gmm::copy(gmm::sub_vector(WW, gmm::sub_slice(1, mf_rhs.nb_dof(),2)), G2);
      
      mf_mult.set_finite_element
	(mesh.convex_index(), 
	 getfem::fem_descriptor("FEM_PK_DISCONTINUOUS(2,1)"));
      
      std::vector<scalar_type> WW1(2*mf_mult.nb_dof());
      std::vector<scalar_type> WW2(2*mf_mult.nb_dof());
      getfem::compute_gradient(mf_rhs, mf_mult, G1, WW1);
      getfem::compute_gradient(mf_rhs, mf_mult, G2, WW2);
      
      std::vector<scalar_type> WW3(4*mf_mult.nb_dof());
      getfem::compute_hessian(mf_u, mf_mult, VV, WW3);
      
      
      mf_rhs.set_finite_element(mesh.convex_index(), 
				getfem::fem_descriptor("FEM_PK(2,1)"));
      
      std::vector<scalar_type> WWW(mf_rhs.nb_dof());
      getfem::interpolation(mf_u, mf_rhs, VV, WWW);
      cout << "ii = " << ii << " point " << mf_u.point_of_dof(ii)
	   << " WW = " << WW << " WW1 = " << WW1 << " WW2 = " << WW2
	   << " WW3 = " << WW3 << " WWW = " << WWW << endl;
      
      getchar();

    }
  }


  std::string dirichlet_fem_name = PARAM.string_value("DIRICHLET_FEM_TYPE");
  if (dirichlet_fem_name.size() == 0)
    mf_mult.set_finite_element(mesh.convex_index(), pf_u);
  else {
    cout << "DIRICHLET_FEM_TYPE="  << dirichlet_fem_name << "\n";
    mf_mult.set_finite_element(mesh.convex_index(), 
			       getfem::fem_descriptor(dirichlet_fem_name));
  }

  /* set the finite element on mf_rhs (same as mf_u is DATA_FEM_TYPE is
     not used in the .param file */
  std::string data_fem_name = PARAM.string_value("DATA_FEM_TYPE");
  if (data_fem_name.size() == 0) {
    if (!pf_u->is_lagrange()) {
      DAL_THROW(dal::failure_error, "You are using a non-lagrange FEM. "
		<< "In that case you need to set "
		<< "DATA_FEM_TYPE in the .param file");
    }
    mf_rhs.set_finite_element(mesh.convex_index(), pf_u);
  } else {
    mf_rhs.set_finite_element(mesh.convex_index(), 
			      getfem::fem_descriptor(data_fem_name));
  }
  
  /* set boundary conditions
   * (Neuman on the upper face, Dirichlet elsewhere) */
  cout << "Selecting Neumann and Dirichlet boundaries\n";
  getfem::mesh_region border_faces;
  getfem::outer_faces_of_mesh(mesh, border_faces);
  for (getfem::mr_visitor i(border_faces); !i.finished(); ++i) {
    base_node un = mesh.normal_of_face_of_convex(i.cv(), i.f());
    un /= gmm::vect_norm2(un);
    if (0 && gmm::abs(un[N-1] - 1.0) <= 1.0E-7)
      mesh.region(NEUMANN_BOUNDARY_NUM).add(i.cv(), i.f());
    else {
      mesh.region(SIMPLE_SUPPORT_BOUNDARY_NUM).add(i.cv(), i.f());
      if (gmm::abs(un[N-1] + 1.0) <= 1.0E-7)
	mesh.region(CLAMPED_BOUNDARY_NUM).add(i.cv(), i.f());
    }
  }
}

/* compute the error with respect to the exact solution */
void bilaplacian_problem::compute_error(plain_vector &U) {
  std::vector<scalar_type> V(mf_rhs.nb_dof());
  getfem::interpolation(mf_u, mf_rhs, U, V);
  for (size_type i = 0; i < mf_rhs.nb_dof(); ++i)
    V[i] -= sol_u(mf_rhs.point_of_dof(i));
  cout.precision(16);
  cout << "L2 error = " << getfem::asm_L2_norm(mim, mf_rhs, V) << endl
       << "H1 error = " << getfem::asm_H1_norm(mim, mf_rhs, V) << endl
       << "Linfty error = " << gmm::vect_norminf(V) << endl;
}

/**************************************************************************/
/*  Model.                                                                */
/**************************************************************************/


bool bilaplacian_problem::solve(plain_vector &U) {
  size_type nb_dof_rhs = mf_rhs.nb_dof();
  size_type N = mesh.dim();

  cout << "Number of dof for u: " << mf_u.nb_dof() << endl;

  // Bilaplacian brick.
  getfem::mdbrick_bilaplacian<> BIL(mim, mf_u);

  // Defining the volumic source term.
  plain_vector F(nb_dof_rhs);
  for (size_type i = 0; i < nb_dof_rhs; ++i)
    F[i] = sol_f(mf_rhs.point_of_dof(i));
  
  // Volumic source term brick.
  getfem::mdbrick_source_term<> VOL_F(BIL, mf_rhs, F);

  // Defining the Neumann condition right hand side.
  base_small_vector un(N);
  gmm::clear(F);
  for (getfem::mr_visitor i(mesh.region(NEUMANN_BOUNDARY_NUM));
       !i.finished(); ++i) {
    size_type cv = i.cv(), f = i.f();
    getfem::pfem pf = mf_rhs.fem_of_element(cv);
    for (size_type l = 0; l< pf->structure(cv)->nb_points_of_face(f); ++l) {
      size_type n = pf->structure(cv)->ind_points_of_face(f)[l];
      un = mesh.normal_of_face_of_convex(cv, f, pf->node_of_dof(cv, n));
      un /= gmm::vect_norm2(un);
      size_type dof = mf_rhs.ind_dof_of_element(cv)[n];
      F[dof] = gmm::vect_sp(neumann_val(mf_rhs.point_of_dof(dof)), un);
    }
  }

  // Neumann condition brick.
  getfem::mdbrick_source_term<>
    NEUMANN(VOL_F, mf_rhs, F, NEUMANN_BOUNDARY_NUM);
  
  // Defining the Dirichlet condition value.
  for (size_type i = 0; i < nb_dof_rhs; ++i)
    F[i] = sol_u(mf_rhs.point_of_dof(i));

  // Dirichlet condition brick.
  getfem::mdbrick_Dirichlet<>
    DIRICHLET(NEUMANN, SIMPLE_SUPPORT_BOUNDARY_NUM, mf_mult);
  DIRICHLET.set_constraints_type(dirichlet_version);
  DIRICHLET.rhs().set(mf_rhs, F);

  // Defining the normal derivative Dirichlet condition value.
  gmm::clear(F);
  for (getfem::mr_visitor i(mesh.region(CLAMPED_BOUNDARY_NUM));
       !i.finished(); ++i) {
    size_type cv = i.cv(), f = i.f();
    getfem::pfem pf = mf_rhs.fem_of_element(cv);
    for (size_type l = 0; l< pf->structure(cv)->nb_points_of_face(f); ++l) {
      size_type n = pf->structure(cv)->ind_points_of_face(f)[l];
      un = mesh.normal_of_face_of_convex(cv, f, pf->node_of_dof(cv, n));
      un /= gmm::vect_norm2(un);
      size_type dof = mf_rhs.ind_dof_of_element(cv)[n];
      F[dof] = gmm::vect_sp(sol_du(mf_rhs.point_of_dof(dof)), un) * 0.0;
    }
  }
  
  // Normal derivative Dirichlet condition brick.
  getfem::mdbrick_normal_derivative_Dirichlet<> 
    final_model(DIRICHLET, CLAMPED_BOUNDARY_NUM, mf_mult);
  final_model.set_constraints_type(dirichlet_version);
  final_model.rhs().set(mf_rhs, F);

   


  // Generic solve.
  cout << "Total number of variables : " << final_model.nb_dof() << endl;
  getfem::standard_model_state MS(final_model);
  gmm::iteration iter(residual, 1, 40000);
  getfem::standard_solve(MS, final_model, iter);

  // Solution extraction
  gmm::copy(BIL.get_solution(MS), U);
  return (iter.converged());
}
  
/**************************************************************************/
/*  main program.                                                         */
/**************************************************************************/

int main(int argc, char *argv[]) {

  GETFEM_MPI_INIT(argc, argv); // For parallelized version
  DAL_SET_EXCEPTION_DEBUG; // Exceptions make a memory fault, to debug.
  FE_ENABLE_EXCEPT;        // Enable floating point exception for Nan.

  try {
    bilaplacian_problem p;
    p.PARAM.read_command_line(argc, argv);
    p.init();
    plain_vector U(p.mf_u.nb_dof());
    if (!p.solve(U)) DAL_THROW(dal::failure_error, "Solve has failed");

    p.compute_error(U);

    cout << "U = " << U << endl;

    if (p.PARAM.int_value("VTK_EXPORT")) {
      cout << "export to " << p.datafilename + ".vtk" << "..\n";
      getfem::vtk_export exp(p.datafilename + ".vtk",
			     p.PARAM.int_value("VTK_EXPORT")==1);
      exp.exporting(p.mf_u); 
      exp.write_point_data(p.mf_u, U, "bilaplacian_displacement");
      cout << "export done, you can view the data file with (for example)\n"
	   << "mayavi -d " << p.datafilename
	   << ".vtk -m BandedSurfaceMap -m Outline\n";
    }
  }
  DAL_STANDARD_CATCH_ERROR;

  GETFEM_MPI_FINALIZE;

  return 0; 
}
