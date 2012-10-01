//
// Copyright (C) 2011-12, Dynamic NDArray Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <dnd/dtypes/ndarray_dtype.hpp>
#include <dnd/raw_iteration.hpp>

using namespace dnd;

ndarray_dtype::ndarray_dtype(intptr_t element_size, int ndim,
                                    intptr_t *shape, intptr_t *strides,
                                    const dtype& element_dtype)
    : extended_dtype(), m_element_size(element_size), m_element_dtype(element_dtype),
      m_shape_and_strides(ndim), m_ndim(ndim)
{
    memcpy(m_shape_and_strides.get(0), shape, m_ndim * sizeof(intptr_t));
    memcpy(m_shape_and_strides.get(1), strides, m_ndim * sizeof(intptr_t));
}

intptr_t product(int n, intptr_t *values)
{
    intptr_t result = 1;
    for (int i = 0; i < n; ++i) {
        result *= values[i];
    }

    return result;
}

ndarray_dtype::ndarray_dtype(int ndim, intptr_t *shape, const dtype& element_dtype)
    : extended_dtype(), m_element_size(element_dtype.element_size() * product(ndim, shape)),
      m_element_dtype(element_dtype), m_shape_and_strides(ndim), m_ndim(ndim)
{
    intptr_t *this_shape = m_shape_and_strides.get(0), *this_strides = m_shape_and_strides.get(1);

    memcpy(this_shape, shape, m_ndim * sizeof(intptr_t));
    intptr_t stride_value = element_dtype.element_size();
    for (int idim = ndim-1; idim >= 0; --idim) {
        this_strides[idim] = stride_value;
        stride_value *= this_shape[idim];
    }
}

static void nested_array_print(std::ostream& o, const dtype& d, const char *data, int ndim, const intptr_t *shape, const intptr_t *strides)
{
    o << "[";
    if (ndim == 1) {
        d.print_element(o, data);
        for (intptr_t i = 1; i < shape[0]; ++i) {
            data += strides[0];
            o << ", ";
            d.print_element(o, data);
        }
    } else {
        intptr_t size = *shape;
        intptr_t stride = *strides;
        for (intptr_t k = 0; k < size; ++k) {
            nested_array_print(o, d, data, ndim - 1, shape + 1, strides + 1);
            if (k + 1 != size) {
                o << ", ";
            }
            data += stride;
        }
    }
    o << "]";
}

void ndarray_dtype::print_element(std::ostream& o, const char * data) const
{
    intptr_t size = *get_shape();
    for (int i = 0; i < size; ++i) {
        nested_array_print(o, m_element_dtype, data, m_ndim, get_shape(), get_strides());
        if (i != size-1) {
            o << ", ";
            data += size;
        }
    }
}

void ndarray_dtype::print_dtype(std::ostream& o) const
{
    o << "array<" << m_element_dtype << ", (";
    const intptr_t *shape = get_shape();
    for (int i = 0; i < m_ndim; ++i) {
        o << shape[i];
        if (i != m_ndim - 1) {
            o << ",";
        }
    }
    o << ")>";
}


bool ndarray_dtype::is_lossless_assignment(const dtype& dst_dt, const dtype& src_dt) const
{
    if (dst_dt.extended() == this) {
        if (src_dt.extended() == this) {
            // Casting from identical types
            return true;
        } else if (src_dt.type_id() == ndarray_type_id) {
            // Casting array to array, check that it can broadcast, and that the
            // element dtype can cast losslessly
            const ndarray_dtype *src_adt = static_cast<const ndarray_dtype *>(src_dt.extended());
            return shape_can_broadcast(m_ndim, get_shape(), src_adt->m_ndim, src_adt->get_shape()) &&
                ::is_lossless_assignment(m_element_dtype, src_adt->m_element_dtype);
        } else {
            // If the src element can losslessly cast to the element, then
            // can broadcast it to everywhere.
            return ::is_lossless_assignment(m_element_dtype, src_dt);
        }

    } else {
        return false;
    }
}

bool ndarray_dtype::operator==(const extended_dtype& rhs) const
{
    if (this == &rhs) {
        return true;
    } else if (rhs.type_id() != ndarray_type_id) {
        return false;
    } else {
        const ndarray_dtype *adt = static_cast<const ndarray_dtype*>(&rhs);

        if (m_ndim != adt->m_ndim || m_element_dtype != adt->m_element_dtype)
            return false;

        const intptr_t *this_shape = get_shape(), *this_strides = get_strides();
        const intptr_t *adt_shape = adt->get_shape(), *adt_strides = get_strides();
        for (int i = 0; i < m_ndim; ++i) {
            if (this_shape[i] != adt_shape[i] || this_strides[i] != adt_strides[i]) {
                return false;
            }
        }

        return true;
    }
}
