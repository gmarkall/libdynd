//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <iostream>
#include <sstream>
#include <stdexcept>
#include "inc_gtest.hpp"

#include <dynd/types/fixed_string_kind_type.hpp>

using namespace std;
using namespace dynd;

TEST(FixedStringKindType, Construction)
{
  ndt::type tp = ndt::make_fixed_string_kind();
  EXPECT_EQ(fixed_string_type_id, tp.get_type_id());
  EXPECT_EQ(kind_kind, tp.get_kind());
  EXPECT_EQ(0u, tp.get_data_alignment());
  EXPECT_EQ(0u, tp.get_data_size());
  EXPECT_FALSE(tp.is_expression());
  EXPECT_TRUE(tp.is_scalar());
  EXPECT_TRUE(tp.is_symbolic());
  // Roundtripping through a string
  EXPECT_EQ(tp, ndt::type(tp.str()));
}