//
// Copyright (C) 2011-13 Mark Wiebe, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <dynd/types/var_dim_type.hpp>
#include <dynd/types/strided_dim_type.hpp>
#include <dynd/types/fixed_dim_type.hpp>
#include <dynd/types/type_alignment.hpp>
#include <dynd/types/pointer_type.hpp>
#include <dynd/memblock/pod_memory_block.hpp>
#include <dynd/memblock/zeroinit_memory_block.hpp>
#include <dynd/memblock/objectarray_memory_block.hpp>
#include <dynd/shape_tools.hpp>
#include <dynd/exceptions.hpp>
#include <dynd/kernels/assignment_kernels.hpp>
#include <dynd/kernels/var_dim_assignment_kernels.hpp>
#include <dynd/gfunc/callable.hpp>
#include <dynd/gfunc/make_callable.hpp>

using namespace std;
using namespace dynd;

var_dim_type::var_dim_type(const ndt::type& element_dtype)
    : base_uniform_dim_type(var_dim_type_id, element_dtype, sizeof(var_dim_type_data),
                    sizeof(const char *), sizeof(var_dim_type_metadata),
                    type_flag_zeroinit|type_flag_blockref)
{
    // NOTE: The element dtype may have type_flag_destructor set. In this case,
    //       the var_dim dtype does NOT need to also set it, because the lifetime
    //       of the elements it allocates is owned by the objectarray_memory_block,
    //       not by the var_dim elements.

    // Copy ndobject properties and functions from the first non-array dimension
    get_scalar_properties_and_functions(m_array_properties, m_array_functions);
}

var_dim_type::~var_dim_type()
{
}

void var_dim_type::print_data(std::ostream& o, const char *metadata, const char *data) const
{
    const var_dim_type_metadata *md = reinterpret_cast<const var_dim_type_metadata *>(metadata);
    const var_dim_type_data *d = reinterpret_cast<const var_dim_type_data *>(data);
    const char *element_data = d->begin + md->offset;
    size_t stride = md->stride;
    metadata += sizeof(var_dim_type_metadata);
    o << "[";
    for (size_t i = 0, i_end = d->size; i != i_end; ++i, element_data += stride) {
        m_element_dtype.print_data(o, metadata, element_data);
        if (i != i_end - 1) {
            o << ", ";
        }
    }
    o << "]";
}

void var_dim_type::print_dtype(std::ostream& o) const
{
    o << "var_dim<" << m_element_dtype << ">";
}

bool var_dim_type::is_expression() const
{
    return m_element_dtype.is_expression();
}

bool var_dim_type::is_unique_data_owner(const char *metadata) const
{
    const var_dim_type_metadata *md = reinterpret_cast<const var_dim_type_metadata *>(metadata);
    if (md->blockref != NULL &&
            (md->blockref->m_use_count != 1 ||
             (md->blockref->m_type != pod_memory_block_type &&
              md->blockref->m_type != zeroinit_memory_block_type &&
              md->blockref->m_type != objectarray_memory_block_type))) {
        return false;
    }
    if (m_element_dtype.is_builtin()) {
        return true;
    } else {
        return m_element_dtype.extended()->is_unique_data_owner(metadata + sizeof(var_dim_type_metadata));
    }
}

void var_dim_type::transform_child_types(type_transform_fn_t transform_fn, void *extra,
                ndt::type& out_transformed_dtype, bool& out_was_transformed) const
{
    ndt::type tmp_dtype;
    bool was_transformed = false;
    transform_fn(m_element_dtype, extra, tmp_dtype, was_transformed);
    if (was_transformed) {
        out_transformed_dtype = ndt::type(new var_dim_type(tmp_dtype), false);
        out_was_transformed = true;
    } else {
        out_transformed_dtype = ndt::type(this, true);
    }
}

ndt::type var_dim_type::get_canonical_type() const
{
    return ndt::type(new var_dim_type(m_element_dtype.get_canonical_type()), false);
}

