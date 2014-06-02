//
// Copyright (C) 2011-14 Mark Wiebe, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <iostream>
#include <sstream>
#include <stdexcept>
#include "inc_gtest.hpp"
#include "dynd_assertions.hpp"

#include <dynd/array.hpp>
#include <dynd/view.hpp>
#include <dynd/types/option_type.hpp>
#include <dynd/types/string_type.hpp>
#include <dynd/json_parser.hpp>

using namespace std;
using namespace dynd;

TEST(OptionType, Create) {
    ndt::type d;

    d = ndt::make_option<int16_t>();
    EXPECT_EQ(option_type_id, d.get_type_id());
    EXPECT_EQ(option_kind, d.get_kind());
    EXPECT_EQ(2u, d.get_data_alignment());
    EXPECT_EQ(2u, d.get_data_size());
    EXPECT_EQ(ndt::make_type<int16_t>(),
              d.tcast<option_type>()->get_value_type());
    EXPECT_FALSE(d.is_expression());
    // Roundtripping through a string
    EXPECT_EQ(d, ndt::type(d.str()));
    EXPECT_EQ("?int16", d.str());
    EXPECT_EQ(d, ndt::type("?int16"));
    EXPECT_EQ(d, ndt::type("option[int16]"));

    d = ndt::make_option(ndt::make_string());
    EXPECT_EQ(option_type_id, d.get_type_id());
    EXPECT_EQ(option_kind, d.get_kind());
    EXPECT_EQ(ndt::make_string().get_data_alignment(), d.get_data_alignment());
    EXPECT_EQ(ndt::make_string().get_data_size(), d.get_data_size());
    EXPECT_EQ(ndt::make_string(), d.tcast<option_type>()->get_value_type());
    EXPECT_FALSE(d.is_expression());
    // Roundtripping through a string
    EXPECT_EQ(d, ndt::type(d.str()));
    EXPECT_EQ("?string", d.str());

    // No option of option allowed
    EXPECT_THROW(ndt::make_option(ndt::make_option(ndt::make_type<int>())), type_error);
    EXPECT_THROW(ndt::type("option[option[bool]]"), type_error);
}

TEST(OptionType, OptionIntAssign) {
    nd::array a, b;

    // Assignment from option[S] to option[T]
    a = parse_json("2 * ?int8", "[-10, null]");
    b = nd::empty("2 * ?int16");
    b.vals() = a;
    EXPECT_JSON_EQ_ARR("[-10, -32768]", nd::view(b, "2 * int16"));
    b.val_assign(a, assign_error_none);
    EXPECT_JSON_EQ_ARR("[-10, -32768]", nd::view(b, "2 * int16"));

    // Assignment from option[T] to T without any NAs
    a = parse_json("3 * ?int32", "[1, 2, 3]");
    b = nd::empty("3 * int32");
    b.vals() = a;
    EXPECT_ARR_EQ(nd::view(a, "3 * int32"), b);

    // Assignment from T to option[T]
    a = parse_json("3 * int32", "[1, 3, 5]");
    b = nd::empty("3 * ?int32");
    b.vals() = a;
    EXPECT_ARR_EQ(a, nd::view(b, "3 * int32"));
}