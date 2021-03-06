//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#pragma once

#include <vector>
#include <string>

#include <dynd/array.hpp>
#include <dynd/string.hpp>
#include <dynd/types/base_dim_type.hpp>

namespace dynd {
namespace ndt {

  class typevar_dim_type : public base_dim_type {
    nd::string m_name;

  public:
    typevar_dim_type(const nd::string &name, const type &element_type);

    virtual ~typevar_dim_type() {}

    inline const nd::string &get_name() const { return m_name; }

    inline std::string get_name_str() const { return m_name.str(); }

    void print_data(std::ostream &o, const char *arrmeta,
                    const char *data) const;

    void print_type(std::ostream &o) const;

    intptr_t get_dim_size(const char *arrmeta, const char *data) const;

    void get_vars(std::unordered_set<std::string> &vars) const;

    bool is_lossless_assignment(const type &dst_tp, const type &src_tp) const;

    bool operator==(const base_type &rhs) const;

    type get_type_at_dimension(char **inout_arrmeta, intptr_t i,
                               intptr_t total_ndim = 0) const;

    void arrmeta_default_construct(char *arrmeta, bool blockref_alloc) const;
    void arrmeta_copy_construct(char *dst_arrmeta, const char *src_arrmeta,
                                memory_block_data *embedded_reference) const;
    size_t
    arrmeta_copy_construct_onedim(char *dst_arrmeta, const char *src_arrmeta,
                                  memory_block_data *embedded_reference) const;
    void arrmeta_destruct(char *arrmeta) const;

    bool match(const char *arrmeta, const type &candidate_tp,
               const char *candidate_arrmeta,
               std::map<nd::string, type> &tp_vars) const;

    void get_dynamic_type_properties(
        const std::pair<std::string, gfunc::callable> **out_properties,
        size_t *out_count) const;
  };

  /** Makes a typevar type with the specified name and element type */
  inline type make_typevar_dim(const nd::string &name, const type &element_type)
  {
    return type(new typevar_dim_type(name, element_type), false);
  }

  inline type make_typevar_dim(const nd::string &name, const type &element_tp,
                               intptr_t ndim)
  {
    type result = element_tp;
    for (intptr_t i = 0; i < ndim; ++i) {
      result = make_typevar_dim(name, result);
    }

    return result;
  }

} // namespace dynd::ndt
} // namespace dynd