bool var_dim_type::is_strided() const
{
    return true;
}

void var_dim_type::process_strided(const char *metadata, const char *data,
                ndt::type& out_dt, const char *&out_origin,
                intptr_t& out_stride, intptr_t& out_dim_size) const
{
    const var_dim_type_metadata *md = reinterpret_cast<const var_dim_type_metadata *>(metadata);
    const var_dim_type_data *d = reinterpret_cast<const var_dim_type_data *>(data);
    out_dt = m_element_dtype;
    out_origin = d->begin;
    out_stride = md->stride;
    out_dim_size = d->size;
}

ndt::type var_dim_type::apply_linear_index(size_t nindices, const irange *indices,
                size_t current_i, const ndt::type& root_dt, bool leading_dimension) const
{
    if (nindices == 0) {
        return ndt::type(this, true);
    } else if (nindices == 1) {
        if (indices->step() == 0) {
            if (leading_dimension) {
                if (m_element_dtype.is_builtin()) {
                    return m_element_dtype;
                } else {
                    return m_element_dtype.apply_linear_index(0, NULL, current_i, root_dt, true);
                }
            } else {
                // TODO: This is incorrect, but is here as a stopgap to be replaced by a sliced<> dtype
                return ndt::make_pointer(m_element_dtype);
            }
        } else {
            if (leading_dimension) {
                // In leading dimensions, we convert var_dim to strided_dim
                return ndt::type(new strided_dim_type(m_element_dtype), false);
            } else {
                if (indices->is_nop()) {
                    // If the indexing operation does nothing, then leave things unchanged
                    return ndt::type(this, true);
                } else {
                    // TODO: sliced_var_dim_type
                    throw runtime_error("TODO: implement var_dim_type::apply_linear_index for general slices");
                }
            }
        }
    } else {
        if (indices->step() == 0) {
            if (leading_dimension) {
                return m_element_dtype.apply_linear_index(nindices-1, indices+1,
                                current_i+1, root_dt, true);
            } else {
                // TODO: This is incorrect, but is here as a stopgap to be replaced by a sliced<> dtype
                return ndt::make_pointer(m_element_dtype.apply_linear_index(nindices-1, indices+1,
                                current_i+1, root_dt, false));
            }
        } else {
            if (leading_dimension) {
                // In leading dimensions, we convert var_dim to strided_dim
                ndt::type edt = m_element_dtype.apply_linear_index(nindices-1, indices+1,
                                current_i+1, root_dt, false);
                return ndt::type(new strided_dim_type(edt), false);
            } else {
                if (indices->is_nop()) {
                    // If the indexing operation does nothing, then leave things unchanged
                    ndt::type edt = m_element_dtype.apply_linear_index(nindices-1, indices+1,
                                    current_i+1, root_dt, false);
                    return ndt::type(new var_dim_type(edt), false);
                } else {
                    // TODO: sliced_var_dim_type
                    throw runtime_error("TODO: implement var_dim_type::apply_linear_index for general slices");
                    //return ndt::type(new var_dim_type(m_element_dtype.apply_linear_index(nindices-1, indices+1, current_i+1, root_dt)), false);
                }
            }
        }
    }
}

