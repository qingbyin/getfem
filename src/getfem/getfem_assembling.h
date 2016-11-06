/* -*- c++ -*- (enables emacs c++ mode) */
/*===========================================================================

 Copyright (C) 2000-2016 Yves Renard, Julien Pommier

 This file is a part of GetFEM++

 GetFEM++  is  free software;  you  can  redistribute  it  and/or modify it
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

 As a special exception, you  may use  this file  as it is a part of a free
 software  library  without  restriction.  Specifically,  if   other  files
 instantiate  templates  or  use macros or inline functions from this file,
 or  you compile this  file  and  link  it  with other files  to produce an
 executable, this file  does  not  by itself cause the resulting executable
 to be covered  by the GNU Lesser General Public License.  This   exception
 does not  however  invalidate  any  other  reasons why the executable file
 might be covered by the GNU Lesser General Public License.

===========================================================================*/

/** @file getfem_assembling.h
    @author  Yves Renard <Yves.Renard@insa-lyon.fr>
    @author  Julien Pommier <Julien.Pommier@insa-toulouse.fr>
    @date November 17, 2000.
    @brief Miscelleanous assembly routines for common terms. Use the low-level
           generic assembly. Prefer the high-level one.
 */

/** @defgroup asm Assembly routines */

#ifndef GETFEM_ASSEMBLING_H__
#define GETFEM_ASSEMBLING_H__

#include "getfem_assembling_tensors.h"
#include "getfem_generic_assembly.h"

namespace getfem {

  /**
     compute @f$ \|U\|_2 @f$, U might be real or complex
     @ingroup asm
   */
  template<typename VEC>
  inline scalar_type asm_L2_norm
  (const mesh_im &mim, const mesh_fem &mf, const VEC &U,
   const mesh_region &rg=mesh_region::all_convexes())
  { return sqrt(asm_L2_norm_sqr(mim, mf, U, rg)); }

  template<typename VEC>
  scalar_type asm_L2_norm_sqr
  (const mesh_im &mim, const mesh_fem &mf, const VEC &U,
   const mesh_region &rg=mesh_region::all_convexes()) {
    return asm_L2_norm_sqr(mim, mf, U, rg,
			   typename gmm::linalg_traits<VEC>::value_type());
  }
  
  template<typename VEC, typename T>
  inline scalar_type asm_L2_norm_sqr(const mesh_im &mim, const mesh_fem &mf,
			      const VEC &U, const mesh_region &rg_, T) {
    ga_workspace workspace;
    model_real_plain_vector UU(mf.nb_dof());
    gmm::copy(U, UU);
    gmm::sub_interval Iu(0, mf.nb_dof());
    workspace.add_fem_variable("u", mf, Iu, UU);
    workspace.add_expression("u.u", mim, rg_);
    workspace.assembly(0);
    return workspace.assembled_potential();
  }

  inline scalar_type asm_L2_norm_sqr(const mesh_im &mim, const mesh_fem &mf,
			      const model_real_plain_vector &U,
			      const mesh_region &rg_, scalar_type) {
    ga_workspace workspace;
    gmm::sub_interval Iu(0, mf.nb_dof());
    workspace.add_fem_variable("u", mf, Iu, U);
    workspace.add_expression("u.u", mim, rg_);
    workspace.assembly(0);
    return workspace.assembled_potential();
  }

  template<typename VEC, typename T>
  inline scalar_type asm_L2_norm_sqr(const mesh_im &mim, const mesh_fem &mf,
			      const VEC &U,
			      const mesh_region &rg, std::complex<T>) {
    ga_workspace workspace;
    model_real_plain_vector UUR(mf.nb_dof()), UUI(mf.nb_dof());
    gmm::copy(gmm::real_part(U), UUR);
    gmm::copy(gmm::imag_part(U), UUI);
    gmm::sub_interval Iur(0, mf.nb_dof()), Iui(mf.nb_dof(), mf.nb_dof());
    workspace.add_fem_variable("u", mf, Iur, UUR);
    workspace.add_fem_variable("v", mf, Iui, UUI);
    workspace.add_expression("u.u + v.v", mim, rg);
    workspace.assembly(0);
    return workspace.assembled_potential();
  }
  
  /**
     Compute the distance between U1 and U2, defined on two different
     mesh_fems (but sharing the same mesh), without interpolating U1 on mf2.
     
     @ingroup asm
  */
  template<typename VEC1, typename VEC2>
  inline scalar_type asm_L2_dist
  (const mesh_im &mim, const mesh_fem &mf1, const VEC1 &U1,
   const mesh_fem &mf2, const VEC2 &U2,
   mesh_region rg = mesh_region::all_convexes()) {
    return sqrt(asm_L2_dist_sqr(mim, mf1, U1, mf2, U2, rg,
			   typename gmm::linalg_traits<VEC1>::value_type()));
  }

  template<typename VEC1, typename VEC2, typename T>
  inline scalar_type asm_L2_dist_sqr
  (const mesh_im &mim, const mesh_fem &mf1, const VEC1 &U1,
   const mesh_fem &mf2, const VEC2 &U2, mesh_region rg, T) {
    ga_workspace workspace;
    model_real_plain_vector UU1(mf1.nb_dof()), UU2(mf2.nb_dof());
    gmm::copy(U1, UU1); gmm::copy(U2, UU2);
    gmm::sub_interval Iu1(0, mf1.nb_dof()), Iu2(mf1.nb_dof(), mf2.nb_dof());
    workspace.add_fem_variable("u1", mf1, Iu1, UU1);
    workspace.add_fem_variable("u2", mf2, Iu2, UU2);
    workspace.add_expression("(u2-u1).(u2-u1)", mim, rg);
    workspace.assembly(0);
    return workspace.assembled_potential();
  }

  inline scalar_type asm_L2_dist_sqr
  (const mesh_im &mim, const mesh_fem &mf1, const  model_real_plain_vector &U1,
   const mesh_fem &mf2, const  model_real_plain_vector &U2, 
   mesh_region rg, scalar_type) {
    ga_workspace workspace;
    gmm::sub_interval Iu1(0, mf1.nb_dof()), Iu2(mf1.nb_dof(), mf2.nb_dof());
    workspace.add_fem_variable("u1", mf1, Iu1, U1);
    workspace.add_fem_variable("u2", mf2, Iu2, U2);
    workspace.add_expression("(u2-u1).(u2-u1)", mim, rg);
    workspace.assembly(0);
    return workspace.assembled_potential();
  }

  template<typename VEC1, typename VEC2, typename T>
  inline scalar_type asm_L2_dist_sqr
  (const mesh_im &mim, const mesh_fem &mf1, const VEC1 &U1,
   const mesh_fem &mf2, const VEC2 &U2, mesh_region rg, std::complex<T>) {
    ga_workspace workspace;
    model_real_plain_vector UU1R(mf1.nb_dof()), UU2R(mf2.nb_dof());
    model_real_plain_vector UU1I(mf1.nb_dof()), UU2I(mf2.nb_dof());
    gmm::copy(gmm::real_part(U1), UU1R); gmm::copy(gmm::imag_part(U1), UU1I);
    gmm::copy(gmm::real_part(U2), UU2R); gmm::copy(gmm::imag_part(U2), UU2I);
    gmm::sub_interval Iu1r(0, mf1.nb_dof()), Iu2r(mf1.nb_dof(), mf2.nb_dof());
    gmm::sub_interval Iu1i(Iu2r.last(), mf1.nb_dof());
    gmm::sub_interval Iu2i(Iu1i.last(), mf2.nb_dof());
    workspace.add_fem_variable("u1", mf1, Iu1r, UU1R);
    workspace.add_fem_variable("u2", mf2, Iu2r, UU2R);
    workspace.add_fem_variable("v1", mf1, Iu1i, UU1I);
    workspace.add_fem_variable("v2", mf2, Iu2i, UU2I);
    workspace.add_expression("(u2-u1).(u2-u1) + (v2-v1).(v2-v1)", mim, rg);
    workspace.assembly(0);
    return workspace.assembled_potential();
  }

  
  /**
     compute @f$\|\nabla U\|_2@f$, U might be real or complex
     @ingroup asm
   */
  template<typename VEC>
  scalar_type asm_H1_semi_norm
  (const mesh_im &mim, const mesh_fem &mf, const VEC &U,
   const mesh_region &rg = mesh_region::all_convexes()) {
    typedef typename gmm::linalg_traits<VEC>::value_type T;
    return sqrt(asm_H1_semi_norm_sqr(mim, mf, U, rg, T()));
  }

  template<typename VEC, typename T>
  inline scalar_type asm_H1_semi_norm_sqr
  (const mesh_im &mim, const mesh_fem &mf, const VEC &U,
   const mesh_region &rg_, T) {
    ga_workspace workspace;
    model_real_plain_vector UU(mf.nb_dof()); gmm::copy(U, UU);
    gmm::sub_interval Iu(0, mf.nb_dof());
    workspace.add_fem_variable("u", mf, Iu, UU);
    workspace.add_expression("Grad_u:Grad_u", mim, rg_);
    workspace.assembly(0);
    return workspace.assembled_potential();
  }

  inline scalar_type asm_H1_semi_norm_sqr
  (const mesh_im &mim, const mesh_fem &mf, const model_real_plain_vector &U,
   const mesh_region &rg_, scalar_type) {
    ga_workspace workspace;
    gmm::sub_interval Iu(0, mf.nb_dof());
    workspace.add_fem_variable("u", mf, Iu, U);
    workspace.add_expression("Grad_u:Grad_u", mim, rg_);
    workspace.assembly(0);
    return workspace.assembled_potential();
  }
  

  template<typename VEC, typename T>
  inline scalar_type asm_H1_semi_norm_sqr
  (const mesh_im &mim, const mesh_fem &mf, const VEC &U,
   const mesh_region &rg, std::complex<T>) {
    ga_workspace workspace;
    model_real_plain_vector UUR(mf.nb_dof()), UUI(mf.nb_dof());
    gmm::copy(gmm::real_part(U), UUR);
    gmm::copy(gmm::imag_part(U), UUI);
    gmm::sub_interval Iur(0, mf.nb_dof()), Iui(mf.nb_dof(), mf.nb_dof());
    workspace.add_fem_variable("u", mf, Iur, UUR);
    workspace.add_fem_variable("v", mf, Iui, UUI);
    workspace.add_expression("Grad_u:Grad_u + Grad_v:Grad_v", mim, rg);
    workspace.assembly(0);
    return workspace.assembled_potential();
  }
  
  /**
     Compute the H1 semi-distance between U1 and U2, defined on two different
     mesh_fems (but sharing the same mesh), without interpolating U1 on mf2.
     
     @ingroup asm
  */
  template<typename VEC1, typename VEC2>
  inline scalar_type asm_H1_semi_dist
  (const mesh_im &mim, const mesh_fem &mf1, const VEC1 &U1,
   const mesh_fem &mf2, const VEC2 &U2,
   mesh_region rg = mesh_region::all_convexes()) {
    return sqrt(asm_H1_semi_dist_sqr(mim, mf1, U1, mf2, U2, rg,
			   typename gmm::linalg_traits<VEC1>::value_type()));
  }

  template<typename VEC1, typename VEC2, typename T>
  inline scalar_type asm_H1_semi_dist_sqr
  (const mesh_im &mim, const mesh_fem &mf1, const VEC1 &U1,
   const mesh_fem &mf2, const VEC2 &U2, mesh_region rg, T) {
    ga_workspace workspace;
    model_real_plain_vector UU1(mf1.nb_dof()), UU2(mf2.nb_dof());
    gmm::copy(U1, UU1); gmm::copy(U2, UU2);
    gmm::sub_interval Iu1(0, mf1.nb_dof()), Iu2(mf1.nb_dof(), mf2.nb_dof());
    workspace.add_fem_variable("u1", mf1, Iu1, UU1);
    workspace.add_fem_variable("u2", mf2, Iu2, UU2);
    workspace.add_expression("(Grad_u2-Grad_u1):(Grad_u2-Grad_u1)", mim, rg);
    workspace.assembly(0);
    return workspace.assembled_potential();
  }

