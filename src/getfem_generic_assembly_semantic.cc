/*===========================================================================

 Copyright (C) 2013-2018 Yves Renard

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

===========================================================================*/

// Semantic analysis of assembly trees and semantic manipulations.


#include <getfem/getfem_generic_assembly_functions_and_operators.h>
#include <getfem/getfem_generic_assembly_semantic.h>
#include <getfem/getfem_generic_assembly_compile_and_exec.h>

namespace getfem {

  extern bool predef_operators_nonlinear_elasticity_initialized;
  extern bool predef_operators_plasticity_initialized;
  extern bool predef_operators_contact_initialized;

  bool ga_extract_variables(const pga_tree_node pnode,
			    const ga_workspace &workspace,
			    const mesh &m,
			    std::set<var_trans_pair> &vars,
			    bool ignore_data) {
    bool expand_groups = !ignore_data;
    bool found_var = false;
    if (pnode->node_type == GA_NODE_VAL ||
        pnode->node_type == GA_NODE_GRAD ||
        pnode->node_type == GA_NODE_HESS ||
        pnode->node_type == GA_NODE_DIVERG ||
        pnode->node_type == GA_NODE_INTERPOLATE_VAL ||
        pnode->node_type == GA_NODE_INTERPOLATE_GRAD ||
        pnode->node_type == GA_NODE_INTERPOLATE_HESS ||
        pnode->node_type == GA_NODE_INTERPOLATE_DIVERG ||
        pnode->node_type == GA_NODE_ELEMENTARY_VAL ||
        pnode->node_type == GA_NODE_ELEMENTARY_GRAD ||
        pnode->node_type == GA_NODE_ELEMENTARY_HESS ||
        pnode->node_type == GA_NODE_ELEMENTARY_DIVERG ||
        pnode->node_type == GA_NODE_XFEM_PLUS_VAL ||
        pnode->node_type == GA_NODE_XFEM_PLUS_GRAD ||
        pnode->node_type == GA_NODE_XFEM_PLUS_HESS ||
        pnode->node_type == GA_NODE_XFEM_PLUS_DIVERG ||
        pnode->node_type == GA_NODE_XFEM_MINUS_VAL ||
        pnode->node_type == GA_NODE_XFEM_MINUS_GRAD ||
        pnode->node_type == GA_NODE_XFEM_MINUS_HESS ||
        pnode->node_type == GA_NODE_XFEM_MINUS_DIVERG) {
      bool group = workspace.variable_group_exists(pnode->name);
      bool iscte = (!group) && workspace.is_constant(pnode->name);
      if (!iscte) found_var = true;
      if (!ignore_data || !iscte) {
        if (group && expand_groups) {
          for (const std::string &t : workspace.variable_group(pnode->name))
            vars.insert(var_trans_pair(t, pnode->interpolate_name));
        } else
          vars.insert(var_trans_pair(pnode->name, pnode->interpolate_name));
      }
    }
    if (pnode->node_type == GA_NODE_INTERPOLATE_VAL ||
        pnode->node_type == GA_NODE_INTERPOLATE_GRAD ||
        pnode->node_type == GA_NODE_INTERPOLATE_HESS ||
        pnode->node_type == GA_NODE_INTERPOLATE_DIVERG ||
        pnode->node_type == GA_NODE_INTERPOLATE_VAL_TEST ||
        pnode->node_type == GA_NODE_INTERPOLATE_GRAD_TEST ||
        pnode->node_type == GA_NODE_INTERPOLATE_HESS_TEST ||
        pnode->node_type == GA_NODE_INTERPOLATE_DIVERG_TEST ||
        pnode->node_type == GA_NODE_INTERPOLATE_X ||
        pnode->node_type == GA_NODE_INTERPOLATE_NORMAL) {
      workspace.interpolate_transformation(pnode->interpolate_name)
        ->extract_variables(workspace, vars, ignore_data, m,
                            pnode->interpolate_name);
    }
    for (auto&& child : pnode->children)
      found_var = ga_extract_variables(child, workspace, m,
                                       vars, ignore_data)
        || found_var;
    return found_var;
  }


  static bool ga_node_mark_tree_for_variable
  (pga_tree_node pnode, const ga_workspace &workspace, const mesh &m,
   const std::string &varname,
   const std::string &interpolatename) {
    bool marked = false;
    for (size_type i = 0; i < pnode->children.size(); ++i)
      if (ga_node_mark_tree_for_variable(pnode->children[i], workspace, m,
                                         varname, interpolatename))
        marked = true;

    bool plain_node(pnode->node_type == GA_NODE_VAL ||
                    pnode->node_type == GA_NODE_GRAD ||
                    pnode->node_type == GA_NODE_HESS ||
                    pnode->node_type == GA_NODE_DIVERG);
    bool interpolate_node(pnode->node_type == GA_NODE_INTERPOLATE_VAL ||
                          pnode->node_type == GA_NODE_INTERPOLATE_GRAD ||
                          pnode->node_type == GA_NODE_INTERPOLATE_HESS ||
                          pnode->node_type == GA_NODE_INTERPOLATE_DIVERG);
    bool elementary_node(pnode->node_type == GA_NODE_ELEMENTARY_VAL ||
                         pnode->node_type == GA_NODE_ELEMENTARY_GRAD ||
                         pnode->node_type == GA_NODE_ELEMENTARY_HESS ||
                         pnode->node_type == GA_NODE_ELEMENTARY_DIVERG);
    bool xfem_node(pnode->node_type == GA_NODE_XFEM_PLUS_VAL ||
                   pnode->node_type == GA_NODE_XFEM_PLUS_GRAD ||
                   pnode->node_type == GA_NODE_XFEM_PLUS_HESS ||
                   pnode->node_type == GA_NODE_XFEM_PLUS_DIVERG ||
                   pnode->node_type == GA_NODE_XFEM_MINUS_VAL ||
                   pnode->node_type == GA_NODE_XFEM_MINUS_GRAD ||
                   pnode->node_type == GA_NODE_XFEM_MINUS_HESS ||
                   pnode->node_type == GA_NODE_XFEM_MINUS_DIVERG);
    bool interpolate_test_node
      (pnode->node_type == GA_NODE_INTERPOLATE_VAL_TEST ||
       pnode->node_type == GA_NODE_INTERPOLATE_GRAD_TEST ||
       pnode->node_type == GA_NODE_INTERPOLATE_HESS_TEST ||
       pnode->node_type == GA_NODE_INTERPOLATE_DIVERG_TEST);

    if ((plain_node || interpolate_node || elementary_node || xfem_node) &&
        (pnode->name.compare(varname) == 0 &&
         pnode->interpolate_name.compare(interpolatename) == 0)) marked = true;

    if (interpolate_node || interpolate_test_node ||
        pnode->node_type == GA_NODE_INTERPOLATE_X ||
        pnode->node_type == GA_NODE_INTERPOLATE_NORMAL) {
      std::set<var_trans_pair> vars;
      workspace.interpolate_transformation(pnode->interpolate_name)
        ->extract_variables(workspace, vars, true,
                            m, pnode->interpolate_name);
      for (std::set<var_trans_pair>::iterator it=vars.begin();
           it != vars.end(); ++it) {
        if (it->varname.compare(varname) == 0 &&
            it->transname.compare(interpolatename) == 0) marked = true;
      }
    }
    pnode->marked = marked;
    return marked;
  }

  static void ga_node_derivation
  (ga_tree &tree, const ga_workspace &workspace, const mesh &m,
   pga_tree_node pnode, const std::string &varname,
   const std::string &interpolatename, size_type order);
  
  //=========================================================================
  // Some hash code functions for node identification
  //=========================================================================

  static scalar_type ga_hash_code(const std::string &s) {
    scalar_type c(0);
    for (size_type i = 0; i < s.size(); ++i)
      c += sin(M_E+scalar_type(s[i]))+M_PI*M_E*scalar_type(i+1);
    return c;
  }

  static scalar_type ga_hash_code(const base_tensor &t) {
    scalar_type c(0);
    for (size_type i = 0; i < t.size(); ++i)
      c += sin(M_E+t[i]+M_E*M_E*scalar_type(i+1))+scalar_type(i+1)*M_PI;
    return c;
  }

  static scalar_type ga_hash_code(GA_NODE_TYPE e) {
    return cos(M_E + scalar_type((e == GA_NODE_ZERO) ? GA_NODE_CONSTANT : e));
  }

  static scalar_type ga_hash_code(const pga_tree_node pnode) {
    scalar_type c = ga_hash_code(pnode->node_type);

    switch (pnode->node_type) {
    case GA_NODE_CONSTANT: case GA_NODE_ZERO:
      c += ga_hash_code(pnode->tensor());
      if (pnode->test_function_type & 1)
        c += 34.731 * ga_hash_code(pnode->name_test1);
      if (pnode->test_function_type & 2)
        c += 34.731 * ga_hash_code(pnode->name_test2);
      break;

    case GA_NODE_OP: c += scalar_type(pnode->op_type)*M_E*M_PI*M_PI; break;
    case GA_NODE_X: c += scalar_type(pnode->nbc1) + M_E*M_PI; break;
    case GA_NODE_VAL: case GA_NODE_GRAD:
    case GA_NODE_HESS: case GA_NODE_DIVERG:
    case GA_NODE_VAL_TEST: case GA_NODE_GRAD_TEST:
    case GA_NODE_HESS_TEST: case GA_NODE_DIVERG_TEST:
      c += ga_hash_code(pnode->name); break;

    case GA_NODE_INTERPOLATE_FILTER:
      c += 1.73*ga_hash_code(pnode->interpolate_name)
        + 2.486*double(pnode->nbc1 + 1);
      break;
    case GA_NODE_INTERPOLATE_DERIVATIVE:
      c += 2.321*ga_hash_code(pnode->interpolate_name_der);
      // No break. The hash code is completed with the next item
    case GA_NODE_INTERPOLATE_VAL: case GA_NODE_INTERPOLATE_GRAD:
    case GA_NODE_INTERPOLATE_HESS: case GA_NODE_INTERPOLATE_DIVERG:
    case GA_NODE_INTERPOLATE_VAL_TEST: case GA_NODE_INTERPOLATE_GRAD_TEST:
    case GA_NODE_INTERPOLATE_HESS_TEST: case GA_NODE_INTERPOLATE_DIVERG_TEST:
      c += 1.33*(1.22+ga_hash_code(pnode->name))
        + 1.66*ga_hash_code(pnode->interpolate_name);
      break;
    case GA_NODE_ELEMENTARY_VAL: case GA_NODE_ELEMENTARY_GRAD:
    case GA_NODE_ELEMENTARY_HESS: case GA_NODE_ELEMENTARY_DIVERG:
    case GA_NODE_ELEMENTARY_VAL_TEST: case GA_NODE_ELEMENTARY_GRAD_TEST:
    case GA_NODE_ELEMENTARY_HESS_TEST: case GA_NODE_ELEMENTARY_DIVERG_TEST:
      c += 1.33*(1.22+ga_hash_code(pnode->name))
        + 2.63*ga_hash_code(pnode->elementary_name);
      break;
    case GA_NODE_XFEM_PLUS_VAL: case GA_NODE_XFEM_PLUS_GRAD:
    case GA_NODE_XFEM_PLUS_HESS: case GA_NODE_XFEM_PLUS_DIVERG:
    case GA_NODE_XFEM_PLUS_VAL_TEST: case GA_NODE_XFEM_PLUS_GRAD_TEST:
    case GA_NODE_XFEM_PLUS_HESS_TEST: case GA_NODE_XFEM_PLUS_DIVERG_TEST:
    case GA_NODE_XFEM_MINUS_VAL: case GA_NODE_XFEM_MINUS_GRAD:
    case GA_NODE_XFEM_MINUS_HESS: case GA_NODE_XFEM_MINUS_DIVERG:
    case GA_NODE_XFEM_MINUS_VAL_TEST: case GA_NODE_XFEM_MINUS_GRAD_TEST:
    case GA_NODE_XFEM_MINUS_HESS_TEST: case GA_NODE_XFEM_MINUS_DIVERG_TEST:
      c += 1.33*(1.22+ga_hash_code(pnode->name));
      break;
    case GA_NODE_INTERPOLATE_X: case GA_NODE_INTERPOLATE_NORMAL:
      c += M_PI*1.33*ga_hash_code(pnode->interpolate_name);
      break;
    case GA_NODE_PREDEF_FUNC: case GA_NODE_SPEC_FUNC: case GA_NODE_OPERATOR:
      c += ga_hash_code(pnode->name)
        + tanh(scalar_type(pnode->der1)/M_PI + scalar_type(pnode->der2)*M_PI);
      break;
    default: break;
    }
    return c;
  }

# define ga_valid_operand(pnode)				\
  {								\
    if (pnode && (pnode->node_type == GA_NODE_PREDEF_FUNC ||	\
                  pnode->node_type == GA_NODE_SPEC_FUNC ||	\
                  pnode->node_type == GA_NODE_NAME ||		\
                  pnode->node_type == GA_NODE_OPERATOR ||	\
                  pnode->node_type == GA_NODE_ALLINDICES))	\
      ga_throw_error(pnode->expr, pnode->pos, "Invalid term");	\
  }

