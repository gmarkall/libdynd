//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <cmath>

#include "inc_gtest.hpp"

#include <dynd/array.hpp>
#include <dynd/types/bytes_type.hpp>
#include <dynd/types/string_type.hpp>

using namespace std;
using namespace dynd;

static void write_string_file(const char *fn,
    const char *data, intptr_t size)
{
    ofstream fout(fn, ios::binary);
    fout.write(data, size);
}

TEST(ArrayMemMap, SimpleString) {
    // Create a file with a simple string
    const char *str = "This is a test of a string.";
    write_string_file("test.txt", str, strlen(str));
    // Open the whole file as a memory map
    nd::array a = nd::memmap("test.txt");
    // Should have type 'bytes'
    EXPECT_EQ(ndt::make_bytes(1), a.get_type());
    // If we view it as a string, should still point to the same data
    nd::array b = a.view_scalars(ndt::make_string());
    EXPECT_EQ(ndt::make_string(), b.get_type());
    EXPECT_EQ(string(str), b.as<string>());

    // Remap a subset of the file
    a = nd::array();
    b = nd::array();
    a = nd::memmap("test.txt", 5, 7);
    EXPECT_EQ(ndt::make_bytes(1), a.get_type());
    EXPECT_EQ("is", a.view_scalars(ndt::make_string()).as<string>());

    // Remap the file using a negative index
    a = nd::array();
    a = nd::memmap("test.txt", -7);
    EXPECT_EQ(ndt::make_bytes(1), a.get_type());
    EXPECT_EQ("string.", a.view_scalars(ndt::make_string()).as<string>());

#ifdef WIN32
    _unlink("test.txt");
#else
    unlink("test.txt");
#endif
}
