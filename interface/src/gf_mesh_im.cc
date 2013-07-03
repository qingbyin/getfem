/*===========================================================================
 
 Copyright (C) 2006-2012 Yves Renard, Julien Pommier.
 
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

#include <getfemint_misc.h>
#include <getfemint_workspace.h>
#include <getfemint_mesh_im.h>
#include <getfemint_mesh.h>
#include <getfemint_mesh_levelset.h>
#include <getfem/getfem_mesh_im_level_set.h>

using namespace getfemint;

void gf_mesh_im_set_integ(getfem::mesh_im *mim, getfemint::mexargs_in& in);

/*@GFDOC
  This object represents an integration method defined on a whole mesh (an 
  potentialy on its boundaries).
@*/


// Object for the declaration of a new sub-command.

typedef  getfemint_mesh_im *pgetfemint_mesh_im;

struct sub_gf_mim : virtual public dal::static_stored_object {
  int arg_in_min, arg_in_max, arg_out_min, arg_out_max;
  virtual void run(getfemint::mexargs_in& in,
		   getfemint::mexargs_out& out,
		   getfemint_mesh *mm, pgetfemint_mesh_im &mim) = 0;
};

typedef boost::intrusive_ptr<sub_gf_mim> psub_command;

// Function to avoid warning in macro with unused arguments.
template <typename T> static inline void dummy_func(T &) {}

#define sub_command(name, arginmin, arginmax, argoutmin, argoutmax, code) { \
    struct subc : public sub_gf_mim {					\
      virtual void run(getfemint::mexargs_in& in,			\
		       getfemint::mexargs_out& out,			\
		       getfemint_mesh *mm, pgetfemint_mesh_im &mim)	\
      { dummy_func(in); dummy_func(out); dummy_func(mm); code }		\
    };									\
    psub_command psubc = new subc;					\
    psubc->arg_in_min = arginmin; psubc->arg_in_max = arginmax;		\
    psubc->arg_out_min = argoutmin; psubc->arg_out_max = argoutmax;	\
    subc_tab[cmd_normalize(name)] = psubc;				\
  }