intptr_t var_dim_type::apply_linear_index(size_t nindices, const irange *indices, const char *metadata,
                const ndt::type& result_dtype, char *out_metadata,
                memory_block_data *embedded_reference,
                size_t current_i, const ndt::type& root_dt,
                bool leading_dimension, char **inout_data,
                memory_block_data **inout_dataref) const
{
    if (nindices == 0) {
        // If there are no more indices, copy the metadata verbatim
        metadata_copy_construct(out_metadata, metadata, embedded_reference);
        return 0;
    } else {
        const var_dim_type_metadata *md = reinterpret_cast<const var_dim_type_metadata *>(metadata);
        if (leading_dimension) {
            const var_dim_type_data *d = reinterpret_cast<const var_dim_type_data *>(*inout_data);
            bool remove_dimension;
            intptr_t start_index, index_stride, dimension_size;
            apply_single_linear_index(*indices, d->size, current_i, &root_dt,
                            remove_dimension, start_index, index_stride, dimension_size);
            if (remove_dimension) {
                // First dereference to point at the actual element
                const var_dim_type_metadata *md = reinterpret_cast<const var_dim_type_metadata *>(metadata);
                const var_dim_type_data *d = reinterpret_cast<const var_dim_type_data *>(*inout_data);
                *inout_data = d->begin + md->offset + start_index * md->stride;
                if (*inout_dataref) {
                    memory_block_decref(*inout_dataref);
                }
                *inout_dataref = md->blockref ? md->blockref : embedded_reference;
                memory_block_incref(*inout_dataref);
                // Then apply a 0-sized index to the element type
                if (!m_element_dtype.is_builtin()) {
                    return m_element_dtype.extended()->apply_linear_index(
                                    nindices - 1, indices + 1,
                                    metadata + sizeof(var_dim_type_metadata),
                                    result_dtype, out_metadata, embedded_reference,
                                    current_i, root_dt,
                                    true, inout_data, inout_dataref);
                } else {
                    return 0;
                }
            } else {
                // We can dereference the pointer as we
                // index it and produce a strided array result
                strided_dim_type_metadata *out_md = reinterpret_cast<strided_dim_type_metadata *>(out_metadata);
                out_md->size = dimension_size;
                out_md->stride = md->stride * index_stride;
                *inout_data = d->begin + md->offset + md->stride * start_index;
                if (*inout_dataref) {
                    memory_block_decref(*inout_dataref);
                }
                *inout_dataref = md->blockref ? md->blockref : embedded_reference;
                memory_block_incref(*inout_dataref);
                if (nindices == 0) {
                    // Copy the rest of the metadata verbatim, because that part of
                    // the dtype didn't change
                    if (!m_element_dtype.is_builtin()) {
                        m_element_dtype.extended()->metadata_copy_construct(
                                        out_metadata + sizeof(strided_dim_type_metadata),
                                        metadata + sizeof(var_dim_type_metadata),
                                        embedded_reference);
                    }
                    return 0;
                } else {
                    if (m_element_dtype.is_builtin()) {
                        return 0;
                    } else {
                        const strided_dim_type *sad = static_cast<const strided_dim_type *>(result_dtype.extended());
                        return m_element_dtype.extended()->apply_linear_index(
                                        nindices - 1, indices + 1,
                                        metadata + sizeof(var_dim_type_metadata),
                                        sad->get_element_type(),
                                        out_metadata + sizeof(strided_dim_type_metadata), embedded_reference,
                                        current_i, root_dt,
                                        false, NULL, NULL);
                    }
                }
            }
        } else {
            if (indices->step() == 0) {
                // TODO: This is incorrect, but is here as a stopgap to be replaced by a sliced<> dtype
                pointer_type_metadata *out_md = reinterpret_cast<pointer_type_metadata *>(out_metadata);
                out_md->blockref = md->blockref ? md->blockref : embedded_reference;
                memory_block_incref(out_md->blockref);
                out_md->offset = indices->start() * md->stride;
                if (!m_element_dtype.is_builtin()) {
                    const pointer_type *result_edtype = static_cast<const pointer_type *>(result_dtype.extended());
                    out_md->offset += m_element_dtype.extended()->apply_linear_index(
                                    nindices - 1, indices + 1,
                                    metadata + sizeof(var_dim_type_metadata),
                                    result_edtype->get_target_dtype(), out_metadata + sizeof(pointer_type_metadata),
                                    embedded_reference, current_i + 1, root_dt,
                                    false, NULL, NULL);
                }
                return 0;
            } else if (indices->is_nop()) {
                // If the indexing operation does nothing, then leave things unchanged
                var_dim_type_metadata *out_md = reinterpret_cast<var_dim_type_metadata *>(out_metadata);
                out_md->blockref = md->blockref ? md->blockref : embedded_reference;
                memory_block_incref(out_md->blockref);
                out_md->stride = md->stride;
                out_md->offset = md->offset;
                if (!m_element_dtype.is_builtin()) {
                    const var_dim_type *vad = static_cast<const var_dim_type *>(result_dtype.extended());
                    out_md->offset += m_element_dtype.extended()->apply_linear_index(
                                    nindices - 1, indices + 1,
                                    metadata + sizeof(var_dim_type_metadata),
                                    vad->get_element_type(),
                                    out_metadata + sizeof(var_dim_type_metadata), embedded_reference,
                                    current_i, root_dt,
                                    false, NULL, NULL);
                }
                return 0;
            } else {
                // TODO: sliced_var_dim_type
                throw runtime_error("TODO: implement var_dim_type::apply_linear_index for general slices");
                //return ndt::type(this, true);
            }
        }
    }
}