  inline scalar_type asm_H1_semi_dist_sqr
  (const mesh_im &mim, const mesh_fem &mf1, const  model_real_plain_vector &U1,
   const mesh_fem &mf2, const  model_real_plain_vector &U2, 
   mesh_region rg, scalar_type) {
    ga_workspace workspace;
    gmm::sub_interval Iu1(0, mf1.nb_dof()), Iu2(mf1.nb_dof(), mf2.nb_dof());
    workspace.add_fem_variable("u1", mf1, Iu1, U1);
    workspace.add_fem_variable("u2", mf2, Iu2, U2);
    workspace.add_expression("(Grad_u2-Grad_u1):(Grad_u2-Grad_u1)", mim, rg);
    workspace.assembly(0);
    return workspace.assembled_potential();
  }

  template<typename VEC1, typename VEC2, typename T>
  inline scalar_type asm_H1_semi_dist_sqr
  (const mesh_im &mim, const mesh_fem &mf1, const VEC1 &U1,
   const mesh_fem &mf2, const VEC2 &U2, mesh_region rg, std::complex<T>) {
    ga_workspace workspace;
    model_real_plain_vector UU1R(mf1.nb_dof()), UU2R(mf2.nb_dof());
    model_real_plain_vector UU1I(mf1.nb_dof()), UU2I(mf2.nb_dof());
    gmm::copy(gmm::real_part(U1), UU1R); gmm::copy(gmm::imag_part(U1), UU1I);
    gmm::copy(gmm::real_part(U2), UU2R); gmm::copy(gmm::imag_part(U2), UU2I);
    gmm::sub_interval Iu1r(0, mf1.nb_dof()), Iu2r(mf1.nb_dof(), mf2.nb_dof());
    gmm::sub_interval Iu1i(Iu2r.last(), mf1.nb_dof());
    gmm::sub_interval Iu2i(Iu1i.last(), mf2.nb_dof());
    workspace.add_fem_variable("u1", mf1, Iu1r, UU1R);
    workspace.add_fem_variable("u2", mf2, Iu2r, UU2R);
    workspace.add_fem_variable("v1", mf1, Iu1i, UU1I);
    workspace.add_fem_variable("v2", mf2, Iu2i, UU2I);
    workspace.add_expression("(Grad_u2-Grad_u1):(Grad_u2-Grad_u1)"
			     "+ (Grad_v2-Grad_v1):(Grad_v2-Grad_v1)", mim, rg);
    workspace.assembly(0);
    return workspace.assembled_potential();
  }

  /** 
      compute the H1 norm of U.
      @ingroup asm
  */

  /**
     compute @f$\|\nabla U\|_2@f$, U might be real or complex
     @ingroup asm
   */
  template<typename VEC>
  scalar_type asm_H1_norm
  (const mesh_im &mim, const mesh_fem &mf, const VEC &U,
   const mesh_region &rg = mesh_region::all_convexes()) {
    typedef typename gmm::linalg_traits<VEC>::value_type T;
    return sqrt(asm_H1_norm_sqr(mim, mf, U, rg, T()));
  }

  template<typename VEC, typename T>
  inline scalar_type asm_H1_norm_sqr
  (const mesh_im &mim, const mesh_fem &mf, const VEC &U,
   const mesh_region &rg_, T) {
    ga_workspace workspace;
    model_real_plain_vector UU(mf.nb_dof()); gmm::copy(U, UU);
    gmm::sub_interval Iu(0, mf.nb_dof());
    workspace.add_fem_variable("u", mf, Iu, UU);
    workspace.add_expression("u.u + Grad_u:Grad_u", mim, rg_);
    workspace.assembly(0);
    return workspace.assembled_potential();
  }

  inline scalar_type asm_H1_norm_sqr
  (const mesh_im &mim, const mesh_fem &mf, const model_real_plain_vector &U,
   const mesh_region &rg_, scalar_type) {
    ga_workspace workspace;
    gmm::sub_interval Iu(0, mf.nb_dof());
    workspace.add_fem_variable("u", mf, Iu, U);
    workspace.add_expression("u.u + Grad_u:Grad_u", mim, rg_);
    workspace.assembly(0);
    return workspace.assembled_potential();
  }
  

  template<typename VEC, typename T>
  inline scalar_type asm_H1_norm_sqr
  (const mesh_im &mim, const mesh_fem &mf, const VEC &U,
   const mesh_region &rg, std::complex<T>) {
    ga_workspace workspace;
    model_real_plain_vector UUR(mf.nb_dof()), UUI(mf.nb_dof());
    gmm::copy(gmm::real_part(U), UUR);
    gmm::copy(gmm::imag_part(U), UUI);
    gmm::sub_interval Iur(0, mf.nb_dof()), Iui(mf.nb_dof(), mf.nb_dof());
    workspace.add_fem_variable("u", mf, Iur, UUR);
    workspace.add_fem_variable("v", mf, Iui, UUI);
    workspace.add_expression("u.u+v.v + Grad_u:Grad_u+Grad_v:Grad_v", mim, rg);
    workspace.assembly(0);
    return workspace.assembled_potential();
  }
  
  /**
     Compute the H1 distance between U1 and U2
     @ingroup asm
   */
  template<typename VEC1, typename VEC2>
  inline scalar_type asm_H1_dist
  (const mesh_im &mim, const mesh_fem &mf1, const VEC1 &U1,
   const mesh_fem &mf2, const VEC2 &U2,
   mesh_region rg = mesh_region::all_convexes()) {
    return sqrt(asm_H1_dist_sqr(mim, mf1, U1, mf2, U2, rg,
			     typename gmm::linalg_traits<VEC1>::value_type()));
  }

  template<typename VEC1, typename VEC2, typename T>
  inline scalar_type asm_H1_dist_sqr
  (const mesh_im &mim, const mesh_fem &mf1, const VEC1 &U1,
   const mesh_fem &mf2, const VEC2 &U2, mesh_region rg, T) {
    ga_workspace workspace;
    model_real_plain_vector UU1(mf1.nb_dof()), UU2(mf2.nb_dof());
    gmm::copy(U1, UU1); gmm::copy(U2, UU2);
    gmm::sub_interval Iu1(0, mf1.nb_dof()), Iu2(mf1.nb_dof(), mf2.nb_dof());
    workspace.add_fem_variable("u1", mf1, Iu1, UU1);
    workspace.add_fem_variable("u2", mf2, Iu2, UU2);
    workspace.add_expression("(u2-u1).(u2-u1)"
			     "+ (Grad_u2-Grad_u1):(Grad_u2-Grad_u1)", mim, rg);
    workspace.assembly(0);
    return workspace.assembled_potential();
  }

  inline scalar_type asm_H1_dist_sqr
  (const mesh_im &mim, const mesh_fem &mf1, const  model_real_plain_vector &U1,
   const mesh_fem &mf2, const  model_real_plain_vector &U2, 
   mesh_region rg, scalar_type) {
    ga_workspace workspace;
    gmm::sub_interval Iu1(0, mf1.nb_dof()), Iu2(mf1.nb_dof(), mf2.nb_dof());
    workspace.add_fem_variable("u1", mf1, Iu1, U1);
    workspace.add_fem_variable("u2", mf2, Iu2, U2);
    workspace.add_expression("(u2-u1).(u2-u1)"
			     "+ (Grad_u2-Grad_u1):(Grad_u2-Grad_u1)", mim, rg);
    workspace.assembly(0);
    return workspace.assembled_potential();
  }

  template<typename VEC1, typename VEC2, typename T>
  inline scalar_type asm_H1_dist_sqr
  (const mesh_im &mim, const mesh_fem &mf1, const VEC1 &U1,
   const mesh_fem &mf2, const VEC2 &U2, mesh_region rg, std::complex<T>) {
    ga_workspace workspace;
    model_real_plain_vector UU1R(mf1.nb_dof()), UU2R(mf2.nb_dof());
    model_real_plain_vector UU1I(mf1.nb_dof()), UU2I(mf2.nb_dof());
    gmm::copy(gmm::real_part(U1), UU1R); gmm::copy(gmm::imag_part(U1), UU1I);
    gmm::copy(gmm::real_part(U2), UU2R); gmm::copy(gmm::imag_part(U2), UU2I);
    gmm::sub_interval Iu1r(0, mf1.nb_dof()), Iu2r(mf1.nb_dof(), mf2.nb_dof());
    gmm::sub_interval Iu1i(Iu2r.last(), mf1.nb_dof());
    gmm::sub_interval Iu2i(Iu1i.last(), mf2.nb_dof());
    workspace.add_fem_variable("u1", mf1, Iu1r, UU1R);
    workspace.add_fem_variable("u2", mf2, Iu2r, UU2R);
    workspace.add_fem_variable("v1", mf1, Iu1i, UU1I);
    workspace.add_fem_variable("v2", mf2, Iu2i, UU2I);
    workspace.add_expression("(u2-u1).(u2-u1) + (v2-v1).(v2-v1)"
			     "+ (Grad_u2-Grad_u1):(Grad_u2-Grad_u1)"
			     "+ (Grad_v2-Grad_v1):(Grad_v2-Grad_v1)", mim, rg);
    workspace.assembly(0);
    return workspace.assembled_potential();
  }

  /**
     compute @f$\|Hess U\|_2@f$, U might be real or complex. For C^1 elements
     @ingroup asm
  */
  
  template<typename VEC>
  scalar_type asm_H2_semi_norm
  (const mesh_im &mim, const mesh_fem &mf, const VEC &U,
   const mesh_region &rg = mesh_region::all_convexes()) {
    typedef typename gmm::linalg_traits<VEC>::value_type T;
    return sqrt(asm_H2_semi_norm_sqr(mim, mf, U, rg, T()));
  }

  template<typename VEC, typename T>
  inline scalar_type asm_H2_semi_norm_sqr
  (const mesh_im &mim, const mesh_fem &mf, const VEC &U,
   const mesh_region &rg_, T) {
    ga_workspace workspace;
    model_real_plain_vector UU(mf.nb_dof()); gmm::copy(U, UU);
    gmm::sub_interval Iu(0, mf.nb_dof());
    workspace.add_fem_variable("u", mf, Iu, UU);
    workspace.add_expression("Hess_u:Hess_u", mim, rg_);
    workspace.assembly(0);
    return workspace.assembled_potential();
  }

  inline scalar_type asm_H2_semi_norm_sqr
  (const mesh_im &mim, const mesh_fem &mf, const model_real_plain_vector &U,
   const mesh_region &rg_, scalar_type) {
    ga_workspace workspace;
    gmm::sub_interval Iu(0, mf.nb_dof());
    workspace.add_fem_variable("u", mf, Iu, U);
    workspace.add_expression("Hess_u:Hess_u", mim, rg_);
    workspace.assembly(0);
    return workspace.assembled_potential();
  }
  

  template<typename VEC, typename T>
  inline scalar_type asm_H2_semi_norm_sqr
  (const mesh_im &mim, const mesh_fem &mf, const VEC &U,
   const mesh_region &rg, std::complex<T>) {
    ga_workspace workspace;
    model_real_plain_vector UUR(mf.nb_dof()), UUI(mf.nb_dof());
    gmm::copy(gmm::real_part(U), UUR);
    gmm::copy(gmm::imag_part(U), UUI);
    gmm::sub_interval Iur(0, mf.nb_dof()), Iui(mf.nb_dof(), mf.nb_dof());
    workspace.add_fem_variable("u", mf, Iur, UUR);
    workspace.add_fem_variable("v", mf, Iui, UUI);
    workspace.add_expression("Hess_u:Hess_u + Hess_v:Hess_v", mim, rg);
    workspace.assembly(0);
    return workspace.assembled_potential();
  }


