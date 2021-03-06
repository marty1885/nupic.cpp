/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2016, Numenta, Inc.  Unless you have an agreement
 * with Numenta, Inc., for a separate license for this software code, the
 * following terms and conditions apply:
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero Public License for more details.
 *
 * You should have received a copy of the GNU Affero Public License
 * along with this program.  If not, see http://www.gnu.org/licenses.
 *
 * http://numenta.org/licenses/
 * ---------------------------------------------------------------------
 */

/** @file
 * Unit tests for the ScalarEncoder and PeriodicScalarEncoder
 */

#include "gtest/gtest.h"
#include <nupic/encoders/ScalarEncoder.hpp>
#include <string>
#include <vector>

using namespace nupic;

template <typename T> std::string vec2str(std::vector<T> vec) {
  std::ostringstream oss("");
  for (size_t i = 0; i < vec.size(); i++) //FIXME VectorHelper would have been used eg. here
    oss << vec[i];
  return oss.str();
}

struct ScalarValueCase
{
  Real32 input;
  std::vector<UInt> expectedOutput;
};

std::vector<UInt> patternFromNZ(int n, std::vector<size_t> patternNZ) //TODO use vectorHelpers sparseToDense
{
  auto v = std::vector<UInt>(n, 0);
  for (auto it = patternNZ.begin(); it != patternNZ.end(); it++)
    {
      v[*it] = 1;
    }
  return v;
}

void doScalarValueCases(ScalarEncoderBase& e, std::vector<ScalarValueCase> cases)
{
  for (auto c = cases.begin(); c != cases.end(); c++)
    {
      auto actualOutput = e.encode(c->input);
      for (UInt i = 0; i < e.getOutputWidth(); i++)
        {
          EXPECT_EQ(c->expectedOutput[i], actualOutput[i])
            << "For input " << c->input << " and index " << i << std::endl
            << "EXPECTED:" << std::endl
            << vec2str(c->expectedOutput) << std::endl
            << "ACTUAL:" << std::endl
            << vec2str(actualOutput);
        }
    }
}


TEST(ScalarEncoder, testClippingInputs) {
  const int n = 10;
  const int w = 2;
  const double minValue = 10;
  const double maxValue = 20;
  const double radius = 0;
  const double resolution = 0;

  {
    const bool clipInput = false;
    ScalarEncoder e(w, minValue, maxValue, n, radius, resolution, clipInput);

    EXPECT_ANY_THROW(e.encode(9.9f));
    EXPECT_NO_THROW(e.encode(10.0f));
    EXPECT_NO_THROW(e.encode(20.0f));
    EXPECT_ANY_THROW(e.encode(20.1f));
  }

  {
    const bool clipInput = true;
    ScalarEncoder e(w, minValue, maxValue, n, radius, resolution, clipInput);

    EXPECT_NO_THROW(e.encode(9.9f));
    EXPECT_NO_THROW(e.encode(20.1f));
  }
}

TEST(PeriodicScalarEncoder, ValidScalarInputs) {
  const int n = 10;
  const int w = 2;
  const double minValue = 10;
  const double maxValue = 20;
  const double radius = 0;
  const double resolution = 0;
  PeriodicScalarEncoder e(w, minValue, maxValue, n, radius, resolution);

  EXPECT_ANY_THROW(e.encode(9.9f));
  EXPECT_NO_THROW(e.encode(10.0f));
  EXPECT_NO_THROW(e.encode(19.9f));
  EXPECT_ANY_THROW(e.encode(20.0f));
}

TEST(ScalarEncoder, NonIntegerBucketWidth) {
  const int n = 7;
  const int w = 3;
  const double minValue = 10.0;
  const double maxValue = 20.0;
  const double radius = 0;
  const double resolution = 0;
  const bool clipInput = false;
  ScalarEncoder encoder(w, minValue, maxValue, n, radius, resolution,
                        clipInput);

  std::vector<ScalarValueCase> cases = {{10.0, patternFromNZ(n, {0, 1, 2})},
                                        {20.0, patternFromNZ(n, {4, 5, 6})}};

  doScalarValueCases(encoder, cases);
}

TEST(PeriodicScalarEncoder, NonIntegerBucketWidth) {
  const int n = 7;
  const int w = 3;
  const double minValue = 10.0;
  const double maxValue = 20.0;
  const double radius = 0;
  const double resolution = 0;
  PeriodicScalarEncoder encoder(w, minValue, maxValue, n, radius, resolution);

  std::vector<ScalarValueCase> cases = {{10.0f, patternFromNZ(n, {6, 0, 1})},
                                        {19.9f, patternFromNZ(n, {5, 6, 0})}};

  doScalarValueCases(encoder, cases);
}

TEST(ScalarEncoder, RoundToNearestMultipleOfResolution) {
  const int n_in = 0;
  const int w = 3;
  const double minValue = 10.0;
  const double maxValue = 20.0;
  const double radius = 0;
  const double resolution = 1;
  const bool clipInput = false;
  ScalarEncoder encoder(w, minValue, maxValue, n_in, radius, resolution, clipInput);

  const unsigned int n = 13u;
  ASSERT_EQ(n, encoder.getOutputWidth());

  std::vector<ScalarValueCase> cases = {
      {10.00f, patternFromNZ(n, {0, 1, 2})},
      {10.49f, patternFromNZ(n, {0, 1, 2})},
      {10.50f, patternFromNZ(n, {1, 2, 3})},
      {11.49f, patternFromNZ(n, {1, 2, 3})},
      {11.50f, patternFromNZ(n, {2, 3, 4})},
      {14.49f, patternFromNZ(n, {4, 5, 6})},
      {14.50f, patternFromNZ(n, {5, 6, 7})},
      {15.49f, patternFromNZ(n, {5, 6, 7})},
      {15.50f, patternFromNZ(n, {6, 7, 8})},
      {19.49f, patternFromNZ(n, {9, 10, 11})},
      {19.50f, patternFromNZ(n, {10, 11, 12})},
      {20.00f, patternFromNZ(n, {10, 11, 12})}};

  doScalarValueCases(encoder, cases);
}

TEST(PeriodicScalarEncoder, FloorToNearestMultipleOfResolution) {
  const int n_in = 0;
  const int w = 3;
  const double minValue = 10.0;
  const double maxValue = 20.0;
  const double radius = 0;
  const double resolution = 1;
  PeriodicScalarEncoder encoder(w, minValue, maxValue, n_in, radius,
                                resolution);

  const unsigned int n = 10u;
  ASSERT_EQ(n, encoder.getOutputWidth());

  std::vector<ScalarValueCase> cases = {{10.00f, patternFromNZ(n, {9, 0, 1})},
                                        {10.99f, patternFromNZ(n, {9, 0, 1})},
                                        {11.00f, patternFromNZ(n, {0, 1, 2})},
                                        {11.99f, patternFromNZ(n, {0, 1, 2})},
                                        {12.00f, patternFromNZ(n, {1, 2, 3})},
                                        {14.00f, patternFromNZ(n, {3, 4, 5})},
                                        {14.99f, patternFromNZ(n, {3, 4, 5})},
                                        {15.00f, patternFromNZ(n, {4, 5, 6})},
                                        {15.99f, patternFromNZ(n, {4, 5, 6})},
                                        {19.00f, patternFromNZ(n, {8, 9, 0})},
                                        {19.99f, patternFromNZ(n, {8, 9, 0})}};

  doScalarValueCases(encoder, cases);
}
