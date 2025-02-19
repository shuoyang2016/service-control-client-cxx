/* Copyright 2016 Google Inc. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "gtest/gtest.h"
#include "src/money_utils.h"
#include "utils/status_test_util.h"

using ::google::type::Money;
using ::google::protobuf::util::Status;
using ::google::protobuf::util::StatusCode;

namespace google {
namespace service_control_client {

class MoneyUtilsTest : public ::testing::Test {};

TEST_F(MoneyUtilsTest, ValidateSuccess) {
  Money money;
  money.set_currency_code("USD");
  money.set_units(1);
  money.set_nanos(0);
  EXPECT_OK(ValidateMoney(money));
  money.set_units(-1);
  EXPECT_OK(ValidateMoney(money));
  money.set_units(0);
  EXPECT_OK(ValidateMoney(money));
  money.set_units(INT64_MAX);
  EXPECT_OK(ValidateMoney(money));
  money.set_units(INT64_MIN);
  EXPECT_OK(ValidateMoney(money));

  money.set_units(0);
  money.set_nanos(999999999);
  EXPECT_OK(ValidateMoney(money));
  money.set_units(0);
  money.set_nanos(-999999999);
  EXPECT_OK(ValidateMoney(money));
  money.set_units(INT64_MAX);
  money.set_nanos(999999999);
  EXPECT_OK(ValidateMoney(money));
  money.set_units(INT64_MIN);
  money.set_nanos(-999999999);
  EXPECT_OK(ValidateMoney(money));
}

TEST_F(MoneyUtilsTest, ValidateCurrencyCodeError) {
  Money money;
  money.set_currency_code("");
  EXPECT_ERROR_CODE(StatusCode::kInvalidArgument, ValidateMoney(money));
  money.set_currency_code("US");
  EXPECT_ERROR_CODE(StatusCode::kInvalidArgument, ValidateMoney(money));
  money.set_currency_code("USD1");
  EXPECT_ERROR_CODE(StatusCode::kInvalidArgument, ValidateMoney(money));
}

TEST_F(MoneyUtilsTest, ValidateSignInconsistency) {
  Money money;
  money.set_currency_code("USD");
  money.set_units(1);
  money.set_nanos(-1);
  EXPECT_ERROR_CODE(StatusCode::kInvalidArgument, ValidateMoney(money));
  money.set_units(-1);
  money.set_nanos(1);
  EXPECT_ERROR_CODE(StatusCode::kInvalidArgument, ValidateMoney(money));
}

TEST_F(MoneyUtilsTest, ValidateOutOfBound) {
  Money money;
  money.set_currency_code("USD");
  money.set_units(1);
  money.set_nanos(1000000000);
  EXPECT_ERROR_CODE(StatusCode::kInvalidArgument, ValidateMoney(money));
  money.set_nanos(INT32_MAX);
  EXPECT_ERROR_CODE(StatusCode::kInvalidArgument, ValidateMoney(money));

  money.set_units(-1);
  money.set_nanos(-1000000000);
  EXPECT_ERROR_CODE(StatusCode::kInvalidArgument, ValidateMoney(money));
  money.set_nanos(INT32_MIN);
  EXPECT_ERROR_CODE(StatusCode::kInvalidArgument, ValidateMoney(money));
}

TEST_F(MoneyUtilsTest, GetAmountSign) {
  Money money;
  money.set_currency_code("USD");

  money.set_units(2);
  money.set_nanos(0);
  EXPECT_EQ(1, GetAmountSign(money));
  money.set_units(2);
  money.set_nanos(2);
  EXPECT_EQ(1, GetAmountSign(money));
  money.set_units(0);
  money.set_nanos(2);
  money.set_units(0);
  money.set_nanos(2);

  money.set_units(0);
  money.set_nanos(0);
  EXPECT_EQ(0, GetAmountSign(money));

  money.set_units(-2);
  money.set_nanos(0);
  EXPECT_EQ(-1, GetAmountSign(money));
  money.set_units(-2);
  money.set_nanos(-2);
  EXPECT_EQ(-1, GetAmountSign(money));
  money.set_units(0);
  money.set_nanos(-2);
  EXPECT_EQ(-1, GetAmountSign(money));
}

TEST_F(MoneyUtilsTest, TryAddMoneySuccess) {
  Money a;
  Money b;
  Money sum;

  a.set_currency_code("CAD");
  b.set_currency_code("CAD");

  a.set_units(2);
  a.set_nanos(3);
  b.set_units(5);
  b.set_nanos(6);
  EXPECT_OK(TryAddMoney(a, b, &sum));
  EXPECT_EQ(7, sum.units());
  EXPECT_EQ(9, sum.nanos());

  a.set_units(-2);
  a.set_nanos(-3);
  b.set_units(-5);
  b.set_nanos(-6);
  EXPECT_OK(TryAddMoney(a, b, &sum));
  EXPECT_EQ(-7, sum.units());
  EXPECT_EQ(-9, sum.nanos());

  a.set_units(-2);
  a.set_nanos(-3);
  b.set_units(5);
  b.set_nanos(7);
  EXPECT_OK(TryAddMoney(a, b, &sum));
  EXPECT_EQ(3, sum.units());
  EXPECT_EQ(4, sum.nanos());

  a.set_units(-2);
  a.set_nanos(-7);
  b.set_units(5);
  b.set_nanos(3);
  EXPECT_OK(TryAddMoney(a, b, &sum));
  EXPECT_EQ(2, sum.units());
  EXPECT_EQ(999999996, sum.nanos());

  a.set_units(2);
  a.set_nanos(7);
  b.set_units(-5);
  b.set_nanos(-3);
  EXPECT_OK(TryAddMoney(a, b, &sum));
  EXPECT_EQ(-2, sum.units());
  EXPECT_EQ(-999999996, sum.nanos());

  a.set_units(2);
  a.set_nanos(300000000);
  b.set_units(5);
  b.set_nanos(699999999);
  EXPECT_OK(TryAddMoney(a, b, &sum));
  EXPECT_EQ(7, sum.units());
  EXPECT_EQ(999999999, sum.nanos());

  a.set_units(2);
  a.set_nanos(300000000);
  b.set_units(5);
  b.set_nanos(700000000);
  EXPECT_OK(TryAddMoney(a, b, &sum));
  EXPECT_EQ(8, sum.units());
  EXPECT_EQ(0, sum.nanos());

  a.set_units(2);
  a.set_nanos(400000000);
  b.set_units(5);
  b.set_nanos(700000000);
  EXPECT_OK(TryAddMoney(a, b, &sum));
  EXPECT_EQ(8, sum.units());
  EXPECT_EQ(100000000, sum.nanos());

  a.set_units(-2);
  a.set_nanos(-300000000);
  b.set_units(-5);
  b.set_nanos(-699999999);
  EXPECT_OK(TryAddMoney(a, b, &sum));
  EXPECT_EQ(-7, sum.units());
  EXPECT_EQ(-999999999, sum.nanos());

  a.set_units(-2);
  a.set_nanos(-300000000);
  b.set_units(-5);
  b.set_nanos(-700000000);
  EXPECT_OK(TryAddMoney(a, b, &sum));
  EXPECT_EQ(-8, sum.units());
  EXPECT_EQ(0, sum.nanos());

  a.set_units(-2);
  a.set_nanos(-400000000);
  b.set_units(-5);
  b.set_nanos(-700000000);
  EXPECT_OK(TryAddMoney(a, b, &sum));
  EXPECT_EQ(-8, sum.units());
  EXPECT_EQ(-100000000, sum.nanos());
}

TEST_F(MoneyUtilsTest, TryAddMoneySpecialValues) {
  Money a;
  Money b;
  Money sum;

  a.set_currency_code("USD");
  b.set_currency_code("USD");

  a.set_units(0);
  a.set_nanos(0);
  b.set_units(0);
  b.set_nanos(0);
  EXPECT_OK(TryAddMoney(a, b, &sum));
  EXPECT_EQ(0, sum.units());
  EXPECT_EQ(0, sum.nanos());

  a.set_units(INT64_MAX);
  a.set_nanos(999999999);
  b.set_units(0);
  b.set_nanos(0);
  EXPECT_OK(TryAddMoney(a, b, &sum));
  EXPECT_EQ(INT64_MAX, sum.units());
  EXPECT_EQ(999999999, sum.nanos());

  a.set_units(INT64_MIN);
  a.set_nanos(-999999999);
  b.set_units(0);
  b.set_nanos(0);
  EXPECT_OK(TryAddMoney(a, b, &sum));
  EXPECT_EQ(INT64_MIN, sum.units());
  EXPECT_EQ(-999999999, sum.nanos());

  a.set_units(INT64_MIN);
  a.set_nanos(-999999999);
  b.set_units(INT64_MAX);
  b.set_nanos(999999999);
  EXPECT_OK(TryAddMoney(a, b, &sum));
  EXPECT_EQ(-1, sum.units());
  EXPECT_EQ(0, sum.nanos());
}

TEST_F(MoneyUtilsTest, TryAddMoneyDifferentCurrencyCode) {
  Money a;
  Money b;
  Money sum;
  a.set_currency_code("USD");
  a.set_units(1);
  a.set_nanos(0);
  b.set_currency_code("CAD");
  a.set_units(2);
  a.set_nanos(0);
  sum.set_currency_code("USD");
  sum.set_units(1);
  sum.set_nanos(0);
  EXPECT_ERROR_CODE(StatusCode::kInvalidArgument, TryAddMoney(a, b, &sum));
  EXPECT_EQ("", sum.currency_code());
  EXPECT_EQ(0, sum.units());
  EXPECT_EQ(0, sum.nanos());
}

TEST_F(MoneyUtilsTest, TryAddMoneyOverflow) {
  Money a;
  Money b;
  Money sum;
  a.set_currency_code("USD");
  b.set_currency_code("USD");

  a.set_units(INT64_MAX);
  a.set_nanos(999999999);
  b.set_units(0);
  b.set_nanos(1);
  EXPECT_ERROR_CODE(StatusCode::kOutOfRange, TryAddMoney(a, b, &sum));
  EXPECT_EQ(INT64_MAX, sum.units());
  EXPECT_EQ(999999999, sum.nanos());

  a.set_units(INT64_MAX);
  a.set_nanos(999999999);
  b.set_units(1);
  b.set_nanos(0);
  EXPECT_ERROR_CODE(StatusCode::kOutOfRange, TryAddMoney(a, b, &sum));
  EXPECT_EQ(INT64_MAX, sum.units());
  EXPECT_EQ(999999999, sum.nanos());

  a.set_units(INT64_MAX);
  a.set_nanos(999999999);
  b.set_units(INT64_MAX);
  b.set_nanos(999999999);
  EXPECT_ERROR_CODE(StatusCode::kOutOfRange, TryAddMoney(a, b, &sum));
  EXPECT_EQ(INT64_MAX, sum.units());
  EXPECT_EQ(999999999, sum.nanos());

  a.set_units(INT64_MAX - 100000000000);
  a.set_nanos(0);
  b.set_units(200000000000);
  b.set_nanos(0);
  EXPECT_ERROR_CODE(StatusCode::kOutOfRange, TryAddMoney(a, b, &sum));
  EXPECT_EQ(INT64_MAX, sum.units());
  EXPECT_EQ(999999999, sum.nanos());

  a.set_units(INT64_MIN);
  a.set_nanos(-999999999);
  b.set_units(0);
  b.set_nanos(-1);
  EXPECT_ERROR_CODE(StatusCode::kOutOfRange, TryAddMoney(a, b, &sum));
  EXPECT_EQ(INT64_MIN, sum.units());
  EXPECT_EQ(-999999999, sum.nanos());

  a.set_units(INT64_MIN);
  a.set_nanos(-999999999);
  b.set_units(-1);
  b.set_nanos(-0);
  EXPECT_ERROR_CODE(StatusCode::kOutOfRange, TryAddMoney(a, b, &sum));
  EXPECT_EQ(INT64_MIN, sum.units());
  EXPECT_EQ(-999999999, sum.nanos());

  a.set_units(INT64_MIN);
  a.set_nanos(-999999999);
  b.set_units(INT64_MIN);
  b.set_nanos(-999999999);
  EXPECT_ERROR_CODE(StatusCode::kOutOfRange, TryAddMoney(a, b, &sum));
  EXPECT_EQ(INT64_MIN, sum.units());
  EXPECT_EQ(-999999999, sum.nanos());

  a.set_units(INT64_MIN + 30000000000000);
  a.set_nanos(0);
  b.set_units(-50000000000000);
  b.set_nanos(0);
  EXPECT_ERROR_CODE(StatusCode::kOutOfRange, TryAddMoney(a, b, &sum));
  EXPECT_EQ(INT64_MIN, sum.units());
  EXPECT_EQ(-999999999, sum.nanos());
}

TEST_F(MoneyUtilsTest, SaturatedAddMoneySuccess) {
  Money a;
  Money b;
  Money sum;

  a.set_currency_code("CAD");
  b.set_currency_code("CAD");

  a.set_units(2);
  a.set_nanos(3);
  b.set_units(5);
  b.set_nanos(6);
  sum = SaturatedAddMoney(a, b);
  EXPECT_EQ(7, sum.units());
  EXPECT_EQ(9, sum.nanos());

  a.set_units(-2);
  a.set_nanos(-3);
  b.set_units(-5);
  b.set_nanos(-6);
  sum = SaturatedAddMoney(a, b);
  EXPECT_EQ(-7, sum.units());
  EXPECT_EQ(-9, sum.nanos());

  a.set_units(-2);
  a.set_nanos(-3);
  b.set_units(5);
  b.set_nanos(7);
  sum = SaturatedAddMoney(a, b);
  EXPECT_EQ(3, sum.units());
  EXPECT_EQ(4, sum.nanos());

  a.set_units(-2);
  a.set_nanos(-7);
  b.set_units(5);
  b.set_nanos(3);
  sum = SaturatedAddMoney(a, b);
  EXPECT_EQ(2, sum.units());
  EXPECT_EQ(999999996, sum.nanos());

  a.set_units(2);
  a.set_nanos(7);
  b.set_units(-5);
  b.set_nanos(-3);
  sum = SaturatedAddMoney(a, b);
  EXPECT_EQ(-2, sum.units());
  EXPECT_EQ(-999999996, sum.nanos());

  a.set_units(2);
  a.set_nanos(300000000);
  b.set_units(5);
  b.set_nanos(699999999);
  sum = SaturatedAddMoney(a, b);
  EXPECT_EQ(7, sum.units());
  EXPECT_EQ(999999999, sum.nanos());

  a.set_units(2);
  a.set_nanos(300000000);
  b.set_units(5);
  b.set_nanos(700000000);
  sum = SaturatedAddMoney(a, b);
  EXPECT_EQ(8, sum.units());
  EXPECT_EQ(0, sum.nanos());

  a.set_units(2);
  a.set_nanos(400000000);
  b.set_units(5);
  b.set_nanos(700000000);
  sum = SaturatedAddMoney(a, b);
  EXPECT_EQ(8, sum.units());
  EXPECT_EQ(100000000, sum.nanos());

  a.set_units(-2);
  a.set_nanos(-300000000);
  b.set_units(-5);
  b.set_nanos(-699999999);
  sum = SaturatedAddMoney(a, b);
  EXPECT_EQ(-7, sum.units());
  EXPECT_EQ(-999999999, sum.nanos());

  a.set_units(-2);
  a.set_nanos(-300000000);
  b.set_units(-5);
  b.set_nanos(-700000000);
  sum = SaturatedAddMoney(a, b);
  EXPECT_EQ(-8, sum.units());
  EXPECT_EQ(0, sum.nanos());

  a.set_units(-2);
  a.set_nanos(-400000000);
  b.set_units(-5);
  b.set_nanos(-700000000);
  sum = SaturatedAddMoney(a, b);
  EXPECT_EQ(-8, sum.units());
  EXPECT_EQ(-100000000, sum.nanos());
}

TEST_F(MoneyUtilsTest, SaturatedAddMoneyOverflow) {
  Money a;
  Money b;
  Money sum;
  a.set_currency_code("USD");
  b.set_currency_code("USD");

  a.set_units(INT64_MAX);
  a.set_nanos(999999999);
  b.set_units(0);
  b.set_nanos(1);
  sum = SaturatedAddMoney(a, b);
  EXPECT_EQ(INT64_MAX, sum.units());
  EXPECT_EQ(999999999, sum.nanos());

  a.set_units(INT64_MAX);
  a.set_nanos(999999999);
  b.set_units(1);
  b.set_nanos(0);
  sum = SaturatedAddMoney(a, b);
  EXPECT_EQ(INT64_MAX, sum.units());
  EXPECT_EQ(999999999, sum.nanos());

  a.set_units(INT64_MAX);
  a.set_nanos(999999999);
  b.set_units(INT64_MAX);
  b.set_nanos(999999999);
  sum = SaturatedAddMoney(a, b);
  EXPECT_EQ(INT64_MAX, sum.units());
  EXPECT_EQ(999999999, sum.nanos());

  a.set_units(INT64_MAX - 100000000000);
  a.set_nanos(0);
  b.set_units(200000000000);
  b.set_nanos(0);
  sum = SaturatedAddMoney(a, b);
  EXPECT_EQ(INT64_MAX, sum.units());
  EXPECT_EQ(999999999, sum.nanos());

  a.set_units(INT64_MIN);
  a.set_nanos(-999999999);
  b.set_units(0);
  b.set_nanos(-1);
  sum = SaturatedAddMoney(a, b);
  EXPECT_EQ(INT64_MIN, sum.units());
  EXPECT_EQ(-999999999, sum.nanos());

  a.set_units(INT64_MIN);
  a.set_nanos(-999999999);
  b.set_units(-1);
  b.set_nanos(-0);
  sum = SaturatedAddMoney(a, b);
  EXPECT_EQ(INT64_MIN, sum.units());
  EXPECT_EQ(-999999999, sum.nanos());

  a.set_units(INT64_MIN);
  a.set_nanos(-999999999);
  b.set_units(INT64_MIN);
  b.set_nanos(-999999999);
  sum = SaturatedAddMoney(a, b);
  EXPECT_EQ(INT64_MIN, sum.units());
  EXPECT_EQ(-999999999, sum.nanos());

  a.set_units(INT64_MIN + 30000000000000);
  a.set_nanos(0);
  b.set_units(-50000000000000);
  b.set_nanos(0);
  sum = SaturatedAddMoney(a, b);
  EXPECT_EQ(INT64_MIN, sum.units());
  EXPECT_EQ(-999999999, sum.nanos());
}

}  // namespace service_control_client
}  // namespace google