  template<typename VEC1, typename VEC2>
  inline scalar_type asm_H2_semi_dist
  (const mesh_im &mim, const mesh_fem &mf1, const VEC1 &U1,
   const mesh_fem &mf2, const VEC2 &U2,
   mesh_region rg = mesh_region::all_convexes()) {
    return sqrt(asm_H2_semi_dist_sqr(mim, mf1, U1, mf2, U2, rg,
			   typename gmm::linalg_traits<VEC1>::value_type()));
  }

  template<typename VEC1, typename VEC2, typename T>
  inline scalar_type asm_H2_semi_dist_sqr
  (const mesh_im &mim, const mesh_fem &mf1, const VEC1 &U1,
   const mesh_fem &mf2, const VEC2 &U2, mesh_region rg, T) {
    ga_workspace workspace;
    model_real_plain_vector UU1(mf1.nb_dof()), UU2(mf2.nb_dof());
    gmm::copy(U1, UU1); gmm::copy(U2, UU2);
    gmm::sub_interval Iu1(0, mf1.nb_dof()), Iu2(mf1.nb_dof(), mf2.nb_dof());
    workspace.add_fem_variable("u1", mf1, Iu1, UU1);
    workspace.add_fem_variable("u2", mf2, Iu2, UU2);
    workspace.add_expression("(Hess_u2-Hess_u1):(Hess_u2-Hess_u1)", mim, rg);
    workspace.assembly(0);
    return workspace.assembled_potential();
  }

  inline scalar_type asm_H2_semi_dist_sqr
  (const mesh_im &mim, const mesh_fem &mf1, const  model_real_plain_vector &U1,
   const mesh_fem &mf2, const model_real_plain_vector &U2, 
   mesh_region rg, scalar_type) {
    ga_workspace workspace;
    gmm::sub_interval Iu1(0, mf1.nb_dof()), Iu2(mf1.nb_dof(), mf2.nb_dof());
    workspace.add_fem_variable("u1", mf1, Iu1, U1);
    workspace.add_fem_variable("u2", mf2, Iu2, U2);
    workspace.add_expression("(Hess_u2-Hess_u1):(Hess_u2-Hess_u1)", mim, rg);
    workspace.assembly(0);
    return workspace.assembled_potential();
  }

  template<typename VEC1, typename VEC2, typename T>
  inline scalar_type asm_H2_semi_dist_sqr
  (const mesh_im &mim, const mesh_fem &mf1, const VEC1 &U1,
   const mesh_fem &mf2, const VEC2 &U2, mesh_region rg, std::complex<T>) {
    ga_workspace workspace;
    model_real_plain_vector UU1R(mf1.nb_dof()), UU2R(mf2.nb_dof());
    model_real_plain_vector UU1I(mf1.nb_dof()), UU2I(mf2.nb_dof());
    gmm::copy(gmm::real_part(U1), UU1R); gmm::copy(gmm::imag_part(U1), UU1I);
    gmm::copy(gmm::real_part(U2), UU2R); gmm::copy(gmm::imag_part(U2), UU2I);
    gmm::sub_interval Iu1r(0, mf1.nb_dof()), Iu2r(mf1.nb_dof(), mf2.nb_dof());
    gmm::sub_interval Iu1i(Iu2r.last(), mf1.nb_dof());
    gmm::sub_interval Iu2i(Iu1i.last(), mf2.nb_dof());
    workspace.add_fem_variable("u1", mf1, Iu1r, UU1R);
    workspace.add_fem_variable("u2", mf2, Iu2r, UU2R);
    workspace.add_fem_variable("v1", mf1, Iu1i, UU1I);
    workspace.add_fem_variable("v2", mf2, Iu2i, UU2I);
    workspace.add_expression("(Hess_u2-Hess_u1):(Hess_u2-Hess_u1)"
			     "+ (Hess_v2-Hess_v1):(Hess_v2-Hess_v1)", mim, rg);
    workspace.assembly(0);
    return workspace.assembled_potential();
  }

  /** 
      compute the H2 norm of U (for C^1 elements).
      @ingroup asm
  */
  template<typename VEC>
  scalar_type asm_H2_norm(const mesh_im &mim, const mesh_fem &mf,
			  const VEC &U,
			  const mesh_region &rg
			  = mesh_region::all_convexes()) {
    typedef typename gmm::linalg_traits<VEC>::value_type T;
    return sqrt(asm_H1_norm_sqr(mim, mf, U, rg, T())
		+ asm_H2_semi_norm_sqr(mim, mf, U, rg, T()));
  }
  
  /**
     Compute the H2 distance between U1 and U2
     @ingroup asm
   */
  template<typename VEC1, typename VEC2>
  scalar_type asm_H2_dist(const mesh_im &mim, 
			  const mesh_fem &mf1, const VEC1 &U1,
			  const mesh_fem &mf2, const VEC2 &U2,
			  const mesh_region &rg
			  = mesh_region::all_convexes()) {
    typedef typename gmm::linalg_traits<VEC1>::value_type T;
    return sqrt(asm_H1_dist_sqr(mim,mf1,U1,mf2,U2,rg,T()) +
		asm_H2_semi_dist_sqr(mim,mf1,U1,mf2,U2,rg,T()));
  }
 
  /** 
      generic mass matrix assembly (on the whole mesh or on the specified
      convex set or boundary) 
      @ingroup asm
  */
  template<typename MAT>
  inline void asm_mass_matrix
  (const MAT &M, const mesh_im &mim, const mesh_fem &mf1,
   const mesh_region &rg = mesh_region::all_convexes()) {

    ga_workspace workspace;
    gmm::sub_interval Iu1(0, mf1.nb_dof());
    base_vector u1(mf1.nb_dof());
    workspace.add_fem_variable("u1", mf1, Iu1, u1);
    workspace.add_expression("Test_u1.Test2_u1", mim, rg);
    workspace.assembly(2);
    gmm::add(workspace.assembled_matrix(), const_cast<MAT &>(M));
  }

  inline void asm_mass_matrix
  (model_real_sparse_matrix &M, const mesh_im &mim,
   const mesh_fem &mf1,
   const mesh_region &rg = mesh_region::all_convexes()) {
    ga_workspace workspace;
    gmm::sub_interval Iu1(0, mf1.nb_dof());
    base_vector u1(mf1.nb_dof());
    workspace.add_fem_variable("u1", mf1, Iu1, u1);
    workspace.add_expression("Test_u1.Test2_u1", mim, rg);
    workspace.set_assembled_matrix(M);
    workspace.assembly(2);
  }

  /** 
   *  generic mass matrix assembly (on the whole mesh or on the specified
   *  boundary) 
   */

  template<typename MAT>
  inline void asm_mass_matrix
  (const MAT &M, const mesh_im &mim, const mesh_fem &mf1, const mesh_fem &mf2,
   const mesh_region &rg = mesh_region::all_convexes()) {
    ga_workspace workspace;
    gmm::sub_interval Iu1(0, mf1.nb_dof()), Iu2(Iu1.last(), mf2.nb_dof());
    base_vector u1(mf1.nb_dof()), u2(mf2.nb_dof());
    workspace.add_fem_variable("u1", mf1, Iu1, u1);
    workspace.add_fem_variable("u2", mf2, Iu2, u2);
    workspace.add_expression("Test_u1.Test2_u2", mim, rg);
    workspace.assembly(2);
    gmm::add(gmm::sub_matrix(workspace.assembled_matrix(), Iu1, Iu2),
	     const_cast<MAT &>(M));
  }
  
  inline void asm_mass_matrix
  (model_real_sparse_matrix &M, const mesh_im &mim,
   const mesh_fem &mf1, const mesh_fem &mf2,
   const mesh_region &rg = mesh_region::all_convexes()) {
    ga_workspace workspace;
    gmm::sub_interval Iu1(0, mf1.nb_dof()), Iu2(0, mf2.nb_dof());
    base_vector u1(mf1.nb_dof()), u2(mf2.nb_dof());
    workspace.add_fem_variable("u1", mf1, Iu1, u1);
    workspace.add_fem_variable("u2", mf2, Iu2, u2);
    workspace.add_expression("Test_u1.Test2_u2", mim, rg);
    workspace.set_assembled_matrix(M);
    workspace.assembly(2);
  }

  /**
     generic mass matrix assembly with an additional parameter
     (on the whole mesh or on the specified boundary) 
     @ingroup asm
  */
  template<typename MAT, typename VECT>
  inline void asm_mass_matrix_param
  (const MAT &M, const mesh_im &mim, const mesh_fem &mf1, const mesh_fem &mf2,
   const mesh_fem &mf_data, const VECT &A,
   const mesh_region &rg = mesh_region::all_convexes()) {

    ga_workspace workspace;
    gmm::sub_interval Iu1(0, mf1.nb_dof()), Iu2(Iu1.last(), mf2.nb_dof());
    base_vector u1(mf1.nb_dof()), u2(mf2.nb_dof()), AA(mf_data.nb_dof());
    gmm::copy(A, AA);
    workspace.add_fem_variable("u1", mf1, Iu1, u1);
    workspace.add_fem_variable("u2", mf2, Iu2, u2);
    workspace.add_fem_constant("A", mf_data, AA);
    workspace.add_expression("(A*Test_u1).Test2_u2", mim, rg);
    workspace.assembly(2);
    gmm::add(gmm::sub_matrix(workspace.assembled_matrix(), Iu1, Iu2),
	     const_cast<MAT &>(M));
  }

  inline void asm_mass_matrix_param
  (model_real_sparse_matrix &M, const mesh_im &mim,
   const mesh_fem &mf1, const mesh_fem &mf2,
   const mesh_fem &mf_data, const model_real_plain_vector &A,
   const mesh_region &rg = mesh_region::all_convexes()) {

    ga_workspace workspace;
    gmm::sub_interval Iu1(0, mf1.nb_dof()), Iu2(0, mf2.nb_dof());
    base_vector u1(mf1.nb_dof()), u2(mf2.nb_dof());
    workspace.add_fem_variable("u1", mf1, Iu1, u1);
    workspace.add_fem_variable("u2", mf2, Iu2, u2);
    workspace.add_fem_constant("A", mf_data, A);
    workspace.add_expression("(A*Test_u1).Test2_u2", mim, rg);
    workspace.set_assembled_matrix(M);
    workspace.assembly(2);
  }
    
  /*
    assembly of a matrix with 1 parameter (real or complex)
    (the most common here for the assembly routines below)
  */
  template <typename MAT, typename VECT>
  inline void asm_real_or_complex_1_param_mat
  (MAT &M, const mesh_im &mim, const mesh_fem &mf_u, const mesh_fem *mf_data,
   const VECT &A, const mesh_region &rg, const char *assembly_description) {
    asm_real_or_complex_1_param_mat_
      (M, mim, mf_u, mf_data, A, rg, assembly_description,
       typename gmm::linalg_traits<VECT>::value_type());
  }

  /* real version */
  template<typename MAT, typename VECT, typename T>
  inline void asm_real_or_complex_1_param_mat_
  (const MAT &M, const mesh_im &mim,  const mesh_fem &mf_u,
   const mesh_fem *mf_data, const VECT &A,  const mesh_region &rg,
   const char *assembly_description, T) {
    // generic_assembly assem(assembly_description);
    // assem.push_mi(mim);
    // assem.push_mf(mf_u);
    // assem.push_mf(mf_data);
    // if (mf_mult) assem.push_mf(*mf_mult);
    // assem.push_data(A);
    // assem.push_mat_or_vec(const_cast<MAT&>(M));
    // assem.assembly(rg);

    ga_workspace workspace;
    gmm::sub_interval Iu(0, mf_u.nb_dof());
    base_vector u(mf_u.nb_dof()), AA(gmm::vect_size(A));
    gmm::copy(A, AA);
    workspace.add_fem_variable("u", mf_u, Iu, u);
    if (mf_data)
      workspace.add_fem_constant("A", *mf_data, AA);
    else
      workspace.add_fixed_size_constant("A", AA);
    workspace.add_expression(assembly_description, mim, rg);
    workspace.assembly(2);
    gmm::add(workspace.assembled_matrix(), const_cast<MAT &>(M));
  }

