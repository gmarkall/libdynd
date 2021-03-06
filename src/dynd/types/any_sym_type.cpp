//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <dynd/types/any_sym_type.hpp>
#include <dynd/types/type_alignment.hpp>
#include <dynd/shape_tools.hpp>
#include <dynd/exceptions.hpp>
#include <dynd/kernels/assignment_kernels.hpp>
#include <dynd/func/callable.hpp>
#include <dynd/func/make_callable.hpp>
#include <dynd/types/builtin_type_properties.hpp>
#include <dynd/kernels/string_assignment_kernels.hpp>

using namespace std;
using namespace dynd;

ndt::any_sym_type::any_sym_type()
    : base_type(any_sym_type_id, kind_kind, 0, 1,
                type_flag_symbolic | type_flag_variadic, 0, 0, 0)
{
}

ndt::any_sym_type::~any_sym_type() {}

size_t ndt::any_sym_type::get_default_data_size() const
{
  stringstream ss;
  ss << "Cannot get default data size of type " << type(this, true);
  throw runtime_error(ss.str());
}

void ndt::any_sym_type::print_data(std::ostream &DYND_UNUSED(o),
                                   const char *DYND_UNUSED(arrmeta),
                                   const char *DYND_UNUSED(data)) const
{
  throw type_error("Cannot store data of symbolic any_sym type");
}

void ndt::any_sym_type::print_type(std::ostream &o) const { o << "Any"; }

bool ndt::any_sym_type::is_expression() const { return false; }

bool
ndt::any_sym_type::is_unique_data_owner(const char *DYND_UNUSED(arrmeta)) const
{
  return false;
}

void ndt::any_sym_type::transform_child_types(
    type_transform_fn_t DYND_UNUSED(transform_fn),
    intptr_t DYND_UNUSED(arrmeta_offset), void *DYND_UNUSED(extra),
    type &out_transformed_tp, bool &DYND_UNUSED(out_was_transformed)) const
{
  out_transformed_tp = type(this, true);
}

ndt::type ndt::any_sym_type::get_canonical_type() const
{
  return type(this, true);
}

ndt::type
ndt::any_sym_type::at_single(intptr_t DYND_UNUSED(i0),
                             const char **DYND_UNUSED(inout_arrmeta),
                             const char **DYND_UNUSED(inout_data)) const
{
  return type(this, true);
}

ndt::type
ndt::any_sym_type::get_type_at_dimension(char **DYND_UNUSED(inout_arrmeta),
                                         intptr_t DYND_UNUSED(i),
                                         intptr_t DYND_UNUSED(total_ndim)) const
{
  return type(this, true);
}

intptr_t ndt::any_sym_type::get_dim_size(const char *DYND_UNUSED(arrmeta),
                                         const char *DYND_UNUSED(data)) const
{
  return -1;
}

void ndt::any_sym_type::get_shape(intptr_t ndim, intptr_t i,
                                  intptr_t *out_shape,
                                  const char *DYND_UNUSED(arrmeta),
                                  const char *DYND_UNUSED(data)) const
{
  for (; i < ndim; ++i) {
    out_shape[i] = -1;
  }
}

bool
ndt::any_sym_type::is_lossless_assignment(const type &DYND_UNUSED(dst_tp),
                                          const type &DYND_UNUSED(src_tp)) const
{
  return false;
}

bool ndt::any_sym_type::operator==(const base_type &rhs) const
{
  if (this == &rhs) {
    return true;
  } else {
    return rhs.get_type_id() == any_sym_type_id;
  }
}

void ndt::any_sym_type::arrmeta_default_construct(
    char *DYND_UNUSED(arrmeta), bool DYND_UNUSED(blockref_alloc)) const
{
  stringstream ss;
  ss << "Cannot default construct arrmeta for symbolic type "
     << type(this, true);
  throw runtime_error(ss.str());
}

void ndt::any_sym_type::arrmeta_copy_construct(
    char *DYND_UNUSED(dst_arrmeta), const char *DYND_UNUSED(src_arrmeta),
    memory_block_data *DYND_UNUSED(embedded_reference)) const
{
  stringstream ss;
  ss << "Cannot copy construct arrmeta for symbolic type " << type(this, true);
  throw runtime_error(ss.str());
}

size_t ndt::any_sym_type::arrmeta_copy_construct_onedim(
    char *DYND_UNUSED(dst_arrmeta), const char *DYND_UNUSED(src_arrmeta),
    memory_block_data *DYND_UNUSED(embedded_reference)) const
{
  stringstream ss;
  ss << "Cannot copy construct arrmeta for symbolic type " << type(this, true);
  throw runtime_error(ss.str());
}

void ndt::any_sym_type::arrmeta_reset_buffers(char *DYND_UNUSED(arrmeta)) const
{
}

void
ndt::any_sym_type::arrmeta_finalize_buffers(char *DYND_UNUSED(arrmeta)) const
{
}

void ndt::any_sym_type::arrmeta_destruct(char *DYND_UNUSED(arrmeta)) const {}

void ndt::any_sym_type::arrmeta_debug_print(
    const char *DYND_UNUSED(arrmeta), std::ostream &DYND_UNUSED(o),
    const std::string &DYND_UNUSED(indent)) const
{
  stringstream ss;
  ss << "Cannot have arrmeta for symbolic type " << type(this, true);
  throw runtime_error(ss.str());
}

void ndt::any_sym_type::data_destruct(const char *DYND_UNUSED(arrmeta),
                                      char *DYND_UNUSED(data)) const
{
  stringstream ss;
  ss << "Cannot have data for symbolic type " << type(this, true);
  throw runtime_error(ss.str());
}

void ndt::any_sym_type::data_destruct_strided(const char *DYND_UNUSED(arrmeta),
                                              char *DYND_UNUSED(data),
                                              intptr_t DYND_UNUSED(stride),
                                              size_t DYND_UNUSED(count)) const
{
  stringstream ss;
  ss << "Cannot have data for symbolic type " << type(this, true);
  throw runtime_error(ss.str());
}

bool
ndt::any_sym_type::match(const char *DYND_UNUSED(arrmeta),
                         const type &DYND_UNUSED(candidate_tp),
                         const char *DYND_UNUSED(candidate_arrmeta),
                         std::map<nd::string, type> &DYND_UNUSED(tp_vars)) const
{
  // "Any" matches against everything
  return true;
}

void ndt::any_sym_type::get_dynamic_type_properties(
    const std::pair<std::string, gfunc::callable> **out_properties,
    size_t *out_count) const
{
  *out_properties = NULL;
  *out_count = 0;
}

void ndt::any_sym_type::get_dynamic_array_properties(
    const std::pair<std::string, gfunc::callable> **out_properties,
    size_t *out_count) const
{
  *out_properties = NULL;
  *out_count = 0;
}

void ndt::any_sym_type::get_dynamic_array_functions(
    const std::pair<std::string, gfunc::callable> **out_functions,
    size_t *out_count) const
{
  *out_functions = NULL;
  *out_count = 0;
}
