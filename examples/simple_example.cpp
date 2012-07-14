//
// Copyright (C) 2011-12, Dynamic NDArray Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <iostream>

#include <dnd/ndarray.hpp>
#include <dnd/dtypes/convert_dtype.hpp>
#include <dnd/dtypes/fixedstring_dtype.hpp>
#include <dnd/dtypes/string_dtype.hpp>
#include <dnd/dtypes/byteswap_dtype.hpp>
#include <dnd/ndarray_arange.hpp>
#include <dnd/codegen/codegen_cache.hpp>
#include <dnd/codegen/unary_kernel_adapter_codegen.hpp>

using namespace std;
using namespace dnd;

typedef complex<double> A0;
typedef float R;

#define EXPECT_EQ(a, b) \
    cout << "first   : " << (a) << endl \
         << "second  : " << (b) << endl

template<class S, class T>
S double_value(T value) {
    return (S)(2 * value);
}

int main()
{
    try {
    codegen_cache cgcache;
    unary_specialization_kernel_instance op_int_float, op_float_float, op_float_double;
    cgcache.codegen_unary_function_adapter(make_dtype<int>(), make_dtype<float>(), cdecl_callconv,
                    (int (*)(float))&double_value<int, float>, op_int_float);
    cgcache.codegen_unary_function_adapter(make_dtype<float>(), make_dtype<float>(), cdecl_callconv,
                    (float (*)(float))&double_value<float, float>, op_float_float);
    cgcache.codegen_unary_function_adapter(make_dtype<float>(), make_dtype<double>(), cdecl_callconv,
                    (float (*)(double))&double_value<float, double>, op_float_double);

    int int_vals[3];
    float float_vals[3];
    double double_vals[3];

    float_vals[0] = 1.f;
    float_vals[1] = 2.5f;
    float_vals[2] = 3.25f;
    op_int_float.specializations[0]((char *)int_vals, sizeof(int), (char *)float_vals, sizeof(float), 3,
                    op_int_float.auxdata);
    EXPECT_EQ(2, int_vals[0]);
    EXPECT_EQ(5, int_vals[1]);
    EXPECT_EQ(6, int_vals[2]);

    op_float_float.specializations[0]((char *)float_vals, sizeof(float), (char *)float_vals, sizeof(float), 3,
                    op_float_float.auxdata);

    EXPECT_EQ(1.f, float_vals[0]);
    EXPECT_EQ(5.f, float_vals[1]);
    EXPECT_EQ(6.5f, float_vals[2]);

    double_vals[0] = -1.f;
    double_vals[1] = 2.5f;
    double_vals[2] = 3.25f;
    op_float_double.specializations[0]((char *)float_vals, sizeof(float), (char *)double_vals, sizeof(double), 3,
                    op_float_double.auxdata);


    } catch(std::exception& e) {
        cout << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