ndt::type var_dim_type::at_single(intptr_t i0, const char **inout_metadata, const char **inout_data) const
{
    if (inout_metadata) {
        const var_dim_type_metadata *md = reinterpret_cast<const var_dim_type_metadata *>(*inout_metadata);
        // Modify the metadata
        *inout_metadata += sizeof(var_dim_type_metadata);
        // If requested, modify the data pointer
        if (inout_data) {
            const var_dim_type_data *d = reinterpret_cast<const var_dim_type_data *>(*inout_data);
            // Bounds-checking of the index
            i0 = apply_single_index(i0, d->size, NULL);
            *inout_data = d->begin + md->offset + i0 * md->stride;
        }
    }
    return m_element_dtype;
}

ndt::type var_dim_type::get_type_at_dimension(char **inout_metadata, size_t i, size_t total_ndim) const
{
    if (i == 0) {
        return ndt::type(this, true);
    } else {
        if (inout_metadata) {
            *inout_metadata += sizeof(var_dim_type_metadata);
        }
        return m_element_dtype.get_type_at_dimension(inout_metadata, i - 1, total_ndim + 1);
    }
}

intptr_t var_dim_type::get_dim_size(const char *DYND_UNUSED(metadata), const char *data) const
{
    if (data != NULL) {
        return reinterpret_cast<const var_dim_type_data *>(data)->size;
    } else {
        return -1;
    }
}

void var_dim_type::get_shape(size_t ndim, size_t i, intptr_t *out_shape, const char *metadata) const
{
    out_shape[i] = -1;

    // Process the later shape values
    if (i+1 < ndim) {
        if (!m_element_dtype.is_builtin()) {
            m_element_dtype.extended()->get_shape(ndim, i+1, out_shape,
                            metadata ? (metadata + sizeof(var_dim_type_metadata)) : NULL);
        } else {
            stringstream ss;
            ss << "requested too many dimensions from type " << ndt::type(this, true);
            throw runtime_error(ss.str());
        }
    }
}

void var_dim_type::get_strides(size_t i, intptr_t *out_strides, const char *metadata) const
{
    const var_dim_type_metadata *md = reinterpret_cast<const var_dim_type_metadata *>(metadata);

    out_strides[i] = md->stride;

    // Process the later shape values
    if (!m_element_dtype.is_builtin()) {
        m_element_dtype.extended()->get_strides(i+1, out_strides, metadata + sizeof(var_dim_type_metadata));
    }
}

axis_order_classification_t var_dim_type::classify_axis_order(const char *metadata) const
{
    // Treat the var_dim dtype as C-order
    if (m_element_dtype.get_undim() > 1) {
        axis_order_classification_t aoc = m_element_dtype.extended()->classify_axis_order(
                        metadata + sizeof(var_dim_type_metadata));
        return (aoc == axis_order_none || aoc == axis_order_c)
                        ? axis_order_c : axis_order_neither;
    } else {
        return axis_order_c;
    }
}

