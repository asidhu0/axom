// Copyright (c) 2017-2021, Lawrence Livermore National Security, LLC and
// other Axom Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

#include "axom/core/StackArray.hpp" /* for axom::StackArray */
#include "axom/core/Map.hpp"
#include "gtest/gtest.h" /* for TEST and EXPECT_* macros */
#include <string>

namespace axom
{
namespace internal
{
template <typename Key, typename T>
experimental::Map<Key, T> init(int N, int len)
{
  experimental::Map<Key, T> test(N, len);
  EXPECT_EQ(N*len, test.max_size());
  EXPECT_EQ(0, test.size());
  EXPECT_EQ(true, test.empty());
  EXPECT_EQ(len, test.bucket_size());
  EXPECT_EQ(N, test.bucket_count());
  return test;
}

template <typename Key, typename T>
void test_storage(experimental::Map<Key, T> &test)
{
  for(int i = 0; i < test.max_size(); i++)
  {
    auto ret_test = test.insert(i, i * 27);
    EXPECT_EQ(true, ret_test.second);
  }
  EXPECT_EQ(false, test.empty());
  for(int i = 0; i < test.max_size(); i++)
  {
    EXPECT_EQ(i * 27, test.find(i).value);
  }
  //This should fail, since we're at capacity.
  auto ret = test.insert(test.max_size(), 900);
  EXPECT_EQ(false, ret.second);
}

template <typename Key, typename T>
void test_brackets(experimental::Map<Key, T> &test)
{
  for(int i = 0; i < test.size(); i++){
    EXPECT_EQ(i*27, test[i]);
  }
}

template <typename Key, typename T>
void test_remove(experimental::Map<Key, T> &test)
{
  test.erase(0);
  auto ret = test.find(0);
  EXPECT_EQ(-2, ret.next);
  test.insert(0, 900);
  ret = test.find(0);
  EXPECT_EQ(900, ret.value);
}

template <typename Key, typename T>
void test_rehash(experimental::Map<Key, T> &test, int num, int fact)
{
  auto original_size = test.size();
  test.rehash(num, fact);

  for(int i = 0; i < original_size; i++)
  {
    EXPECT_EQ(i * 27, test.find(i).value);
  }

  for(int i = original_size; i < test.max_size(); i++)
  {
    test.insert(i, i * 27);
  }

  for(int i = original_size; i < test.max_size(); i++)
  {
    EXPECT_EQ(i * 27, test.find(i).value);
  }
  auto ret = test.insert(test.max_size(), 900);
  EXPECT_EQ(false, ret.second);
}

}  // namespace internal

TEST(core_map, initialization)
{
  for(int i : {1, 2, 5, 10, 20, 100}){
    for(int j : {1, 2, 5, 10}){
      experimental::Map<int, int> test = internal::init<int, int>(i, j);
    }
  }
}

TEST(core_map, insertion)
{
  for(int i: {1, 2, 5, 10, 20, 100}){
    for(int j: {1, 2, 5, 10}){
      experimental::Map<int, int> test = internal::init<int, int>(i, j);
      internal::test_storage<int, int>(test);
    }
  }
}

TEST(core_map, brackets)
{
 for(int i: {1, 2, 5, 10, 20, 100}){
    for(int j: {1, 2, 5, 10}){
      experimental::Map<int, int> test = internal::init<int, int>(i, j);
      internal::test_storage<int, int>(test);
      internal::test_brackets<int, int>(test);
    }
  }
}

TEST(core_map, removal)
{

  for(int i: {1, 2, 5, 10, 20, 100}){
    for(int j: {1, 2, 5, 10}){
      experimental::Map<int, int> test = internal::init<int, int>(i, j);
      internal::test_storage<int, int>(test);
      internal::test_remove<int, int>(test);
    }
  }
}

TEST(core_map, rehash)
{
  for(int i: {1, 2, 5, 10, 20, 100}){
    for(int j: {1, 2, 5, 10}){
      for(int k: {2, 4, 8}){
        experimental::Map<int, int> test = internal::init<int, int>(i, j);
        internal::test_storage<int, int>(test);
        internal::test_rehash<int, int>(test, -1, k);
        internal::test_remove<int, int>(test);
      }
    }
  }
  for(int i: {1, 2, 5, 10, 20, 100}){
    for(int j: {1, 2, 5, 10}){
      for(int k = 0; k < 3; k++){
        experimental::Map<int, int> test = internal::init<int, int>(i, j);
        internal::test_storage<int, int>(test);
        internal::test_rehash<int, int>(test, test.size()+20*k, -1);
        internal::test_remove<int, int>(test);
      }
    }
  }
}
} /* namespace axom */
