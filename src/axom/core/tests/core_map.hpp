// Copyright (c) 2017-2021, Lawrence Livermore National Security, LLC and
// other Axom Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

#include "axom/core/StackArray.hpp" /* for axom::StackArray */
#include "axom/core/Map.hpp"
#include "gtest/gtest.h"            /* for TEST and EXPECT_* macros */
#include <string>

namespace axom
{

namespace internal
{

template <typename Key, typename T, int N>
Map <Key, T>  init()
{
  Map<Key, T> test(N/2, 2);
  return test;

}

template <typename Key, typename T, int N>
void  test_storage(Map <Key, T> &test)
{
  
  for(int i = 0; i < N; i++)
    {
      test.insert(i, i*27-N/2);
    }
  
  for(int i = 0; i < N; i++){
    EXPECT_EQ(i*27-N/2, test.find(i).value);
  }
  auto ret = test.insert(N, 900);
  EXPECT_EQ(false, ret.second);
}

template <typename Key, typename T, int N>
void  test_remove(Map <Key,T> &test)
{ 
  test.remove(13);
  auto ret = test.find(13);
  EXPECT_EQ(-2, ret.next);
  test.insert(13, 900);
  ret = test.find(13);
  EXPECT_EQ(900, ret.value);
}

template <typename Key, typename T, int N>
void test_rehash(Map <Key, T> &test)
{
  test.rehash();
    
  for(int i = 0; i < N; i++){
    EXPECT_EQ(i*27-N/2, test.find(i).value);
  }

  for(int i = N; i < 2*N; i++)
    {
      test.insert(i, i*27-N/2);
    }

  for(int i = 0; i < 2*N; i++){
    EXPECT_EQ(i*27-N/2, test.find(i).value);
  }
  auto ret = test.insert(N, 900);
  EXPECT_EQ(false, ret.second);
}


}

TEST(core_map, initialization)
{
  constexpr int N = 20; /* Number of values to store */
  Map <int, int> test = internal::init<int, int, N>();

}

TEST(core_map, insertion)
{
  constexpr int N = 20; /* Number of values to store */
  Map <int, int> test = internal::init<int, int, N>();
  internal::test_storage<int,int,N>(test);

}

TEST(core_map, removal)
{
  constexpr int N = 20; /* Number of values to store */
  Map <int, int> test = internal::init<int, int, N>();
  internal::test_storage<int, int, N>(test);
  internal::test_remove<int,int,N>(test);

}

TEST(core_map, rehash)
{
  constexpr int N = 20; /* Number of values to store */
  Map <int, int> test = internal::init<int, int, N>();
  internal::test_storage<int, int, N>(test);
  internal::test_rehash<int, int, N>(test);
  internal::test_remove<int,int,N>(test);

}
} /* namespace axom */