  static void ga_node_analysis(ga_tree &tree,
                               const ga_workspace &workspace,
                               pga_tree_node pnode, const mesh &me,
                               size_type ref_elt_dim, bool eval_fixed_size,
                               bool ignore_X, int option) {
    bool all_cte = true, all_sc = true;
    size_type meshdim = &me ? me.dim() : 1;
    pnode->symmetric_op = false;

    for (size_type i = 0; i < pnode->children.size(); ++i) {
      ga_node_analysis(tree, workspace, pnode->children[i], me,
                       ref_elt_dim, eval_fixed_size, ignore_X, option);
      all_cte = all_cte && (pnode->children[i]->node_type == GA_NODE_CONSTANT);
      all_sc = all_sc && (pnode->children[i]->tensor_proper_size() == 1);
      GMM_ASSERT1(pnode->children[i]->test_function_type != size_type(-1),
                  "internal error on child " << i);
      if (pnode->node_type != GA_NODE_PARAMS)
        ga_valid_operand(pnode->children[i]);
    }

    size_type nbch = pnode->children.size();
    pga_tree_node child0 = (nbch > 0) ? pnode->children[0] : 0;
    pga_tree_node child1 = (nbch > 1) ? pnode->children[1] : 0;
    bgeot::multi_index mi;
    const bgeot::multi_index &size0 = child0 ? child0->t.sizes() : mi;
    const bgeot::multi_index &size1 = child1 ? child1->t.sizes() : mi;
    size_type dim0 = child0 ? child0->tensor_order() : 0;
    size_type dim1 = child1 ? child1->tensor_order() : 0;

    // cout << "child1 = " << child1 << endl;
    // cout << "child0 = " << child0 << endl;
    // cout << "nbch = " << nbch << endl;
    // cout<<"begin analysis of node "; ga_print_node(pnode, cout); cout<<endl;

    const ga_predef_function_tab &PREDEF_FUNCTIONS
      = dal::singleton<ga_predef_function_tab>::instance(0);
    const ga_predef_operator_tab &PREDEF_OPERATORS
      = dal::singleton<ga_predef_operator_tab>::instance(0);
    const ga_spec_function_tab &SPEC_FUNCTIONS
      = dal::singleton<ga_spec_function_tab>::instance(0);
 
    switch (pnode->node_type) {
    case GA_NODE_PREDEF_FUNC: case GA_NODE_OPERATOR: case GA_NODE_SPEC_FUNC :
    case GA_NODE_CONSTANT: case GA_NODE_X: case GA_NODE_ELT_SIZE:
    case GA_NODE_ELT_K:  case GA_NODE_ELT_B:
    case GA_NODE_NORMAL: case GA_NODE_RESHAPE:
    case GA_NODE_INTERPOLATE_X: case GA_NODE_INTERPOLATE_NORMAL:
      pnode->test_function_type = 0; break;

    case GA_NODE_ALLINDICES: pnode->test_function_type = 0; break;
    case GA_NODE_VAL:
      if (eval_fixed_size && !(workspace.associated_mf(pnode->name))
          && !(workspace.associated_im_data(pnode->name))) {
        gmm::copy(workspace.value(pnode->name), pnode->tensor().as_vector());
        pnode->node_type = GA_NODE_CONSTANT;
      }
      break;

    case GA_NODE_ZERO: case GA_NODE_GRAD:
    case GA_NODE_HESS: case GA_NODE_DIVERG:
    case GA_NODE_INTERPOLATE_VAL:  case GA_NODE_INTERPOLATE_GRAD:
    case GA_NODE_INTERPOLATE_HESS: case GA_NODE_INTERPOLATE_DIVERG:
    case GA_NODE_ELEMENTARY_VAL: case GA_NODE_ELEMENTARY_GRAD:
    case GA_NODE_ELEMENTARY_HESS: case GA_NODE_ELEMENTARY_DIVERG:
    case GA_NODE_XFEM_PLUS_VAL: case GA_NODE_XFEM_PLUS_GRAD:
    case GA_NODE_XFEM_PLUS_HESS: case GA_NODE_XFEM_PLUS_DIVERG:
    case GA_NODE_XFEM_MINUS_VAL: case GA_NODE_XFEM_MINUS_GRAD:
    case GA_NODE_XFEM_MINUS_HESS: case GA_NODE_XFEM_MINUS_DIVERG:
      break;

    case GA_NODE_VAL_TEST: case GA_NODE_GRAD_TEST:
    case GA_NODE_HESS_TEST: case GA_NODE_DIVERG_TEST:
    case GA_NODE_INTERPOLATE_VAL_TEST: case GA_NODE_INTERPOLATE_GRAD_TEST:
    case GA_NODE_INTERPOLATE_HESS_TEST: case GA_NODE_INTERPOLATE_DIVERG_TEST:
    case GA_NODE_INTERPOLATE_DERIVATIVE:
    case GA_NODE_ELEMENTARY_VAL_TEST: case GA_NODE_ELEMENTARY_GRAD_TEST:
    case GA_NODE_ELEMENTARY_HESS_TEST: case GA_NODE_ELEMENTARY_DIVERG_TEST:
    case GA_NODE_XFEM_PLUS_VAL_TEST: case GA_NODE_XFEM_PLUS_GRAD_TEST:
    case GA_NODE_XFEM_PLUS_HESS_TEST: case GA_NODE_XFEM_PLUS_DIVERG_TEST:
    case GA_NODE_XFEM_MINUS_VAL_TEST: case GA_NODE_XFEM_MINUS_GRAD_TEST:
    case GA_NODE_XFEM_MINUS_HESS_TEST: case GA_NODE_XFEM_MINUS_DIVERG_TEST:
      {
        const mesh_fem *mf = workspace.associated_mf(pnode->name);
        size_type t_type = pnode->test_function_type;
        if (t_type == 1) {
          pnode->name_test1 = pnode->name;
          pnode->interpolate_name_test1 = pnode->interpolate_name;
          pnode->interpolate_name_test2 = pnode->name_test2 = "";
          pnode->qdim1 = (mf ? workspace.qdim(pnode->name)
                          : gmm::vect_size(workspace.value(pnode->name)));
          if (option == 1)
            workspace.test1.insert
              (var_trans_pair(pnode->name_test1,
                              pnode->interpolate_name_test1));
          if (!(pnode->qdim1))
            ga_throw_error(pnode->expr, pnode->pos,
			   "Invalid null size of variable");
        } else {
          pnode->interpolate_name_test1 = pnode->name_test1 = "";
          pnode->name_test2 = pnode->name;
          pnode->interpolate_name_test2 = pnode->interpolate_name;
          pnode->qdim2 = (mf ? workspace.qdim(pnode->name)
                          : gmm::vect_size(workspace.value(pnode->name)));
          if (option == 1)
            workspace.test2.insert
              (var_trans_pair(pnode->name_test2,
                              pnode->interpolate_name_test2));
          if (!(pnode->qdim2))
            ga_throw_error(pnode->expr, pnode->pos,
			   "Invalid null size of variable");
        }
        if (!mf) {
          size_type n = workspace.qdim(pnode->name);
          if (!n)
            ga_throw_error(pnode->expr, pnode->pos,
			   "Invalid null size of variable");
          if (n == 1) {
            pnode->init_vector_tensor(1);
            pnode->tensor()[0] = scalar_type(1);
            pnode->test_function_type = t_type;
          } else {
            pnode->init_matrix_tensor(n,n);
            pnode->test_function_type = t_type;
            for (size_type i = 0; i < n; ++i)
              for (size_type j = 0; j < n; ++j)
                pnode->tensor()(i,j) = (i==j) ? scalar_type(1) : scalar_type(0);
          }
        }
      }
      break;

    case GA_NODE_INTERPOLATE:
      if (pnode->name.compare("X") == 0) {
        pnode->node_type = GA_NODE_INTERPOLATE_X;
        pnode->init_vector_tensor(meshdim);
        break;
      }
      if (pnode->name.compare("Normal") == 0) {
        pnode->node_type = GA_NODE_INTERPOLATE_NORMAL;
        pnode->init_vector_tensor(meshdim);
        break;
      }
      // else continue with what follows
    case GA_NODE_ELEMENTARY: // and ... case GA_NODE_INTERPOLATE:
    case GA_NODE_XFEM_PLUS:
    case GA_NODE_XFEM_MINUS:
      {
        int ndt = ((pnode->node_type == GA_NODE_INTERPOLATE) ? 1 : 0)
          + ((pnode->node_type == GA_NODE_ELEMENTARY) ? 2 : 0)
          + ((pnode->node_type == GA_NODE_XFEM_PLUS) ? 3 : 0)
          + ((pnode->node_type == GA_NODE_XFEM_MINUS) ? 4 : 0);
        std::string op__name =
          (pnode->node_type == GA_NODE_INTERPOLATE) ? "Interpolation" : ""
          + (pnode->node_type == GA_NODE_ELEMENTARY) ?
             "Elementary transformation" : ""
          + (pnode->node_type == GA_NODE_XFEM_PLUS) ? "Xfem_plus" : ""
          + (pnode->node_type == GA_NODE_XFEM_MINUS) ? "Xfem_minus" : "";

        std::string name = pnode->name;
        size_type prefix_id = ga_parse_prefix_operator(name);
        size_type test = ga_parse_prefix_test(name);
        pnode->name = name;

        // Group must be tested and it should be a fem variable
        if (!(workspace.variable_or_group_exists(name)))
          ga_throw_error(pnode->expr, pnode->pos,
                         "Unknown variable or group of variables");

        const mesh_fem *mf = workspace.associated_mf(name);
        if (!mf)
          ga_throw_error(pnode->expr, pnode->pos, op__name
                         << " can only apply to finite element variables/data");

        size_type q = workspace.qdim(name), n = mf->linked_mesh().dim();
        if (!q) ga_throw_error(pnode->expr, pnode->pos,
                               "Invalid null size of variable");

        bgeot::multi_index mii = workspace.qdims(name);
        if (mii.size() > 6)
          ga_throw_error(pnode->expr, pnode->pos,
                         "Tensor with too many dimensions. Limited to 6");

        if (test == 1) {
          pnode->name_test1 = name;
          pnode->interpolate_name_test1 = pnode->interpolate_name;
          if (option == 1)
            workspace.test1.insert
              (var_trans_pair(pnode->name_test1,
                              pnode->interpolate_name_test1));
          pnode->qdim1 = workspace.qdim(name);
          if (!(pnode->qdim1))
            ga_throw_error(pnode->expr, pnode->pos,
                           "Invalid null size of variable");
        } else if (test == 2) {
          pnode->name_test2 = name;
          pnode->interpolate_name_test2 = pnode->interpolate_name;
          if (option == 1)
            workspace.test2.insert
              (var_trans_pair(pnode->name_test2,
                              pnode->interpolate_name_test2));
          pnode->qdim2 = workspace.qdim(name);
          if (!(pnode->qdim2))
            ga_throw_error(pnode->expr, pnode->pos,
                           "Invalid null size of variable");
        }

        switch (prefix_id) {
        case 0: // value
          if (!test) {
            switch (ndt) {
            case 1: pnode->node_type = GA_NODE_INTERPOLATE_VAL; break;
            case 2: pnode->node_type = GA_NODE_ELEMENTARY_VAL; break;
            case 3: pnode->node_type = GA_NODE_XFEM_PLUS_VAL; break;
            case 4: pnode->node_type = GA_NODE_XFEM_MINUS_VAL; break;
            default: GMM_ASSERT1(false, "internal error");
            }
          } else {
            switch (ndt) {
            case 1: pnode->node_type = GA_NODE_INTERPOLATE_VAL_TEST; break;
            case 2: pnode->node_type = GA_NODE_ELEMENTARY_VAL_TEST; break;
            case 3: pnode->node_type = GA_NODE_XFEM_PLUS_VAL_TEST; break;
            case 4: pnode->node_type = GA_NODE_XFEM_MINUS_VAL_TEST; break;
            default: GMM_ASSERT1(false, "internal error");
            }
            if (q == 1 && mii.size() <= 1) {
              mii.resize(1);
              mii[0] = 2;
            } else
              mii.insert(mii.begin(), 2);
          }
          break;
        case 1: // grad
          if (!test) {
            switch (ndt) {
            case 1: pnode->node_type = GA_NODE_INTERPOLATE_GRAD; break;
            case 2: pnode->node_type = GA_NODE_ELEMENTARY_GRAD; break;
            case 3: pnode->node_type = GA_NODE_XFEM_PLUS_GRAD; break;
            case 4: pnode->node_type = GA_NODE_XFEM_MINUS_GRAD; break;
            default: GMM_ASSERT1(false, "internal error");
            }
            if (n > 1) {
              if (q == 1 && mii.size() == 1) mii[0] = n;
              else mii.push_back(n);
            }
          } else {
            switch (ndt) {
            case 1: pnode->node_type = GA_NODE_INTERPOLATE_GRAD_TEST; break;
            case 2: pnode->node_type = GA_NODE_ELEMENTARY_GRAD_TEST; break;
            case 3: pnode->node_type = GA_NODE_XFEM_PLUS_GRAD_TEST; break;
            case 4: pnode->node_type = GA_NODE_XFEM_MINUS_GRAD_TEST; break;
            default: GMM_ASSERT1(false, "internal error");
            }
            if (q == 1 && mii.size() <= 1) {
              mii.resize(1);
              mii[0] = 2;
            } else
              mii.insert(mii.begin(), 2);
            if (n > 1) mii.push_back(n);
          }
          break;
        case 2: // Hessian
          if (!test) {
            switch (ndt) {
            case 1: pnode->node_type = GA_NODE_INTERPOLATE_HESS; break;
            case 2: pnode->node_type = GA_NODE_ELEMENTARY_HESS; break;
            case 3: pnode->node_type = GA_NODE_XFEM_PLUS_HESS; break;
            case 4: pnode->node_type = GA_NODE_XFEM_MINUS_HESS; break;
            default: GMM_ASSERT1(false, "internal error");
            }
            if (n > 1) {
              if (q == 1 && mii.size() == 1) { mii[0] = n;  mii.push_back(n); }
              else { mii.push_back(n); mii.push_back(n); }
            }
          } else {
            switch (ndt) {
            case 1: pnode->node_type = GA_NODE_INTERPOLATE_HESS_TEST; break;
            case 2: pnode->node_type = GA_NODE_ELEMENTARY_HESS_TEST; break;
            case 3: pnode->node_type = GA_NODE_XFEM_PLUS_HESS_TEST; break;
            case 4: pnode->node_type = GA_NODE_XFEM_MINUS_HESS_TEST; break;
            default: GMM_ASSERT1(false, "internal error");
            }
            if (q == 1 && mii.size() <= 1) {
              mii.resize(1);
              mii[0] = 2;
            } else
              mii.insert(mii.begin(), 2);
            if (n > 1) { mii.push_back(n); mii.push_back(n); }
          }
          break;
        case 3: // divergence
          if (q != n)
            ga_throw_error(pnode->expr, pnode->pos,
                           "Divergence operator requires fem qdim ("
                           << q << ") to be equal to dim (" << n << ")");
          if (!test) {
            switch (ndt) {
            case 1: pnode->node_type = GA_NODE_INTERPOLATE_DIVERG; break;
            case 2: pnode->node_type = GA_NODE_ELEMENTARY_DIVERG; break;
            case 3: pnode->node_type = GA_NODE_XFEM_PLUS_DIVERG; break;
            case 4: pnode->node_type = GA_NODE_XFEM_MINUS_DIVERG; break;
            default: GMM_ASSERT1(false, "internal error");
            }
            mii.resize(1);
            mii[0] = 1;
          } else {
            switch (ndt) {
            case 1: pnode->node_type = GA_NODE_INTERPOLATE_DIVERG_TEST; break;
            case 2: pnode->node_type = GA_NODE_ELEMENTARY_DIVERG_TEST; break;
            case 3: pnode->node_type = GA_NODE_XFEM_PLUS_DIVERG_TEST; break;
            case 4: pnode->node_type = GA_NODE_XFEM_MINUS_DIVERG_TEST; break;
            default: GMM_ASSERT1(false, "internal error");
            }
            mii.resize(1);
            mii[0] = 2;
          }
          break;
        }
        pnode->t.adjust_sizes(mii);
        pnode->test_function_type = test;

        if (ndt == 1) {
          if (!(workspace.interpolate_transformation_exists
                (pnode->interpolate_name)))  {
            ga_throw_error(pnode->expr, pnode->pos,
                           "Unknown interpolate transformation");
          }
        } else if (ndt == 2) {
          if (!(workspace.elementary_transformation_exists
                (pnode->elementary_name))) {
            ga_throw_error(pnode->expr, pnode->pos,
                           "Unknown elementary transformation");
          }
        }
      }
      break;

    case GA_NODE_INTERPOLATE_FILTER:
      {
        if (pnode->children.size() == 2) {
          bool valid = (child1->node_type == GA_NODE_CONSTANT);
          int n = valid ? int(round(child1->tensor()[0])) : -1;
          if (n < 0 || n > 100 || child1->tensor_order() > 0)
            ga_throw_error(pnode->expr, pnode->pos, "The third argument of "
                           "Interpolate_filter should be a (small) "
                           "non-negative integer.");
          pnode->nbc1 = size_type(n);
          tree.clear_node(child1);
        }
        if (!(workspace.interpolate_transformation_exists
              (pnode->interpolate_name)))
          ga_throw_error(pnode->expr, pnode->pos,
                         "Unknown interpolate transformation");
        pnode->t = child0->t;
        pnode->test_function_type = child0->test_function_type;
        pnode->name_test1 = child0->name_test1;
        pnode->name_test2 = child0->name_test2;
        pnode->interpolate_name_test1 = child0->interpolate_name_test1;
        pnode->interpolate_name_test2 = child0->interpolate_name_test2;
        pnode->qdim1 = child0->qdim1;
        pnode->qdim2 = child0->qdim2;
      }
      break;


    case GA_NODE_OP:
      switch(pnode->op_type) {

      case GA_PLUS: case GA_MINUS:
        {
          if (pnode->op_type == GA_PLUS) pnode->symmetric_op = true;
          size_type c_size = std::min(size0.size(), size1.size());
          bool compatible = true;

          size_type f_ind = 0;
          if (child0->test_function_type &&
              child1->test_function_type == child0->test_function_type)
            f_ind = (child0->test_function_type == 3) ? 2:1;

          for (size_type i = f_ind; i < c_size; ++i)
            if (size0[i] != size1[i]) compatible = false;
          for (size_type i = c_size; i < size0.size(); ++i)
            if (size0[i] != 1) compatible = false;
          for (size_type i = c_size; i < size1.size(); ++i)
            if (size1[i] != 1) compatible = false;

          if (!compatible)
            ga_throw_error(pnode->expr, pnode->pos,
			   "Addition or subtraction of expressions of "
			   "different sizes: " << size0 << " != " << size1);
          if (child0->test_function_type || child1->test_function_type) {
            switch (option) {
            case 0: case 2:
              if (child0->name_test1.compare(child1->name_test1) ||
                  child0->name_test2.compare(child1->name_test2) ||
                  child0->interpolate_name_test1.compare
                  (child1->interpolate_name_test1) ||
                  child0->interpolate_name_test2.compare
                  (child1->interpolate_name_test2))
                compatible = false;
              break;
            case 1: case 3: break;
            default: GMM_ASSERT1(false, "Unknown option");
            }
          }

          if (child0->test_function_type != child1->test_function_type ||
              (!compatible && option != 2))
            ga_throw_error(pnode->expr, pnode->pos, "Addition or subtraction "
			   "of incompatible test functions");
          if (all_cte) {
            pnode->node_type = GA_NODE_CONSTANT;
            pnode->test_function_type = 0;
            pnode->tensor() = pnode->children[0]->tensor();
            if (pnode->op_type == GA_MINUS)
              pnode->tensor() -= pnode->children[1]->tensor();
            else
              pnode->tensor() += pnode->children[1]->tensor();
            tree.clear_children(pnode);
          } else {
            pnode->t = child0->t;
            pnode->test_function_type = child0->test_function_type;
            pnode->name_test1 = child0->name_test1;
            pnode->name_test2 = child0->name_test2;
            pnode->interpolate_name_test1 = child0->interpolate_name_test1;
            pnode->interpolate_name_test2 = child0->interpolate_name_test2;
            pnode->qdim1 = child0->qdim1;
            pnode->qdim2 = child0->qdim2;

            // simplification if one of the two operands is constant and zero
            if (child0->tensor_is_zero()) {
              if (pnode->op_type == GA_MINUS) {
                pnode->op_type = GA_UNARY_MINUS;
                tree.clear_node(child0);
              } else {
                tree.replace_node_by_child(pnode, 1);
                pnode = child1;
              }
            } else if (child1->tensor_is_zero()) {
              tree.replace_node_by_child(pnode, 0);
              pnode = child0;
            } else if (option == 2 && !compatible) {
              bool child0_compatible = true, child1_compatible = true;
              if (pnode->test_function_type & 1) {
                if (child0->name_test1.compare(workspace.selected_test1.varname)
                    || child0->interpolate_name_test1.compare
                    (workspace.selected_test1.transname))
                  child0_compatible = false;
                if (child1->name_test1.compare(workspace.selected_test1.varname)
                    || child1->interpolate_name_test1.compare
                    (workspace.selected_test1.transname))
                  child1_compatible = false;
              }
              if (pnode->test_function_type & 2) {
                if (child0->name_test2.compare(workspace.selected_test2.varname)
                    || child0->interpolate_name_test2.compare
                    (workspace.selected_test2.transname))
                  child0_compatible = false;
                if (child1->name_test2.compare(workspace.selected_test2.varname)
                    || child1->interpolate_name_test1.compare
                    (workspace.selected_test2.transname))
                  child1_compatible = false;
              }
              if (child0_compatible) {
                tree.replace_node_by_child(pnode, 0);
                pnode = child0;
              } else if (child1_compatible) {
                if (pnode->op_type == GA_MINUS) {
                  pnode->op_type = GA_UNARY_MINUS;
                  pnode->t = child1->t;
                  pnode->test_function_type = child1->test_function_type;
                  pnode->name_test1 = child1->name_test1;
                  pnode->name_test2 = child1->name_test2;
                  pnode->interpolate_name_test1=child1->interpolate_name_test1;
                  pnode->interpolate_name_test2=child1->interpolate_name_test2;
                  pnode->qdim1 = child1->qdim1;
                  pnode->qdim2 = child1->qdim2;
                  tree.clear_node(child0);
                } else {
                  tree.replace_node_by_child(pnode, 1);
                  pnode = child1;
                }
              }
            }
          }
        }
        break;

      case GA_DOTMULT: case GA_DOTDIV:
        {
          if (pnode->op_type == GA_DOTMULT) pnode->symmetric_op = true;
          bool compatible = true;
          if (child0->tensor_proper_size() != child1->tensor_proper_size())
            compatible = false;

          if (child0->tensor_proper_size() != 1) {
            if (child0->tensor_order() != child1->tensor_order())
              compatible = false;

            for (size_type i = 0; i < child0->tensor_order(); ++i)
              if (child0->tensor_proper_size(i)!=child1->tensor_proper_size(i))
                compatible = false;
          }

          if (!compatible)
            ga_throw_error(pnode->expr, pnode->pos,
                           "Arguments of different sizes for .* or ./");

          if (pnode->op_type == GA_DOTDIV && child1->test_function_type)
            ga_throw_error(pnode->expr, pnode->pos,
                           "Division by test functions is not allowed");

          pnode->mult_test(child0, child1);
          mi = pnode->t.sizes();
          for (size_type i = 0; i < child0->tensor_order(); ++i)
            mi.push_back(child0->tensor_proper_size(i));
          pnode->t.adjust_sizes(mi);

          if (all_cte) {
            pnode->node_type = GA_NODE_CONSTANT;
            pnode->test_function_type = 0;
            if (pnode->op_type == GA_DOTMULT) {
              for (size_type i = 0; i < child0->tensor().size(); ++i)
                pnode->tensor()[i] = child0->tensor()[i] * child1->tensor()[i];
            } else {
              for (size_type i = 0; i < child0->tensor().size(); ++i) {
                if (child1->tensor()[i] == scalar_type(0))
                  ga_throw_error(pnode->expr, pnode->pos, "Division by zero.");
                pnode->tensor()[i] = child0->tensor()[i] / child1->tensor()[i];
              }
            }
            tree.clear_children(pnode);
          } else {
            if (child0->tensor_is_zero() || child1->tensor_is_zero()) {
              gmm::clear(pnode->tensor().as_vector());
              pnode->node_type = GA_NODE_ZERO;
              tree.clear_children(pnode);
            }
            if (child1->tensor_is_zero() && pnode->op_type == GA_DOTDIV)
              ga_throw_error(pnode->expr, pnode->pos, "Division by zero.");
          }
        }
        break;

      case GA_UNARY_MINUS:
        pnode->t = child0->t;
        pnode->test_function_type = child0->test_function_type;
        pnode->name_test1 = child0->name_test1;
        pnode->name_test2 = child0->name_test2;
        pnode->interpolate_name_test1 = child0->interpolate_name_test1;
        pnode->interpolate_name_test2 = child0->interpolate_name_test2;
        pnode->qdim1 = child0->qdim1;
        pnode->qdim2 = child0->qdim2;
        if (all_cte) {
          pnode->node_type = GA_NODE_CONSTANT;
          pnode->test_function_type = 0;
          gmm::scale(pnode->tensor().as_vector(), scalar_type(-1));
          tree.clear_children(pnode);
        } else if (child0->node_type == GA_NODE_ZERO) {
          tree.replace_node_by_child(pnode, 0);
          pnode = child0;
        }
        break;

      case GA_QUOTE:
        if (dim0 > 2)
          ga_throw_error(pnode->expr, pnode->pos, "Transpose operator is for "
                         "vectors or matrices only.");
        mi = size0;
        if (child0->tensor_proper_size() == 1)
          { tree.replace_node_by_child(pnode, 0); pnode = child0; break; }
        else if (dim0 == 2) std::swap(mi.back(), mi[size0.size()-2]);
        else { size_type N = mi.back(); mi.back() = 1; mi.push_back(N); }

        pnode->t.adjust_sizes(mi);
        pnode->test_function_type = child0->test_function_type;
        pnode->name_test1 = child0->name_test1;
        pnode->name_test2 = child0->name_test2;
        pnode->interpolate_name_test1 = child0->interpolate_name_test1;
        pnode->interpolate_name_test2 = child0->interpolate_name_test2;
        pnode->qdim1 = child0->qdim1;
        pnode->qdim2 = child0->qdim2;
        if (all_cte) {
          pnode->node_type = GA_NODE_CONSTANT;
          pnode->test_function_type = 0;
          if (dim0 == 2) {
            for (size_type i = 0; i < mi.back(); ++i)
              for (size_type j = 0; j < mi[size0.size()-2]; ++j)
                pnode->tensor()(j, i) = child0->tensor()(i,j);
          } else if (dim0 == 1) {
            for (size_type i = 0; i < mi.back(); ++i)
              pnode->tensor()(0, i) = child0->tensor()[i];
          }
          tree.clear_children(pnode);
        } else if (child0->node_type == GA_NODE_ZERO) {
          pnode->node_type = GA_NODE_ZERO;
          gmm::clear(pnode->tensor().as_vector());
          tree.clear_children(pnode);
        }
        break;

      case GA_SYM: case GA_SKEW:
        if (child0->tensor_proper_size() != 1 &&
            (dim0 != 2 || size0.back() != size0[size0.size()-2]))
          ga_throw_error(pnode->expr, pnode->pos, "Sym and Skew operators are "
			 "for square matrices only.");
        mi = size0;
        if (child0->tensor_proper_size() == 1) {
          if (pnode->op_type == GA_SYM)
            { tree.replace_node_by_child(pnode, 0); pnode = child0; break; }
          else {
            pnode->node_type = GA_NODE_ZERO;
            gmm::clear(pnode->tensor().as_vector());
            tree.clear_children(pnode);
            break;
          }
        }

        pnode->t.adjust_sizes(mi);
        pnode->test_function_type = child0->test_function_type;
        pnode->name_test1 = child0->name_test1;
        pnode->name_test2 = child0->name_test2;
        pnode->interpolate_name_test1 = child0->interpolate_name_test1;
        pnode->interpolate_name_test2 = child0->interpolate_name_test2;
        pnode->qdim1 = child0->qdim1;
        pnode->qdim2 = child0->qdim2;
        if (all_cte) {
          pnode->node_type = GA_NODE_CONSTANT;
          pnode->test_function_type = 0;
          for (size_type i = 0; i < mi.back(); ++i)
            for (size_type j = 0; j < mi.back(); ++j)
              if (pnode->op_type == GA_SYM)
                pnode->tensor()(j, i) = 0.5*(child0->tensor()(j,i)
                                             + child0->tensor()(i,j));
              else
                pnode->tensor()(j, i) = 0.5*(child0->tensor()(j,i)
                                             - child0->tensor()(i,j));
          tree.clear_children(pnode);
        } else if (child0->node_type == GA_NODE_ZERO) {
          pnode->node_type = GA_NODE_ZERO;
          gmm::clear(pnode->tensor().as_vector());
          tree.clear_children(pnode);
        }
        break;

      case GA_TRACE:
        {
          mi = size0;
          size_type N = (child0->tensor_proper_size() == 1) ? 1 : mi.back();

          if (child0->tensor_proper_size() == 1)
            { tree.replace_node_by_child(pnode, 0); pnode = child0; break; }

          if ((dim0 != 2 && child0->tensor_proper_size() != 1) ||
              (dim0 == 2 && mi[mi.size()-2] != N))
            ga_throw_error(pnode->expr, pnode->pos,
                           "Trace operator is for square matrices only.");

          if (dim0 == 2) { mi.pop_back(); mi.pop_back(); }
          pnode->t.adjust_sizes(mi);
          pnode->test_function_type = child0->test_function_type;
          pnode->name_test1 = child0->name_test1;
          pnode->name_test2 = child0->name_test2;
          pnode->interpolate_name_test1 = child0->interpolate_name_test1;
          pnode->interpolate_name_test2 = child0->interpolate_name_test2;
          pnode->qdim1 = child0->qdim1;
          pnode->qdim2 = child0->qdim2;
          if (all_cte) {
            pnode->node_type = GA_NODE_CONSTANT;
            pnode->test_function_type = 0;
            if (dim0 == 2) {
              pnode->tensor()[0] = scalar_type(0);
              for (size_type i = 0; i < N; ++i)
                pnode->tensor()[0] += child0->tensor()(i,i);
            } else {
              pnode->tensor()[0] += child0->tensor()[0];
            }
            tree.clear_children(pnode);
          } else if (child0->node_type == GA_NODE_ZERO) {
            pnode->node_type = GA_NODE_ZERO;
            gmm::clear(pnode->tensor().as_vector());
            tree.clear_children(pnode);
          }
        }
        break;

      case GA_DEVIATOR:
        {
          mi = size0;
          size_type N = (child0->tensor_proper_size() == 1) ? 1 : mi.back();

          if ((dim0 != 2 && child0->tensor_proper_size() != 1) ||
              (dim0 == 2 && mi[mi.size()-2] != N))
            ga_throw_error(pnode->expr, pnode->pos,
                           "Deviator operator is for square matrices only.");

          if (child0->tensor_proper_size() == 1) {
            pnode->node_type = GA_NODE_ZERO;
            gmm::clear(pnode->tensor().as_vector());
            tree.clear_children(pnode);
            break;
          }

          pnode->t.adjust_sizes(mi);
          pnode->test_function_type = child0->test_function_type;
          pnode->name_test1 = child0->name_test1;
          pnode->name_test2 = child0->name_test2;
          pnode->interpolate_name_test1 = child0->interpolate_name_test1;
          pnode->interpolate_name_test2 = child0->interpolate_name_test2;
          pnode->qdim1 = child0->qdim1;
          pnode->qdim2 = child0->qdim2;
          if (all_cte) {
            pnode->node_type = GA_NODE_CONSTANT;
            pnode->test_function_type = 0;
            if (dim0 == 2) {
              scalar_type tr(0);
              gmm::copy(child0->tensor().as_vector(),
                        pnode->tensor().as_vector());
              for (size_type i = 0; i < N; ++i)
                tr += child0->tensor()(i,i);
              for (size_type i = 0; i < N; ++i)
                pnode->tensor()(i,i) -= tr / scalar_type(N);
            } else {
              pnode->tensor()[0] = scalar_type(0);
            }
            tree.clear_children(pnode);
          } else if (child0->node_type == GA_NODE_ZERO) {
            pnode->node_type = GA_NODE_ZERO;
            gmm::clear(pnode->tensor().as_vector());
            tree.clear_children(pnode);
          }
        }
        break;

      case GA_PRINT:
        {
          pnode->t = child0->t;
          pnode->test_function_type = child0->test_function_type;
          pnode->name_test1 = child0->name_test1;
          pnode->name_test2 = child0->name_test2;
          pnode->interpolate_name_test1 = child0->interpolate_name_test1;
          pnode->interpolate_name_test2 = child0->interpolate_name_test2;
          pnode->qdim1 = child0->qdim1;
          pnode->qdim2 = child0->qdim2;
          if (all_cte) {
            pnode->node_type = GA_NODE_CONSTANT;
            cout << "Print constant term "; ga_print_node(child0, cout);
            cout << ": " << pnode->tensor() << endl;
            tree.clear_children(pnode);
          } else if (child0->node_type == GA_NODE_ZERO) {
            pnode->node_type = GA_NODE_ZERO;
            gmm::clear(pnode->tensor().as_vector());
            cout << "Print zero term "; ga_print_node(child0, cout);
            cout << ": " << pnode->tensor() << endl;
            tree.clear_children(pnode);
          }
        }
        break;

      case GA_DOT:
        if (dim1 > 1)
          ga_throw_error(pnode->expr, pnode->pos, "The second argument of the "
			 "dot product has to be a vector.")
        else {
          size_type s0 = dim0 == 0 ? 1 : size0.back();
          size_type s1 = dim1 == 0 ? 1 : size1.back();
          if (s0 != s1) ga_throw_error(pnode->expr, pnode->pos, "Dot product "
                                       "of expressions of different sizes ("
                                       << s0 << " != " << s1 << ").");
          if (child0->tensor_order() <= 1) pnode->symmetric_op = true;
          pnode->mult_test(child0, child1);
          if (dim0 > 1) {
            mi = pnode->t.sizes();
            for (size_type i = 1; i < dim0; ++i)
              mi.push_back(child0->tensor_proper_size(i-1));
            pnode->t.adjust_sizes(mi);
          }

          if (all_cte) {
            pnode->node_type = GA_NODE_CONSTANT;
            gmm::clear(pnode->tensor().as_vector());
            size_type k = 0;
            for (size_type i = 0, j = 0; i < child0->tensor().size(); ++i) {
             pnode->tensor()[j] += child0->tensor()[i] * child1->tensor()[k];
             ++j; if (j == pnode->tensor().size()) { j = 0; ++k; }
            }
            GMM_ASSERT1(k == child1->tensor().size(), "Internal error");
            tree.clear_children(pnode);
          } else {
            if (child0->tensor_is_zero() || child1->tensor_is_zero()) {
              gmm::clear(pnode->tensor().as_vector());
              pnode->node_type = GA_NODE_ZERO;
              tree.clear_children(pnode);
            }
          }
        }
        break;

      case GA_COLON:
        if (dim1 > 2)
          ga_throw_error(pnode->expr, pnode->pos,
                          "Frobenius product acts only on matrices.")
        else {
          size_type s00 = (dim0 == 0) ? 1
            : (dim0 == 1 ? size0.back() : size0[size0.size()-2]);
          size_type s01 = (dim0 >= 2) ? size0.back() : 1;
          size_type s10 = (dim1 == 0) ? 1
            : (dim1 == 1 ? size1.back() : size1[size1.size()-2]);
          size_type s11 = (dim1 >= 2) ? size1.back() : 1;
          if (s00 != s10 || s01 != s11)
            ga_throw_error(pnode->expr, pnode->pos, "Frobenius product "
                           "of expressions of different sizes ("
                           << s00 << "," << s01 << " != " << s10 << ","
                           << s11 << ").");
          if (child0->tensor_order() <= 2) pnode->symmetric_op = true;
          pnode->mult_test(child0, child1);
          if (dim0 > 2) {
            mi = pnode->t.sizes();
            for (size_type i = 2; i < dim0; ++i)
              mi.push_back(child0->tensor_proper_size(i-2));
            pnode->t.adjust_sizes(mi);
          }
          if (all_cte) {
            pnode->node_type = GA_NODE_CONSTANT;
            gmm::clear(pnode->tensor().as_vector());
            size_type k = 0;
            for (size_type i = 0, j = 0; i < child0->tensor().size(); ++i) {
             pnode->tensor()[j] += child0->tensor()[i] * child1->tensor()[k];
             ++j; if (j == pnode->tensor().size()) { j = 0; ++k; }
            }
            GMM_ASSERT1(k == child1->tensor().size(), "Internal error");
            tree.clear_children(pnode);
          } else {
            if (child0->tensor_is_zero() || child1->tensor_is_zero()) {
              gmm::clear(pnode->tensor().as_vector());
              pnode->node_type = GA_NODE_ZERO;
              tree.clear_children(pnode);
            }
          }
        }
        break;

      case GA_TMULT:
        if (all_cte) {
          pnode->node_type = GA_NODE_CONSTANT;
          pnode->test_function_type = 0;
          if (child0->tensor().size() == 1 && child1->tensor().size() == 1) {
            pnode->init_scalar_tensor
              (child0->tensor()[0] * child1->tensor()[0]);
          } else if (child0->tensor().size() == 1) {
            pnode->t = child1->t;
            gmm::scale(pnode->tensor().as_vector(),
                       scalar_type(child0->tensor()[0]));
          } else if (child1->tensor().size() == 1) {
            pnode->t = child0->t;
            gmm::scale(pnode->tensor().as_vector(),
                       scalar_type(child1->tensor()[0]));
          } else {
            if (dim0+dim1 > 6)
              ga_throw_error(pnode->expr, pnode->pos, "Unauthorized "
                              "tensor multiplication.");
            for (size_type i = 0; i < dim0; ++i)
              mi.push_back(child0->tensor().size(i));
            for (size_type i = 0; i < dim1; ++i)
              mi.push_back(child1->tensor().size(i));
            pnode->t.adjust_sizes(mi);
            size_type n0 = child0->tensor().size();
            size_type n1 = child1->tensor().size();
            for (size_type i = 0; i < n0; ++i)
              for (size_type j = 0; j < n1; ++j)
                pnode->tensor()[i+j*n0]=child0->tensor()[i]*child1->tensor()[j];
          }
          tree.clear_children(pnode);
        } else {
          pnode->mult_test(child0, child1);
          mi = pnode->t.sizes();
          if (child0->tensor_proper_size() != 1
              || child1->tensor_proper_size() != 1) {
            if (child0->tensor_proper_size() == 1) {
              for (size_type i = 0; i < dim1; ++i)
                mi.push_back(child1->tensor_proper_size(i));
            } else if (child1->tensor().size() == 1) {
              for (size_type i = 0; i < dim0; ++i)
                mi.push_back(child0->tensor_proper_size(i));
            } else {
              if (dim0+dim1 > 6)
                ga_throw_error(pnode->expr, pnode->pos, "Unauthorized "
                                "tensor multiplication.");
              for (size_type i = 0; i < dim0; ++i)
                mi.push_back(child0->tensor_proper_size(i));
              for (size_type i = 0; i < dim1; ++i)
                mi.push_back(child1->tensor_proper_size(i));
            }
            pnode->t.adjust_sizes(mi);
          }
          if (child0->tensor_is_zero() || child1->tensor_is_zero()) {
            gmm::clear(pnode->tensor().as_vector());
            pnode->node_type = GA_NODE_ZERO;
            tree.clear_children(pnode);
          }
        }
        break;

      case GA_MULT:
        if (all_cte) {
          pnode->node_type = GA_NODE_CONSTANT;
          pnode->test_function_type = 0;
          if (child0->tensor_proper_size() == 1 &&
              child1->tensor_proper_size() == 1) {
            pnode->init_scalar_tensor(child0->tensor()[0]*child1->tensor()[0]);
          } else if (child0->tensor_proper_size() == 1) {
            pnode->t = child1->t;
            gmm::scale(pnode->tensor().as_vector(), child0->tensor()[0]);
          } else if (child1->tensor_proper_size() == 1) {
            pnode->t = child0->t;
            gmm::scale(pnode->tensor().as_vector(), child1->tensor()[0]);
          } else if (dim0 == 2 && dim1 == 1) {
            size_type m=child0->tensor().size(0), n=child0->tensor().size(1);
            if (n != child1->tensor().size(0))
              ga_throw_error(pnode->expr, pnode->pos,
                             "Incompatible sizes in matrix-vector "
                             "multiplication (" << n << " != "
                             << child1->tensor().size(0) << ").");
            pnode->init_vector_tensor(m);
            gmm::clear(pnode->tensor().as_vector());
            for (size_type i = 0; i < m; ++i)
              for (size_type j = 0; j < n; ++j)
                pnode->tensor()[i] += child0->tensor()(i,j)*child1->tensor()[j];
          } else if (dim0 == 2 && dim1 == 2) {
            size_type m = child0->tensor().size(0);
            size_type n = child0->tensor().size(1);
            size_type p = child1->tensor().size(1);
            if (n != child1->tensor().size(0))
              ga_throw_error(pnode->expr, pnode->pos,
                             "Incompatible sizes in matrix-matrix "
                             "multiplication (" << n << " != "
                             << child1->tensor().size(0) << ").");
            pnode->init_matrix_tensor(m,p);
            gmm::clear(pnode->tensor().as_vector());
            for (size_type i = 0; i < m; ++i)
              for (size_type j = 0; j < n; ++j)
                for (size_type k = 0; k < p; ++k)
                  pnode->tensor()(i,k) += child0->tensor()(i,j)
                                          * child1->tensor()(j,k);
          }
          else if (dim0 == 4 && dim1 == 2) {
            size_type m=child0->tensor().size(0), n=child0->tensor().size(1);
            size_type o=child0->tensor().size(2), p=child0->tensor().size(3);
            if (o != child1->tensor().size(0) || p != child1->tensor().size(1))
              ga_throw_error(pnode->expr, pnode->pos,
                             "Incompatible sizes in tensor-matrix "
                             "multiplication (" << o << "," << p << " != "
                             << child1->tensor().size(0) << ","
                             << child1->tensor().size(1) << ").");
            pnode->init_matrix_tensor(m,n);
            gmm::clear(pnode->tensor().as_vector());
            for (size_type i = 0; i < m; ++i)
              for (size_type j = 0; j < n; ++j)
                for (size_type k = 0; k < o; ++k)
                  for (size_type l = 0; l < p; ++l)
                    pnode->tensor()(i,j) += child0->tensor()(i,j,k,l)
                                            * child1->tensor()(k,l);
          } else ga_throw_error(pnode->expr, pnode->pos,
                                 "Unauthorized multiplication.");
          tree.clear_children(pnode);
        } else {
          pnode->mult_test(child0, child1);
          mi = pnode->t.sizes();

          if (child0->tensor_proper_size() == 1 &&
              child1->tensor_proper_size() == 1) {
            pnode->symmetric_op = true;
          } else if (child0->tensor_proper_size() == 1) {
            pnode->symmetric_op = true;
            for (size_type i = 0; i < dim1; ++i)
                mi.push_back(child1->tensor_proper_size(i));
          } else if (child1->tensor_proper_size() == 1) {
            pnode->symmetric_op = true;
            for (size_type i = 0; i < dim0; ++i)
                mi.push_back(child0->tensor_proper_size(i));
          } else if (child0->tensor_order() == 2 &&
                     child1->tensor_order() == 1) {
            size_type m = child0->tensor_proper_size(0);
            size_type n = child0->tensor_proper_size(1);
            mi.push_back(m);
            if (n != child1->tensor_proper_size(0))
              ga_throw_error(pnode->expr, pnode->pos,
                             "Incompatible sizes in matrix-vector "
                             "multiplication (" << n << " != "
                             << child1->tensor_proper_size(0) << ").");
          } else if (child0->tensor_order() == 2 &&
                     child1->tensor_order() == 2) {
            size_type m = child0->tensor_proper_size(0);
            size_type n = child0->tensor_proper_size(1);
            size_type p = child1->tensor_proper_size(1);
            mi.push_back(m); mi.push_back(p);
            if (n != child1->tensor_proper_size(0))
              ga_throw_error(pnode->expr, pnode->pos,
                             "Incompatible sizes in matrix-matrix "
                             "multiplication (" << n << " != "
                             << child1->tensor_proper_size(0) << ").");
          }
          else if (pnode->children[0]->tensor_order() == 4 &&
                   pnode->children[1]->tensor_order() == 2) {
            size_type m = child0->tensor_proper_size(0);
            size_type n = child0->tensor_proper_size(1);
            size_type o = child0->tensor_proper_size(2);
            size_type p = child0->tensor_proper_size(3);
            mi.push_back(m); mi.push_back(n);
            if (o != child1->tensor_proper_size(0) ||
                p != child1->tensor_proper_size(1))
              ga_throw_error(pnode->expr, pnode->pos,
                             "Incompatible sizes in tensor-matrix "
                             "multiplication (" << o << "," << p << " != "
                             << child1->tensor_proper_size(0) << ","
                             << child1->tensor_proper_size(1) << ").");
          } else ga_throw_error(pnode->expr, pnode->pos,
                                "Unauthorized multiplication.");
          pnode->t.adjust_sizes(mi);
          // Simplifications
          if (child0->tensor_is_zero() || child1->tensor_is_zero()) {
            gmm::clear(pnode->tensor().as_vector());
            pnode->node_type = GA_NODE_ZERO;
            tree.clear_children(pnode);
          } else if (child0->node_type == GA_NODE_CONSTANT &&
                     child0->tensor().size() == 1 &&
                     child0->tensor()[0] == scalar_type(1)) {
            tree.replace_node_by_child(pnode, 1);
            pnode = child1;
          } else if (child1->node_type == GA_NODE_CONSTANT &&
                     child1->tensor().size() == 1 &&
                     child1->tensor()[0] == scalar_type(1)) {
            tree.replace_node_by_child(pnode, 0);
            pnode = child0;
          }
        }
        break;

      case GA_DIV:
        if (child1->tensor_proper_size() > 1)
          ga_throw_error(pnode->expr, pnode->pos,
                         "Only the division by a scalar is allowed. "
                         "Got a size of " << child1->tensor_proper_size());
        if (child1->test_function_type)
          ga_throw_error(pnode->expr, pnode->pos,
                         "Division by test functions is not allowed.");
        if (child1->node_type == GA_NODE_CONSTANT &&
            child1->tensor()[0] == scalar_type(0))
          ga_throw_error(pnode->expr, pnode->children[1]->pos,
			 "Division by zero");

        pnode->t = child0->t;
        pnode->test_function_type = child0->test_function_type;
        pnode->name_test1 = child0->name_test1;
        pnode->name_test2 = child0->name_test2;
        pnode->interpolate_name_test1 = child0->interpolate_name_test1;
        pnode->interpolate_name_test2 = child0->interpolate_name_test2;
        pnode->qdim1 = child0->qdim1;
        pnode->qdim2 = child0->qdim2;

        if (all_cte) {
          pnode->node_type = GA_NODE_CONSTANT;
          pnode->t = pnode->children[0]->t;
          pnode->test_function_type = 0;
          gmm::scale(pnode->tensor().as_vector(),
                     scalar_type(1) / pnode->children[1]->tensor()[0]);
          tree.clear_children(pnode);
        } else if (child0->tensor_is_zero()) {
          gmm::clear(pnode->tensor().as_vector());
          pnode->node_type = GA_NODE_ZERO;
          tree.clear_children(pnode);
        } else if (child1->node_type == GA_NODE_CONSTANT &&
                   child1->tensor().size() == 1 &&
                   child1->tensor()[0] == scalar_type(1)) {
          tree.replace_node_by_child(pnode, 0);
          pnode = child0;
        }
        break;

      default:GMM_ASSERT1(false, "Unexpected operation. Internal error.");
      }
      break;

    case GA_NODE_C_MATRIX:
      {
        if (!all_sc) {
          ga_throw_error(pnode->expr, pnode->pos,
			 "Constant vector/matrix/tensor "
                         "components should be scalar valued.");
        }

        size_type nbc1 = pnode->nbc1, nbc2 = pnode->nbc2, nbc3 = pnode->nbc3;
        size_type nbl = pnode->children.size() / (nbc1*nbc2*nbc3);
        if (all_cte) pnode->node_type = GA_NODE_CONSTANT;
        pnode->test_function_type = 0;
        for (size_type i = 0; i < pnode->children.size(); ++i) {
          if (pnode->children[i]->test_function_type) {
            if (pnode->test_function_type == 0) {
              pnode->test_function_type=pnode->children[i]->test_function_type;
              pnode->name_test1 = pnode->children[i]->name_test1;
              pnode->name_test2 = pnode->children[i]->name_test2;
              pnode->interpolate_name_test1
                = pnode->children[i]->interpolate_name_test1;
              pnode->interpolate_name_test2
                = pnode->children[i]->interpolate_name_test2;
              pnode->qdim1 = pnode->children[i]->qdim1;
              pnode->qdim2 = pnode->children[i]->qdim2;
            } else {
              if (pnode->test_function_type !=
                  pnode->children[i]->test_function_type ||
                  pnode->name_test1.compare(pnode->children[i]->name_test1) ||
                  pnode->name_test2.compare(pnode->children[i]->name_test2) ||
                  pnode->interpolate_name_test1.compare
                  (pnode->children[i]->interpolate_name_test1) ||
                  pnode->interpolate_name_test2.compare
                  (pnode->children[i]->interpolate_name_test2))
                ga_throw_error(pnode->expr, pnode->pos, "Inconsistent use of "
			       "test function in constant matrix.");
            }
          }
        }
        mi.resize(0);
        if (pnode->test_function_type) mi.push_back(2);
        if (pnode->test_function_type >= 3) mi.push_back(2);
        if (nbc1 == 1 && nbc2 == 1 && nbc3 == 1 && nbl == 1) {
          pnode->t.adjust_sizes(mi);
          if (all_cte) pnode->tensor()[0] = child0->tensor()[0];
        } else {
          mi.push_back(nbl);
          if (nbc3 != 1) mi.push_back(nbc3);
          if (nbc2 != 1) mi.push_back(nbc2);
          if (nbc1 != 1) mi.push_back(nbc1);
          pnode->t.adjust_sizes(mi);
          if (all_cte) {
            size_type n = 0;
            if (nbc1 == 1 && nbc2 == 1 && nbc3 == 1)
              for (size_type i = 0; i < nbl; ++i)
                pnode->tensor()[i] = pnode->children[i]->tensor()[0];
            else if (nbc2 == 1 && nbc3 == 1) // TODO: verify order
              for (size_type i = 0; i < nbl; ++i)
                for (size_type j = 0; j < nbc1; ++j)
                  pnode->tensor()(i,j) = pnode->children[n++]->tensor()[0];
            else if (nbc3 == 1) // TODO: verify order
              for (size_type i = 0; i < nbl; ++i)
                for (size_type j = 0; j < nbc2; ++j)
                  for (size_type k = 0; k < nbc1; ++k)
                    pnode->tensor()(i,j,k) = pnode->children[n++]->tensor()[0];
            else // TODO: verify order
              for (size_type i = 0; i < nbl; ++i)
                for (size_type j = 0; j < nbc3; ++j)
                  for (size_type k = 0; k < nbc2; ++k)
                    for (size_type l = 0; l < nbc1; ++l)
                      pnode->tensor()(i,j,k,l)
                        = pnode->children[n++]->tensor()[0];
          }
        }
        if (all_cte) tree.clear_children(pnode);
      }
      break;


    case GA_NODE_NAME:
      {
        std::string name = pnode->name;

        if (!ignore_X && !(name.compare("X"))) {
          pnode->node_type = GA_NODE_X;
          pnode->nbc1 = 0;
          pnode->init_vector_tensor(meshdim);
          break;
        }
	if (!(name.compare("Diff"))) {
	  pnode->test_function_type = 0;
          break;
        }
        if (!(name.compare("element_size"))) {
          pnode->node_type = GA_NODE_ELT_SIZE;
          pnode->init_scalar_tensor(0);
          break;
        }
        if (!(name.compare("element_K"))) {
          pnode->node_type = GA_NODE_ELT_K;
	  if (ref_elt_dim == 1)
	    pnode->init_vector_tensor(meshdim);
	  else
	    pnode->init_matrix_tensor(meshdim, ref_elt_dim);
          break;
        }
        if (!(name.compare("element_B"))) {
          pnode->node_type = GA_NODE_ELT_B;
          pnode->init_matrix_tensor(ref_elt_dim, meshdim);
          break;
        }
        if (!(name.compare("Normal"))) {
          pnode->node_type = GA_NODE_NORMAL;
          pnode->init_vector_tensor(meshdim);
          break;
        }
        if (!(name.compare("Reshape"))) {
          pnode->node_type = GA_NODE_RESHAPE;
          pnode->init_vector_tensor(meshdim);
          break;
        }

        if (name.compare(0, 11, "Derivative_") == 0) {
          name = name.substr(11);
          bool valid = true;
          pnode->der1 = 1; pnode->der2 = 0;
          char *p;
          size_type d = strtol(name.c_str(), &p, 10);
          size_type s = p - name.c_str();
          if (s > 0) {
            pnode->der1 = d;
            if (name[s] != '_') valid = false; else
              name = name.substr(s+1);
          }
          d = strtol(name.c_str(), &p, 10);
          s = p - name.c_str();
          if (s > 0) {
            pnode->der2 = d;
            if (name[s] != '_') valid = false; else
              name = name.substr(s+1);
          }
          if (!valid || pnode->der1 == 0)
            ga_throw_error(pnode->expr, pnode->pos,"Invalid derivative format");
        }

        ga_predef_function_tab::const_iterator it=PREDEF_FUNCTIONS.find(name);
        if (it != PREDEF_FUNCTIONS.end()) {
          // Predefined function found
          pnode->node_type = GA_NODE_PREDEF_FUNC;
          pnode->name = name;
          pnode->test_function_type = 0;
          if (pnode->der1) {
            if (pnode->der1 > it->second.nbargs()
                || pnode->der2 > it->second.nbargs())
              ga_throw_error(pnode->expr, pnode->pos, "Invalid derivative.");
            const ga_predef_function &F = it->second;
            if ((F.ftype() == 0 || F.dtype() == 2) && !(pnode->der2)) {
              pnode->name = ((pnode->der1 == 1) ?
                             F.derivative1() : F.derivative2());
              pnode->der1 = pnode->der2 = 0;
            }
          }
        } else if (SPEC_FUNCTIONS.find(name) != SPEC_FUNCTIONS.end()) {
          // Special function found
          if (pnode->der1)
            ga_throw_error(pnode->expr, pnode->pos, "Special functions do not "
                           "support derivatives.");
          pnode->node_type = GA_NODE_SPEC_FUNC;
          pnode->name = name;
          pnode->test_function_type = 0;
          if (!name.compare("pi")) {
            pnode->node_type = GA_NODE_CONSTANT;
            pnode->init_scalar_tensor(M_PI);
          } else if (!name.compare("meshdim")) {
            pnode->node_type = GA_NODE_CONSTANT;
            pnode->init_scalar_tensor(scalar_type(meshdim));
          } else if (!name.compare("timestep")) {
            pnode->node_type = GA_NODE_CONSTANT;
            pnode->init_scalar_tensor(scalar_type(workspace.get_time_step()));
          }
        } else if (PREDEF_OPERATORS.tab.find(name)
                   != PREDEF_OPERATORS.tab.end()) {
          // Nonlinear operator found
          pnode->node_type = GA_NODE_OPERATOR;
          pnode->name = name;
          pnode->test_function_type = 0;
        } else {
          // Search for a variable name with optional gradient, Hessian,
          // divergence or test functions

          size_type prefix_id = ga_parse_prefix_operator(name);
          size_type test = ga_parse_prefix_test(name);

          if (!(workspace.variable_exists(name)))
            ga_throw_error(pnode->expr, pnode->pos, "Unknown variable, "
			   "function, operator or data " + name);

          if (pnode->der1)
            ga_throw_error(pnode->expr, pnode->pos, "Derivative is for "
			   "functions or operators, not for variables. "
			   "Use Grad instead.");
          pnode->name = name;

          const mesh_fem *mf = workspace.associated_mf(name);
          const im_data *imd = workspace.associated_im_data(name);

          if (test && workspace.is_constant(name) &&
              !(workspace.is_disabled_variable(name)))
            ga_throw_error(pnode->expr, pnode->pos, "Test functions of "
			   "constants are not allowed.");
          if (test == 1) {
            pnode->name_test1 = name;
            pnode->interpolate_name_test1 = "";
            if (option == 1)
              workspace.test1.insert
                (var_trans_pair(pnode->name_test1,
                                pnode->interpolate_name_test1));
            pnode->qdim1 = mf ? workspace.qdim(name)
                              : gmm::vect_size(workspace.value(name));
            if (!(pnode->qdim1))
              ga_throw_error(pnode->expr, pnode->pos,
                             "Invalid null size of variable");
          } else if (test == 2) {
            pnode->name_test2 = name;
            pnode->interpolate_name_test2 = "";
            if (option == 1)
              workspace.test2.insert
                (var_trans_pair(pnode->name_test2,
                                pnode->interpolate_name_test2));
            pnode->qdim2 = mf ? workspace.qdim(name)
                              : gmm::vect_size(workspace.value(name));
            if (!(pnode->qdim2))
              ga_throw_error(pnode->expr, pnode->pos,
                             "Invalid null size of variable");
          }

          if (!mf && (test || !imd)) {
            if (prefix_id)
              ga_throw_error(pnode->expr, pnode->pos, "Gradient, Hessian or "
			"Divergence cannot be evaluated for fixed size data.");
            if (test)
              pnode->node_type = GA_NODE_VAL_TEST;
            else if (eval_fixed_size)
              pnode->node_type = GA_NODE_CONSTANT;
            else
              pnode->node_type = GA_NODE_VAL;

            size_type n = gmm::vect_size(workspace.value(name));
            if (n == 1) {
              if (test) {
                pnode->init_vector_tensor(1);
                pnode->tensor()[0]=scalar_type(1);
              }
              else pnode->init_scalar_tensor(workspace.value(name)[0]);
            } else {
              if (test) {
                pnode->init_matrix_tensor(n,n);
                for (size_type i = 0; i < n; ++i)
                  for (size_type j = 0; j < n; ++j)
                    pnode->tensor()(i,j)
                      = ((i == j) ? scalar_type(1) : scalar_type(0));
              } else {
                pnode->t.adjust_sizes(workspace.qdims(name));
                gmm::copy(workspace.value(name), pnode->tensor().as_vector());
              }
            }
          } else if (!test && imd) {
            if (prefix_id)
              ga_throw_error(pnode->expr, pnode->pos, "Gradient, Hessian or "
			     "Divergence cannot be evaluated for im data.");
            pnode->node_type = GA_NODE_VAL;
            pnode->t.adjust_sizes(workspace.qdims(name));
          } else {
            size_type q = workspace.qdim(name);
            size_type n = mf->linked_mesh().dim();
            bgeot::multi_index mii = workspace.qdims(name);

            if (!q) ga_throw_error(pnode->expr, pnode->pos,
                                   "Invalid null size of variable " << name);
            if (mii.size() > 6)
              ga_throw_error(pnode->expr, pnode->pos,
                            "Tensor with too much dimensions. Limited to 6");

            switch (prefix_id) {
            case 0: // value
              pnode->node_type = test ? GA_NODE_VAL_TEST : GA_NODE_VAL;
              // For Test nodes a first dimension of size equal to 2 has to be
              // prepended by convention (to be adapted later)
              if (test && q == 1 && mii.size() <= 1) {
                mii.resize(1);
                mii[0] = 2;
              } else if (test) {
                mii.insert(mii.begin(), 2);
                pnode->t.adjust_sizes(mii);
              }
              break;
            case 1: // grad
              pnode->node_type = test ? GA_NODE_GRAD_TEST : GA_NODE_GRAD;
              if (test) {
                if (q == 1 && mii.size() <= 1) {
                  mii.resize(1);
                  mii[0] = 2;
                } else
                  mii.insert(mii.begin(), 2);
              }
              if (n > 1) {
                if (mii.size() == 1 && mii[0] == 1) mii[0] = n;
                else mii.push_back(n);
              }
              break;
            case 2: // Hessian
              pnode->node_type = test ? GA_NODE_HESS_TEST : GA_NODE_HESS;
              if (test) {
                if (q == 1 && mii.size() <= 1) {
                  mii.resize(1);
                  mii[0] = 2;
                } else
                  mii.insert(mii.begin(), 2);
              }
              if (n > 1) {
                if (mii.size() == 1 && mii[0] == 1) mii[0] = n;
                else mii.push_back(n);
                mii.push_back(n);
              }
              break;
            case 3: // divergence
              pnode->node_type = test ? GA_NODE_DIVERG_TEST : GA_NODE_DIVERG;
              if (q != n)
                ga_throw_error(pnode->expr, pnode->pos,
                               "Divergence operator can only be applied to"
                               "Fields with qdim (" << q << ") equal to dim ("
                               << n << ")");
              mii.resize(1);
              mii[0] = test ? 2 : 1;
              break;
            }
            pnode->t.adjust_sizes(mii);
          }
          pnode->test_function_type = test;
        }
      }
      break;

    case GA_NODE_PARAMS:
      if (child0->node_type == GA_NODE_NAME) {
	if (child0->name.compare("Diff") == 0) { // Diff operator
	  
	  if (pnode->children.size() != 3)
	    ga_throw_error(pnode->expr, child0->pos,
			   "Bad number of parameters for Diff operator");
	  pga_tree_node child2 = pnode->children[2];
	  if (child2->node_type != GA_NODE_VAL)
	    ga_throw_error(pnode->expr, child2->pos, "Second parameter of "
			   "Diff operator has to be a variable name");
	  std::string vardiff = child2->name;
	  size_type order = child1->test_function_type;
	  if (order > 1)
	    ga_throw_error(pnode->expr, child2->pos, "Cannot derive further "
			   "this order two expression");
	  
	  if (ga_node_mark_tree_for_variable(child1,workspace,me,vardiff,"")) {
	    ga_node_derivation(tree, workspace, me, child1, vardiff,"",order+1);
	    child1 = pnode->children[1];
	    ga_node_analysis(tree, workspace, child1, me, ref_elt_dim,
			     eval_fixed_size, ignore_X, option);
	    child1 = pnode->children[1];
	  } else {
	    mi = child1->t.sizes(); mi.insert(mi.begin(), 2);
	    child1->t.adjust_sizes(mi);
	    child1->node_type = GA_NODE_ZERO;
	    child1->test_function_type = order ? 3 : 1;
	    gmm::clear(child1->tensor().as_vector());
	    tree.clear_children(child1);
	  }
	  tree.replace_node_by_child(pnode, 1);
	  pnode = child1;
	} else
	  ga_throw_error(pnode->expr, child0->pos, "Unknown special operator");
      } else if (child0->node_type == GA_NODE_X) {
        child0->init_scalar_tensor(0);
        if (pnode->children.size() != 2)
          ga_throw_error(pnode->expr, child1->pos,
			 "X stands for the coordinates on "
                         "the real elements. It accepts only one index.");
        if (!(child1->node_type == GA_NODE_CONSTANT) ||
            child1->tensor().size() != 1)
          ga_throw_error(pnode->expr, child1->pos,
			 "Index for X has to be constant and of size 1.");
        child0->nbc1 = size_type(round(child1->tensor()[0]));
        if (child0->nbc1 == 0 || child0->nbc1 > meshdim)
          ga_throw_error(pnode->expr, child1->pos,"Index for X not convenient. "
                         "Found " << child0->nbc1 << " with meshdim = "
                         << meshdim);
        tree.replace_node_by_child(pnode, 0);
        pnode = child0;

      } else if (child0->node_type == GA_NODE_RESHAPE) {
        if (pnode->children.size() < 3)
          ga_throw_error(pnode->expr, child1->pos,
                         "Not enough parameters for Reshape");
        if (pnode->children.size() > 8)
          ga_throw_error(pnode->expr, child1->pos,
                         "Too many parameters for Reshape");
        pnode->t = child1->t;
        pnode->test_function_type = child1->test_function_type;
        pnode->name_test1 = child1->name_test1;
        pnode->name_test2 = child1->name_test2;
        pnode->interpolate_name_test1 = child1->interpolate_name_test1;
        pnode->interpolate_name_test2 = child1->interpolate_name_test2;
        pnode->qdim1 = child1->qdim1;
        pnode->qdim2 = child1->qdim2;
        mi.resize(0);
        for (size_type i = 0; i < pnode->nb_test_functions(); ++i)
          mi.push_back(size1[i]);

        for (size_type i = 2; i < pnode->children.size(); ++i) {
          if (pnode->children[i]->node_type != GA_NODE_CONSTANT)
            ga_throw_error(pnode->expr, pnode->children[i]->pos,"Reshape sizes "
                           "should be constant positive integers.");
          mi.push_back(size_type(round(pnode->children[i]->tensor()[0])));
          if (mi.back() == 0)
            ga_throw_error(pnode->expr, pnode->children[i]->pos,
			   "Wrong zero size for Reshape.");
        }
        size_type total_size(1);
        for (size_type i = 0; i < mi.size(); ++i)
          total_size *= mi[i];
        if (total_size != pnode->tensor().size())
           ga_throw_error(pnode->expr, pnode->pos,"Invalid sizes for reshape.");
        pnode->t.adjust_sizes(mi);

        if (child1->node_type == GA_NODE_CONSTANT) {
          pnode->node_type = GA_NODE_CONSTANT;
          tree.clear_children(pnode);
        } else if (child1->node_type == GA_NODE_ZERO) {
          pnode->node_type = GA_NODE_ZERO;
          tree.clear_children(pnode);
        }
      } else if (child0->node_type == GA_NODE_PREDEF_FUNC) {

        // Evaluation of a predefined function

        for (size_type i = 1; i < pnode->children.size(); ++i)
          ga_valid_operand(pnode->children[i]);
        std::string name = child0->name;
        ga_predef_function_tab::const_iterator it = PREDEF_FUNCTIONS.find(name);
        const ga_predef_function &F = it->second;
        size_type nbargs = F.nbargs();
        if (nbargs+1 != pnode->children.size()) {
            ga_throw_error(pnode->expr, pnode->pos, "Bad number of arguments "
		 "for predefined function " << name << ". Found "
                 << pnode->children.size()-1 << ", should be "<<nbargs << ".");
        }
        pnode->test_function_type = 0;
        pga_tree_node child2 = (nbargs == 2) ? pnode->children[2] : child1;
        all_cte = child1->node_type == GA_NODE_CONSTANT;
        if (nbargs == 2)
          all_cte = all_cte && (child2->node_type == GA_NODE_CONSTANT);
        if (child1->test_function_type || child2->test_function_type)
          ga_throw_error(pnode->expr, pnode->pos, "Test functions cannot be "
			 "passed as argument of a predefined function.");
        if (child1->tensor_order() > 2 || child2->tensor_order() > 2)
          ga_throw_error(pnode->expr, pnode->pos, "Sorry, function can be "
			 "applied to scalar, vector and matrices only.");
        size_type s1 = child1->tensor().size();
        size_type s2 = (nbargs == 2) ? child2->tensor().size() : s1;
        if (s1 != s2 && (s1 != 1 || s2 != 1))
          ga_throw_error(pnode->expr, pnode->pos,
                         "Invalid argument size for a scalar function. "
                         "Size of first argument: " << s1 <<
                         ". Size of second argument: " << s2 << ".");

        if (nbargs == 1) {
          pnode->t = child1->t;
        } else {
          if (s1 == s2) {
            pnode->t = child1->t;
          } else if (s1 == 1) {
            pnode->t = child2->t;
          } else {
            pnode->t = child1->t;
          }
        }

        if (all_cte) {
          if (pnode->der1)
            GMM_ASSERT1(false, "Sorry, to be done");
          pnode->node_type = GA_NODE_CONSTANT;
          if (nbargs == 1) {
            for (size_type i = 0; i < s1; ++i)
              pnode->tensor()[i] = F(child1->tensor()[i]);
          } else {
            if (s1 == s2) {
              for (size_type i = 0; i < s1; ++i)
                pnode->tensor()[i] = F(child1->tensor()[i],
                                       child2->tensor()[i]);
            } else if (s1 == 1) {
              for (size_type i = 0; i < s2; ++i)
                pnode->tensor()[i] = F(child1->tensor()[0],
                                       child2->tensor()[i]);
            } else {
              for (size_type i = 0; i < s1; ++i)
                pnode->tensor()[i] = F(child1->tensor()[i],
                                       child2->tensor()[0]);
            }
          }
          tree.clear_children(pnode);
        }
      } else if (child0->node_type == GA_NODE_SPEC_FUNC) {

        // Special constant functions: meshdim, qdim(u) ...

        for (size_type i = 1; i < pnode->children.size(); ++i)
          ga_valid_operand(pnode->children[i]);
        if (pnode->children.size() != 2)
          ga_throw_error(pnode->expr, pnode->pos,
                         "One and only one argument is allowed for function "
                         +child0->name+".");

        if (!(child0->name.compare("qdim"))) {
          if (child1->node_type != GA_NODE_VAL)
            ga_throw_error(pnode->expr, pnode->pos, "The argument of qdim "
                           "function can only be a variable name.");
          pnode->node_type = GA_NODE_CONSTANT;
          pnode->init_scalar_tensor(scalar_type(workspace.qdim(child1->name)));
          if (pnode->tensor()[0] <= 0)
            ga_throw_error(pnode->expr, pnode->pos,
                           "Invalid null size of variable");
        } else if (!(child0->name.compare("qdims"))) {
          if (child1->node_type != GA_NODE_VAL)
            ga_throw_error(pnode->expr, pnode->pos, "The argument of qdim "
                           "function can only be a variable name.");
          pnode->node_type = GA_NODE_CONSTANT;
          bgeot::multi_index mii = workspace.qdims(child1->name);
          if (mii.size() > 6)
            ga_throw_error(pnode->expr, pnode->pos,
                           "Tensor with too much dimensions. Limited to 6");
          if (mii.size() == 0 || scalar_type(mii[0]) <= 0)
            ga_throw_error(pnode->expr, pnode->pos,
                           "Invalid null size of variable");
          if (mii.size() == 1)
            pnode->init_scalar_tensor(scalar_type(mii[0]));
          if (mii.size() >= 1) {
            pnode->init_vector_tensor(mii.size());
            for (size_type i = 0; i < mii.size(); ++i)
              pnode->tensor()[i] = scalar_type(mii[i]);
          }
        } else if (!(child0->name.compare("Id"))) {
          bool valid = (child1->node_type == GA_NODE_CONSTANT);
          int n = valid ? int(round(child1->tensor()[0])) : -1;
          if (n <= 0 || n > 100 || child1->tensor_order() > 0)
            ga_throw_error(pnode->expr, pnode->pos, "The argument of Id "
                           "should be a (small) positive integer.");
          pnode->node_type = GA_NODE_CONSTANT;
          if (n == 1)
            pnode->init_scalar_tensor(scalar_type(1));
          else {
            pnode->init_matrix_tensor(n,n);
            for (int i = 0; i < n; ++i) pnode->tensor()(i,i) = scalar_type(1);
          }
        } else ga_throw_error(pnode->expr, pnode->children[0]->pos,
                              "Unknown special function.");
        tree.clear_children(pnode);
      } else if (child0->node_type == GA_NODE_OPERATOR) {

        // Call to a nonlinear operator

        for (size_type i = 1; i < pnode->children.size(); ++i)
          ga_valid_operand(pnode->children[i]);
        all_cte = true;
        ga_nonlinear_operator::arg_list args;
        for (size_type i = 1; i < pnode->children.size(); ++i) {
          all_cte = all_cte
            && (pnode->children[i]->node_type == GA_NODE_CONSTANT);
          args.push_back(&(pnode->children[i]->tensor()));
          if (pnode->children[i]->node_type == GA_NODE_ALLINDICES)
            ga_throw_error(pnode->expr, pnode->children[i]->pos,
                           "Colon operator is not allowed in nonlinear "
                           "operator call.");
          if (pnode->children[i]->test_function_type)
            ga_throw_error(pnode->expr, pnode->pos, "Test functions cannot be "
			   "passed as argument of a nonlinear operator.");
          if (pnode->children[i]->tensor_order() > 2)
            ga_throw_error(pnode->expr, pnode->pos,
			   "Sorry, arguments to nonlinear operators should "
			   "only be scalar, vector or matrices");
        }
        ga_predef_operator_tab::T::const_iterator it
          = PREDEF_OPERATORS.tab.find(child0->name);
        const ga_nonlinear_operator &OP = *(it->second);
        mi.resize(0);
        if (!(OP.result_size(args, mi)))
          ga_throw_error(pnode->expr, pnode->pos,
                         "Wrong number or wrong size of arguments for the "
                         "call of nonlinear operator " + child0->name);

        pnode->test_function_type = 0;

        if (child0->der1 > args.size() || child0->der2 > args.size())
           ga_throw_error(pnode->expr, child0->pos,
                         "Invalid derivative number for nonlinear operator "
                          + child0->name);

        if (child0->der1 && child0->der2 == 0) {
          for (size_type i = 0; i < args[child0->der1-1]->sizes().size(); ++i)
            mi.push_back(args[child0->der1-1]->sizes()[i]);
          pnode->t.adjust_sizes(mi);
          if (all_cte) {
            pnode->node_type = GA_NODE_CONSTANT;
            OP.derivative(args, child0->der1, pnode->tensor());
            tree.clear_children(pnode);
          }
        } else if (child0->der1 && child0->der2) {
          for (size_type i = 0; i < args[child0->der1-1]->sizes().size(); ++i)
            mi.push_back(args[child0->der1-1]->sizes()[i]);
          for (size_type i = 0; i < args[child0->der2-1]->sizes().size(); ++i)
            mi.push_back(args[child0->der2-1]->sizes()[i]);
          pnode->t.adjust_sizes(mi);
          if (all_cte) {
            pnode->node_type = GA_NODE_CONSTANT;
            OP.second_derivative(args, child0->der1, child0->der2,
                                 pnode->tensor());
            tree.clear_children(pnode);
          }
        } else {
          pnode->t.adjust_sizes(mi);
          if (all_cte) {
            pnode->node_type = GA_NODE_CONSTANT;
            OP.value(args, pnode->tensor());
            tree.clear_children(pnode);
          }
        }
      } else {

        // Access to components of a tensor
        all_cte = (child0->node_type == GA_NODE_CONSTANT);
        // cout << "child0->tensor_order() = " << child0->tensor_order();
        // cout << endl << "child0->t.sizes() = "
        //      << child0->t.sizes() << endl;
        if (pnode->children.size() != child0->tensor_order() + 1)
          ga_throw_error(pnode->expr, pnode->pos, "Bad number of indices.");
        for (size_type i = 1; i < pnode->children.size(); ++i)
          if (pnode->children[i]->node_type != GA_NODE_ALLINDICES &&
              (pnode->children[i]->node_type != GA_NODE_CONSTANT ||
               pnode->children[i]->tensor().size() != 1))
            ga_throw_error(pnode->expr, pnode->children[i]->pos,
                            "Indices should be constant integers or colon.");

        bgeot::multi_index mi1(size0.size()), mi2, indices;
        for (size_type i = 0; i < child0->tensor_order(); ++i) {
          if (pnode->children[i+1]->node_type == GA_NODE_ALLINDICES) {
            mi2.push_back(child0->tensor_proper_size(i));
            indices.push_back(i);
            mi1[i] = 0;
          } else {
            mi1[i] = size_type(round(pnode->children[i+1]->tensor()[0])-1);
            if (mi1[i] >= child0->tensor_proper_size(i))
              ga_throw_error(pnode->expr, pnode->children[i+1]->pos,
                             "Index out of range, " << mi1[i]+1
                             << ". Should be between 1 and "
                             << child0->tensor_proper_size(i) << ".");
          }
        }
        mi.resize(0);
        for (size_type i = 0; i < child0->nb_test_functions(); ++i)
          mi.push_back(child0->t.sizes()[i]);
        for (size_type i = 0; i < mi2.size(); ++i) mi.push_back(mi2[i]);
        pnode->t.adjust_sizes(mi);
        pnode->test_function_type = child0->test_function_type;
        pnode->name_test1 = child0->name_test1;
        pnode->name_test2 = child0->name_test2;
        pnode->interpolate_name_test1 = child0->interpolate_name_test1;
        pnode->interpolate_name_test2 = child0->interpolate_name_test2;
        pnode->qdim1 = child0->qdim1;
        pnode->qdim2 = child0->qdim2;

        if (all_cte) {
          pnode->node_type = GA_NODE_CONSTANT;
          for (bgeot::multi_index mi3(mi2.size()); !mi3.finished(mi2);
               mi3.incrementation(mi2)) {
            for (size_type j = 0; j < mi2.size(); ++j) {
              mi1[indices[j]] = mi3[j];
            }
            pnode->tensor()(mi3) = pnode->children[0]->tensor()(mi1);
          }
          tree.clear_children(pnode);
        } else {
          if (child0->tensor_is_zero() || child1->tensor_is_zero()) {
            gmm::clear(pnode->tensor().as_vector());
            pnode->node_type = GA_NODE_ZERO;
            tree.clear_children(pnode);
          }
        }
      }
      break;

    default:GMM_ASSERT1(false, "Unexpected node type " << pnode->node_type
                        << " in semantic analysis. Internal error.");
    }
    // cout << " begin hash code = " << pnode->hash_value << endl;
    pnode->hash_value = ga_hash_code(pnode);
    // cout << "node_type = " << pnode->node_type << " op_type = "
    //      << pnode->op_type << " proper hash code = " << pnode->hash_value;
    for (size_type i = 0; i < pnode->children.size(); ++i) {
      pnode->hash_value += (pnode->children[i]->hash_value)
        * 1.0101 * (pnode->symmetric_op ? scalar_type(1) : scalar_type(i+1));
    }
    // cout << " final hash code = " << pnode->hash_value << endl;
  }