  inline void asm_real_or_complex_1_param_mat_
  (model_real_sparse_matrix &M, const mesh_im &mim,  const mesh_fem &mf_u,
   const mesh_fem *mf_data, const model_real_plain_vector &A,
   const mesh_region &rg,
   const char *assembly_description, scalar_type) {
    ga_workspace workspace;
    gmm::sub_interval Iu(0, mf_u.nb_dof());
    base_vector u(mf_u.nb_dof());
    workspace.add_fem_variable("u", mf_u, Iu, u);
    if (mf_data)
      workspace.add_fem_constant("A", *mf_data, A);
    else
      workspace.add_fixed_size_constant("A", A);
    workspace.add_expression(assembly_description, mim, rg);
    workspace.set_assembled_matrix(M);
    workspace.assembly(2);
  }

  /* complex version */
  template<typename MAT, typename VECT, typename T>
  inline void asm_real_or_complex_1_param_mat_
  (MAT &M, const mesh_im &mim, const mesh_fem &mf_u, const mesh_fem *mf_data,
   const VECT &A, const mesh_region &rg,const char *assembly_description,
   std::complex<T>) {
    asm_real_or_complex_1_param_mat_(gmm::real_part(M),mim,mf_u,mf_data,
					 gmm::real_part(A),rg,
					 assembly_description, T());
    asm_real_or_complex_1_param_mat_(gmm::imag_part(M),mim,mf_u,mf_data,
					 gmm::imag_part(A),rg,
					 assembly_description, T());
  }

  /*
    assembly of a vector with 1 parameter (real or complex)
    (the most common here for the assembly routines below)
  */
  template <typename MAT, typename VECT>
  inline void asm_real_or_complex_1_param_vec
  (MAT &M, const mesh_im &mim, const mesh_fem &mf_u, const mesh_fem *mf_data,
   const VECT &A, const mesh_region &rg, const char *assembly_description) {
    asm_real_or_complex_1_param_vec_
      (M, mim, mf_u, mf_data, A, rg, assembly_description,
       typename gmm::linalg_traits<VECT>::value_type());
  }

  /* real version */
  template<typename VECTA, typename VECT, typename T>
  inline void asm_real_or_complex_1_param_vec_
  (const VECTA &V, const mesh_im &mim,  const mesh_fem &mf_u,
   const mesh_fem *mf_data, const VECT &A,  const mesh_region &rg,
   const char *assembly_description, T) {
    ga_workspace workspace;
    gmm::sub_interval Iu(0, mf_u.nb_dof());
    base_vector u(mf_u.nb_dof()), AA(gmm::vect_size(A));
    gmm::copy(A, AA);
    workspace.add_fem_variable("u", mf_u, Iu, u);
    if (mf_data)
      workspace.add_fem_constant("A", *mf_data, AA);
    else
      workspace.add_fixed_size_constant("A", AA);
    workspace.add_expression(assembly_description, mim, rg);
    workspace.assembly(1);
    gmm::add(workspace.assembled_vector(), const_cast<VECTA &>(V));
  }

  inline void asm_real_or_complex_1_param_vec_
  (model_real_plain_vector &V, const mesh_im &mim,  const mesh_fem &mf_u,
   const mesh_fem *mf_data, const model_real_plain_vector &A,
   const mesh_region &rg,
   const char *assembly_description, scalar_type) {
    ga_workspace workspace;
    gmm::sub_interval Iu(0, mf_u.nb_dof());
    base_vector u(mf_u.nb_dof());
    workspace.add_fem_variable("u", mf_u, Iu, u);
    if (mf_data)
      workspace.add_fem_constant("A", *mf_data, A);
    else
      workspace.add_fixed_size_constant("A", A);
    workspace.add_expression(assembly_description, mim, rg);
    workspace.set_assembled_vector(V);
    workspace.assembly(1);
  }

  /* complex version */
  template<typename MAT, typename VECT, typename T>
  inline void asm_real_or_complex_1_param_vec_
  (MAT &M, const mesh_im &mim, const mesh_fem &mf_u, const mesh_fem *mf_data,
   const VECT &A, const mesh_region &rg,const char *assembly_description,
   std::complex<T>) {
    asm_real_or_complex_1_param_vec_(gmm::real_part(M),mim,mf_u,mf_data,
					 gmm::real_part(A),rg,
					 assembly_description, T());
    asm_real_or_complex_1_param_vec_(gmm::imag_part(M),mim,mf_u,mf_data,
					 gmm::imag_part(A),rg,
					 assembly_description, T());
  }

  /** 
     generic mass matrix assembly with an additional parameter
     (on the whole mesh or on the specified boundary) 
     @ingroup asm
   */
  template<typename MAT, typename VECT>
  void asm_mass_matrix_param
  (MAT &M, const mesh_im &mim, const mesh_fem &mf_u, const mesh_fem &mf_data,
   const VECT &F, const mesh_region &rg = mesh_region::all_convexes()) {
    asm_real_or_complex_1_param_mat
      (M, mim, mf_u, &mf_data, F, rg, "(A*Test_u):Test2_u");
  }
    
  /** 
      source term (for both volumic sources and boundary (Neumann) sources).
      @ingroup asm
   */
  template<typename VECT1, typename VECT2>
  void asm_source_term(const VECT1 &B, const mesh_im &mim, const mesh_fem &mf,
		       const mesh_fem &mf_data, const VECT2 &F,
		       const mesh_region &rg = mesh_region::all_convexes()) {
    GMM_ASSERT1(mf_data.get_qdim() == 1 ||
		mf_data.get_qdim() == mf.get_qdim(),
		"invalid data mesh fem (same Qdim or Qdim=1 required)");
    asm_real_or_complex_1_param_vec
      (const_cast<VECT1 &>(B), mim, mf, &mf_data, F, rg, "A:Test_u");
  }

  /** 
      source term (for both volumic sources and boundary (Neumann) sources).
      For an homogeneous term.
      @ingroup asm
   */
  template<typename VECT1, typename VECT2>
  void asm_homogeneous_source_term(const VECT1 &B, const mesh_im &mim,
				   const mesh_fem &mf, const VECT2 &F,
		       const mesh_region &rg = mesh_region::all_convexes()) {
    // const char *st;
    // if (mf.get_qdim() == 1)
    //   st = "F=data(1); V(#1)+=comp(Base(#1))(:).F(i);";
    // else
    //   st = "F=data(qdim(#1)); V(#1)+=comp(vBase(#1))(:,i).F(i);";
    
    // asm_real_or_complex_1_param(const_cast<VECT1 &>(B),mim,mf,mf,F,rg,st);


    asm_real_or_complex_1_param_vec
      (const_cast<VECT1 &>(B), mim, mf, 0, F, rg, "A:Test_u");
  }






  // -------- Before this : cleaned ----------






  /*
    assembly of a matrix with 1 parameter (real or complex)
    (the most common here for the assembly routines below)
  */
  template <typename MAT, typename VECT>
  void asm_real_or_complex_1_param
  (MAT &M, const mesh_im &mim, const mesh_fem &mf_u, const mesh_fem &mf_data,
   const VECT &A, const mesh_region &rg, const char *assembly_description,
   const mesh_fem *mf_mult = 0) {
    asm_real_or_complex_1_param_
      (M, mim, mf_u, mf_data, A, rg, assembly_description, mf_mult,
       typename gmm::linalg_traits<VECT>::value_type());
  }

  /* real version */
  template<typename MAT, typename VECT, typename T>
  void asm_real_or_complex_1_param_
  (const MAT &M, const mesh_im &mim,  const mesh_fem &mf_u,
   const mesh_fem &mf_data, const VECT &A,  const mesh_region &rg,
   const char *assembly_description, const mesh_fem *mf_mult, T) {
    generic_assembly assem(assembly_description);
    assem.push_mi(mim);
    assem.push_mf(mf_u);
    assem.push_mf(mf_data);
    if (mf_mult) assem.push_mf(*mf_mult);
    assem.push_data(A);
    assem.push_mat_or_vec(const_cast<MAT&>(M));
    assem.assembly(rg);
  }

  /* complex version */
  template<typename MAT, typename VECT, typename T>
  void asm_real_or_complex_1_param_
  (MAT &M, const mesh_im &mim, const mesh_fem &mf_u, const mesh_fem &mf_data,
   const VECT &A, const mesh_region &rg,const char *assembly_description,
   const mesh_fem *mf_mult, std::complex<T>) {
    asm_real_or_complex_1_param_(gmm::real_part(M),mim,mf_u,mf_data,
				 gmm::real_part(A),rg,
				 assembly_description, mf_mult, T());
    asm_real_or_complex_1_param_(gmm::imag_part(M),mim,mf_u,mf_data,
				 gmm::imag_part(A),rg,
				 assembly_description, mf_mult, T());
  }



  

  /** 
      Normal source term (for boundary (Neumann) condition).
      @ingroup asm
   */
  template<typename VECT1, typename VECT2>
  void asm_normal_source_term(VECT1 &B, const mesh_im &mim,
			      const mesh_fem &mf,
			      const mesh_fem &mf_data, const VECT2 &F,
			      const mesh_region &rg) {
    GMM_ASSERT1(mf_data.get_qdim() == 1 ||
		mf_data.get_qdim() == mf.get_qdim(),
		"invalid data mesh_fem (same Qdim or Qdim=1 required)");

    const char *st;
    if (mf.get_qdim() == 1)
      st = "F=data(mdim(#1),#2);"
	"V(#1)+=comp(Base(#1).Base(#2).Normal())(:,j,k).F(k,j);";
    else if (mf_data.get_qdim() == 1)
      st = "F=data(qdim(#1),mdim(#1),#2);"
	"V(#1)+=comp(vBase(#1).Base(#2).Normal())(:,i,j,k).F(i,k,j);";
    else
      st = "F=data(mdim(#1),#2);"
	"V(#1)+=comp(vBase(#1).vBase(#2).Normal())(:,i,j,i,k).F(k,j);";

    asm_real_or_complex_1_param(B, mim, mf, mf_data, F, rg, st);

    // asm_real_or_complex_1_param_vec(B, mim, mf, mf_data, F, rg,
    //          			    "(A*Normal):Test_u);
  }

  /** 
      Homogeneous normal source term (for boundary (Neumann) condition).
      @ingroup asm
   */
  template<typename VECT1, typename VECT2>
  void asm_homogeneous_normal_source_term(VECT1 &B, const mesh_im &mim,
					  const mesh_fem &mf,
					  const VECT2 &F,
					  const mesh_region &rg) {
    const char *st;
    if (mf.get_qdim() == 1)
      st = "F=data(mdim(#1));"
	"V(#1)+=comp(Base(#1).Normal())(:,k).F(k);";
    else
      st = "F=data(qdim(#1),mdim(#1));"
	"V(#1)+=comp(vBase(#1).Normal())(:,i,j).F(i,j);";

    asm_real_or_complex_1_param(B, mim, mf, mf, F, rg, st);
  }

  template <typename V> bool is_Q_symmetric(const V& Q, size_type q,
					    size_type nbd) {
    /* detect the symmetricity of Q (in that case the symmetricity of
     * the final matrix will be ensured, and computations will be
     * slightly speed up */
    for (size_type k=0; k < nbd; ++k)
      for (size_type i=1; i < q; ++i)
	for (size_type j=0; j < i; ++j)
	  if (Q[k*q*q+i*q+j] != Q[k*q*q+j*q+i])
	    return false;
    return true;
  }