bool var_dim_type::is_lossless_assignment(const ndt::type& dst_dt, const ndt::type& src_dt) const
{
    if (dst_dt.extended() == this) {
        if (src_dt.extended() == this) {
            return true;
        } else if (src_dt.get_type_id() == var_dim_type_id) {
            return *dst_dt.extended() == *src_dt.extended();
        }
    }

    return false;
}

bool var_dim_type::operator==(const base_type& rhs) const
{
    if (this == &rhs) {
        return true;
    } else if (rhs.get_type_id() != var_dim_type_id) {
        return false;
    } else {
        const var_dim_type *dt = static_cast<const var_dim_type*>(&rhs);
        return m_element_dtype == dt->m_element_dtype;
    }
}

void var_dim_type::metadata_default_construct(char *metadata, size_t ndim, const intptr_t* shape) const
{
    size_t element_size = m_element_dtype.is_builtin() ? m_element_dtype.get_data_size()
                    : m_element_dtype.extended()->get_default_data_size(ndim-1, shape+1);

    var_dim_type_metadata *md = reinterpret_cast<var_dim_type_metadata *>(metadata);
    md->stride = element_size;
    md->offset = 0;
    // Allocate a memory block
    base_type::flags_type flags = m_element_dtype.get_flags();
    if (flags&type_flag_destructor) {
        md->blockref = make_objectarray_memory_block(m_element_dtype, metadata, element_size).release();
    } else if (flags&type_flag_zeroinit) {
        md->blockref = make_zeroinit_memory_block().release();
    } else {
        md->blockref = make_pod_memory_block().release();
    }
    if (!m_element_dtype.is_builtin()) {
        m_element_dtype.extended()->metadata_default_construct(
                        metadata + sizeof(var_dim_type_metadata), ndim ? (ndim-1) : 0, shape+1);
    }
}

void var_dim_type::metadata_copy_construct(char *dst_metadata, const char *src_metadata, memory_block_data *embedded_reference) const
{
    const var_dim_type_metadata *src_md = reinterpret_cast<const var_dim_type_metadata *>(src_metadata);
    var_dim_type_metadata *dst_md = reinterpret_cast<var_dim_type_metadata *>(dst_metadata);
    dst_md->stride = src_md->stride;
    dst_md->offset = src_md->offset;
    dst_md->blockref = src_md->blockref ? src_md->blockref : embedded_reference;
    if (dst_md->blockref) {
        memory_block_incref(dst_md->blockref);
    }
    if (!m_element_dtype.is_builtin()) {
        m_element_dtype.extended()->metadata_copy_construct(dst_metadata + sizeof(var_dim_type_metadata),
                        src_metadata + sizeof(var_dim_type_metadata), embedded_reference);
    }
}

size_t var_dim_type::metadata_copy_construct_onedim(char *dst_metadata, const char *src_metadata,
                memory_block_data *embedded_reference) const
{
    const var_dim_type_metadata *src_md = reinterpret_cast<const var_dim_type_metadata *>(src_metadata);
    var_dim_type_metadata *dst_md = reinterpret_cast<var_dim_type_metadata *>(dst_metadata);
    dst_md->stride = src_md->stride;
    dst_md->offset = src_md->offset;
    dst_md->blockref = src_md->blockref ? src_md->blockref : embedded_reference;
    if (dst_md->blockref) {
        memory_block_incref(dst_md->blockref);
    }
    return sizeof(var_dim_type_metadata);
}