  void ga_semantic_analysis(ga_tree &tree,
			    const ga_workspace &workspace,
			    const mesh &m,
			    size_type ref_elt_dim,
			    bool eval_fixed_size,
			    bool ignore_X, int option) {
    GMM_ASSERT1(predef_operators_nonlinear_elasticity_initialized &&
                predef_operators_plasticity_initialized &&
                predef_operators_contact_initialized, "Internal error");
    if (!(tree.root)) return;
    if (option == 1) { workspace.test1.clear(); workspace.test2.clear(); }
    // cout << "semantic analysis of " << ga_tree_to_string(tree) << endl;
    ga_node_analysis(tree, workspace, tree.root, m, ref_elt_dim,
                     eval_fixed_size, ignore_X, option);
    if (tree.root && option == 2) {
      if (((tree.root->test_function_type & 1) &&
           (tree.root->name_test1.compare(workspace.selected_test1.varname)
            || tree.root->interpolate_name_test1.compare
            (workspace.selected_test1.transname)))
          ||
          ((tree.root->test_function_type & 2) &&
           (tree.root->name_test2.compare(workspace.selected_test2.varname)
            || tree.root->interpolate_name_test2.compare
            (workspace.selected_test2.transname))))
        tree.clear();
    }
    // cout << "semantic analysis done " << endl;
    ga_valid_operand(tree.root);
  }