  /**
     assembly of @f$\int{qu.v}@f$

     (if @f$u@f$ is a vector field of size @f$N@f$, @f$q@f$ is a square
     matrix @f$N\times N@f$ used by assem_general_boundary_conditions

     convention: Q is of the form 
     Q1_11 Q2_11 ..... Qn_11
     Q1_21 Q2_21 ..... Qn_21
     Q1_12 Q2_12 ..... Qn_12
     Q1_22 Q2_22 ..... Qn_22
     if  N = 2, and mf_d has n/N degree of freedom

     Q is a vector, so the matrix is assumed to be stored by columns
     (fortran style)

     Works for both volumic assembly and boundary assembly
     @ingroup asm
  */
  template<typename MAT, typename VECT>
  void asm_qu_term(MAT &M, const mesh_im &mim, const mesh_fem &mf_u, 
		   const mesh_fem &mf_d, const VECT &Q, 
		   const mesh_region &rg) {
    generic_assembly assem;
    GMM_ASSERT1(mf_d.get_qdim() == 1,
		"invalid data mesh fem (Qdim=1 required)");
    const char *asm_str = "";
    if (mf_u.get_qdim() == 1)
      asm_str = "Q=data$1(#2);"
	"M(#1,#1)+=comp(Base(#1).Base(#1).Base(#2))(:,:,k).Q(k);";
    else
      if (is_Q_symmetric(Q,mf_u.get_qdim(),mf_d.nb_dof()))
	asm_str = "Q=data$1(qdim(#1),qdim(#1),#2);"
		  "M(#1,#1)+=sym(comp(vBase(#1).vBase(#1).Base(#2))"
		  "(:,i,:,j,k).Q(i,j,k));";
      else
        asm_str = "Q=data$1(qdim(#1),qdim(#1),#2);"
		  "M(#1,#1)+=comp(vBase(#1).vBase(#1).Base(#2))"
		  "(:,i,:,j,k).Q(i,j,k);";
    asm_real_or_complex_1_param(M, mim, mf_u, mf_d, Q, rg, asm_str);
  }

  template<typename MAT, typename VECT>
  void asm_homogeneous_qu_term(MAT &M, const mesh_im &mim,
			       const mesh_fem &mf_u, const VECT &Q, 
			       const mesh_region &rg) {
    generic_assembly assem;
    const char *asm_str = "";
    if (mf_u.get_qdim() == 1)
      asm_str = "Q=data$1(1);"
	"M(#1,#1)+=comp(Base(#1).Base(#1))(:,:).Q(i);";
    else
      if (is_Q_symmetric(Q,mf_u.get_qdim(),1))
	asm_str = "Q=data$1(qdim(#1),qdim(#1));"
		  "M(#1,#1)+=sym(comp(vBase(#1).vBase(#1))"
		  "(:,i,:,j).Q(i,j));";
      else
        asm_str = "Q=data$1(qdim(#1),qdim(#1));"
		  "M(#1,#1)+=comp(vBase(#1).vBase(#1))"
		  "(:,i,:,j).Q(i,j);";
    asm_real_or_complex_1_param(M, mim, mf_u, mf_u, Q, rg, asm_str);
  }

  /** 
      Stiffness matrix for linear elasticity, with Lam� coefficients
      @ingroup asm
  */
  template<class MAT, class VECT>
  void asm_stiffness_matrix_for_linear_elasticity
  (const MAT &RM_, const mesh_im &mim, const mesh_fem &mf,
   const mesh_fem &mf_data, const VECT &LAMBDA, const VECT &MU,
   const mesh_region &rg = mesh_region::all_convexes()) {
    MAT &RM = const_cast<MAT &>(RM_);
    GMM_ASSERT1(mf_data.get_qdim() == 1,
		"invalid data mesh fem (Qdim=1 required)");
    
    GMM_ASSERT1(mf.get_qdim() == mf.linked_mesh().dim(),
		"wrong qdim for the mesh_fem");
    /* e = strain tensor,
       M = 2*mu*e(u):e(v) + lambda*tr(e(u))*tr(e(v))
    */
    generic_assembly assem("lambda=data$1(#2); mu=data$2(#2);"
			   "t=comp(vGrad(#1).vGrad(#1).Base(#2));"
			   //"e=(t{:,2,3,:,5,6,:}+t{:,3,2,:,5,6,:}"
			   //"+t{:,2,3,:,6,5,:}+t{:,3,2,:,6,5,:})/4;"
			   //"e=(t{:,2,3,:,5,6,:}+t{:,3,2,:,5,6,:})*0.5;"
			   /*"M(#1,#1)+= sym(2*e(:,i,j,:,i,j,k).mu(k)"
                             " + e(:,i,i,:,j,j,k).lambda(k))");*/
                           "M(#1,#1)+= sym(t(:,i,j,:,i,j,k).mu(k)"
			   "+ t(:,j,i,:,i,j,k).mu(k)"
			   "+ t(:,i,i,:,j,j,k).lambda(k))");
    assem.push_mi(mim);
    assem.push_mf(mf);
    assem.push_mf(mf_data);
    assem.push_data(LAMBDA);
    assem.push_data(MU);
    assem.push_mat(RM);
    assem.assembly(rg);
  }


  /** 
      Stiffness matrix for linear elasticity, with constant Lam� coefficients
      @ingroup asm
  */
  template<class MAT, class VECT>
  void asm_stiffness_matrix_for_homogeneous_linear_elasticity
  (const MAT &RM_, const mesh_im &mim, const mesh_fem &mf,
   const VECT &LAMBDA, const VECT &MU,
   const mesh_region &rg = mesh_region::all_convexes()) {
    MAT &RM = const_cast<MAT &>(RM_);
    GMM_ASSERT1(mf.get_qdim() == mf.linked_mesh().dim(),
		"wrong qdim for the mesh_fem");
    generic_assembly assem("lambda=data$1(1); mu=data$2(1);"
			   "t=comp(vGrad(#1).vGrad(#1));"
                           "M(#1,#1)+= sym(t(:,i,j,:,i,j).mu(1)"
			   "+ t(:,j,i,:,i,j).mu(1)"
			   "+ t(:,i,i,:,j,j).lambda(1))");
    assem.push_mi(mim);
    assem.push_mf(mf);
    assem.push_data(LAMBDA);
    assem.push_data(MU);
    assem.push_mat(RM);
    assem.assembly(rg);
  }

  /** 
      Stiffness matrix for linear elasticity, with a general Hooke
      tensor. This is more a demonstration of generic assembly than
      something useful !  

      Note that this function is just an alias for
      asm_stiffness_matrix_for_vector_elliptic.

      @ingroup asm
  */
  template<typename MAT, typename VECT> void
  asm_stiffness_matrix_for_linear_elasticity_Hooke
  (MAT &RM, const mesh_im &mim, const mesh_fem &mf, const mesh_fem &mf_data, 
   const VECT &H, const mesh_region &rg = mesh_region::all_convexes()) {
    asm_stiffness_matrix_for_vector_elliptic(RM, mim, mf, mf_data, H, rg);
  }


  /** two-in-one assembly of stokes equation:
     linear elasticty part and p.div(v) term are assembled at the
     same time. 

     @ingroup asm
   */
  template<typename MAT, typename VECT>
  void asm_stokes(MAT &K, MAT &BT, 
		  const mesh_im &mim, 
		  const mesh_fem &mf_u, const mesh_fem &mf_p,
		  const mesh_fem &mf_d, const VECT &viscos,
		  const mesh_region &rg = mesh_region::all_convexes()) {
    GMM_ASSERT1(mf_d.get_qdim() == 1,
		"invalid data mesh fem (Qdim=1 required)");
    generic_assembly assem("visc=data$1(#3); "
			   "t=comp(vGrad(#1).vGrad(#1).Base(#3));"
			   "e=(t{:,2,3,:,5,6,:}+t{:,3,2,:,5,6,:}"
			   "  +t{:,2,3,:,6,5,:}+t{:,3,2,:,6,5,:})/4;"
			   // visc*D(u):D(v)
			   "M$1(#1,#1)+=sym(e(:,i,j,:,i,j,k).visc(k));"
			   // p.div v
			   "M$2(#1,#2)+=comp(vGrad(#1).Base(#2))(:,i,i,:);");
    assem.push_mi(mim);
    assem.push_mf(mf_u);
    assem.push_mf(mf_p);
    assem.push_mf(mf_d);
    assem.push_data(viscos);
    assem.push_mat(K);
    assem.push_mat(BT);
    assem.assembly(rg);
  }

  /**
     Build the mixed pressure term @f$ B = - \int p.div u @f$

     @ingroup asm
  */
     
  template<typename MAT>
  void asm_stokes_B(MAT &B, const mesh_im &mim, const mesh_fem &mf_u,
		    const mesh_fem &mf_p, 
		    const mesh_region &rg = mesh_region::all_convexes()) {
    GMM_ASSERT1(mf_p.get_qdim() == 1,
		"invalid data mesh fem (Qdim=1 required)");
    //generic_assembly assem("M$1(#1,#2)+=comp(vGrad(#1).Base(#2))(:,i,i,:);");
    generic_assembly assem("M$1(#1,#2)+=-comp(Base(#1).vGrad(#2))(:,:,i,i);");
    assem.push_mi(mim);
    assem.push_mf(mf_p);
    assem.push_mf(mf_u);
    assem.push_mat(B);
    assem.assembly(rg);
  }

  /**
     assembly of @f$\int_\Omega \nabla u.\nabla v@f$.

     @ingroup asm
   */
  template<typename MAT>
  void asm_stiffness_matrix_for_homogeneous_laplacian
  (const MAT &M_, const mesh_im &mim, const mesh_fem &mf,
   const mesh_region &rg = mesh_region::all_convexes()) {
    MAT &M = const_cast<MAT &>(M_);
    generic_assembly 
      assem("M$1(#1,#1)+=sym(comp(Grad(#1).Grad(#1))(:,i,:,i))");
    assem.push_mi(mim);
    assem.push_mf(mf);
    assem.push_mat(M);
    assem.assembly(rg);
  }


  /**
     assembly of @f$\int_\Omega \nabla u.\nabla v@f$.
     @ingroup asm
   */
  template<typename MAT>
  void asm_stiffness_matrix_for_homogeneous_laplacian_componentwise
  (const MAT &M_, const mesh_im &mim, const mesh_fem &mf, 
   const mesh_region &rg = mesh_region::all_convexes()) {
    MAT &M = const_cast<MAT &>(M_);
     generic_assembly
       assem("M$1(#1,#1)+="
	     "sym(comp(vGrad(#1).vGrad(#1))(:,k,i,:,k,i))");
    assem.push_mi(mim);
    assem.push_mf(mf);
    assem.push_mat(M);
    assem.assembly(rg);
  }

  /**
     assembly of @f$\int_\Omega a(x)\nabla u.\nabla v@f$ , where @f$a(x)@f$
     is scalar.
     @ingroup asm
   */
  template<typename MAT, typename VECT>
  void asm_stiffness_matrix_for_laplacian
  (MAT &M, const mesh_im &mim, const mesh_fem &mf, const mesh_fem &mf_data,
   const VECT &A, const mesh_region &rg = mesh_region::all_convexes()) {
    GMM_ASSERT1(mf_data.get_qdim() == 1, 
		"invalid data mesh fem (Qdim=1 required)");
    asm_real_or_complex_1_param
      (M, mim, mf, mf_data, A, rg, "a=data$1(#2); M$1(#1,#1)+="
       "sym(comp(Grad(#1).Grad(#1).Base(#2))(:,i,:,i,j).a(j))");
  }
  


