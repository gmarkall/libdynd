//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <dynd/kernels/base_kernel.hpp>
#include <dynd/kernels/date_adapter_kernels.hpp>
#include <dynd/parser_util.hpp>
#include <dynd/types/date_parser.hpp>
#include <dynd/types/datetime_type.hpp>
#include <dynd/types/arrfunc_type.hpp>
#include <dynd/func/arrfunc.hpp>
#include <dynd/string.hpp>

using namespace std;
using namespace dynd;

/**
 * Matches netcdf date metadata like "days since 2001-1-1".
 */
static bool parse_days_since(const char *begin, const char *end,
                             int32_t &out_epoch_date)
{
  if (!parse::parse_token(begin, end, "days")) {
    return false;
  }
  if (!parse::skip_required_whitespace(begin, end)) {
    return false;
  }
  // The tokens supported by netcdf, as from the udunits libarary
  if (!parse::parse_token(begin, end, "since") &&
      !parse::parse_token(begin, end, "after") &&
      !parse::parse_token(begin, end, "from") &&
      !parse::parse_token(begin, end, "ref") &&
      !parse::parse_token(begin, end, '@')) {
    return false;
  }
  if (!parse::skip_required_whitespace(begin, end)) {
    return false;
  }
  date_ymd epoch;
  if (!parse::parse_date(begin, end, epoch, date_parse_no_ambig, 0)) {
    int year;
    if (parse::parse_4digit_int_no_ws(begin, end, year)) {
      epoch.year = static_cast<int16_t>(year);
      epoch.month = 1;
      epoch.day = 1;
    } else {
      return false;
    }
  }
  parse::skip_whitespace(begin, end);
  out_epoch_date = epoch.to_days();
  return begin == end;
}

namespace {
template <class Tsrc, class Tdst>
struct int_offset_ck
    : nd::base_kernel<int_offset_ck<Tsrc, Tdst>, kernel_request_host, 1> {
  Tdst m_offset;

  void single(char *dst, char *const *src)
  {
    Tsrc value = *reinterpret_cast<Tsrc *>(src[0]);
    *reinterpret_cast<Tdst *>(dst) = value != std::numeric_limits<Tsrc>::min()
                                         ? static_cast<Tdst>(value) + m_offset
                                         : std::numeric_limits<Tdst>::min();
  }
};

template <class Tsrc, class Tdst>
static intptr_t instantiate_int_offset_arrfunc(
    char *static_data, size_t DYND_UNUSED(data_size), char *DYND_UNUSED(data),
    void *ckb, intptr_t ckb_offset, const ndt::type &DYND_UNUSED(dst_tp),
    const char *DYND_UNUSED(dst_arrmeta), intptr_t DYND_UNUSED(nsrc),
    const ndt::type *DYND_UNUSED(src_tp),
    const char *const *DYND_UNUSED(src_arrmeta), kernel_request_t kernreq,
    const eval::eval_context *DYND_UNUSED(ectx),
    const nd::array &DYND_UNUSED(kwds),
    const std::map<nd::string, ndt::type> &DYND_UNUSED(tp_vars))
{
  typedef int_offset_ck<Tsrc, Tdst> self_type;
  self_type *self = self_type::make(ckb, kernreq, ckb_offset);
  self->m_offset = *reinterpret_cast<Tdst *>(static_data);
  return ckb_offset;
}

template <class Tsrc, class Tdst>
nd::arrfunc make_int_offset_arrfunc(Tdst offset, const ndt::type &func_proto)
{
  return nd::arrfunc(func_proto, offset, 0, NULL, NULL,
                     &instantiate_int_offset_arrfunc<Tsrc, Tdst>);
}
} // anonymous namespace

bool dynd::make_date_adapter_arrfunc(const ndt::type &operand_tp,
                                     const nd::string &op,
                                     nd::arrfunc &out_forward,
                                     nd::arrfunc &out_reverse)
{
  int32_t epoch_date;
  if (parse_days_since(op.begin(), op.end(), epoch_date)) {
    switch (operand_tp.get_type_id()) {
    case int32_type_id:
      out_forward = make_int_offset_arrfunc<int32_t, int32_t>(
          epoch_date,
          ndt::make_arrfunc(ndt::make_tuple(ndt::make_type<int32_t>()),
                            ndt::make_date()));
      out_reverse = make_int_offset_arrfunc<int32_t, int32_t>(
          -epoch_date, ndt::make_arrfunc(ndt::make_tuple(ndt::make_date()),
                                         ndt::make_type<int32_t>()));
      return true;
    case int64_type_id:
      out_forward = make_int_offset_arrfunc<int64_t, int32_t>(
          epoch_date,
          ndt::make_arrfunc(ndt::make_tuple(ndt::make_type<int64_t>()),
                            ndt::make_date()));
      out_reverse = make_int_offset_arrfunc<int32_t, int64_t>(
          -epoch_date, ndt::make_arrfunc(ndt::make_tuple(ndt::make_date()),
                                         ndt::make_type<int64_t>()));
      return true;
    default:
      return false;
    }
  } else {
    return false;
  }
}