  void ga_extract_factor(ga_tree &result_tree, pga_tree_node pnode,
                                pga_tree_node &new_pnode) {
    result_tree.clear();
    result_tree.copy_node(pnode, 0,  result_tree.root);
    new_pnode = result_tree.root;

    bool minus_sign = false;

    pga_tree_node pnode_child = pnode;
    pnode = pnode->parent;

    while (pnode) {

      size_type nbch = pnode->children.size();
      pga_tree_node child0 = (nbch > 0) ? pnode->children[0] : 0;
      pga_tree_node child1 = (nbch > 1) ? pnode->children[1] : 0;

      switch (pnode->node_type) {

      case GA_NODE_OP:
        switch(pnode->op_type) {
        case GA_PLUS:
          // Nothing to do
          break;
        case GA_MINUS:
          if (child1 == pnode_child) minus_sign = !(minus_sign);
          // A remaining minus sign is added at the end if necessary.
          break;
        case GA_UNARY_MINUS: case GA_QUOTE: case GA_SYM: case GA_SKEW:
        case GA_TRACE: case GA_DEVIATOR: case GA_PRINT:
          // Copy of the term
          result_tree.insert_node(result_tree.root, pnode->node_type);
          result_tree.root->op_type = pnode->op_type;
          result_tree.root->pos = pnode->pos;
          break;
        case GA_DOT: case GA_MULT: case GA_COLON: case GA_TMULT:
        case GA_DOTMULT: case GA_DIV: case GA_DOTDIV:
          // Copy of the term and other child
          result_tree.insert_node(result_tree.root, pnode->node_type);
          result_tree.root->op_type = pnode->op_type;
          result_tree.root->pos = pnode->pos;
          result_tree.root->children.resize(2, nullptr);
	  if (child0 == pnode_child) {
	    result_tree.copy_node(child1, result_tree.root,
                                  result_tree.root->children[1]);
          } else if (child1 == pnode_child) {
            std::swap(result_tree.root->children[1],
		      result_tree.root->children[0]);
            result_tree.copy_node(child0, result_tree.root,
                                  result_tree.root->children[0]);
          } else GMM_ASSERT1(false, "Corrupted tree");
          break;
        default: GMM_ASSERT1(false, "Unexpected operation. Internal error.");
        }
        break;

      case GA_NODE_PARAMS:
        GMM_ASSERT1(child0->node_type == GA_NODE_RESHAPE, "Cannot extract a "
                    "factor which is a parameter of a nonlinear "
                    "operator/function");
        GMM_ASSERT1(child1 == pnode_child, "Cannot extract a factor of a "
                    "Reshape size parameter");
        // Copy of the term and other children
        result_tree.insert_node(result_tree.root, pnode->node_type);
        result_tree.root->pos = pnode->pos;
        result_tree.root->children.resize(pnode->children.size(), nullptr);
	std::swap(result_tree.root->children[1], result_tree.root->children[0]);
        for (size_type i = 0; i < pnode->children.size(); ++i)
          if (i != 1)
            result_tree.copy_node(pnode->children[i], result_tree.root,
                                  result_tree.root->children[i]);
        break;

      case GA_NODE_C_MATRIX:
        result_tree.insert_node(result_tree.root, pnode->node_type);
        result_tree.root->pos = pnode->pos;
        result_tree.root->children.resize(pnode->children.size());
        for (size_type i = 0; i < pnode->children.size(); ++i)
          if (pnode_child == pnode->children[i]) {
            result_tree.root->children[i] = result_tree.root->children[0];
            result_tree.root->children[0] = 0;
          }

        for (size_type i = 0; i < pnode->children.size(); ++i) {
          if (pnode_child == pnode->children[i]) {
            pnode->children[i] = new ga_tree_node(GA_NODE_ZERO, pnode->pos,
						  pnode->expr);
            pnode->children[i]->init_scalar_tensor(scalar_type(0));
            pnode->children[i]->parent = pnode;
          }
        }
        break;

      default: GMM_ASSERT1(false, "Unexpected node type " << pnode->node_type
                           << " in extract constant term. Internal error.");
      }

      pnode_child = pnode;
      pnode = pnode->parent;
    }

    if (minus_sign) {
      result_tree.insert_node(result_tree.root, GA_NODE_OP);
      result_tree.root->op_type = GA_UNARY_MINUS;
      result_tree.root->pos = pnode->children[0]->pos;
    }
  }