  /** The same as getfem::asm_stiffness_matrix_for_laplacian , but on
      each component of mf when mf has a qdim > 1
  */
  template<typename MAT, typename VECT>
  void asm_stiffness_matrix_for_laplacian_componentwise
  (MAT &M, const mesh_im &mim, const mesh_fem &mf, const mesh_fem &mf_data,
   const VECT &A, const mesh_region &rg = mesh_region::all_convexes()) {
    GMM_ASSERT1(mf_data.get_qdim() == 1,
		"invalid data mesh fem (Qdim=1 required)");
    asm_real_or_complex_1_param
      (M, mim, mf, mf_data, A, rg, "a=data$1(#2); M$1(#1,#1)+="
       "sym(comp(vGrad(#1).vGrad(#1).Base(#2))(:,k,i,:,k,i,j).a(j))");
  }

  /**
     assembly of @f$\int_\Omega A(x)\nabla u.\nabla v@f$, where @f$A(x)@f$
     is a (symmetric positive definite) NxN matrix.
     Arguments:
     @param M a sparse matrix of dimensions mf.nb_dof() x mf.nb_dof()

     @param mim the mesh_im.

     @param mf : the mesh_fem that describes the solution, with
     @c mf.get_qdim() == @c N.

     @param mf_data the mesh_fem that describes the coefficients of @c A
     (@c mf_data.get_qdim() == 1).

     @param A a (very large) vector, which is a flattened (n x n x
     mf_data.nb_dof()) 3D array. For each dof of mf_data, it contains
     the n x n coefficients of @f$A@f$. As usual, the order is the
     "fortran-order", i.e. @c A = [A_11(dof1) A_21(dof1) A_31(dof1)
     A_12(dof1) A_22(dof1) ... A_33(dof) A_11(dof2)
     .... A_33(lastdof)]

     @ingroup asm
  */
  template<typename MAT, typename VECT>
  void asm_stiffness_matrix_for_scalar_elliptic
  (MAT &M, const mesh_im &mim, const mesh_fem &mf, const mesh_fem &mf_data,
   const VECT &A, const mesh_region &rg = mesh_region::all_convexes()) {
    /*GMM_ASSERT1(mf_data.get_qdim() == 1,
      "invalid data mesh fem (Qdim=1 required)");*/
    asm_real_or_complex_1_param(M,mim,mf,mf_data,A,rg,
				"a=data$1(mdim(#1),mdim(#1),#2);"
				"M$1(#1,#1)+=comp(Grad(#1).Grad(#1).Base(#2))"
				"(:,i,:,j,k).a(j,i,k)");
  }

  /** The same but with a constant matrix
   */
  template<typename MAT, typename VECT>
  void asm_stiffness_matrix_for_homogeneous_scalar_elliptic
  (MAT &M, const mesh_im &mim, const mesh_fem &mf,
   const VECT &A, const mesh_region &rg = mesh_region::all_convexes()) {
    /*GMM_ASSERT1(mf_data.get_qdim() == 1,
      "invalid data mesh fem (Qdim=1 required)");*/
    asm_real_or_complex_1_param(M,mim,mf,mf,A,rg,
				"a=data$1(mdim(#1),mdim(#1));"
				"M$1(#1,#1)+=comp(Grad(#1).Grad(#1))"
				"(:,i,:,j).a(j,i)");
  }

  /** The same but on each component of mf when mf has a qdim > 1 
   */
  template<typename MAT, typename VECT>
  void asm_stiffness_matrix_for_scalar_elliptic_componentwise
  (MAT &M, const mesh_im &mim, const mesh_fem &mf,
   const mesh_fem &mf_data, const VECT &A, 
   const mesh_region &rg = mesh_region::all_convexes()) {
    /* GMM_ASSERT1(mf_data.get_qdim() == 1,
       "invalid data mesh fem (Qdim=1 required)");*/
    asm_real_or_complex_1_param
      (M,mim,mf,mf_data,A,rg, "a=data$1(mdim(#1),mdim(#1),#2);"
       "M$1(#1,#1)+=comp(vGrad(#1).vGrad(#1).Base(#2))"
       "(:,l,i,:,l,j,k).a(j,i,k)");
  }

  /** The same but with a constant matrix 
   */
  template<typename MAT, typename VECT>
  void asm_stiffness_matrix_for_homogeneous_scalar_elliptic_componentwise
  (MAT &M, const mesh_im &mim, const mesh_fem &mf, const VECT &A, 
   const mesh_region &rg = mesh_region::all_convexes()) {
    /* GMM_ASSERT1(mf_data.get_qdim() == 1,
       "invalid data mesh fem (Qdim=1 required)");*/
    asm_real_or_complex_1_param
      (M,mim,mf,mf,A,rg, "a=data$1(mdim(#1),mdim(#1));"
       "M$1(#1,#1)+=comp(vGrad(#1).vGrad(#1))"
       "(:,l,i,:,l,j).a(j,i)");
  }


  /**
     Assembly of @f$\int_\Omega A(x)\nabla u.\nabla v@f$, where @f$A(x)@f$
     is a NxNxNxN (symmetric positive definite) tensor defined on mf_data.
  */
  template<typename MAT, typename VECT> void
  asm_stiffness_matrix_for_vector_elliptic
  (MAT &M, const mesh_im &mim, const mesh_fem &mf, const mesh_fem &mf_data, 
   const VECT &A, const mesh_region &rg = mesh_region::all_convexes()) {
    /* GMM_ASSERT1(mf_data.get_qdim() == 1,
       "invalid data mesh fem (Qdim=1 required)");*/
    /* 
       M = a_{i,j,k,l}D_{i,j}(u)D_{k,l}(v)
    */
    asm_real_or_complex_1_param
      (M,mim,mf,mf_data,A,rg, 
       "a=data$1(qdim(#1),mdim(#1),qdim(#1),mdim(#1),#2);"
       "t=comp(vGrad(#1).vGrad(#1).Base(#2));"
       "M(#1,#1)+= t(:,i,j,:,k,l,p).a(i,j,k,l,p)");
  }

  /**
     Assembly of @f$\int_\Omega A(x)\nabla u.\nabla v@f$, where @f$A(x)@f$
     is a NxNxNxN (symmetric positive definite) constant tensor.
  */
  template<typename MAT, typename VECT> void
  asm_stiffness_matrix_for_homogeneous_vector_elliptic
  (MAT &M, const mesh_im &mim, const mesh_fem &mf,
   const VECT &A, const mesh_region &rg = mesh_region::all_convexes()) {
    /* 
       M = a_{i,j,k,l}D_{i,j}(u)D_{k,l}(v)
    */
    asm_real_or_complex_1_param
      (M,mim,mf,mf,A,rg, 
       "a=data$1(qdim(#1),mdim(#1),qdim(#1),mdim(#1));"
       "t=comp(vGrad(#1).vGrad(#1));"
       "M(#1,#1)+= t(:,i,j,:,k,l).a(i,j,k,l)");
  }



  /** 
      assembly of the term @f$\int_\Omega Kuv - \nabla u.\nabla v@f$, 
      for the helmholtz equation (@f$\Delta u + k^2u = 0@f$, with @f$K=k^2@f$).

      The argument K_squared may be a real or a complex-valued vector.

     @ingroup asm
  */
  template<typename MAT, typename VECT>
  void asm_Helmholtz(MAT &M, const mesh_im &mim, const mesh_fem &mf_u,
		     const mesh_fem &mf_data, const VECT &K_squared, 
		     const mesh_region &rg = mesh_region::all_convexes()) {
    asm_Helmholtz(M, mim, mf_u, mf_data, K_squared,rg,
		  typename gmm::linalg_traits<VECT>::value_type());
  }

  template<typename MAT, typename VECT, typename T>
  void asm_Helmholtz(MAT &M, const mesh_im &mim, const mesh_fem &mf_u,
		     const mesh_fem &mf_data,
		     const VECT &K_squared, const mesh_region &rg, T) {
    asm_Helmholtz_real(M, mim, mf_u, mf_data, K_squared, rg);
  }

  template<typename MAT, typename VECT, typename T>
  void asm_Helmholtz(MAT &M, const mesh_im &mim, const mesh_fem &mf_u,
		     const mesh_fem &mf_data, const VECT &K_squared,
		     const mesh_region &rg, std::complex<T>) {
    asm_Helmholtz_cplx(gmm::real_part(M), gmm::imag_part(M), mim, mf_u,
		       mf_data, gmm::real_part(K_squared),
		       gmm::imag_part(K_squared), rg);
  }


  template<typename MATr, typename MATi, typename VECTr, typename VECTi>  
  void asm_Helmholtz_cplx(const MATr &Mr, const MATi &Mi, const mesh_im &mim,
			  const mesh_fem &mf_u, const mesh_fem &mf_data,
			  const VECTr &K_squaredr, const VECTi &K_squaredi, 
			  const mesh_region &rg=mesh_region::all_convexes()) {
    generic_assembly assem("Kr=data$1(#2); Ki=data$2(#2);"
			   "m = comp(Base(#1).Base(#1).Base(#2)); "
			   "M$1(#1,#1)+=sym(m(:,:,i).Kr(i) - "
			   "comp(Grad(#1).Grad(#1))(:,i,:,i));"
			   "M$2(#1,#1)+=sym(m(:,:,i).Ki(i));");
    assem.push_mi(mim);
    assem.push_mf(mf_u);
    assem.push_mf(mf_data);
    assem.push_data(K_squaredr); //gmm::real_part(K_squared));
    assem.push_data(K_squaredi); //gmm::imag_part(K_squared));
    assem.push_mat(const_cast<MATr&>(Mr));
    assem.push_mat(const_cast<MATi&>(Mi));
    assem.assembly(rg);
  }

  template<typename MAT, typename VECT>  
  void asm_Helmholtz_real(const MAT &M, const mesh_im &mim,
			  const mesh_fem &mf_u, const mesh_fem &mf_data,
			  const VECT &K_squared, 
			  const mesh_region &rg=mesh_region::all_convexes()) {
    generic_assembly assem("K=data$1(#2);"
			   "m = comp(Base(#1).Base(#1).Base(#2)); "
			   "M$1(#1,#1)+=sym(m(:,:,i).K(i) - "
			   "comp(Grad(#1).Grad(#1))(:,i,:,i));");
    assem.push_mi(mim);
    assem.push_mf(mf_u);
    assem.push_mf(mf_data);
    assem.push_data(K_squared);
    assem.push_mat(const_cast<MAT&>(M));
    assem.assembly(rg);
  }


  /** 
      assembly of the term @f$\int_\Omega Kuv - \nabla u.\nabla v@f$, 
      for the helmholtz equation (@f$\Delta u + k^2u = 0@f$, with @f$K=k^2@f$).

      The argument K_squared may be a real or a complex-valued scalar.

     @ingroup asm
  */
  template<typename MAT, typename VECT>
  void asm_homogeneous_Helmholtz
  (MAT &M, const mesh_im &mim, const mesh_fem &mf_u, const VECT &K_squared, 
   const mesh_region &rg = mesh_region::all_convexes()) {
    asm_homogeneous_Helmholtz(M, mim, mf_u, K_squared, rg,
		  typename gmm::linalg_traits<VECT>::value_type());
  }

  template<typename MAT, typename VECT, typename T>
  void asm_homogeneous_Helmholtz(MAT &M, const mesh_im &mim,
				 const mesh_fem &mf_u,
				 const VECT &K_squared,
				 const mesh_region &rg, T) {
    asm_homogeneous_Helmholtz_real(M, mim, mf_u, K_squared, rg);
  }

  template<typename MAT, typename VECT, typename T>
  void asm_homogeneous_Helmholtz(MAT &M, const mesh_im &mim,
				 const mesh_fem &mf_u,
				 const VECT &K_squared,
				 const mesh_region &rg, std::complex<T>) {
    asm_homogeneous_Helmholtz_cplx(gmm::real_part(M),
				   gmm::imag_part(M), mim, mf_u,
				   gmm::real_part(K_squared),
				   gmm::imag_part(K_squared), rg);
  }