void var_dim_type::metadata_reset_buffers(char *metadata) const
{
    const var_dim_type_metadata *md = reinterpret_cast<const var_dim_type_metadata *>(metadata);

    if (m_element_dtype.get_metadata_size() > 0) {
        m_element_dtype.extended()->metadata_reset_buffers(
                        metadata + sizeof(var_dim_type_metadata));
    }

    if (md->blockref != NULL) {
        uint32_t br_type = md->blockref->m_type;
        if (br_type == zeroinit_memory_block_type || br_type == pod_memory_block_type) {
            memory_block_pod_allocator_api *allocator =
                            get_memory_block_pod_allocator_api(md->blockref);
            allocator->reset(md->blockref);
            return;
        } else if (br_type == objectarray_memory_block_type) {
            memory_block_objectarray_allocator_api *allocator =
                            get_memory_block_objectarray_allocator_api(md->blockref);
            allocator->reset(md->blockref);
            return;
        }
    }

    stringstream ss;
    ss << "can only reset the buffers of a var_dim dtype ";
    ss << "if it was default-constructed. Its blockref is ";
    if (md->blockref == NULL) {
        ss << "NULL";
    } else {
        ss << "of the wrong type " << (memory_block_type_t)md->blockref->m_type;
    }
    throw runtime_error(ss.str());
}

void var_dim_type::metadata_finalize_buffers(char *metadata) const
{
    // Finalize any child metadata
    if (!m_element_dtype.is_builtin()) {
        m_element_dtype.extended()->metadata_finalize_buffers(metadata + sizeof(var_dim_type_metadata));
    }

    // Finalize the blockref buffer we own
    var_dim_type_metadata *md = reinterpret_cast<var_dim_type_metadata *>(metadata);
    if (md->blockref != NULL) {
        // Finalize the memory block
        if (m_element_dtype.get_flags()&type_flag_destructor) {
            memory_block_objectarray_allocator_api *allocator =
                            get_memory_block_objectarray_allocator_api(md->blockref);
            if (allocator != NULL) {
                allocator->finalize(md->blockref);
            }
        } else {
            memory_block_pod_allocator_api *allocator =
                            get_memory_block_pod_allocator_api(md->blockref);
            if (allocator != NULL) {
                allocator->finalize(md->blockref);
            }
        }
    }
}

void var_dim_type::metadata_destruct(char *metadata) const
{
    var_dim_type_metadata *md = reinterpret_cast<var_dim_type_metadata *>(metadata);
    if (md->blockref) {
        memory_block_decref(md->blockref);
    }
    if (!m_element_dtype.is_builtin()) {
        m_element_dtype.extended()->metadata_destruct(metadata + sizeof(var_dim_type_metadata));
    }
}

void var_dim_type::metadata_debug_print(const char *metadata, std::ostream& o, const std::string& indent) const
{
    const var_dim_type_metadata *md = reinterpret_cast<const var_dim_type_metadata *>(metadata);
    o << indent << "var_dim metadata\n";
    o << indent << " stride: " << md->stride << "\n";
    o << indent << " offset: " << md->offset << "\n";
    memory_block_debug_print(md->blockref, o, indent + " ");
    if (!m_element_dtype.is_builtin()) {
        m_element_dtype.extended()->metadata_debug_print(metadata + sizeof(var_dim_type_metadata), o, indent + "  ");
    }
}

size_t var_dim_type::get_iterdata_size(size_t DYND_UNUSED(ndim)) const
{
    throw runtime_error("TODO: implement var_dim_type::get_iterdata_size");
}

size_t var_dim_type::iterdata_construct(iterdata_common *DYND_UNUSED(iterdata), const char **DYND_UNUSED(inout_metadata), size_t DYND_UNUSED(ndim), const intptr_t* DYND_UNUSED(shape), ndt::type& DYND_UNUSED(out_uniform_dtype)) const
{
    throw runtime_error("TODO: implement var_dim_type::iterdata_construct");
}

size_t var_dim_type::iterdata_destruct(iterdata_common *DYND_UNUSED(iterdata), size_t DYND_UNUSED(ndim)) const
{
    throw runtime_error("TODO: implement var_dim_type::iterdata_destruct");
}