  bool ga_node_extract_constant_term
  (ga_tree &tree, pga_tree_node pnode, const ga_workspace &workspace,
   const mesh &m) {
    bool is_constant = true;
    size_type nbch = pnode->children.size();
    pga_tree_node child0 = (nbch > 0) ? pnode->children[0] : 0;
    // pga_tree_node child1 = (nbch > 1) ? pnode->children[1] : 0;
    bool child_0_is_constant = (nbch <= 0) ? true :
      ga_node_extract_constant_term(tree, pnode->children[0], workspace, m);
    bool child_1_is_constant = (nbch <= 1) ? true :
      ga_node_extract_constant_term(tree, pnode->children[1], workspace, m);

    switch (pnode->node_type) {
    case GA_NODE_ZERO: is_constant = false; break;

    case GA_NODE_ELEMENTARY_VAL_TEST: case GA_NODE_ELEMENTARY_GRAD_TEST:
    case GA_NODE_ELEMENTARY_HESS_TEST: case GA_NODE_ELEMENTARY_DIVERG_TEST:
    case GA_NODE_XFEM_PLUS_VAL_TEST: case GA_NODE_XFEM_PLUS_GRAD_TEST:
    case GA_NODE_XFEM_PLUS_HESS_TEST: case GA_NODE_XFEM_PLUS_DIVERG_TEST:
    case GA_NODE_XFEM_MINUS_VAL_TEST: case GA_NODE_XFEM_MINUS_GRAD_TEST:
    case GA_NODE_XFEM_MINUS_HESS_TEST: case GA_NODE_XFEM_MINUS_DIVERG_TEST:
    case GA_NODE_VAL_TEST: case GA_NODE_GRAD_TEST: case GA_NODE_PREDEF_FUNC:
    case GA_NODE_HESS_TEST: case GA_NODE_DIVERG_TEST: case GA_NODE_RESHAPE:
    case GA_NODE_ELT_SIZE: case GA_NODE_ELT_K: case GA_NODE_ELT_B:
    case GA_NODE_CONSTANT: case GA_NODE_X:
    case GA_NODE_NORMAL: case GA_NODE_OPERATOR:
      is_constant = true; break;

    case GA_NODE_ELEMENTARY_VAL: case GA_NODE_ELEMENTARY_GRAD:
    case GA_NODE_ELEMENTARY_HESS: case GA_NODE_ELEMENTARY_DIVERG:
    case GA_NODE_XFEM_PLUS_VAL: case GA_NODE_XFEM_PLUS_GRAD:
    case GA_NODE_XFEM_PLUS_HESS: case GA_NODE_XFEM_PLUS_DIVERG:
    case GA_NODE_XFEM_MINUS_VAL: case GA_NODE_XFEM_MINUS_GRAD:
    case GA_NODE_XFEM_MINUS_HESS: case GA_NODE_XFEM_MINUS_DIVERG:
    case GA_NODE_VAL: case GA_NODE_GRAD: case GA_NODE_HESS:
    case GA_NODE_DIVERG:
      is_constant = workspace.is_constant(pnode->name);
      break;

    case GA_NODE_INTERPOLATE_VAL:
    case GA_NODE_INTERPOLATE_GRAD:
    case GA_NODE_INTERPOLATE_HESS:
    case GA_NODE_INTERPOLATE_DIVERG:
      {
        if (!(workspace.is_constant(pnode->name))) {
          is_constant = false; break;
        }
        std::set<var_trans_pair> vars;
        workspace.interpolate_transformation(pnode->interpolate_name)
          ->extract_variables(workspace, vars, true, m,
                              pnode->interpolate_name);
        for (const var_trans_pair &var : vars) {
          if (!(workspace.is_constant(var.varname)))
            { is_constant = false; break; }
        }
      }
      break;

    case GA_NODE_INTERPOLATE_FILTER:
      if (!child_0_is_constant) { is_constant = false; break; }
      // No break intentionally
    case GA_NODE_INTERPOLATE_VAL_TEST:
    case GA_NODE_INTERPOLATE_GRAD_TEST:
    case GA_NODE_INTERPOLATE_DIVERG_TEST:
    case GA_NODE_INTERPOLATE_HESS_TEST:
    case GA_NODE_INTERPOLATE_X:
    case GA_NODE_INTERPOLATE_NORMAL:
    case GA_NODE_INTERPOLATE_DERIVATIVE:
      {
        std::set<var_trans_pair> vars;
        workspace.interpolate_transformation(pnode->interpolate_name)
          ->extract_variables(workspace, vars, true, m,
                              pnode->interpolate_name);
        for (const var_trans_pair &var : vars) {
          if (!(workspace.is_constant(var.varname)))
            { is_constant = false; break; }
        }
      }
      break;

    case GA_NODE_OP:
      switch(pnode->op_type) {
        case GA_PLUS: case GA_MINUS:
          if (!child_0_is_constant && !child_1_is_constant)
            { is_constant = false; break; }
          break;

      case GA_UNARY_MINUS: case GA_QUOTE: case GA_SYM: case GA_SKEW:
      case GA_TRACE: case GA_DEVIATOR: case GA_PRINT:
        is_constant = child_0_is_constant;
        break;

      case GA_DOT: case GA_MULT: case GA_COLON: case GA_TMULT:
      case GA_DOTMULT: case GA_DIV: case GA_DOTDIV:
        is_constant = (child_0_is_constant && child_1_is_constant);
        break;

      default: GMM_ASSERT1(false, "Unexpected operation. Internal error.");
      }
      break;

    case GA_NODE_C_MATRIX:
      for (size_type i = 0; i < pnode->children.size(); ++i) {
        if (!(ga_node_extract_constant_term(tree, pnode->children[i],
                                            workspace, m)))
          { is_constant = false; break; }
      }
      break;

    case GA_NODE_PARAMS:
      if (child0->node_type == GA_NODE_RESHAPE) {
        is_constant = child_1_is_constant;
      } else if (child0->node_type == GA_NODE_PREDEF_FUNC) {
        for (size_type i = 1; i < pnode->children.size(); ++i) {
          if (!(ga_node_extract_constant_term(tree, pnode->children[i],
                                              workspace, m)))
            { is_constant = false; break; }
        }
      } else if (child0->node_type == GA_NODE_SPEC_FUNC) {
        GMM_ASSERT1(false, "internal error");
      } else if (child0->node_type == GA_NODE_OPERATOR) {
        for (size_type i = 1; i < pnode->children.size(); ++i) {
          if (!(ga_node_extract_constant_term(tree, pnode->children[i],
                                              workspace, m)))
            { is_constant = false; break; }
        }
      } else {
        is_constant = child_0_is_constant;
      }
      break;

    default: GMM_ASSERT1(false, "Unexpected node type " << pnode->node_type
                         << " in extract constant term. Internal error.");
    }

    if (!is_constant) {
      pnode->node_type = GA_NODE_ZERO;
      tree.clear_children(pnode);
    }
    return is_constant;
  }

 
  //=========================================================================
  // Extract Neumann terms
  //=========================================================================

  static std::string ga_extract_one_Neumann_term
  (const std::string &varname,
   ga_workspace &workspace, pga_tree_node pnode) {
    size_type N = workspace.qdim(varname);
    const mesh_fem *mf = workspace.associated_mf(varname);
    GMM_ASSERT1(mf, "Works only with fem variables.");
    size_type meshdim = mf->linked_mesh().dim();
    ga_tree factor;
    pga_tree_node new_pnode = nullptr;
    ga_extract_factor(factor, pnode, new_pnode);
    std::string result;
    pga_tree_node nnew_pnode = new_pnode;

    int cas = new_pnode->node_type == GA_NODE_GRAD_TEST ? 0 : 1;
    // Allow to detect Trace(Grad_Test_u)
    if (cas == 0 && new_pnode->parent &&
        new_pnode->parent->node_type == GA_NODE_OP &&
        new_pnode->parent->op_type == GA_TRACE) {
      cas = 2; nnew_pnode = new_pnode->parent;
    }
    bool ok = true;
    pga_tree_node colon_pnode = nullptr;
    bool quote_before_colon = false;

    // A:Grad_Test_u --> A*Normal if A is a matrix
    // Grad_Test_u:A --> A*Normal if A is a matrix
    // A*Div_Test_u  --> A*Normal if A is a scalar
    // Div_Test_u*A  --> Normal*A if A is a scalar
    // A*(Grad_Test_u)' --> (A)'*Normal if A is a matrix
    // intercaled scalar multplications and divisions are taken into account
    while (nnew_pnode->parent) {
      pga_tree_node previous_node = nnew_pnode;
      nnew_pnode = nnew_pnode->parent;

      if (nnew_pnode->node_type == GA_NODE_OP &&
          nnew_pnode->op_type == GA_MULT &&
          nnew_pnode->children[0] == previous_node &&
          nnew_pnode->children[1]->tensor_proper_size() == 1) {
      } else if (nnew_pnode->node_type == GA_NODE_OP &&
                 nnew_pnode->op_type == GA_MULT &&
                 nnew_pnode->children[1] == previous_node &&
                 nnew_pnode->children[0]->tensor_proper_size() == 1) {

      } else if (nnew_pnode->node_type == GA_NODE_OP &&
                 nnew_pnode->op_type == GA_DIV &&
                 nnew_pnode->children[0] == previous_node &&
                 nnew_pnode->children[1]->tensor_proper_size() == 1) {

      } else if (nnew_pnode->node_type == GA_NODE_OP &&
                 nnew_pnode->op_type == GA_COLON &&
                 nnew_pnode->children[0] == previous_node &&
                 nnew_pnode->children[1]->tensor_order() == 2 &&
                 colon_pnode == 0 && cas == 0) {
        std::swap(nnew_pnode->children[0],  nnew_pnode->children[1]);
        colon_pnode = nnew_pnode;
      } else if (nnew_pnode->node_type == GA_NODE_OP &&
                 nnew_pnode->op_type == GA_COLON &&
                 nnew_pnode->children[1] == previous_node &&
                 nnew_pnode->children[0]->tensor_order() == 2 &&
                 colon_pnode == 0 && cas == 0) {
        colon_pnode = nnew_pnode;
      } else if (nnew_pnode->node_type == GA_NODE_OP &&
                 nnew_pnode->op_type == GA_QUOTE &&
                 colon_pnode == 0 && cas == 0 && !quote_before_colon) {
        quote_before_colon = true;
      } else ok = false;
    }

    // cout << "analyzing factor : " <<  ga_tree_to_string(factor) << endl;
    // cout << "ok = " << int(ok) << endl;
    // cout << "colon_pnode = " << colon_pnode << endl;

    if (ok && cas == 0 && !colon_pnode) ok = false;

    if (N == 1) {
      new_pnode->node_type = GA_NODE_NORMAL;
      result = "(" + ga_tree_to_string(factor) + ")";
    } else if (ok) {
      switch (cas) {
      case 0:
        new_pnode->node_type = GA_NODE_NORMAL;
        colon_pnode->op_type = GA_MULT;
        if (quote_before_colon) {
          factor.insert_node(colon_pnode->children[0], GA_NODE_OP);
          colon_pnode->children[0]->op_type = GA_QUOTE;
          nnew_pnode = new_pnode->parent;
          while(nnew_pnode->node_type != GA_NODE_OP ||
                nnew_pnode->op_type != GA_QUOTE)
            nnew_pnode = nnew_pnode->parent;
          factor.replace_node_by_child(nnew_pnode,0);
        }
        break;
      case 1:
        new_pnode->node_type = GA_NODE_NORMAL;
        break;
      case 2:
        new_pnode->parent->node_type = GA_NODE_NORMAL;
        factor.clear_children(new_pnode->parent);
        break;
      }
      result = "(" + ga_tree_to_string(factor) + ")";

    } else {
      // General case

      result = "[";
      bgeot::multi_index mi(2); mi[0] = N; mi[1] = meshdim;

      for (size_type i = 0; i < N; ++i) {
        factor.clear_children(new_pnode);
        new_pnode->node_type = GA_NODE_C_MATRIX;
        new_pnode->nbc1 = meshdim;
        new_pnode->nbc2 = new_pnode->nbc3 = 1;
        new_pnode->t.adjust_sizes(mi);
        new_pnode->children.resize(N*meshdim);
        for (size_type j = 0; j < N; ++j) {
          for (size_type k = 0; k < meshdim; ++k) {
            if (j == i) {
              pga_tree_node param_node = new_pnode->children[k*N+j]
                = new ga_tree_node(GA_NODE_PARAMS, pnode->pos, pnode->expr);
              new_pnode->children[k*N+j]->parent = new_pnode;
              param_node->children.resize(2);
              param_node->children[0]
		= new ga_tree_node(GA_NODE_NORMAL, pnode->pos, pnode->expr);
              param_node->children[0]->parent = param_node;
              param_node->children[1]
		= new ga_tree_node(GA_NODE_CONSTANT, pnode->pos, pnode->expr);
              param_node->children[1]->parent = param_node;
              param_node->children[1]->init_scalar_tensor(scalar_type(k));

            } else {
              new_pnode->children[k*N+j]
		= new ga_tree_node(GA_NODE_ZERO, pnode->pos, pnode->expr);
              new_pnode->children[k*N+j]->init_scalar_tensor(scalar_type(0));
              new_pnode->children[k*N+j]->parent = new_pnode;
            }
          }
        }
        result += "(" + ga_tree_to_string(factor) + ")";
        if (i < N-1) result += ";";
      }
      result += "]";
      GMM_TRACE2("Warning, generic Neumann term used: " << result);
    }

    return result;
  }