  template<typename MATr, typename MATi, typename VECTr, typename VECTi>  
  void asm_homogeneous_Helmholtz_cplx(const MATr &Mr, const MATi &Mi,
				      const mesh_im &mim,
				      const mesh_fem &mf_u,
				      const VECTr &K_squaredr,
				      const VECTi &K_squaredi, 
				      const mesh_region &rg) {
    generic_assembly assem("Kr=data$1(1); Ki=data$2(1);"
			   "m = comp(Base(#1).Base(#1)); "
			   "M$1(#1,#1)+=sym(m(:,:).Kr(j) - "
			   "comp(Grad(#1).Grad(#1))(:,i,:,i));"
			   "M$2(#1,#1)+=sym(m(:,:).Ki(j));");
    assem.push_mi(mim);
    assem.push_mf(mf_u);
    assem.push_data(K_squaredr);
    assem.push_data(K_squaredi);
    assem.push_mat(const_cast<MATr&>(Mr));
    assem.push_mat(const_cast<MATi&>(Mi));
    assem.assembly(rg);
  }

  template<typename MAT, typename VECT>  
  void asm_homogeneous_Helmholtz_real(const MAT &M, const mesh_im &mim,
				      const mesh_fem &mf_u,
				      const VECT &K_squared, 
				      const mesh_region &rg) {
    generic_assembly assem("K=data(1);"
			   "m = comp(Base(#1).Base(#1)); "
			   "M$1(#1,#1)+=sym(m(:,:).K(j) - "
			   "comp(Grad(#1).Grad(#1))(:,i,:,i));");
    assem.push_mi(mim);
    assem.push_mf(mf_u);
    assem.push_data(K_squared);
    assem.push_mat(const_cast<MAT&>(M));
    assem.assembly(rg);
  }

  enum { ASMDIR_BUILDH = 1, ASMDIR_BUILDR = 2, ASMDIR_SIMPLIFY = 4,
	 ASMDIR_BUILDALL = 7 };

  /**
     Assembly of Dirichlet constraints @f$ u(x) = r(x) @f$ in a weak form
     @f[ \int_{\Gamma} u(x)v(x) = \int_{\Gamma} r(x)v(x) \forall v@f],
     where @f$ v @f$ is in
     the space of multipliers corresponding to mf_mult.

     size(r_data) = Q   * nb_dof(mf_rh);

     A simplification can be done when the fem for u and r are the same and
     when the fem for the multipliers is of same dimension as the one for u.
     version = |ASMDIR_BUILDH : build H
     |ASMDIR_BUILDR : build R
     |ASMDIR_SIMPLIFY : simplify
     |ASMDIR_BUILDALL : do everything.

     @ingroup asm
  */

  template<typename MAT, typename VECT1, typename VECT2>
  void asm_dirichlet_constraints
  (MAT &H, VECT1 &R, const mesh_im &mim, const mesh_fem &mf_u,
   const mesh_fem &mf_mult, const mesh_fem &mf_r,
   const VECT2 &r_data, const mesh_region &region,
   int version =  ASMDIR_BUILDALL) {
    typedef typename gmm::linalg_traits<VECT1>::value_type value_type;
    // typedef typename gmm::number_traits<value_type>::magnitude_type magn_type;

    if ((version & ASMDIR_SIMPLIFY) &&
	(mf_u.is_reduced() || mf_mult.is_reduced() || mf_r.is_reduced())) {
      GMM_WARNING1("Sorry, no simplification for reduced fems");
      version = (version & (ASMDIR_BUILDR | ASMDIR_BUILDH));
    }      
    
    region.from_mesh(mim.linked_mesh()).error_if_not_faces();
    GMM_ASSERT1(mf_r.get_qdim() == 1,
		"invalid data mesh fem (Qdim=1 required)");
    if (version & ASMDIR_BUILDH) {
      asm_mass_matrix(H, mim, mf_mult, mf_u, region);
//       gmm::clean(H, gmm::default_tol(magn_type()) // � remettre !
// 		 * gmm::mat_maxnorm(H) * magn_type(1000));
    }
    if (version & ASMDIR_BUILDR)
      asm_source_term(R, mim, mf_mult, mf_r, r_data, region);

    return; // � enlever !

    // Verifications and simplifications

    pfem pf_u, pf_r, pf_m;
    bool warning_msg1 = false, warning_msg2 = false;
    dal::bit_vector simplifiable_dofs, nonsimplifiable_dofs;
    std::vector<size_type> simplifiable_indices(mf_mult.nb_basic_dof());
    std::vector<value_type> simplifiable_values(mf_mult.nb_basic_dof());
    std::vector<value_type> v1, v2, v3;

    for (mr_visitor v(region); !v.finished(); v.next()) {
      GMM_ASSERT1(v.is_face(), "attempt to impose a dirichlet "
		  "on the interior of the domain!");
      size_type cv = v.cv(); short_type f = v.f();

      GMM_ASSERT1(mf_u.convex_index().is_in(cv) &&
		  mf_r.convex_index().is_in(cv) &&
		  mf_mult.convex_index().is_in(cv), 
		  "attempt to impose a dirichlet "
		  "condition on a convex with no FEM!");
      pf_u = mf_u.fem_of_element(cv); 
      pf_r = mf_r.fem_of_element(cv);
      pf_m = mf_mult.fem_of_element(cv);

      if (!pf_m->is_lagrange() && !warning_msg1) {
	GMM_WARNING3("Dirichlet condition with non-lagrange multiplier fem. "
		     "see the documentation about Dirichlet conditions.");
	warning_msg1 = true;
      }
      
      if (!(version & ASMDIR_SIMPLIFY)) continue;
      
      mesh_fem::ind_dof_face_ct pf_u_ct
	= mf_u.ind_basic_dof_of_face_of_element(cv, f);
      mesh_fem::ind_dof_face_ct pf_r_ct
	= mf_r.ind_basic_dof_of_face_of_element(cv, f);
      mesh_fem::ind_dof_face_ct pf_m_ct
	= mf_mult.ind_basic_dof_of_face_of_element(cv, f);
      
      size_type pf_u_nbdf = pf_u_ct.size();
      size_type pf_m_nbdf = pf_m_ct.size();
      size_type pf_u_nbdf_loc = pf_u->structure(cv)->nb_points_of_face(f);
      size_type pf_m_nbdf_loc = pf_m->structure(cv)->nb_points_of_face(f);
      // size_type pf_r_nbdf_loc = pf_r->structure(cv)->nb_points_of_face(f);

      if (pf_u_nbdf < pf_m_nbdf && !warning_msg2) {
	GMM_WARNING2("Dirichlet condition with a too rich multiplier fem. "
		     "see the documentation about Dirichlet conditions.");
	warning_msg2 = true;
      }
      
      if (pf_u != pf_r || pf_u_nbdf != pf_m_nbdf || 
	  ((pf_u != pf_r) && (pf_u_nbdf_loc != pf_m_nbdf_loc))) { 
	for (size_type i = 0; i < pf_m_nbdf; ++i)
	  nonsimplifiable_dofs.add(pf_m_ct[i]);
	continue;
      }
      
      for (size_type i = 0; i < pf_m_nbdf; ++i) {
	simplifiable_dofs.add(pf_m_ct[i]);
	simplifiable_indices[pf_m_ct[i]] = pf_u_ct[i];
      }

      if (!(version & ASMDIR_BUILDR)) continue;

      if (pf_u == pf_r) { // simplest simplification.
	size_type Qratio = mf_u.get_qdim() / mf_r.get_qdim();
	for (size_type i = 0; i < pf_m_nbdf; ++i) {
	  simplifiable_values[pf_m_ct[i]]
	    = r_data[pf_r_ct[i/Qratio]*Qratio+(i%Qratio)];
	}
      }
    }
    
    if (version & ASMDIR_SIMPLIFY) {
      if (simplifiable_dofs.card() > 0)
	{ GMM_TRACE3("Simplification of the Dirichlet condition"); }
      else
	GMM_TRACE3("Sorry, no simplification of the Dirichlet condition");
      if (nonsimplifiable_dofs.card() > 0 && simplifiable_dofs.card() > 0)
	GMM_WARNING3("Partial simplification of the Dirichlet condition");

      for (dal::bv_visitor i(simplifiable_dofs); !i.finished(); ++i)
	if (!(nonsimplifiable_dofs[i])) {
	  if (version & ASMDIR_BUILDH) {  /* "erase" the row i */
	    const mesh::ind_cv_ct &cv_ct = mf_mult.convex_to_basic_dof(i);
	    for (size_type j = 0; j < cv_ct.size(); ++j) {
	      size_type cv = cv_ct[j];
	      for (size_type k=0; k < mf_u.nb_basic_dof_of_element(cv); ++k)
		H(i, mf_u.ind_basic_dof_of_element(cv)[k]) = value_type(0);
	    }
	    H(i, simplifiable_indices[i]) = value_type(1);
	  }
	  if (version & ASMDIR_BUILDR) R[i] = simplifiable_values[i];
	}
    }
  }



    /**
     Assembly of Dirichlet constraints on the normal component of a
     vector field: u(x)n = r(x) (where n is the outward unit normal)
     in a weak form
     @f[ \int_{\Gamma} (u(x)n)v(x) = \int_{\Gamma} r(x)v(x) \forall v@f],
     where @f$ v @f$ is in the space of multipliers corresponding to
     mf_mult.

     size(r_data) = Q   * nb_dof(mf_rh) or Q * N * nb_dof(mf_rh);

     In the case size(r_data) = Q * N * nb_dof(mf_rh), the right hand
     side is @f[ \int_{\Gamma} (r(x).n(x))v(x) \forall v@f]

     version = |ASMDIR_BUILDH : build H
     |ASMDIR_BUILDR : build R
     |ASMDIR_BUILDALL : do everything.

     @ingroup asm
  */

  template<typename MAT, typename VECT1, typename VECT2>
  void asm_normal_component_dirichlet_constraints
  (MAT &H, VECT1 &R, const mesh_im &mim, const mesh_fem &mf_u,
   const mesh_fem &mf_mult, const mesh_fem &mf_r,
   const VECT2 &r_data, const mesh_region &region,
   int version =  ASMDIR_BUILDALL) {
    typedef typename gmm::linalg_traits<VECT1>::value_type value_type;
    typedef typename gmm::number_traits<value_type>::magnitude_type magn_type;
    size_type N = mf_u.linked_mesh().dim(), Q = mf_mult.get_qdim();
    
    region.from_mesh(mim.linked_mesh()).error_if_not_faces();
    GMM_ASSERT1(mf_mult.get_qdim() == mf_u.get_qdim() / N,
		"invalid mesh fem for the normal component Dirichlet "
		"constraint (Qdim=" << mf_u.get_qdim() / N << " required)");
    if (version & ASMDIR_BUILDH) {
      generic_assembly assem;
      if (Q == 1)
	assem.set("M(#2,#1)+=comp(Base(#2).vBase(#1).Normal())(:,:,i,i);");
      else
	assem.set("M(#2,#1)+=comp(vBase(#2).mBase(#1).Normal())(:,i,:,i,j,j);");
      assem.push_mi(mim);
      assem.push_mf(mf_u);
      assem.push_mf(mf_mult);
      assem.push_mat(H);
      assem.assembly(region);
    }
    if (version & ASMDIR_BUILDR) {
      if (gmm::vect_size(r_data) == mf_r.nb_dof() * Q)
	asm_source_term(R, mim, mf_mult, mf_r, r_data, region);
      else if (gmm::vect_size(r_data) == mf_r.nb_dof() * Q * N)
	asm_normal_source_term(R, mim, mf_mult, mf_r, r_data, region);
      else GMM_ASSERT1(false, "Wrong size of data vector");
    }
    gmm::clean(H, gmm::default_tol(magn_type())
	       * gmm::mat_maxnorm(H) * magn_type(100));
  }

