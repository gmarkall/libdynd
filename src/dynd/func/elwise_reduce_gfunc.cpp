//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#if 0 // TODO reenable

#include <algorithm>

#include <dynd/nodes/elwise_reduce_kernel_node.hpp>
#include <dynd/gfunc/elwise_reduce_gfunc.hpp>

using namespace std;
using namespace dynd;

const dynd::gfunc::elwise_reduce_kernel *
dynd::gfunc::elwise_reduce::find_matching_kernel(const std::vector<ndt::type>& paramtypes) const
{
    for(size_t i = 0, i_end = m_kernels.size(); i != i_end; ++i) {
        const std::vector<ndt::type>& kparamtypes = m_kernels[i].m_paramtypes;
        if (kparamtypes == paramtypes) {
            return &m_kernels[i];
        }
    }

    return NULL;
}

void dynd::gfunc::elwise_reduce::add_kernel(elwise_reduce_kernel& ergk)
{
    const elwise_reduce_kernel *check = find_matching_kernel(ergk.m_paramtypes);
    if (check == NULL) {
        m_kernels.push_back(elwise_reduce_kernel());
        m_kernels.back().swap(ergk);
    } else {
        stringstream ss;
        ss << "Cannot add kernel to gfunc " << m_name << " because a kernel with the same arguments, (";
        for (size_t j = 0, j_end = ergk.m_paramtypes.size(); j != j_end; ++j) {
            ss << ergk.m_paramtypes[j];
            if (j != j_end - 1) {
                ss << ", ";
            }
        }
        ss << "), already exists in the gfunc";
        throw runtime_error(ss.str());
    }
}

void dynd::gfunc::elwise_reduce::debug_print(std::ostream& o, const std::string& indent) const
{
    o << indent << "------ elwise_reduce_gfunc\n";
    o << indent << "name: " << m_name << "\n";
    o << indent << "kernel count: " << m_kernels.size() << "\n";
    for (deque<elwise_reduce_kernel>::size_type i = 0; i < m_kernels.size(); ++i) {
        const elwise_reduce_kernel &k = m_kernels[i];
        o << indent << "kernel " << i << "\n";
        o << indent << " signature: " << k.m_returntype << " (";
        for (size_t j = 0, j_end = k.m_paramtypes.size(); j != j_end; ++j) {
            o << k.m_paramtypes[j];
            if (j != j_end - 1) {
                o << ", ";
            }
        }
        o << ")\n";
        o << indent << " associative: " << (k.m_associative ? "true" : "false") << "\n";
        o << indent << " commutative: " << (k.m_commutative ? "true" : "false") << "\n";
        if (k.m_left_associative_reduction_kernel.kernel != NULL) {
            o << indent << " left associative kernel aux data: ";
            o << (const void *)(const dynd::AuxDataBase *)k.m_left_associative_reduction_kernel.auxdata << "\n";
        }
        if (k.m_right_associative_reduction_kernel.kernel != NULL) {
            o << indent << " right associative kernel aux data: ";
            o << (const void *)(const dynd::AuxDataBase *)k.m_right_associative_reduction_kernel.auxdata << "\n";
        }
        if (!k.m_identity.empty()) {
            o << indent << " reduction identity:\n";
            k.m_identity.debug_print(o, indent + "  ");
        } else {
            o << indent << " reduction identity: NULL\n";
        }
    }
    o << indent << "------" << endl;
}

#endif // TODO reenable