  void ga_extract_Neumann_term
  (ga_tree &tree, const std::string &varname,
   ga_workspace &workspace, pga_tree_node pnode, std::string &result) {

    for (size_type i = 0; i < pnode->children.size(); ++i)
      ga_extract_Neumann_term(tree, varname, workspace,
                                  pnode->children[i], result);


    switch (pnode->node_type) {
    case GA_NODE_DIVERG_TEST: case GA_NODE_GRAD_TEST:
    case GA_NODE_ELEMENTARY_GRAD_TEST: case GA_NODE_ELEMENTARY_DIVERG_TEST:
      if (pnode->name.compare(varname) == 0) {
        if (result.size()) result += " + ";
        result += ga_extract_one_Neumann_term(varname, workspace, pnode);
      }
      break;
    case GA_NODE_INTERPOLATE_GRAD_TEST: case GA_NODE_INTERPOLATE_DIVERG_TEST:
      if (pnode->name.compare(varname) == 0)
        GMM_ASSERT1(false, "Do not know how to extract a "
                    "Neumann term with an interpolate transformation");
      break;
    default:
      break;
    }
  }

  //=========================================================================
  // Derivation algorithm: derivation of a tree with respect to a variable
  //   The result tree is not ready to use. It has to be passed again in
  //   ga_semantic_analysis for enrichment.
  //=========================================================================

  static void ga_node_derivation(ga_tree &tree, const ga_workspace &workspace,
                                 const mesh &m,
                                 pga_tree_node pnode,
                                 const std::string &varname,
                                 const std::string &interpolatename,
                                 size_type order) {

    size_type nbch = pnode->children.size();
    pga_tree_node child0 = (nbch > 0) ? pnode->children[0] : 0;
    pga_tree_node child1 = (nbch > 1) ? pnode->children[1] : 0;
    bool mark0 = ((nbch > 0) ? child0->marked : false);
    bool mark1 = ((nbch > 1) ? child1->marked : false);
    bgeot::multi_index mi;

    const ga_predef_function_tab &PREDEF_FUNCTIONS
      = dal::singleton<ga_predef_function_tab>::instance(0);

    switch (pnode->node_type) {
    case GA_NODE_VAL: case GA_NODE_GRAD:
    case GA_NODE_HESS: case GA_NODE_DIVERG:
      mi.resize(1); mi[0] = 2;
      for (size_type i = 0; i < pnode->tensor_order(); ++i)
        mi.push_back(pnode->tensor_proper_size(i));
      pnode->t.adjust_sizes(mi);
      if (pnode->node_type == GA_NODE_VAL)
        pnode->node_type = GA_NODE_VAL_TEST;
      else if (pnode->node_type == GA_NODE_GRAD)
        pnode->node_type = GA_NODE_GRAD_TEST;
      else if (pnode->node_type == GA_NODE_HESS)
        pnode->node_type = GA_NODE_HESS_TEST;
      else if (pnode->node_type == GA_NODE_DIVERG)
        pnode->node_type = GA_NODE_DIVERG_TEST;
      pnode->test_function_type = order;
      break;

    case GA_NODE_INTERPOLATE_VAL:
    case GA_NODE_INTERPOLATE_GRAD:
    case GA_NODE_INTERPOLATE_HESS:
    case GA_NODE_INTERPOLATE_DIVERG:
      {
        bool is_val(pnode->node_type == GA_NODE_INTERPOLATE_VAL);
        bool is_grad(pnode->node_type == GA_NODE_INTERPOLATE_GRAD);
        bool is_hess(pnode->node_type == GA_NODE_INTERPOLATE_HESS);
        bool is_diverg(pnode->node_type == GA_NODE_INTERPOLATE_DIVERG);

        bool ivar = (pnode->name.compare(varname) == 0 &&
                     pnode->interpolate_name.compare(interpolatename) == 0);
        bool itrans = !ivar;
        if (!itrans) {
          std::set<var_trans_pair> vars;
          workspace.interpolate_transformation(pnode->interpolate_name)
            ->extract_variables(workspace, vars, true, m,
                                pnode->interpolate_name);
          for (const var_trans_pair &var : vars) {
            if (var.varname.compare(varname) == 0 &&
                var.transname.compare(interpolatename) == 0)
              itrans = true;
          }
        }

        pga_tree_node pnode_trans = pnode;
        if (is_hess) {
          GMM_ASSERT1(!itrans, "Sorry, cannot derive a hessian once more");
        } else if (itrans && ivar) {
          tree.duplicate_with_addition(pnode);
          pnode_trans = pnode->parent->children[1];
        }

        if (ivar) {
          mi.resize(1); mi[0] = 2;
          for (size_type i = 0; i < pnode->tensor_order(); ++i)
            mi.push_back(pnode->tensor_proper_size(i));
          pnode->t.adjust_sizes(mi);
          if (is_val) // --> t(Qmult*ndof,Qmult*target_dim)
            pnode->node_type = GA_NODE_INTERPOLATE_VAL_TEST;
          else if (is_grad) // --> t(Qmult*ndof,Qmult*target_dim,N)
            pnode->node_type = GA_NODE_INTERPOLATE_GRAD_TEST;
          else if (is_hess) // --> t(Qmult*ndof,Qmult*target_dim,N,N)
            pnode->node_type = GA_NODE_INTERPOLATE_HESS_TEST;
          else if (is_diverg) // --> t(Qmult*ndof)
            pnode->node_type = GA_NODE_INTERPOLATE_DIVERG_TEST;
          pnode->test_function_type = order;
        }

        if (itrans) {
          const mesh_fem *mf = workspace.associated_mf(pnode_trans->name);
          size_type q = workspace.qdim(pnode_trans->name);
          size_type n = mf->linked_mesh().dim();
          bgeot::multi_index mii = workspace.qdims(pnode_trans->name);

          if (is_val)  // --> t(target_dim*Qmult,N)
            pnode_trans->node_type = GA_NODE_INTERPOLATE_GRAD;
          else if (is_grad || is_diverg)  // --> t(target_dim*Qmult,N,N)
            pnode_trans->node_type = GA_NODE_INTERPOLATE_HESS;

          if (n > 1) {
            if (q == 1 && mii.size() <= 1) { mii.resize(1); mii[0] = n; }
            else mii.push_back(n);

            if (is_grad || is_diverg) mii.push_back(n);
          }
          pnode_trans->t.adjust_sizes(mii);
          tree.duplicate_with_operation(pnode_trans,
                                        (n > 1) ? GA_DOT : GA_MULT);
          pga_tree_node pnode_der = pnode_trans->parent->children[1];
          pnode_der->node_type = GA_NODE_INTERPOLATE_DERIVATIVE;
          if (n == 1)
            pnode_der->init_vector_tensor(2);
          else
            pnode_der->init_matrix_tensor(2, n);
          pnode_der->test_function_type = order;
          pnode_der->name = varname;
          pnode_der->interpolate_name_der = pnode_der->interpolate_name;
          pnode_der->interpolate_name = interpolatename;

          if (is_diverg) { // --> t(Qmult*ndof)
            tree.insert_node(pnode_trans->parent, GA_NODE_OP);
            pga_tree_node pnode_tr = pnode_trans->parent->parent;
            pnode_tr->op_type = GA_TRACE;
            pnode_tr->init_vector_tensor(2);
//            pnode_tr->test_function_type = order;
//            pnode_tr->name_test1 = pnode_trans->name_test1;
//            pnode_tr->name_test2 = pnode_trans->name_test2;
          }
        }
      }
      break;

    case GA_NODE_INTERPOLATE_VAL_TEST:
    case GA_NODE_INTERPOLATE_GRAD_TEST:
    case GA_NODE_INTERPOLATE_DIVERG_TEST:
      {
        bool is_val(pnode->node_type == GA_NODE_INTERPOLATE_VAL_TEST);
        bool is_grad(pnode->node_type == GA_NODE_INTERPOLATE_GRAD_TEST);
        bool is_diverg(pnode->node_type == GA_NODE_INTERPOLATE_DIVERG_TEST);

        pga_tree_node pnode_trans = pnode;
        const mesh_fem *mf = workspace.associated_mf(pnode_trans->name);
        size_type q = workspace.qdim(pnode_trans->name);
        size_type n = mf->linked_mesh().dim();
        bgeot::multi_index mii = workspace.qdims(pnode_trans->name);
        if (is_val) // --> t(Qmult*ndof,Qmult*target_dim,N)
          pnode_trans->node_type = GA_NODE_INTERPOLATE_GRAD_TEST;
        else if (is_grad || is_diverg) // --> t(Qmult*ndof,Qmult*target_dim,N,N)
          pnode_trans->node_type = GA_NODE_INTERPOLATE_HESS_TEST;

        if (q == 1 && mii.size() <= 1) { mii.resize(1); mii[0] = 2; }
        else mii.insert(mii.begin(), 2);

        if (n > 1) {
          mii.push_back(n);
          if (is_grad || is_diverg) mii.push_back(n);
        }
        pnode_trans->t.adjust_sizes(mii);
        // pnode_trans->test_function_type = order;
        tree.duplicate_with_operation(pnode_trans,
                                      (n > 1 ? GA_DOT : GA_MULT));
        pga_tree_node pnode_der = pnode_trans->parent->children[1];
        pnode_der->node_type = GA_NODE_INTERPOLATE_DERIVATIVE;
        if (n == 1)
          pnode_der->init_vector_tensor(2);
        else
          pnode_der->init_matrix_tensor(2, n);
        pnode_der->test_function_type = order;
        pnode_der->name = varname;
        pnode_der->interpolate_name_der = pnode_der->interpolate_name;
        pnode_der->interpolate_name = interpolatename;

        if (is_diverg) { // --> t(Qmult*ndof)
          tree.insert_node(pnode_trans->parent, GA_NODE_OP);
          pga_tree_node pnode_tr = pnode_trans->parent->parent;
          pnode_tr->op_type = GA_TRACE;
          pnode_tr->init_vector_tensor(2);
//          pnode_tr->test_function_type = order;
//          pnode_tr->name_test1 = pnode_trans->name_test1;
//          pnode_tr->name_test2 = pnode_trans->name_test2;
        }
      }
      break;

    case GA_NODE_INTERPOLATE_HESS_TEST:
      GMM_ASSERT1(false, "Sorry, cannot derive a hessian once more");
      break;

    case GA_NODE_INTERPOLATE_X:
      {
        size_type n = m.dim();
        pga_tree_node pnode_der = pnode;
        pnode_der->node_type = GA_NODE_INTERPOLATE_DERIVATIVE;
        if (n == 1)
          pnode_der->init_vector_tensor(2);
        else
          pnode_der->init_matrix_tensor(2, n);
        pnode_der->test_function_type = order;
        pnode_der->name = varname;
        pnode_der->interpolate_name_der = pnode_der->interpolate_name;
        pnode_der->interpolate_name = interpolatename;
      }
      break;

    case GA_NODE_INTERPOLATE_NORMAL:
      GMM_ASSERT1(false, "Sorry, cannot derive the interpolated Normal");
      break;

    case GA_NODE_INTERPOLATE_DERIVATIVE:
      GMM_ASSERT1(false, "Sorry, second order transformation derivative "
                  "not taken into account");
      break;

    case GA_NODE_INTERPOLATE_FILTER:
      ga_node_derivation(tree, workspace, m, child0, varname,
                         interpolatename, order);
      break;

    case GA_NODE_ELEMENTARY_VAL:
    case GA_NODE_ELEMENTARY_GRAD:
    case GA_NODE_ELEMENTARY_HESS:
    case GA_NODE_ELEMENTARY_DIVERG:
    case GA_NODE_XFEM_PLUS_VAL:
    case GA_NODE_XFEM_PLUS_GRAD:
    case GA_NODE_XFEM_PLUS_HESS:
    case GA_NODE_XFEM_PLUS_DIVERG:
    case GA_NODE_XFEM_MINUS_VAL:
    case GA_NODE_XFEM_MINUS_GRAD:
    case GA_NODE_XFEM_MINUS_HESS:
    case GA_NODE_XFEM_MINUS_DIVERG:
      mi.resize(1); mi[0] = 2;
      for (size_type i = 0; i < pnode->tensor_order(); ++i)
        mi.push_back(pnode->tensor_proper_size(i));
      pnode->t.adjust_sizes(mi);
      switch(pnode->node_type) {
      case GA_NODE_ELEMENTARY_VAL:
        pnode->node_type = GA_NODE_ELEMENTARY_VAL_TEST; break;
      case GA_NODE_ELEMENTARY_GRAD:
        pnode->node_type = GA_NODE_ELEMENTARY_GRAD_TEST; break;
      case GA_NODE_ELEMENTARY_HESS:
        pnode->node_type = GA_NODE_ELEMENTARY_HESS_TEST; break;
      case GA_NODE_ELEMENTARY_DIVERG:
        pnode->node_type = GA_NODE_ELEMENTARY_DIVERG_TEST; break;
      case GA_NODE_XFEM_PLUS_VAL:
        pnode->node_type = GA_NODE_XFEM_PLUS_VAL_TEST; break;
      case GA_NODE_XFEM_PLUS_GRAD:
        pnode->node_type = GA_NODE_XFEM_PLUS_GRAD_TEST; break;
      case GA_NODE_XFEM_PLUS_HESS:
        pnode->node_type = GA_NODE_XFEM_PLUS_HESS_TEST; break;
      case GA_NODE_XFEM_PLUS_DIVERG:
        pnode->node_type = GA_NODE_XFEM_PLUS_DIVERG_TEST; break;
      case GA_NODE_XFEM_MINUS_VAL:
        pnode->node_type = GA_NODE_XFEM_MINUS_VAL_TEST; break;
      case GA_NODE_XFEM_MINUS_GRAD:
        pnode->node_type = GA_NODE_XFEM_MINUS_GRAD_TEST; break;
      case GA_NODE_XFEM_MINUS_HESS:
        pnode->node_type = GA_NODE_XFEM_MINUS_HESS_TEST; break;
      case GA_NODE_XFEM_MINUS_DIVERG:
        pnode->node_type = GA_NODE_XFEM_MINUS_DIVERG_TEST; break;
      default : GMM_ASSERT1(false, "internal error");
      }
      pnode->test_function_type = order;
      break;

    case GA_NODE_OP:
      switch(pnode->op_type) {
        case GA_PLUS: case GA_MINUS:
          if (mark0 && mark1) {
            ga_node_derivation(tree, workspace, m, child0, varname,
                               interpolatename, order);
            ga_node_derivation(tree, workspace, m, child1, varname,
                               interpolatename, order);
          } else if (mark0) {
            ga_node_derivation(tree, workspace, m, child0, varname,
                               interpolatename, order);
            tree.replace_node_by_child(pnode, 0);
          } else {
            ga_node_derivation(tree, workspace, m, child1, varname,
                               interpolatename, order);
            if (pnode->op_type == GA_MINUS) {
              pnode->op_type = GA_UNARY_MINUS;
              tree.clear_node(child0);
            }
            else
              tree.replace_node_by_child(pnode, 1);
          }
          break;

      case GA_UNARY_MINUS: case GA_QUOTE: case GA_SYM: case GA_SKEW:
      case GA_TRACE: case GA_DEVIATOR: case GA_PRINT:
        ga_node_derivation(tree, workspace, m, child0, varname,
                           interpolatename, order);
        break;

      case GA_DOT: case GA_MULT: case GA_COLON: case GA_TMULT:
      case GA_DOTMULT:
        if (mark0 && mark1) {
          if (sub_tree_are_equal(child0, child1, workspace, 0) &&
              (pnode->op_type != GA_MULT || child0->tensor_order() < 2)) {
            ga_node_derivation(tree, workspace, m, child1, varname,
                               interpolatename, order);
            tree.insert_node(pnode, GA_NODE_OP);
            pnode->parent->op_type = GA_MULT;
            tree.add_child(pnode->parent);
            pnode->parent->children[1]->node_type = GA_NODE_CONSTANT;
            pnode->parent->children[1]->init_scalar_tensor(scalar_type(2));
          } else {
            tree.duplicate_with_addition(pnode);
            if ((pnode->op_type == GA_COLON && child0->tensor_order() == 2) ||
                (pnode->op_type == GA_DOT && child0->tensor_order() == 1) ||
                pnode->op_type == GA_DOTMULT ||
                (child0->tensor_proper_size()== 1 &&
                 child1->tensor_proper_size()== 1))
              std::swap(pnode->children[0], pnode->children[1]);
            ga_node_derivation(tree, workspace, m, child0, varname,
                               interpolatename, order);
            ga_node_derivation(tree, workspace, m,
                               pnode->parent->children[1]->children[1],
                               varname, interpolatename, order);
          }
        } else if (mark0) {
          ga_node_derivation(tree, workspace, m, child0, varname,
                             interpolatename, order);
        } else
          ga_node_derivation(tree, workspace, m, child1, varname,
                             interpolatename, order);
        break;

      case GA_DIV: case GA_DOTDIV:
        if (mark1) {
          if (pnode->children[0]->node_type == GA_NODE_CONSTANT)
            gmm::scale(pnode->children[0]->tensor().as_vector(),
                       scalar_type(-1));
          else {
            if (mark0) {
              tree.duplicate_with_subtraction(pnode);
              ga_node_derivation(tree, workspace, m, child0, varname,
                                 interpolatename, order);
              pnode = pnode->parent->children[1];
            } else {
              tree.insert_node(pnode, GA_NODE_OP);
              pnode->parent->op_type = GA_UNARY_MINUS;
            }
          }
          tree.insert_node(pnode->children[1], GA_NODE_PARAMS);
          pga_tree_node pnode_param = pnode->children[1];
          tree.add_child(pnode_param);
          std::swap(pnode_param->children[0], pnode_param->children[1]);
          pnode_param->children[0]->node_type = GA_NODE_PREDEF_FUNC;
          pnode_param->children[0]->name = "sqr";
          tree.insert_node(pnode, GA_NODE_OP);
          pga_tree_node pnode_mult = pnode->parent;
          if (pnode->op_type == GA_DOTDIV)
            pnode_mult->op_type = GA_DOTMULT;
          else
            pnode_mult->op_type = GA_MULT;
          pnode_mult->children.resize(2, nullptr);
          tree.copy_node(pnode_param->children[1],
                         pnode_mult, pnode_mult->children[1]);
          ga_node_derivation(tree, workspace, m, pnode_mult->children[1],
                             varname, interpolatename, order);
        } else {
          ga_node_derivation(tree, workspace, m, child0, varname,
                             interpolatename, order);
        }
        break;

      default: GMM_ASSERT1(false, "Unexpected operation. Internal error.");
      }
      break;

    case GA_NODE_C_MATRIX:
      for (size_type i = 0; i < pnode->children.size(); ++i) {
        if (pnode->children[i]->marked)
          ga_node_derivation(tree, workspace, m, pnode->children[i],
                             varname, interpolatename, order);
        else {
          pnode->children[i]->init_scalar_tensor(scalar_type(0));
          pnode->children[i]->node_type = GA_NODE_ZERO;
          tree.clear_children(pnode->children[i]);
        }
      }
      break;

    case GA_NODE_PARAMS:
      if (child0->node_type == GA_NODE_RESHAPE) {
        ga_node_derivation(tree, workspace, m, pnode->children[1],
                           varname, interpolatename, order);
      } else if (child0->node_type == GA_NODE_PREDEF_FUNC) {
        std::string name = child0->name;
        ga_predef_function_tab::const_iterator it = PREDEF_FUNCTIONS.find(name);
        const ga_predef_function &F = it->second;

        if (F.nbargs() == 1) {
          switch (F.dtype()) {
          case 0:
            GMM_ASSERT1(false, "Cannot derive function " << child0->name
                     << ". No derivative provided or not derivable function.");
          case 1:
            child0->name = F.derivative1();
            break;
          case 2: case 3:
            {
              child0->name = "DER_PDFUNC_" + child0->name;
              if (!(ga_function_exists(child0->name))) {
                if (F.dtype() == 2)
                  ga_define_function(child0->name, 1, F.derivative1());
                else {
                  std::string expr=ga_derivative_scalar_function(F.expr(),"t");
                  ga_define_function(child0->name, 1, expr);
                }
              }
              // Inline extension if the derivative is affine (for instance
              // for sqr)
              ga_predef_function_tab::const_iterator
                itp = PREDEF_FUNCTIONS.find(child0->name);
              const ga_predef_function &Fp = itp->second;
              if (Fp.is_affine("t")) {
                scalar_type b = Fp(scalar_type(0));
                scalar_type a = Fp(scalar_type(1)) - b;
                pnode->node_type = GA_NODE_OP;
                pnode->op_type = GA_MULT;
                child0->init_scalar_tensor(a);
                child0->node_type = ((a == scalar_type(0)) ?
                                     GA_NODE_ZERO : GA_NODE_CONSTANT);
                if (b != scalar_type(0)) {
                  tree.insert_node(pnode, GA_NODE_OP);
                  pnode->parent->op_type = (b > 0) ? GA_PLUS : GA_MINUS;
                  tree.add_child(pnode->parent);
                  pga_tree_node pnode_cte = pnode->parent->children[1];
                  pnode_cte->node_type = GA_NODE_CONSTANT;
                  pnode_cte->t = pnode->t;
                  std::fill(pnode_cte->tensor().begin(),
                            pnode_cte->tensor().end(), gmm::abs(b));
                  pnode = pnode->parent;
                }
              }
            }
            break;
          }
          if (pnode->children.size() >= 2) {
            tree.insert_node(pnode, GA_NODE_OP);
            pga_tree_node pnode_op = pnode->parent;
            if (child1->tensor_order() == 0)
              pnode_op->op_type = GA_MULT;
            else
              pnode_op->op_type = GA_DOTMULT;
            pnode_op->children.resize(2, nullptr);
            tree.copy_node(child1, pnode_op, pnode_op->children[1]);
            ga_node_derivation(tree, workspace, m, pnode_op->children[1],
                               varname, interpolatename, order);
          }
        } else {
          pga_tree_node child2 = pnode->children[2];

          if (child1->marked && child2->marked)
            tree.duplicate_with_addition(pnode);

          if (child1->marked) {
            switch (F.dtype()) {
            case 0:
              GMM_ASSERT1(false, "Cannot derive function " << child0->name
                          << ". No derivative provided");
            case 1:
              child0->name = F.derivative1();
              break;
            case 2:
              child0->name = "DER_PDFUNC1_" + child0->name;
              if (!(ga_function_exists(child0->name)))
                ga_define_function(child0->name, 2, F.derivative1());
              break;
            case 3:
              child0->name = "DER_PDFUNC1_" + child0->name;
              if (!(ga_function_exists(child0->name))) {
                std::string expr = ga_derivative_scalar_function(F.expr(), "t");
                ga_define_function(child0->name, 2, expr);
              }
              break;
            }
            tree.insert_node(pnode, GA_NODE_OP);
            pga_tree_node pnode_op = pnode->parent;
            if (child1->tensor_order() == 0)
              pnode_op->op_type = GA_MULT;
            else
              pnode_op->op_type = GA_DOTMULT;
            pnode_op->children.resize(2, nullptr);
            tree.copy_node(child1, pnode_op, pnode_op->children[1]);
            ga_node_derivation(tree, workspace, m, pnode_op->children[1],
                               varname, interpolatename, order);
          }
          if (child2->marked) {
            if (child1->marked && child2->marked)
              pnode = pnode->parent->parent->children[1];
            child0 = pnode->children[0]; child1 = pnode->children[1];
            child2 = pnode->children[2];

            switch (F.dtype()) {
            case 0:
              GMM_ASSERT1(false, "Cannot derive function " << child0->name
                          << ". No derivative provided");
            case 1:
              child0->name = F.derivative2();
              break;
            case 2:
              child0->name = "DER_PDFUNC2_" + child0->name;
              if (!(ga_function_exists(child0->name)))
                ga_define_function(child0->name, 2, F.derivative2());
              break;
            case 3:
              child0->name = "DER_PDFUNC2_" + child0->name;
              if (!(ga_function_exists(child0->name))) {
                std::string expr = ga_derivative_scalar_function(F.expr(), "u");
                ga_define_function(child0->name, 2, expr);
              }
              break;
            }
            tree.insert_node(pnode, GA_NODE_OP);
            pga_tree_node pnode_op = pnode->parent;
            if (child2->tensor_order() == 0)
              pnode_op->op_type = GA_MULT;
            else
              pnode_op->op_type = GA_DOTMULT;
            pnode_op->children.resize(2, nullptr);
            tree.copy_node(child2, pnode_op, pnode_op->children[1]);
            ga_node_derivation(tree, workspace, m, pnode_op->children[1],
                               varname, interpolatename, order);
          }
        }
      } else if (child0->node_type == GA_NODE_SPEC_FUNC) {
        GMM_ASSERT1(false, "internal error");
      } else if (child0->node_type == GA_NODE_OPERATOR) {
        if (child0->der2)
          GMM_ASSERT1(false, "Error in derivation of the assembly string. "
                      "Cannot derive again operator " <<  child0->name);

        size_type nbargs_der = 0;
        for (size_type i = 1; i < pnode->children.size(); ++i)
          if (pnode->children[i]->marked) ++nbargs_der;
        pga_tree_node pnode2 = 0;

        size_type j = 0;
        for (size_type i = 1; i < pnode->children.size(); ++i) {
          if (pnode->children[i]->marked) {
            ++j;
            if (j != nbargs_der) {
              tree.insert_node(pnode, GA_NODE_OP);
              pga_tree_node pnode_op = pnode->parent;
              pnode_op->node_type = GA_NODE_OP;
              pnode_op->op_type = GA_PLUS;
              pnode_op->children.resize(2, nullptr);
              tree.copy_node(pnode, pnode_op , pnode_op->children[1]);
              pnode2 = pnode_op->children[1];
            }
            else pnode2 = pnode;

            if (child0->der1)
              pnode2->children[0]->der2 = i;
            else
              pnode2->children[0]->der1 = i;
            tree.insert_node(pnode2, GA_NODE_OP);
            pga_tree_node pnode_op = pnode2->parent;
            // calcul de l'ordre de reduction
            size_type red = pnode->children[i]->tensor_order();
            switch (red) {
            case 0 : pnode_op->op_type = GA_MULT; break;
            case 1 : pnode_op->op_type = GA_DOT; break;
            case 2 : pnode_op->op_type = GA_COLON; break;
            default: GMM_ASSERT1(false, "Error in derivation of the assembly "
                                 "string. Bad reduction order.")
            }
            pnode_op->children.resize(2, nullptr);
            tree.copy_node(pnode->children[i], pnode_op,
                           pnode_op->children[1]);
            ga_node_derivation(tree, workspace, m, pnode_op->children[1],
                               varname, interpolatename, order);

            if (pnode2->children[0]->name.compare("Norm_sqr") == 0
                && pnode2->children[0]->der1 == 1) {
              pnode2->node_type = GA_NODE_OP;
              pnode2->op_type = GA_MULT;
              pnode2->children[0]->node_type = GA_NODE_CONSTANT;
              pnode2->children[0]->init_scalar_tensor(scalar_type(2));
            }


          }
        }

      } else {
        ga_node_derivation(tree, workspace, m, child0, varname,
                           interpolatename, order);
      }
      break;

    default: GMM_ASSERT1(false, "Unexpected node type " << pnode->node_type
                         << " in derivation. Internal error.");
    }
  }