  /**
     Assembly of generalized Dirichlet constraints h(x)u(x) = r(x),
     where h is a QxQ matrix field (Q == mf_u.get_qdim()), outputs a
     (under-determined) linear system HU=R.

     size(h_data) = Q^2 * nb_dof(mf_rh);
     size(r_data) = Q   * nb_dof(mf_rh);

     This function tries hard to make H diagonal or mostly diagonal:
     this function is able to "simplify" the dirichlet constraints (see below)
     version = |ASMDIR_BUILDH : build H
     |ASMDIR_BUILDR : build R
     |ASMDIR_SIMPLIFY : simplify
     |ASMDIR_BUILDALL : do everything.

     @ingroup asm
  */

  template<typename MAT, typename VECT1, typename VECT2, typename VECT3>
  void asm_generalized_dirichlet_constraints
  (MAT &H, VECT1 &R, const mesh_im &mim, const mesh_fem &mf_u,
   const mesh_fem &mf_h, const mesh_fem &mf_r, const VECT2 &h_data,
   const VECT3 &r_data, const mesh_region &region,
   int version =  ASMDIR_BUILDALL) {
    pfem pf_u, pf_rh;

    if ((version & ASMDIR_SIMPLIFY) &&
	(mf_u.is_reduced() || mf_h.is_reduced() || mf_r.is_reduced())) {
      GMM_WARNING1("Sorry, no simplification for reduced fems");
      version = (version & ASMDIR_BUILDR);
    }

    region.from_mesh(mim.linked_mesh()).error_if_not_faces();
    GMM_ASSERT1(mf_h.get_qdim() == 1 && mf_r.get_qdim() == 1,
		"invalid data mesh fem (Qdim=1 required)");
    if (version & ASMDIR_BUILDH) {
      asm_qu_term(H, mim, mf_u, mf_h, h_data, region);
      std::vector<size_type> ind(0);
      dal::bit_vector bdof = mf_u.basic_dof_on_region(region);
      // gmm::clean(H, 1E-15 * gmm::mat_maxnorm(H));
      for (size_type i = 0; i < mf_u.nb_dof(); ++i)
	if (!(bdof[i])) ind.push_back(i);
      gmm::clear(gmm::sub_matrix(H, gmm::sub_index(ind)));
    }
    if (version & ASMDIR_BUILDR)
      asm_source_term(R, mim, mf_u, mf_r, r_data, region);
    if (!(version & ASMDIR_SIMPLIFY)) return;

    /* step 2 : simplification of simple dirichlet conditions */
    if (&mf_r == &mf_h) {
      for (mr_visitor v(region); !v.finished(); v.next()) {
	size_type cv = v.cv();
	short_type f = v.f();

	GMM_ASSERT1(mf_u.convex_index().is_in(cv) &&
		    mf_r.convex_index().is_in(cv), 
		    "attempt to impose a dirichlet "
		    "condition on a convex with no FEM!");

	if (f >= mf_u.linked_mesh().structure_of_convex(cv)->nb_faces())
	  continue;
	pf_u = mf_u.fem_of_element(cv); 
	pf_rh = mf_r.fem_of_element(cv);
	/* don't try anything with vector elements */
	if (mf_u.fem_of_element(cv)->target_dim() != 1) continue;
	bgeot::pconvex_structure cvs_u = pf_u->structure(cv);
	bgeot::pconvex_structure cvs_rh = pf_rh->structure(cv);
	for (size_type i = 0; i < cvs_u->nb_points_of_face(f); ++i) {
	  
	  size_type Q = mf_u.get_qdim();  // pf_u->target_dim() (==1)
	  
	  size_type ind_u = cvs_u->ind_points_of_face(f)[i];
	  pdof_description tdof_u = pf_u->dof_types()[ind_u];
	  
	  for (size_type j = 0; j < cvs_rh->nb_points_of_face(f); ++j) {
	    size_type ind_rh = cvs_rh->ind_points_of_face(f)[j];
	    pdof_description tdof_rh = pf_rh->dof_types()[ind_rh];
	    /*
	      same kind of dof and same location of dof ? 
	      => then the previous was not useful for this dofs (introducing
	      a mass matrix which is not diagonal in the constraints matrix)
	      -> the constraint is simplified:
	      we replace \int{(H_j.psi_j)*phi_i}=\int{R_j.psi_j} (sum over j)
	      with             H_j*phi_i = R_j     
	      --> Le principe peut �tre faux : non identique � la projection
	      L^2 et peut entrer en conccurence avec les autres ddl -> a revoir
	    */
	    if (tdof_u == tdof_rh &&
		gmm::vect_dist2_sqr((*(pf_u->node_tab(cv)))[ind_u], 
				    (*(pf_rh->node_tab(cv)))[ind_rh])
		< 1.0E-14) {
	      /* the dof might be "duplicated" */
	      for (size_type q = 0; q < Q; ++q) {
		size_type dof_u = mf_u.ind_basic_dof_of_element(cv)[ind_u*Q + q];
		
		/* "erase" the row */
		if (version & ASMDIR_BUILDH)
		  for (size_type k=0; k < mf_u.nb_basic_dof_of_element(cv); ++k)
		    H(dof_u, mf_u.ind_basic_dof_of_element(cv)[k]) = 0.0;
		
		size_type dof_rh = mf_r.ind_basic_dof_of_element(cv)[ind_rh];
		/* set the "simplified" row */
		if (version & ASMDIR_BUILDH)
		  for (unsigned jj=0; jj < Q; jj++) {
		    size_type dof_u2
		      = mf_u.ind_basic_dof_of_element(cv)[ind_u*Q+jj];
		    H(dof_u, dof_u2) = h_data[(jj*Q+q) + Q*Q*(dof_rh)];
		  }
		if (version & ASMDIR_BUILDR) R[dof_u] = r_data[dof_rh*Q+q];
	      }
	    }
	  }
	}
      }
    }
  }

  /** 
      Build an orthogonal basis of the kernel of H in NS, gives the
      solution of minimal norm of H*U = R in U0 and return the
      dimension of the kernel. The function is based on a
      Gramm-Schmidt algorithm.

      @ingroup asm
  */
  template<typename MAT1, typename MAT2, typename VECT1, typename VECT2>
  size_type Dirichlet_nullspace(const MAT1 &H, MAT2 &NS,
				const VECT1 &R, VECT2 &U0) {

    // To be finalized.
    //  . In order to be used with any sparse matrix type
    //  . transpose the result and give the effective dimension of the kernel
    //  . Compute the ctes / H.
    //  . Optimization (suppress temporary ...). 
    //  . Verify sizes of data

    typedef typename gmm::linalg_traits<MAT1>::value_type T;
    typedef typename gmm::number_traits<T>::magnitude_type MAGT;
    typedef typename gmm::temporary_vector<MAT1>::vector_type TEMP_VECT;
    MAGT tol = gmm::default_tol(MAGT());
    MAGT norminfH = gmm::mat_maxnorm(H);
    size_type nbd = gmm::mat_ncols(H), nbase = 0, nbr = gmm::mat_nrows(H);
    TEMP_VECT aux(nbr), e(nbd), f(nbd);
    dal::dynamic_array<TEMP_VECT> base_img;
    dal::dynamic_array<TEMP_VECT> base_img_inv;
    size_type nb_bimg = 0;
    gmm::clear(NS);

    if (!(gmm::is_col_matrix(H)))
      GMM_WARNING2("Dirichlet_nullspace inefficient for a row matrix H");
    // First, detection of null columns of H, and already orthogonals 
    // vectors of the image of H.
    dal::bit_vector nn;
    for (size_type i = 0; i < nbd; ++i) {
      gmm::clear(e); e[i] = T(1);
      gmm::mult(H, e, aux);
      MAGT n = gmm::vect_norm2(aux);

      if (n < norminfH*tol*1000) {
	NS(i, nbase++) = T(1); nn[i] = true;
      }
      else {
	bool good = true;
	for (size_type j = 0; j < nb_bimg; ++j)
	  if (gmm::abs(gmm::vect_sp(aux, base_img[j])) > MAGT(0))
	    { good = false; break; }
	if (good) {
	  gmm::copy(e, f);
	  gmm::scale(f, T(MAGT(1)/n)); gmm::scale(aux, T(MAGT(1)/n));
	  base_img_inv[nb_bimg] = TEMP_VECT(nbd);
	  gmm::copy(f, base_img_inv[nb_bimg]);
	  gmm::clean(aux, gmm::vect_norminf(aux)*tol);
	  base_img[nb_bimg] = TEMP_VECT(nbr);
	  gmm::copy(aux, base_img[nb_bimg++]);
	  nn[i] = true;
	}
      }
    }
    size_type nb_triv_base = nbase;

    for (size_type i = 0; i < nbd; ++i) {
      if (!(nn[i])) {
	gmm::clear(e); e[i] = T(1); gmm::clear(f); f[i] = T(1);
	gmm::mult(H, e, aux);
	for (size_type j = 0; j < nb_bimg; ++j) { 
	  T c = gmm::vect_sp(aux, base_img[j]);
	  // if (gmm::abs(c > 1.0E-6) { // � scaler sur l'ensemble de H ...
	  if (c != T(0)) {
	    gmm::add(gmm::scaled(base_img[j], -c), aux);
	    gmm::add(gmm::scaled(base_img_inv[j], -c), f);
	  }
	}
	if (gmm::vect_norm2(aux) < norminfH*tol*MAGT(10000)) {
	  gmm::copy(f, gmm::mat_col(NS, nbase++));
	}
	else {
	  MAGT n = gmm::vect_norm2(aux);
	  gmm::scale(f, T(MAGT(1)/n)); gmm::scale(aux, T(MAGT(1)/n));
	  gmm::clean(f, tol*gmm::vect_norminf(f));
	  gmm::clean(aux, tol*gmm::vect_norminf(aux));
	  base_img_inv[nb_bimg] = TEMP_VECT(nbd);
      	  gmm::copy(f, base_img_inv[nb_bimg]);
	  base_img[nb_bimg] = TEMP_VECT(nbr);
	  gmm::copy(aux, base_img[nb_bimg++]);
	}
      }
    }
    // Compute a solution in U0
    gmm::clear(U0);
    for (size_type i = 0; i < nb_bimg; ++i) {
      T c = gmm::vect_sp(base_img[i], R);
      gmm::add(gmm::scaled(base_img_inv[i], c), U0);
    }
    // Orthogonalisation of the basis of the kernel of H.
    for (size_type i = nb_triv_base; i < nbase; ++i) {
      for (size_type j = nb_triv_base; j < i; ++j) {
	T c = gmm::vect_sp(gmm::mat_col(NS,i), gmm::mat_col(NS,j));
	if (c != T(0))
	  gmm::add(gmm::scaled(gmm::mat_col(NS,j), -c), gmm::mat_col(NS,i));
      }
      gmm::scale(gmm::mat_col(NS,i),
		 T(1) / gmm::vect_norm2(gmm::mat_col(NS,i)));
    }
    // projection of U0 on the orthogonal to the kernel.
    for (size_type j = nb_triv_base; j < nbase; ++j) {
      T c = gmm::vect_sp(gmm::mat_col(NS,j), U0);
      if (c != T(0))
	gmm::add(gmm::scaled(gmm::mat_col(NS,j), -c), U0);
    }
    // Test ...
    gmm::mult(H, U0, gmm::scaled(R, T(-1)), aux);
    if (gmm::vect_norm2(aux) > gmm::vect_norm2(U0)*tol*MAGT(10000))
      GMM_WARNING2("Dirichlet condition not well inverted: residu="
		  << gmm::vect_norm2(aux));
    
    return nbase;
  }

}  /* end of namespace getfem.                                             */


#endif /* GETFEM_ASSEMBLING_H__  */