void gf_mesh_im(getfemint::mexargs_in& m_in, getfemint::mexargs_out& m_out) {
  typedef std::map<std::string, psub_command > SUBC_TAB;
  static SUBC_TAB subc_tab;

  if (subc_tab.size() == 0) {


    /*@INIT MIM = ('load', @str fname[, @tmesh m])
      Load a @tmim from a file.

      If the mesh `m` is not supplied (this kind of file does not store the
      mesh), then it is read from the file and its descriptor is returned as
      the second output argument.@*/
    sub_command
      ("load", 1, 2, 0, 1,
       std::string fname = in.pop().to_string();
       if (in.remaining()) mm = in.pop().to_getfemint_mesh();
       else {
	 getfem::mesh *m = new getfem::mesh();
	 m->read_from_file(fname);
	 mm = getfemint_mesh::get_from(m);
       }
       mim = getfemint_mesh_im::new_from(mm);
       mim->mesh_im().read_from_file(fname);
       );


    /*@INIT MIM = ('from string', @str s[, @tmesh m])
      Create a @tmim object from its string description.

      See also ``MESH_IM:GET('char')``@*/
    sub_command
      ("from string", 1, 2, 0, 1,
       std::stringstream ss(in.pop().to_string());
       if (in.remaining()) mm = in.pop().to_getfemint_mesh();
       else {
	 getfem::mesh *m = new getfem::mesh();
	 m->read_from_file(ss);
	 mm = getfemint_mesh::get_from(m);
       }
       mim = getfemint_mesh_im::new_from(mm);
       mim->mesh_im().read_from_file(ss);
       );


    /*@INIT MIM = ('clone', @tmim mim)
      Create a copy of a @tmim.@*/
    sub_command
      ("clone", 1, 1, 0, 1,
       getfemint_mesh_im *mim2 = in.pop().to_getfemint_mesh_im();
       mm = object_to_mesh(workspace().object(mim2->linked_mesh_id()));
       mim = getfemint_mesh_im::new_from(mm);
       std::stringstream ss; /* not very elegant ! */
       mim2->mesh_im().write_to_file(ss);
       mim->mesh_im().read_from_file(ss);
       );


    /*@INIT MIM = ('levelset', @tmls mls, @str where, @tinteg im[, @tinteg im_tip[, @tinteg im_set]])
      Build an integration method conformal to a partition defined
      implicitely by a levelset.

      The `where` argument define the domain of integration with respect to
      the levelset, it has to be chosen among 'ALL', 'INSIDE', 'OUTSIDE' and
      'BOUNDARY'.

      it can be completed by a string defining the boolean operation
      to define the integration domain when there is more than one levelset.

      the syntax is very simple, for example if there are 3 different
      levelset,
       
       "a*b*c" is the intersection of the domains defined by each
       levelset (this is the default behaviour if this function is not
       called).

       "a+b+c" is the union of their domains.

       "c-(a+b)" is the domain of the third levelset minus the union of
       the domains of the two others.
       
       "!a" is the complementary of the domain of a (i.e. it is the
       domain where a(x)>0)

       The first levelset is always referred to with "a", the second
       with "b", and so on.
      for intance INSIDE(a*b*c)

      CAUTION: this integration method will be defined only on the element
      cut by the level-set. For the 'ALL', 'INSIDE' and 'OUTSIDE' options
      it is mandatory to use the method ``MESH_IM:SET('integ')`` to define
      the integration method on the remaining elements.
      @*/
    sub_command
      ("levelset", 3, 5, 0, 1,
       getfemint_mesh_levelset *gmls = in.pop().to_getfemint_mesh_levelset();
       std::string swhere = in.pop().to_string();
       getfem::pintegration_method pim  = in.pop().to_integration_method();
       getfem::pintegration_method pim2 = 0;
       getfem::pintegration_method pim3 = 0;
       if (in.remaining()) pim2 = in.pop().to_integration_method();
       if (in.remaining()) pim3 = in.pop().to_integration_method();
       int where = 0;
       std::string csg_description;
       if (cmd_strmatch(swhere, "all"))
 	 where = getfem::mesh_im_level_set::INTEGRATE_ALL;
       else {
	 const char *slst[4];
	 slst[0] = "inside";
	 slst[1] = "outside";
	 slst[2] = "boundary";
	 slst[3] = "all";
 	 for (unsigned i=0; i < 4; ++i) {
 	   if (cmd_strmatchn(swhere, slst[i], unsigned(strlen(slst[i])))) {
 	     csg_description.assign(swhere.begin() + strlen(slst[i]),
				    swhere.end());
 	     if (i == 0)
	       where = getfem::mesh_im_level_set::INTEGRATE_INSIDE;
 	     else if (i == 1)
	       where = getfem::mesh_im_level_set::INTEGRATE_OUTSIDE;
 	     else if (i == 2)
	       where = getfem::mesh_im_level_set::INTEGRATE_BOUNDARY;
 	     else if (i == 3)
	       where = getfem::mesh_im_level_set::INTEGRATE_ALL;
 	   }
	 }
       }
       if (where == 0) {
 	 THROW_BADARG("expecting 'inside', 'outside', 'boundary' or 'all'");
       }
       if (pim->type() != getfem::IM_APPROX) {
 	 THROW_BADARG("expecting an approximate integration method");
       }

       getfem::mesh_im_level_set *mimls =
       new getfem::mesh_im_level_set(gmls->mesh_levelset(),
 				     where, pim, pim2);
       if (pim3)
	 mimls->set_integration_method(mimls->linked_mesh().convex_index(),
				       pim3);
       else
	 mimls->set_integration_method(mimls->linked_mesh().convex_index(), 1);
       if (csg_description.size()) {
 	 mimls->set_level_set_boolean_operations(csg_description);
       }
       mim = getfemint_mesh_im::get_from(mimls);
       mimls->adapt();
       workspace().set_dependance(mim, gmls);
       );

  }


  if (m_in.narg() < 1) THROW_BADARG("Wrong number of input arguments");
  getfemint_mesh *mm = NULL;
  getfemint_mesh_im *mim = NULL;
  if (m_in.front().is_string()) {

    std::string init_cmd   = m_in.pop().to_string();
    std::string cmd        = cmd_normalize(init_cmd);


    SUBC_TAB::iterator it = subc_tab.find(cmd);
    if (it != subc_tab.end()) {
      check_cmd(cmd, it->first.c_str(), m_in, m_out, it->second->arg_in_min,
		it->second->arg_in_max, it->second->arg_out_min,
		it->second->arg_out_max);
      it->second->run(m_in, m_out, mm, mim);
    }
    else bad_cmd(init_cmd);

  } else {
    /*@INIT MIM = ('.mesh', @tmesh m, [{@tinteg im|int im_degree}])
      Build a new @tmim object.

      For convenience, optional arguments (`im` or `im_degree`) can be
      provided, in that case a call to ``MESH_IM:GET('integ')`` is issued
      with these arguments.@*/
    if (!m_out.narg_in_range(1, 1))
      THROW_BADARG("Wrong number of output arguments");
    mm = m_in.pop().to_getfemint_mesh();
    mim = getfemint_mesh_im::new_from(mm);
    if (m_in.remaining()) {
      gf_mesh_im_set_integ(&mim->mesh_im(), m_in);
    }
    if (m_in.remaining()) THROW_BADARG("Wrong number of input arguments");
  }

  m_out.pop().from_object_id(mim->get_id(), MESHIM_CLASS_ID);
}