  // The tree is modified. Should be copied first and passed to
  // ga_semantic_analysis after for enrichment
  void ga_derivative(ga_tree &tree, const ga_workspace &workspace,
		     const mesh &m, const std::string &varname,
		     const std::string &interpolatename, size_type order) {
    // cout << "compute derivative of " << ga_tree_to_string(tree)
    //  << " with respect to " << varname << " : " << interpolatename << endl;
    if (!(tree.root)) return;
    if (ga_node_mark_tree_for_variable(tree.root, workspace, m, varname,
                                       interpolatename))
      ga_node_derivation(tree, workspace, m, tree.root, varname,
                         interpolatename, order);
    else
      tree.clear();
    // cout << "derived tree : " << ga_tree_to_string(tree) << endl;
  }

  //=========================================================================
  // Gradient algorithm: gradient of a tree.
  //   The result tree is not ready to use. It has to be passed again in
  //   ga_semantic_analysis for enrichment.
  //=========================================================================

  static bool ga_node_mark_tree_for_grad(pga_tree_node pnode) {
    bool marked = false;
    for (size_type i = 0; i < pnode->children.size(); ++i)
      if (ga_node_mark_tree_for_grad(pnode->children[i]))
        marked = true;

    bool plain_node(pnode->node_type == GA_NODE_VAL ||
                    pnode->node_type == GA_NODE_GRAD ||
                    pnode->node_type == GA_NODE_HESS ||
                    pnode->node_type == GA_NODE_DIVERG);
    bool test_node(pnode->node_type == GA_NODE_VAL_TEST ||
		   pnode->node_type == GA_NODE_GRAD_TEST ||
		   pnode->node_type == GA_NODE_HESS_TEST ||
		   pnode->node_type == GA_NODE_DIVERG_TEST);
    bool interpolate_node(pnode->node_type == GA_NODE_INTERPOLATE_VAL ||
                          pnode->node_type == GA_NODE_INTERPOLATE_GRAD ||
                          pnode->node_type == GA_NODE_INTERPOLATE_HESS ||
                          pnode->node_type == GA_NODE_INTERPOLATE_DIVERG);
    bool elementary_node(pnode->node_type == GA_NODE_ELEMENTARY_VAL ||
                         pnode->node_type == GA_NODE_ELEMENTARY_GRAD ||
                         pnode->node_type == GA_NODE_ELEMENTARY_HESS ||
                         pnode->node_type == GA_NODE_ELEMENTARY_DIVERG);
    bool xfem_node(pnode->node_type == GA_NODE_XFEM_PLUS_VAL ||
                   pnode->node_type == GA_NODE_XFEM_PLUS_GRAD ||
                   pnode->node_type == GA_NODE_XFEM_PLUS_HESS ||
                   pnode->node_type == GA_NODE_XFEM_PLUS_DIVERG ||
                   pnode->node_type == GA_NODE_XFEM_MINUS_VAL ||
                   pnode->node_type == GA_NODE_XFEM_MINUS_GRAD ||
                   pnode->node_type == GA_NODE_XFEM_MINUS_HESS ||
                   pnode->node_type == GA_NODE_XFEM_MINUS_DIVERG);
    bool interpolate_test_node
      (pnode->node_type == GA_NODE_INTERPOLATE_VAL_TEST ||
       pnode->node_type == GA_NODE_INTERPOLATE_GRAD_TEST ||
       pnode->node_type == GA_NODE_INTERPOLATE_HESS_TEST ||
       pnode->node_type == GA_NODE_INTERPOLATE_DIVERG_TEST);

    if (plain_node || test_node || interpolate_node ||
	elementary_node || xfem_node ||
	pnode->node_type == GA_NODE_X ||
	pnode->node_type == GA_NODE_NORMAL) marked = true;

    if (interpolate_node || interpolate_test_node ||
        pnode->node_type == GA_NODE_INTERPOLATE_X ||
        pnode->node_type == GA_NODE_INTERPOLATE_NORMAL) marked = true;
    
    pnode->marked = marked;
    return marked;
  }