size_t var_dim_type::make_assignment_kernel(
                hierarchical_kernel *out, size_t offset_out,
                const ndt::type& dst_dt, const char *dst_metadata,
                const ndt::type& src_dt, const char *src_metadata,
                kernel_request_t kernreq, assign_error_mode errmode,
                const eval::eval_context *ectx) const
{
    if (this == dst_dt.extended()) {
        if (src_dt.get_undim() < dst_dt.get_undim()) {
            // If the src has fewer dimensions, broadcast it across this one
            return make_broadcast_to_var_dim_assignment_kernel(out, offset_out,
                            dst_dt, dst_metadata,
                            src_dt, src_metadata,
                            kernreq, errmode, ectx);
        } else if (src_dt.get_type_id() == var_dim_type_id) {
            // var_dim to var_dim
            return make_var_dim_assignment_kernel(out, offset_out,
                            dst_dt, dst_metadata,
                            src_dt, src_metadata,
                            kernreq, errmode, ectx);
        } else if (src_dt.get_type_id() == strided_dim_type_id ||
                        src_dt.get_type_id() == fixed_dim_type_id) {
            // strided_dim to var_dim
            return make_strided_to_var_dim_assignment_kernel(out, offset_out,
                            dst_dt, dst_metadata,
                            src_dt, src_metadata,
                            kernreq, errmode, ectx);
        } else if (!src_dt.is_builtin()) {
            // Give the src dtype a chance to make a kernel
            return src_dt.extended()->make_assignment_kernel(out, offset_out,
                            dst_dt, dst_metadata,
                            src_dt, src_metadata,
                            kernreq, errmode, ectx);
        } else {
            stringstream ss;
            ss << "Cannot assign from " << src_dt << " to " << dst_dt;
            throw runtime_error(ss.str());
        }
    } else if (dst_dt.get_undim() < src_dt.get_undim()) {
        throw broadcast_error(dst_dt, dst_metadata, src_dt, src_metadata);
    } else {
        if (dst_dt.get_type_id() == strided_dim_type_id ||
                        dst_dt.get_type_id() == fixed_dim_type_id) {
            // var_dim to strided_dim
            return make_var_to_strided_dim_assignment_kernel(out, offset_out,
                            dst_dt, dst_metadata,
                            src_dt, src_metadata,
                            kernreq, errmode, ectx);
        } else {
            stringstream ss;
            ss << "Cannot assign from " << src_dt << " to " << dst_dt;
            throw runtime_error(ss.str());
        }
    }
}

void var_dim_type::foreach_leading(char *data, const char *metadata, foreach_fn_t callback, void *callback_data) const
{
    const var_dim_type_metadata *md = reinterpret_cast<const var_dim_type_metadata *>(metadata);
    const char *child_metadata = metadata + sizeof(var_dim_type_metadata);
    const var_dim_type_data *d = reinterpret_cast<const var_dim_type_data *>(data);
    data = d->begin + md->offset;
    intptr_t stride = md->stride;
    for (intptr_t i = 0, i_end = d->size; i < i_end; ++i, data += stride) {
        callback(m_element_dtype, data, child_metadata, callback_data);
    }
}

static ndt::type get_element_type(const ndt::type& dt) {
    const strided_dim_type *d = static_cast<const strided_dim_type *>(dt.extended());
    return d->get_element_type();
}

static pair<string, gfunc::callable> var_dim_type_properties[] = {
    pair<string, gfunc::callable>("element_type", gfunc::make_callable(&get_element_type, "self"))
};

void var_dim_type::get_dynamic_dtype_properties(
                const std::pair<std::string, gfunc::callable> **out_properties,
                size_t *out_count) const
{
    *out_properties = var_dim_type_properties;
    *out_count = sizeof(var_dim_type_properties) / sizeof(var_dim_type_properties[0]);
}

void var_dim_type::get_dynamic_array_properties(const std::pair<std::string, gfunc::callable> **out_properties, size_t *out_count) const
{
    *out_properties = m_array_properties.empty() ? NULL : &m_array_properties[0];
    *out_count = (int)m_array_properties.size();
}

void var_dim_type::get_dynamic_array_functions(const std::pair<std::string, gfunc::callable> **out_functions, size_t *out_count) const
{
    *out_functions = m_array_functions.empty() ? NULL : &m_array_functions[0];
    *out_count = (int)m_array_functions.size();
}