  static void ga_node_grad(ga_tree &tree, const ga_workspace &workspace,
			   const mesh &m, pga_tree_node pnode) {
    
    size_type meshdim = &m ? m.dim() : 1;
    size_type nbch = pnode->children.size();
    pga_tree_node child0 = (nbch > 0) ? pnode->children[0] : 0;
    pga_tree_node child1 = (nbch > 1) ? pnode->children[1] : 0;
    bool mark0 = ((nbch > 0) ? child0->marked : false);
    bool mark1 = ((nbch > 1) ? child1->marked : false);
    bgeot::multi_index mi;

    const ga_predef_function_tab &PREDEF_FUNCTIONS
      = dal::singleton<ga_predef_function_tab>::instance(0);

    switch (pnode->node_type) {
    case GA_NODE_VAL: pnode->node_type = GA_NODE_GRAD; break;
    case GA_NODE_GRAD: pnode->node_type = GA_NODE_HESS; break;
    case GA_NODE_HESS:
      GMM_ASSERT1(false, "Sorry, cannot derive an Hessian once more");
    case GA_NODE_DIVERG: // Hess_u : Id(meshdim)
      pnode->node_type = GA_NODE_HESS;
      tree.duplicate_with_operation(pnode, GA_COLON);
      child0 = pnode; pnode = pnode->parent; child1 = pnode->children[1];
      child1->init_matrix_tensor(meshdim, meshdim);
      for (size_type i = 0; i < meshdim; ++i)
	child1->tensor()(i,i) = scalar_type(1);
      child1->node_type = GA_NODE_CONSTANT;
      break;


    case GA_NODE_INTERPOLATE_HESS_TEST:
    case GA_NODE_INTERPOLATE_HESS:
      GMM_ASSERT1(false, "Sorry, cannot derive a hessian once more");
      break;
      
    case GA_NODE_INTERPOLATE_VAL:
    case GA_NODE_INTERPOLATE_GRAD:
    case GA_NODE_INTERPOLATE_DIVERG:
      {
        bool is_val(pnode->node_type == GA_NODE_INTERPOLATE_VAL);
        bool is_grad(pnode->node_type == GA_NODE_INTERPOLATE_GRAD);
        bool is_diverg(pnode->node_type == GA_NODE_INTERPOLATE_DIVERG);
	
	std::string &tname = pnode->interpolate_name;
	std::string expr_trans = 
	  workspace.interpolate_transformation(tname)->expression();
	if (expr_trans.size() == 0)
	  GMM_ASSERT1(false, "Sorry, the gradient of tranformation "
		      << tname << " cannot be calculated. "
		      "The gradient computation is available only for "
		      "transformations having an explicit expression");


	// bool ivar = (pnode->name.compare(varname) == 0 &&
        //              pnode->interpolate_name.compare(interpolatename) == 0);
        // bool itrans = !ivar;
        // if (!itrans) {
        //   std::set<var_trans_pair> vars;
        //   workspace.interpolate_transformation(pnode->interpolate_name)
        //     ->extract_variables(workspace, vars, true, m,
        //                         pnode->interpolate_name);
        //   for (const var_trans_pair &var : vars) {
        //     if (var.varname.compare(varname) == 0 &&
        //         var.transname.compare(interpolatename) == 0)
        //       itrans = true;
        //   }
        // }
	
	pga_tree_node pnode_trans = pnode;
	tree.duplicate_with_operation(pnode_trans, GA_MULT);
	pga_tree_node pnode_grad_trans = pnode_trans->parent->children[1];
	
	if (is_val) pnode_trans->node_type = GA_NODE_INTERPOLATE_GRAD;
	if (is_grad) pnode_trans->node_type = GA_NODE_INTERPOLATE_HESS;
	// if (is_diverg) ??;
	  
	// Pour le Hessien, il s'agit vraiment d'une multiplication ? ou d'un "." ?

	// Calcul et insertion du gradient de la transformation
	
//         if (ivar) {
//           mi.resize(1); mi[0] = 2;
//           for (size_type i = 0; i < pnode->tensor_order(); ++i)
//             mi.push_back(pnode->tensor_proper_size(i));
//           pnode->t.adjust_sizes(mi);
//           if (is_val) // --> t(Qmult*ndof,Qmult*target_dim)
//             pnode->node_type = GA_NODE_INTERPOLATE_VAL_TEST;
//           else if (is_grad) // --> t(Qmult*ndof,Qmult*target_dim,N)
//             pnode->node_type = GA_NODE_INTERPOLATE_GRAD_TEST;
//           else if (is_hess) // --> t(Qmult*ndof,Qmult*target_dim,N,N)
//             pnode->node_type = GA_NODE_INTERPOLATE_HESS_TEST;
//           else if (is_diverg) // --> t(Qmult*ndof)
//             pnode->node_type = GA_NODE_INTERPOLATE_DIVERG_TEST;
//           pnode->test_function_type = order;
//         }

//         if (itrans) {
//           const mesh_fem *mf = workspace.associated_mf(pnode_trans->name);
//           size_type q = workspace.qdim(pnode_trans->name);
//           size_type n = mf->linked_mesh().dim();
//           bgeot::multi_index mii = workspace.qdims(pnode_trans->name);

//           if (is_val)  // --> t(target_dim*Qmult,N)
//             pnode_trans->node_type = GA_NODE_INTERPOLATE_GRAD;
//           else if (is_grad || is_diverg)  // --> t(target_dim*Qmult,N,N)
//             pnode_trans->node_type = GA_NODE_INTERPOLATE_HESS;

//           if (n > 1) {
//             if (q == 1 && mii.size() <= 1) { mii.resize(1); mii[0] = n; }
//             else mii.push_back(n);

//             if (is_grad || is_diverg) mii.push_back(n);
//           }
//           pnode_trans->t.adjust_sizes(mii);
//           tree.duplicate_with_operation(pnode_trans,
//                                         (n > 1) ? GA_DOT : GA_MULT);
//           pga_tree_node pnode_der = pnode_trans->parent->children[1];
//           pnode_der->node_type = GA_NODE_INTERPOLATE_DERIVATIVE;
//           if (n == 1)
//             pnode_der->init_vector_tensor(2);
//           else
//             pnode_der->init_matrix_tensor(2, n);
//           pnode_der->test_function_type = order;
//           pnode_der->name = varname;
//           pnode_der->interpolate_name_der = pnode_der->interpolate_name;
//           pnode_der->interpolate_name = interpolatename;

//           if (is_diverg) { // --> t(Qmult*ndof)
//             tree.insert_node(pnode_trans->parent, GA_NODE_OP);
//             pga_tree_node pnode_tr = pnode_trans->parent->parent;
//             pnode_tr->op_type = GA_TRACE;
//             pnode_tr->init_vector_tensor(2);
// //            pnode_tr->test_function_type = order;
// //            pnode_tr->name_test1 = pnode_trans->name_test1;
// //            pnode_tr->name_test2 = pnode_trans->name_test2;
//           }
//        }
      }
      break;
  #ifdef continue_here

    case GA_NODE_INTERPOLATE_VAL_TEST:
    case GA_NODE_INTERPOLATE_GRAD_TEST:
    case GA_NODE_INTERPOLATE_DIVERG_TEST:
      {
        bool is_val(pnode->node_type == GA_NODE_INTERPOLATE_VAL_TEST);
        bool is_grad(pnode->node_type == GA_NODE_INTERPOLATE_GRAD_TEST);
        bool is_diverg(pnode->node_type == GA_NODE_INTERPOLATE_DIVERG_TEST);

        pga_tree_node pnode_trans = pnode;
        const mesh_fem *mf = workspace.associated_mf(pnode_trans->name);
        size_type q = workspace.qdim(pnode_trans->name);
        size_type n = mf->linked_mesh().dim();
        bgeot::multi_index mii = workspace.qdims(pnode_trans->name);
        if (is_val) // --> t(Qmult*ndof,Qmult*target_dim,N)
          pnode_trans->node_type = GA_NODE_INTERPOLATE_GRAD_TEST;
        else if (is_grad || is_diverg) // --> t(Qmult*ndof,Qmult*target_dim,N,N)
          pnode_trans->node_type = GA_NODE_INTERPOLATE_HESS_TEST;

        if (q == 1 && mii.size() <= 1) { mii.resize(1); mii[0] = 2; }
        else mii.insert(mii.begin(), 2);

        if (n > 1) {
          mii.push_back(n);
          if (is_grad || is_diverg) mii.push_back(n);
        }
        pnode_trans->t.adjust_sizes(mii);
        // pnode_trans->test_function_type = order;
        tree.duplicate_with_operation(pnode_trans,
                                      (n > 1 ? GA_DOT : GA_MULT));
        pga_tree_node pnode_der = pnode_trans->parent->children[1];
        pnode_der->node_type = GA_NODE_INTERPOLATE_DERIVATIVE;
        if (n == 1)
          pnode_der->init_vector_tensor(2);
        else
          pnode_der->init_matrix_tensor(2, n);
        pnode_der->test_function_type = order;
        pnode_der->name = varname;
        pnode_der->interpolate_name_der = pnode_der->interpolate_name;
        pnode_der->interpolate_name = interpolatename;

        if (is_diverg) { // --> t(Qmult*ndof)
          tree.insert_node(pnode_trans->parent, GA_NODE_OP);
          pga_tree_node pnode_tr = pnode_trans->parent->parent;
          pnode_tr->op_type = GA_TRACE;
          pnode_tr->init_vector_tensor(2);
//          pnode_tr->test_function_type = order;
//          pnode_tr->name_test1 = pnode_trans->name_test1;
//          pnode_tr->name_test2 = pnode_trans->name_test2;
        }
      }
      break;


    case GA_NODE_INTERPOLATE_X:
      {
        size_type n = m.dim();
        pga_tree_node pnode_der = pnode;
        pnode_der->node_type = GA_NODE_INTERPOLATE_DERIVATIVE;
        if (n == 1)
          pnode_der->init_vector_tensor(2);
        else
          pnode_der->init_matrix_tensor(2, n);
        pnode_der->test_function_type = order;
        pnode_der->name = varname;
        pnode_der->interpolate_name_der = pnode_der->interpolate_name;
        pnode_der->interpolate_name = interpolatename;
      }
      break;

    case GA_NODE_INTERPOLATE_NORMAL:
      GMM_ASSERT1(false, "Sorry, cannot derive the interpolated Normal");
      break;

    case GA_NODE_INTERPOLATE_DERIVATIVE:
      GMM_ASSERT1(false, "Sorry, second order transformation derivative "
                  "not taken into account");
      break;

    case GA_NODE_INTERPOLATE_FILTER:
      ga_node_derivation(tree, workspace, m, child0, varname,
                         interpolatename, order);
      break;

    case GA_NODE_ELEMENTARY_VAL:
    case GA_NODE_ELEMENTARY_GRAD:
    case GA_NODE_ELEMENTARY_HESS:
    case GA_NODE_ELEMENTARY_DIVERG:
    case GA_NODE_XFEM_PLUS_VAL:
    case GA_NODE_XFEM_PLUS_GRAD:
    case GA_NODE_XFEM_PLUS_HESS:
    case GA_NODE_XFEM_PLUS_DIVERG:
    case GA_NODE_XFEM_MINUS_VAL:
    case GA_NODE_XFEM_MINUS_GRAD:
    case GA_NODE_XFEM_MINUS_HESS:
    case GA_NODE_XFEM_MINUS_DIVERG:
      mi.resize(1); mi[0] = 2;
      for (size_type i = 0; i < pnode->tensor_order(); ++i)
        mi.push_back(pnode->tensor_proper_size(i));
      pnode->t.adjust_sizes(mi);
      switch(pnode->node_type) {
      case GA_NODE_ELEMENTARY_VAL:
        pnode->node_type = GA_NODE_ELEMENTARY_VAL_TEST; break;
      case GA_NODE_ELEMENTARY_GRAD:
        pnode->node_type = GA_NODE_ELEMENTARY_GRAD_TEST; break;
      case GA_NODE_ELEMENTARY_HESS:
        pnode->node_type = GA_NODE_ELEMENTARY_HESS_TEST; break;
      case GA_NODE_ELEMENTARY_DIVERG:
        pnode->node_type = GA_NODE_ELEMENTARY_DIVERG_TEST; break;
      case GA_NODE_XFEM_PLUS_VAL:
        pnode->node_type = GA_NODE_XFEM_PLUS_VAL_TEST; break;
      case GA_NODE_XFEM_PLUS_GRAD:
        pnode->node_type = GA_NODE_XFEM_PLUS_GRAD_TEST; break;
      case GA_NODE_XFEM_PLUS_HESS:
        pnode->node_type = GA_NODE_XFEM_PLUS_HESS_TEST; break;
      case GA_NODE_XFEM_PLUS_DIVERG:
        pnode->node_type = GA_NODE_XFEM_PLUS_DIVERG_TEST; break;
      case GA_NODE_XFEM_MINUS_VAL:
        pnode->node_type = GA_NODE_XFEM_MINUS_VAL_TEST; break;
      case GA_NODE_XFEM_MINUS_GRAD:
        pnode->node_type = GA_NODE_XFEM_MINUS_GRAD_TEST; break;
      case GA_NODE_XFEM_MINUS_HESS:
        pnode->node_type = GA_NODE_XFEM_MINUS_HESS_TEST; break;
      case GA_NODE_XFEM_MINUS_DIVERG:
        pnode->node_type = GA_NODE_XFEM_MINUS_DIVERG_TEST; break;
      default : GMM_ASSERT1(false, "internal error");
      }
      pnode->test_function_type = order;
      break;

    case GA_NODE_OP:
      switch(pnode->op_type) {
        case GA_PLUS: case GA_MINUS:case GA_NODE_DIVERG:
          if (mark0 && mark1) {
            ga_node_derivation(tree, workspace, m, child0, varname,
                               interpolatename, order);
            ga_node_derivation(tree, workspace, m, child1, varname,
                               interpolatename, order);
          } else if (mark0) {
            ga_node_derivation(tree, workspace, m, child0, varname,
                               interpolatename, order);
            tree.replace_node_by_child(pnode, 0);
          } else {
            ga_node_derivation(tree, workspace, m, child1, varname,
                               interpolatename, order);
            if (pnode->op_type == GA_MINUS) {
              pnode->op_type = GA_UNARY_MINUS;
              tree.clear_node(child0);
            }
            else
              tree.replace_node_by_child(pnode, 1);
          }
          break;

      case GA_UNARY_MINUS: case GA_QUOTE: case GA_SYM: case GA_SKEW:
      case GA_TRACE: case GA_DEVIATOR: case GA_PRINT:
        ga_node_derivation(tree, workspace, m, child0, varname,
                           interpolatename, order);
        break;

      case GA_DOT: case GA_MULT: case GA_COLON: case GA_TMULT:
      case GA_DOTMULT:
        if (mark0 && mark1) {
          if (sub_tree_are_equal(child0, child1, workspace, 0) &&
              (pnode->op_type != GA_MULT || child0->tensor_order() < 2)) {
            ga_node_derivation(tree, workspace, m, child1, varname,
                               interpolatename, order);
            tree.insert_node(pnode, GA_NODE_OP);
            pnode->parent->op_type = GA_MULT;
            tree.add_child(pnode->parent);
            pnode->parent->children[1]->node_type = GA_NODE_CONSTANT;
            pnode->parent->children[1]->init_scalar_tensor(scalar_type(2));
          } else {
            tree.duplicate_with_addition(pnode);
            if ((pnode->op_type == GA_COLON && child0->tensor_order() == 2) ||
                (pnode->op_type == GA_DOT && child0->tensor_order() == 1) ||
                pnode->op_type == GA_DOTMULT ||
                (child0->tensor_proper_size()== 1 &&
                 child1->tensor_proper_size()== 1))
              std::swap(pnode->children[0], pnode->children[1]);
            ga_node_derivation(tree, workspace, m, child0, varname,
                               interpolatename, order);
            ga_node_derivation(tree, workspace, m,
                               pnode->parent->children[1]->children[1],
                               varname, interpolatename, order);
          }
        } else if (mark0) {
          ga_node_derivation(tree, workspace, m, child0, varname,
                             interpolatename, order);
        } else
          ga_node_derivation(tree, workspace, m, child1, varname,
                             interpolatename, order);
        break;

      case GA_DIV: case GA_DOTDIV:
        if (mark1) {
          if (pnode->children[0]->node_type == GA_NODE_CONSTANT)
            gmm::scale(pnode->children[0]->tensor().as_vector(),
                       scalar_type(-1));
          else {
            if (mark0) {
              tree.duplicate_with_subtraction(pnode);
              ga_node_derivation(tree, workspace, m, child0, varname,
                                 interpolatename, order);
              pnode = pnode->parent->children[1];
            } else {
              tree.insert_node(pnode, GA_NODE_OP);
              pnode->parent->op_type = GA_UNARY_MINUS;
            }
          }
          tree.insert_node(pnode->children[1], GA_NODE_PARAMS);
          pga_tree_node pnode_param = pnode->children[1];
          tree.add_child(pnode_param);
          std::swap(pnode_param->children[0], pnode_param->children[1]);
          pnode_param->children[0]->node_type = GA_NODE_PREDEF_FUNC;
          pnode_param->children[0]->name = "sqr";
          tree.insert_node(pnode, GA_NODE_OP);
          pga_tree_node pnode_mult = pnode->parent;
          if (pnode->op_type == GA_DOTDIV)
            pnode_mult->op_type = GA_DOTMULT;
          else
            pnode_mult->op_type = GA_MULT;
          pnode_mult->children.resize(2, nullptr);
          tree.copy_node(pnode_param->children[1],
                         pnode_mult, pnode_mult->children[1]);
          ga_node_derivation(tree, workspace, m, pnode_mult->children[1],
                             varname, interpolatename, order);
        } else {
          ga_node_derivation(tree, workspace, m, child0, varname,
                             interpolatename, order);
        }
        break;

      default: GMM_ASSERT1(false, "Unexpected operation. Internal error.");
      }
      break;

    case GA_NODE_C_MATRIX:
      for (size_type i = 0; i < pnode->children.size(); ++i) {
        if (pnode->children[i]->marked)
          ga_node_derivation(tree, workspace, m, pnode->children[i],
                             varname, interpolatename, order);
        else {
          pnode->children[i]->init_scalar_tensor(scalar_type(0));
          pnode->children[i]->node_type = GA_NODE_ZERO;
          tree.clear_children(pnode->children[i]);
        }
      }
      break;

    case GA_NODE_PARAMS:
      if (child0->node_type == GA_NODE_RESHAPE) {
        ga_node_derivation(tree, workspace, m, pnode->children[1],
                           varname, interpolatename, order);
      } else if (child0->node_type == GA_NODE_PREDEF_FUNC) {
        std::string name = child0->name;
        ga_predef_function_tab::const_iterator it = PREDEF_FUNCTIONS.find(name);
        const ga_predef_function &F = it->second;

        if (F.nbargs() == 1) {
          switch (F.dtype()) {
          case 0:
            GMM_ASSERT1(false, "Cannot derive function " << child0->name
                     << ". No derivative provided or not derivable function.");
          case 1:
            child0->name = F.derivative1();
            break;
          case 2: case 3:
            {
              child0->name = "DER_PDFUNC_" + child0->name;
              if (!(ga_function_exists(child0->name))) {
                if (F.dtype() == 2)
                  ga_define_function(child0->name, 1, F.derivative1());
                else {
                  std::string expr=ga_derivative_scalar_function(F.expr(),"t");
                  ga_define_function(child0->name, 1, expr);
                }
              }
              // Inline extension if the derivative is affine (for instance
              // for sqr)
              ga_predef_function_tab::const_iterator
                itp = PREDEF_FUNCTIONS.find(child0->name);
              const ga_predef_function &Fp = itp->second;
              if (Fp.is_affine("t")) {
                scalar_type b = Fp(scalar_type(0));
                scalar_type a = Fp(scalar_type(1)) - b;
                pnode->node_type = GA_NODE_OP;
                pnode->op_type = GA_MULT;
                child0->init_scalar_tensor(a);
                child0->node_type = ((a == scalar_type(0)) ?
                                     GA_NODE_ZERO : GA_NODE_CONSTANT);
                if (b != scalar_type(0)) {
                  tree.insert_node(pnode, GA_NODE_OP);
                  pnode->parent->op_type = (b > 0) ? GA_PLUS : GA_MINUS;
                  tree.add_child(pnode->parent);
                  pga_tree_node pnode_cte = pnode->parent->children[1];
                  pnode_cte->node_type = GA_NODE_CONSTANT;
                  pnode_cte->t = pnode->t;
                  std::fill(pnode_cte->tensor().begin(),
                            pnode_cte->tensor().end(), gmm::abs(b));
                  pnode = pnode->parent;
                }
              }
            }
            break;
          }
          if (pnode->children.size() >= 2) {
            tree.insert_node(pnode, GA_NODE_OP);
            pga_tree_node pnode_op = pnode->parent;
            if (child1->tensor_order() == 0)
              pnode_op->op_type = GA_MULT;
            else
              pnode_op->op_type = GA_DOTMULT;
            pnode_op->children.resize(2, nullptr);
            tree.copy_node(child1, pnode_op, pnode_op->children[1]);
            ga_node_derivation(tree, workspace, m, pnode_op->children[1],
                               varname, interpolatename, order);
          }
        } else {
          pga_tree_node child2 = pnode->children[2];

          if (child1->marked && child2->marked)
            tree.duplicate_with_addition(pnode);

          if (child1->marked) {
            switch (F.dtype()) {
            case 0:
              GMM_ASSERT1(false, "Cannot derive function " << child0->name
                          << ". No derivative provided");
            case 1:
              child0->name = F.derivative1();
              break;
            case 2:
              child0->name = "DER_PDFUNC1_" + child0->name;
              if (!(ga_function_exists(child0->name)))
                ga_define_function(child0->name, 2, F.derivative1());
              break;
            case 3:
              child0->name = "DER_PDFUNC1_" + child0->name;
              if (!(ga_function_exists(child0->name))) {
                std::string expr = ga_derivative_scalar_function(F.expr(), "t");
                ga_define_function(child0->name, 2, expr);
              }
              break;
            }
            tree.insert_node(pnode, GA_NODE_OP);
            pga_tree_node pnode_op = pnode->parent;
            if (child1->tensor_order() == 0)
              pnode_op->op_type = GA_MULT;
            else
              pnode_op->op_type = GA_DOTMULT;
            pnode_op->children.resize(2, nullptr);
            tree.copy_node(child1, pnode_op, pnode_op->children[1]);
            ga_node_derivation(tree, workspace, m, pnode_op->children[1],
                               varname, interpolatename, order);
          }
          if (child2->marked) {
            if (child1->marked && child2->marked)
              pnode = pnode->parent->parent->children[1];
            child0 = pnode->children[0]; child1 = pnode->children[1];
            child2 = pnode->children[2];

            switch (F.dtype()) {
            case 0:
              GMM_ASSERT1(false, "Cannot derive function " << child0->name
                          << ". No derivative provided");
            case 1:
              child0->name = F.derivative2();
              break;
            case 2:
              child0->name = "DER_PDFUNC2_" + child0->name;
              if (!(ga_function_exists(child0->name)))
                ga_define_function(child0->name, 2, F.derivative2());
              break;
            case 3:
              child0->name = "DER_PDFUNC2_" + child0->name;
              if (!(ga_function_exists(child0->name))) {
                std::string expr = ga_derivative_scalar_function(F.expr(), "u");
                ga_define_function(child0->name, 2, expr);
              }
              break;
            }
            tree.insert_node(pnode, GA_NODE_OP);
            pga_tree_node pnode_op = pnode->parent;
            if (child2->tensor_order() == 0)
              pnode_op->op_type = GA_MULT;
            else
              pnode_op->op_type = GA_DOTMULT;
            pnode_op->children.resize(2, nullptr);
            tree.copy_node(child2, pnode_op, pnode_op->children[1]);
            ga_node_derivation(tree, workspace, m, pnode_op->children[1],
                               varname, interpolatename, order);
          }
        }
      } else if (child0->node_type == GA_NODE_SPEC_FUNC) {
        GMM_ASSERT1(false, "internal error");
      } else if (child0->node_type == GA_NODE_OPERATOR) {
        if (child0->der2)
          GMM_ASSERT1(false, "Error in derivation of the assembly string. "
                      "Cannot derive again operator " <<  child0->name);

        size_type nbargs_der = 0;
        for (size_type i = 1; i < pnode->children.size(); ++i)
          if (pnode->children[i]->marked) ++nbargs_der;
        pga_tree_node pnode2 = 0;

        size_type j = 0;
        for (size_type i = 1; i < pnode->children.size(); ++i) {
          if (pnode->children[i]->marked) {
            ++j;
            if (j != nbargs_der) {
              tree.insert_node(pnode, GA_NODE_OP);
              pga_tree_node pnode_op = pnode->parent;
              pnode_op->node_type = GA_NODE_OP;
              pnode_op->op_type = GA_PLUS;
              pnode_op->children.resize(2, nullptr);
              tree.copy_node(pnode, pnode_op , pnode_op->children[1]);
              pnode2 = pnode_op->children[1];
            }
            else pnode2 = pnode;

            if (child0->der1)
              pnode2->children[0]->der2 = i;
            else
              pnode2->children[0]->der1 = i;
            tree.insert_node(pnode2, GA_NODE_OP);
            pga_tree_node pnode_op = pnode2->parent;
            // calcul de l'ordre de reduction
            size_type red = pnode->children[i]->tensor_order();
            switch (red) {
            case 0 : pnode_op->op_type = GA_MULT; break;
            case 1 : pnode_op->op_type = GA_DOT; break;
            case 2 : pnode_op->op_type = GA_COLON; break;
            default: GMM_ASSERT1(false, "Error in derivation of the assembly "
                                 "string. Bad reduction order.")
            }
            pnode_op->children.resize(2, nullptr);
            tree.copy_node(pnode->children[i], pnode_op,
                           pnode_op->children[1]);
            ga_node_derivation(tree, workspace, m, pnode_op->children[1],
                               varname, interpolatename, order);

            if (pnode2->children[0]->name.compare("Norm_sqr") == 0
                && pnode2->children[0]->der1 == 1) {
              pnode2->node_type = GA_NODE_OP;
              pnode2->op_type = GA_MULT;
              pnode2->children[0]->node_type = GA_NODE_CONSTANT;
              pnode2->children[0]->init_scalar_tensor(scalar_type(2));
            }


          }
        }

      } else {
        ga_node_derivation(tree, workspace, m, child0, varname,
                           interpolatename, order);
      }
      break;

#endif

    default: GMM_ASSERT1(false, "Unexpected node type " << pnode->node_type
                         << " in derivation. Internal error.");
    }
  }
 
  
  // The tree is modified. Should be copied first and passed to
  // ga_semantic_analysis after for enrichment
  void ga_grad(ga_tree &tree, const ga_workspace &workspace, const mesh &m) {
    // cout << "compute gradient of " << ga_tree_to_string(tree) << endl;
    if (!(tree.root)) return;
    if (ga_node_mark_tree_for_grad(tree.root))
      ga_node_grad(tree, workspace, m, tree.root);
    else
      tree.clear();
    // cout << "gradient tree : " << ga_tree_to_string(tree) << endl;
  }



  static void ga_replace_test_by_cte(pga_tree_node pnode,  bool full_replace) {
    for (size_type i = 0; i < pnode->children.size(); ++i)
      ga_replace_test_by_cte(pnode->children[i], full_replace);
    GMM_ASSERT1(pnode->node_type != GA_NODE_GRAD_TEST, "Invalid tree");
    GMM_ASSERT1(pnode->node_type != GA_NODE_HESS_TEST, "Invalid tree");
    GMM_ASSERT1(pnode->node_type != GA_NODE_DIVERG_TEST, "Invalid tree");
    if (pnode->node_type == GA_NODE_VAL_TEST) {
      pnode->node_type = GA_NODE_CONSTANT;
      if (full_replace) pnode->init_scalar_tensor(scalar_type(1));
    }
  }

  std::string ga_derivative_scalar_function(const std::string &expr,
					    const std::string &var) {
    base_vector t(1), u(1);
    ga_workspace workspace;
    workspace.add_fixed_size_variable("t", gmm::sub_interval(0,1), t);
    workspace.add_fixed_size_variable("u", gmm::sub_interval(0,1), u);
    workspace.add_function_expression(expr);
    GMM_ASSERT1(workspace.nb_trees() <= 1, "Internal error");
    if (workspace.nb_trees()) {
      ga_tree tree = *(workspace.tree_info(0).ptree);
      ga_derivative(tree, workspace, *((const mesh *)(0)), var, "", 1);
      if (tree.root) {
        ga_replace_test_by_cte(tree.root, true);
        ga_semantic_analysis(tree, workspace, *((const mesh *)(0)), 1,
			     false, true);
      }
      return ga_tree_to_string(tree);
    } else return "0";
  }

  static bool ga_node_is_affine(const pga_tree_node pnode) {

    size_type nbch = pnode->children.size();
    pga_tree_node child0 = (nbch > 0) ? pnode->children[0] : 0;
    pga_tree_node child1 = (nbch > 1) ? pnode->children[1] : 0;
    bool mark0 = ((nbch > 0) ? child0->marked : false);
    bool mark1 = ((nbch > 1) ? child1->marked : false);

    switch (pnode->node_type) {
    case GA_NODE_VAL: case GA_NODE_GRAD:
    case GA_NODE_HESS: case GA_NODE_DIVERG:
    case GA_NODE_INTERPOLATE_VAL: case GA_NODE_INTERPOLATE_GRAD:
    case GA_NODE_INTERPOLATE_HESS: case GA_NODE_INTERPOLATE_DIVERG:
    case GA_NODE_INTERPOLATE_DERIVATIVE:
    case GA_NODE_ELEMENTARY_VAL: case GA_NODE_ELEMENTARY_GRAD:
    case GA_NODE_ELEMENTARY_HESS: case GA_NODE_ELEMENTARY_DIVERG:
    case GA_NODE_XFEM_PLUS_VAL: case GA_NODE_XFEM_PLUS_GRAD:
    case GA_NODE_XFEM_PLUS_HESS: case GA_NODE_XFEM_PLUS_DIVERG:
    case GA_NODE_XFEM_MINUS_VAL: case GA_NODE_XFEM_MINUS_GRAD:
    case GA_NODE_XFEM_MINUS_HESS: case GA_NODE_XFEM_MINUS_DIVERG:
      return true;
    case GA_NODE_INTERPOLATE_FILTER:
       return ga_node_is_affine(child0);
    case GA_NODE_OP:
      switch(pnode->op_type) {
      case GA_PLUS: case GA_MINUS:
        if (mark0 && mark1)
          return ga_node_is_affine(child0) &&
            ga_node_is_affine(child1);
        if (mark0) return ga_node_is_affine(child0);
        return ga_node_is_affine(child1);

      case GA_UNARY_MINUS: case GA_QUOTE: case GA_SYM: case GA_SKEW:
      case GA_TRACE: case GA_DEVIATOR: case GA_PRINT:
        return ga_node_is_affine(child0);

      case GA_DOT: case GA_MULT: case GA_COLON: case GA_TMULT:
      case GA_DOTMULT:
        if (mark0 && mark1) return false;
        if (mark0) return ga_node_is_affine(child0);
        return ga_node_is_affine(child1);

      case GA_DIV: case GA_DOTDIV:
        if (mark1) return false;
        if (mark0) return ga_node_is_affine(child0);

      default: GMM_ASSERT1(false, "Unexpected operation. Internal error.");
      }
      break;

    case GA_NODE_C_MATRIX:
        for (size_type i = 0; i < pnode->children.size(); ++i)
          if (pnode->children[i]->marked &&
              !(ga_node_is_affine(pnode->children[i])))
            return false;
        return true;

    case GA_NODE_PARAMS:
      if (child0->node_type == GA_NODE_RESHAPE)
        return ga_node_is_affine(child1);
      if (child0->node_type == GA_NODE_PREDEF_FUNC)
        return false;
      if (child0->node_type == GA_NODE_OPERATOR)
        return false;
      return ga_node_is_affine(child0);

    default: GMM_ASSERT1(false, "Unexpected node type " << pnode->node_type
                         << " in derivation. Internal error.");
    }
  }

  bool ga_is_affine(const ga_tree &tree, const ga_workspace &workspace,
		    const std::string &varname,
		    const std::string &interpolatename) {
    const mesh &m = *((const mesh *)(0));
    if (tree.root && ga_node_mark_tree_for_variable(tree.root, workspace, m,
                                                    varname, interpolatename))
      return ga_node_is_affine(tree.root);
    return true;
  }

} /* end of namespace */
