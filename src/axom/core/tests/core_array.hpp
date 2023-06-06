// Copyright (c) 2017-2023, Lawrence Livermore National Security, LLC and
// other Axom Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

#include "axom/core/Array.hpp"
#include "axom/core/ArrayView.hpp"
#include "axom/core/memory_management.hpp"

#include "gtest/gtest.h"

#include <algorithm>

namespace axom
{
namespace internal
{
/*!
 * \brief Calculate the new capacity for an Array given an increase in the
 *  size.
 * \param [in] v, the Array in question.
 * \param [in] increase, the amount the size will increase by
 * \return the new capacity.
 */
template <typename T>
IndexType calc_new_capacity(Array<T>& v, IndexType increase)
{
  IndexType new_num_elements = v.size() + increase;
  if(new_num_elements > v.capacity())
  {
    return new_num_elements * v.getResizeRatio() + 0.5;
  }

  return v.capacity();
}

/*!
 * \brief Check if two Arrays are copies. Does not check the resize ratio.
 * \param [in] lhs, the first Array to compare.
 * \param [in] rhs, the second Array to compare.
 * \return the new capacity.
 */
template <typename T>
void check_copy(const Array<T>& lhs, const Array<T>& rhs)
{
  EXPECT_EQ(lhs.size(), rhs.size());
  EXPECT_EQ(lhs.capacity(), rhs.capacity());

  const T* lhs_data = lhs.data();
  const T* rhs_data = rhs.data();
  EXPECT_EQ(lhs_data, rhs_data);
}

/*!
 * \brief Check that the storage of an Array is working properly.
 * \param [in] v the Array to check.
 */
template <typename T>
void check_storage(Array<T>& v)
{
  EXPECT_TRUE(v.empty());
  EXPECT_EQ(v.size(), 0);

  IndexType capacity = v.capacity();
  const T* data_ptr = v.data();

  /* Push back up to half the capacity. */
  for(T i = 0; i < capacity / 2; ++i)
  {
    v.push_back(i);
  }

  /* Check the array metadata. */
  EXPECT_TRUE(!v.empty());
  EXPECT_EQ(v.size(), capacity / 2);
  EXPECT_EQ(v.capacity(), capacity);
  EXPECT_EQ(v.data(), data_ptr);

  /* Push back up to the full capacity. */
  for(T i = capacity / 2; i < capacity; ++i)
  {
    v.push_back(i);
  }

  /* Check the array metadata. */
  EXPECT_TRUE(!v.empty());
  EXPECT_EQ(v.size(), capacity);
  EXPECT_EQ(v.capacity(), capacity);
  EXPECT_EQ(v.data(), data_ptr);

  /* Check the array data using the [] operator and the raw pointer. */
  for(IndexType i = 0; i < capacity; ++i)
  {
    EXPECT_EQ(v[i], i);
    EXPECT_EQ(data_ptr[i], i);
  }

  /* Set the array data to new values using the [] operator. */
  for(IndexType i = 0; i < capacity; ++i)
  {
    v[i] = i - 5 * i + 7;
  }

  /* Check the array data using the [] operator and the raw pointer. */
  for(IndexType i = 0; i < capacity; ++i)
  {
    EXPECT_EQ(v[i], i - 5 * i + 7);
    EXPECT_EQ(data_ptr[i], i - 5 * i + 7);
  }

  /* Check the array metadata. */
  EXPECT_TRUE(!v.empty());
  EXPECT_EQ(v.size(), capacity);
  EXPECT_EQ(v.capacity(), capacity);
  EXPECT_EQ(v.data(), data_ptr);
}

/*!
 * \brief Check that the fill method is working properly.
 * \param [in] v the Array to check.
 */
template <typename T, int DIM, MemorySpace SPACE>
void check_fill(Array<T, DIM, SPACE>& v)
{
  constexpr T MAGIC_NUM_0 = 55;
  constexpr T MAGIC_NUM_1 = 6834;
  const IndexType capacity = v.capacity();
  const IndexType size = v.size();
  const double ratio = v.getResizeRatio();
  const T* const data_ptr = v.data();

  /* Fill the Array with MAGIC_NUM_0. */
  v.fill(MAGIC_NUM_0);

  /* Check the meta data. */
  EXPECT_EQ(capacity, v.capacity());
  EXPECT_EQ(size, v.size());
  EXPECT_EQ(ratio, v.getResizeRatio());
  EXPECT_EQ(data_ptr, v.data());

  // To check entries, we copy data to a dynamic array
  int host_alloc_id = axom::getDefaultAllocatorID();
  Array<T, DIM> v_host(v, host_alloc_id);

  /* Check that the entries are all MAGIC_NUM_0. */
  for(IndexType i = 0; i < size; ++i)
  {
    EXPECT_EQ(v_host[i], MAGIC_NUM_0);
  }

  /* Fill the Array with MAGIC_NUM_1. */
  v.fill(MAGIC_NUM_1);

  /* Check the meta data. */
  EXPECT_EQ(capacity, v.capacity());
  EXPECT_EQ(size, v.size());
  EXPECT_EQ(ratio, v.getResizeRatio());
  EXPECT_EQ(data_ptr, v.data());

  v_host = Array<T, DIM>(v, host_alloc_id);

  /* Check that the entries are all MAGIC_NUM_1. */
  for(IndexType i = 0; i < size; ++i)
  {
    EXPECT_EQ(v_host[i], MAGIC_NUM_1);
  }
}

/*!
 * \brief Check that the set method is working properly.
 * \param [in] v the Array to check.
 */
template <typename T>
void check_set(Array<T>& v)
{
  constexpr T ZERO = 0;
  const IndexType capacity = v.capacity();
  const IndexType size = v.size();
  const double ratio = v.getResizeRatio();
  const T* const data_ptr = v.data();

  /* Allocate a buffer half the size of the array. Fill it up with sequential
   * values. */
  const IndexType buffer_size = size / 2;
  T* buffer = allocate<T>(buffer_size);
  for(IndexType i = 0; i < buffer_size; ++i)
  {
    buffer[i] = i;
  }

  /* Set all the values in the array to zero. */
  v.fill(ZERO);

  /* Set the first half of the elements in the array to the sequential values in
   * buffer. */
  v.set(buffer, buffer_size, 0);

  /* Check the array metadata. */
  EXPECT_EQ(capacity, v.capacity());
  EXPECT_EQ(size, v.size());
  EXPECT_EQ(ratio, v.getResizeRatio());
  EXPECT_EQ(data_ptr, v.data());

  /* Check that the first half of the elements in the array are equivalent to
   * those in buffer. */
  for(IndexType i = 0; i < buffer_size; ++i)
  {
    EXPECT_EQ(v[i], buffer[i]);
  }

  /* Check that the second half of the elements in the array are all zero. */
  for(IndexType i = buffer_size; i < size; ++i)
  {
    EXPECT_EQ(v[i], ZERO);
  }

  /* Reset the values in buffer to the next sequential values. */
  for(IndexType i = 0; i < buffer_size; ++i)
  {
    buffer[i] = i + buffer_size;
  }

  /* Set the second half of the elements in the array to the new sequential
   * values in buffer. */
  v.set(buffer, buffer_size, buffer_size);

  /* Check the array metadata. */
  EXPECT_EQ(capacity, v.capacity());
  EXPECT_EQ(size, v.size());
  EXPECT_EQ(ratio, v.getResizeRatio());
  EXPECT_EQ(data_ptr, v.data());

  /* Check that all the elements in the array now hold sequential values. */
  for(IndexType i = 0; i < 2 * buffer_size; ++i)
  {
    EXPECT_EQ(v[i], i);
  }

  deallocate(buffer);
}

/*!
 * \brief Check that the resizing of an Array is working properly.
 * \param [in] v the Array to check.
 */
template <typename T>
void check_resize(Array<T>& v)
{
  /* Resize the array up to the capacity */
  IndexType capacity = v.capacity();
  v.resize(capacity);
  IndexType size = capacity;

  /* Check that the size equals the capacity. */
  EXPECT_EQ(v.size(), v.capacity());

  /* Set the existing data in v */
  for(IndexType i = 0; i < size; ++i)
  {
    v[i] = static_cast<T>(i - 5 * i + 7);
  }

  /* Push back a new element, should resize. */
  IndexType old_capacity = capacity;
  capacity = calc_new_capacity(v, 1);
  v.push_back(size - 5 * size + 7);
  size++;

  /* Check that it resized properly */
  EXPECT_GT(capacity, old_capacity);
  EXPECT_EQ(v.capacity(), capacity);
  EXPECT_EQ(v.size(), size);

  /* Check that the data is still intact. */
  for(IndexType i = 0; i < size; ++i)
  {
    EXPECT_EQ(v[i], i - 5 * i + 7);
  }

  /* Push back 1000 elements */
  const IndexType n_elements = 1000;

  T* values = allocate<T>(n_elements);
  for(IndexType i = 0; i < n_elements; ++i)
  {
    IndexType i_real = i + size;
    values[i] = i_real - 5 * i_real + 7;
  }

  /* Push back the new elements. */
  capacity = calc_new_capacity(v, n_elements);
  v.insert(size, n_elements, values);
  size += n_elements;

  /* Check that size and capacity are as expected. */
  EXPECT_EQ(v.capacity(), capacity);
  EXPECT_EQ(v.size(), size);

  /* Check that the data is still intact. */
  for(IndexType i = 0; i < size; ++i)
  {
    EXPECT_EQ(v[i], i - 5 * i + 7);
  }

  /* Reduce the size down to 500 elements */
  T* data_address = v.data();
  size = 500;
  v.resize(size);

  /* Check the metadata. */
  EXPECT_EQ(v.size(), size);
  EXPECT_EQ(v.capacity(), capacity);
  EXPECT_EQ(v.data(), data_address);

  /* Check the data. */
  for(IndexType i = 0; i < size; ++i)
  {
    EXPECT_EQ(v[i], i - 5 * i + 7);
  }

  /* Shrink the vector */
  capacity = size;
  v.shrink();

  /* Check that the capacity and size are as expected. */
  EXPECT_EQ(v.capacity(), capacity);
  EXPECT_EQ(v.size(), size);

  /* Check that the data is intact. */
  for(IndexType i = 0; i < size; ++i)
  {
    EXPECT_EQ(v[i], i - 5 * i + 7);
  }

  /* Push back a new element, should resize. */
  old_capacity = capacity;
  capacity = calc_new_capacity(v, 1);
  v.push_back(size - 5 * size + 7);
  size++;

  /* Check the new size and capacity. */
  EXPECT_GT(capacity, old_capacity);
  EXPECT_EQ(v.capacity(), capacity);
  EXPECT_EQ(v.size(), size);

  /* Check that the data is intact. */
  for(IndexType i = 0; i < size; ++i)
  {
    EXPECT_EQ(v[i], i - 5 * i + 7);
  }

  /* Reset the data */
  T* data_ptr = v.data();
  for(IndexType i = 0; i < size; ++i)
  {
    data_ptr[i] = i;
  }

  /* Push back a bunch of elements to fill in up to the capacity. Resize should
   * not occur. */
  old_capacity = capacity;
  for(IndexType i = size; i < old_capacity; ++i)
  {
    v.push_back(i);
    size++;
    EXPECT_EQ(v.capacity(), old_capacity);
    EXPECT_EQ(v.size(), size);
    EXPECT_EQ(v.data(), data_ptr);
  }

  EXPECT_EQ(v.size(), old_capacity);

  /* Push back a final element that should trigger a resize. */
  capacity = calc_new_capacity(v, old_capacity - size + 1);
  v.push_back(size);
  size++;

  /* Check the new capacity and size. */
  EXPECT_GT(capacity, old_capacity);
  EXPECT_EQ(v.capacity(), capacity);
  EXPECT_EQ(v.size(), size);

  /* Check the data. */
  for(IndexType i = 0; i < size; ++i)
  {
    EXPECT_EQ(v[i], i);
  }

  deallocate(values);
  values = nullptr;
}

/*!
 * \brief Check that the insertion into an Array is working properly.
 * \param [in] v the Array to check.
 */
template <typename T>
void check_insert(Array<T>& v)
{
  /* Resize the array up to the capacity */
  IndexType capacity = v.capacity();
  v.resize(capacity);
  IndexType size = capacity;

  EXPECT_EQ(v.size(), v.capacity());

  /* Set the existing data in v */
  for(IndexType i = 0; i < size; ++i)
  {
    v[i] = i - 5 * i + 7;
  }

  /* Insert a new element, should resize. */
  IndexType old_capacity = capacity;
  capacity = calc_new_capacity(v, 1);
  v.insert(v.size(), 1, size - 5 * size + 7);
  size++;

  /* Check that it resized properly */
  EXPECT_GT(capacity, old_capacity);
  EXPECT_EQ(v.capacity(), capacity);
  EXPECT_EQ(v.size(), size);
  for(IndexType i = 0; i < size; ++i)
  {
    EXPECT_EQ(v[i], i - 5 * i + 7);
  }

  /* Insert 1000 elements */
  const IndexType n_elements = 1000;
  T* values = allocate<T>(n_elements);
  for(IndexType i = 0; i < n_elements; ++i)
  {
    IndexType i_real = i + size;
    values[i] = i_real - 5 * i_real + 7;
  }

  capacity = calc_new_capacity(v, n_elements);
  v.insert(size, n_elements, values);
  size += n_elements;

  /* Check that it resizes properly */
  EXPECT_EQ(v.capacity(), capacity);
  EXPECT_EQ(v.size(), size);
  for(IndexType i = 0; i < size; ++i)
  {
    EXPECT_EQ(v[i], i - 5 * i + 7);
  }

  capacity = size;
  v.shrink();
  IndexType n_insert_front = 100;

  /* Reset the data */
  T* data_ptr = v.data();
  for(IndexType i = 0; i < size; ++i)
  {
    data_ptr[i] = i + n_insert_front;
  }

  /* Insert into the front of the Array. */
  for(IndexType i = n_insert_front - 1; i >= 0; i--)
  {
    capacity = calc_new_capacity(v, 1);
    v.insert(0, 1, i);
    size++;
  }

  /* Check that the insertion worked as expected */
  EXPECT_EQ(v.capacity(), capacity);
  EXPECT_EQ(v.size(), size);
  for(IndexType i = 0; i < size; ++i)
  {
    EXPECT_EQ(v[i], i);
  }

  deallocate(values);
  values = nullptr;
}

/*!
 * \brief Check that the insertion into an Array is working properly
 *        for iterators.
 * \param [in] v the Array to check.
 */
template <typename T>
void check_insert_iterator(Array<T>& v)
{
  /* Resize the array up to the capacity */
  IndexType capacity = v.capacity();
  v.resize(capacity);
  IndexType size = capacity;

  EXPECT_EQ(v.size(), v.capacity());

  /* Set the existing data in v */
  for(IndexType i = 0; i < size; ++i)
  {
    v[i] = i - 5 * i + 7;
  }

  /* Insert a new element, should resize. */
  IndexType old_capacity = capacity;
  capacity = calc_new_capacity(v, 1);
  typename axom::Array<T>::ArrayIterator ret =
    v.insert(v.end(), 1, size - 5 * size + 7);
  size++;

  /* Check that it resized properly */
  EXPECT_GT(capacity, old_capacity);
  EXPECT_EQ(v.capacity(), capacity);
  EXPECT_EQ(v.size(), size);
  EXPECT_EQ(ret, v.end() - 1);
  for(IndexType i = 0; i < size; ++i)
  {
    EXPECT_EQ(v[i], i - 5 * i + 7);
  }

  /* Insert 1000 elements */
  const IndexType n_elements = 1000;
  T* values = allocate<T>(n_elements);
  for(IndexType i = 0; i < n_elements; ++i)
  {
    IndexType i_real = i + size;
    values[i] = i_real - 5 * i_real + 7;
  }

  capacity = calc_new_capacity(v, n_elements);
  typename axom::Array<T>::ArrayIterator ret2 =
    v.insert(v.end(), n_elements, values);

  EXPECT_EQ(ret2, v.begin() + size);
  size += n_elements;

  /* Check that it resizes properly */
  EXPECT_EQ(v.capacity(), capacity);
  EXPECT_EQ(v.size(), size);
  for(IndexType i = 0; i < size; ++i)
  {
    EXPECT_EQ(v[i], i - 5 * i + 7);
  }

  capacity = size;
  v.shrink();
  IndexType n_insert_front = 100;

  /* Reset the data */
  T* data_ptr = v.data();
  for(IndexType i = 0; i < size; ++i)
  {
    data_ptr[i] = i + n_insert_front;
  }

  /* Insert into the front of the Array. */
  for(IndexType i = n_insert_front - 1; i >= 0; i--)
  {
    capacity = calc_new_capacity(v, 1);
    typename axom::Array<T>::ArrayIterator ret3 = v.insert(v.begin(), 1, i);
    EXPECT_EQ(ret3, v.begin());
    EXPECT_EQ(i, v.front());
    size++;
  }

  /* Check that the insertion worked as expected */
  EXPECT_EQ(v.capacity(), capacity);
  EXPECT_EQ(v.size(), size);
  for(IndexType i = 0; i < size; ++i)
  {
    EXPECT_EQ(v[i], i);
  }

  deallocate(values);
  values = nullptr;
}

/*!
 * \brief Check that emplace() into an Array is working properly.
 * \param [in] v the Array to check.
 */
template <typename T>
void check_emplace(Array<T>& v)
{
  /* Resize the array up to the capacity */
  IndexType capacity = v.capacity();
  v.resize(capacity);
  IndexType size = capacity;

  EXPECT_EQ(v.size(), v.capacity());

  /* Set the existing data in v */
  for(IndexType i = 0; i < size; ++i)
  {
    v[i] = i - 5 * i + 7;
  }

  /* Emplace a new element, should resize. */
  IndexType old_capacity = capacity;
  capacity = calc_new_capacity(v, 1);
  typename axom::Array<T>::ArrayIterator ret =
    v.emplace(v.end(), size - 5 * size + 7);
  size++;

  /* Check that it resized properly */
  EXPECT_GT(capacity, old_capacity);
  EXPECT_EQ(v.size(), size);
  EXPECT_EQ(ret, v.end() - 1);
  for(IndexType i = 0; i < size; ++i)
  {
    EXPECT_EQ(v[i], i - 5 * i + 7);
  }

  /* Emplace_back 1000 elements */
  const IndexType n_elements = 1000;
  for(IndexType i = 0; i < n_elements; ++i)
  {
    IndexType i_real = i + size;
    v.emplace_back(i_real - 5 * i_real + 7);
  }

  size += n_elements;

  /* Check that it resizes properly */
  EXPECT_EQ(v.size(), size);
  for(IndexType i = 0; i < size; ++i)
  {
    EXPECT_EQ(v[i], i - 5 * i + 7);
  }

  capacity = size;
  v.shrink();
  IndexType n_insert_front = 100;

  /* Reset the data */
  T* data_ptr = v.data();
  for(IndexType i = 0; i < size; ++i)
  {
    data_ptr[i] = i + n_insert_front;
  }

  /* Emplace into the front of the Array. */
  for(IndexType i = n_insert_front - 1; i >= 0; i--)
  {
    capacity = calc_new_capacity(v, 1);
    typename axom::Array<T>::ArrayIterator ret3 = v.emplace(v.begin(), i);
    EXPECT_EQ(ret3, v.begin());
    EXPECT_EQ(i, v.front());
    size++;
  }

  /* Check that the emplace worked as expected */
  EXPECT_EQ(v.capacity(), capacity);
  EXPECT_EQ(v.size(), size);
  for(IndexType i = 0; i < size; ++i)
  {
    EXPECT_EQ(v[i], i);
  }
}

// Clumsy implementation of https://numpy.org/doc/stable/reference/generated/numpy.zeros_like.html
// for testing purposes
template <typename T, int DIM>
Array<T, DIM> zeros_like(const Array<T, DIM>& other)
{
  Array<T, DIM> zeros(other);
  zeros.fill(T {});
  return zeros;
}

template <typename T, int DIM>
void check_swap(Array<T, DIM>& v)
{
  auto v_two = zeros_like(v);

  /* Push 0...size elements */
  for(int i = 0; i < v.size(); i++)
  {
    v.flatIndex(i) = i;
    v_two.flatIndex(i) = -i;
  }

  /* Create copies */
  axom::Array<T, DIM> v_copy(v);
  axom::Array<T, DIM> v_two_copy(v_two);

  EXPECT_EQ(v, v_copy);
  EXPECT_EQ(v_two, v_two_copy);

  /* Swap */
  v.swap(v_two);

  EXPECT_NE(v, v_two);
  EXPECT_NE(v, v_copy);

  /* Swap back */
  v.swap(v_two);

  EXPECT_EQ(v, v_copy);
  EXPECT_EQ(v_two, v_two_copy);
}

template <typename T, int DIM, axom::MemorySpace SPACE>
void check_alloc(Array<T, DIM, SPACE>& v, const int id)
{
  // Verify allocation
  EXPECT_EQ(v.getAllocatorID(), id);
  EXPECT_EQ(v.size(), v.capacity());

// Use introspection to verify pointer if available
#if defined(AXOM_USE_UMPIRE)
  umpire::ResourceManager& rm = umpire::ResourceManager::getInstance();
  auto v_allocator = rm.getAllocator(v.data());
  EXPECT_EQ(v_allocator.getId(), id);
#endif
}

/*!
 * \brief Check an external array for defects.
 * \param [in] v the external array to check.
 */
template <typename T>
void check_external_view(ArrayView<T>& v)
{
  const IndexType size = v.size();
  const IndexType num_values = size;
  T* const data_ptr = v.data();

  /* Set the elements in the array. */
  for(IndexType i = 0; i < size; ++i)
  {
    v[i] = i;
  }

  /* Check the elements using the raw pointer. */
  for(IndexType i = 0; i < num_values; ++i)
  {
    EXPECT_EQ(data_ptr[i], i);
  }

  /* Set the elements using the raw pointer. */
  for(IndexType i = 0; i < size; ++i)
  {
    data_ptr[i] = i * i;
  }

  /* Check the elements using the () operator. */
  for(IndexType i = 0; i < size; ++i)
  {
    EXPECT_EQ(v[i], i * i);
  }

  EXPECT_EQ(size, v.size());
  EXPECT_EQ(data_ptr, v.data());
}

#if defined(AXOM_USE_GPU) && defined(AXOM_GPUCC) && defined(AXOM_USE_UMPIRE)

template <typename T>
__global__ void assign_raw(T* data, int N)
{
  for(int i = 0; i < N; i++)
  {
    data[i] = i;
  }
}

template <typename T, int DIM, axom::MemorySpace SPACE>
__global__ void assign_view(ArrayView<T, DIM, SPACE> view)
{
  for(int i = 0; i < view.size(); i++)
  {
    view[i] = i * 2;
  }
}

/*!
 * \brief Check that an array can be modified/accessed from device code
 * \param [in] v the array to check.
 */
template <typename T, int DIM, axom::MemorySpace SPACE>
void check_device(Array<T, DIM, SPACE>& v)
{
  const IndexType size = v.size();
  // Then assign to it via a raw device pointer
  assign_raw<<<1, 1>>>(v.data(), size);

  // Check the contents of the array by assigning to a Dynamic array
  // The Umpire allocator should be Host, so we can access it from the CPU
  int host_alloc_id = axom::getDefaultAllocatorID();
  Array<T, 1> check_raw_array_dynamic(v, host_alloc_id);
  EXPECT_EQ(check_raw_array_dynamic.size(), size);
  for(int i = 0; i < check_raw_array_dynamic.size(); i++)
  {
    EXPECT_EQ(check_raw_array_dynamic[i], i);
  }

  // Then check the contents by assigning to an explicitly Host array
  Array<T, 1, axom::MemorySpace::Host> check_raw_array_host = v;
  EXPECT_EQ(check_raw_array_host.size(), size);
  for(int i = 0; i < check_raw_array_host.size(); i++)
  {
    EXPECT_EQ(check_raw_array_host[i], i);
  }

  // Then modify the underlying data via a view
  ArrayView<T, DIM, SPACE> view(v);
  assign_view<<<1, 1>>>(view);

  // Check the contents of the array by assigning to a Dynamic array
  // The Umpire allocator should be Host, so we can access it from the CPU
  Array<T, 1> check_view_array_dynamic(view, host_alloc_id);
  EXPECT_EQ(check_view_array_dynamic.size(), size);
  for(int i = 0; i < check_view_array_dynamic.size(); i++)
  {
    EXPECT_EQ(check_view_array_dynamic[i], i * 2);
  }

  // Then check the contents by assigning to an explicitly Host array
  Array<T, 1, axom::MemorySpace::Host> check_view_array_host = view;
  EXPECT_EQ(check_view_array_host.size(), size);
  for(int i = 0; i < check_view_array_host.size(); i++)
  {
    EXPECT_EQ(check_view_array_host[i], i * 2);
  }
}

template <typename T>
__global__ void assign_raw_2d(T* data, int M, int N)
{
  for(int i = 0; i < M; i++)
  {
    for(int j = 0; j < N; j++)
    {
      data[i * N + j] = i * i + j;
    }
  }
}

template <typename T, axom::MemorySpace SPACE>
__global__ void assign_view_2d(ArrayView<T, 2, SPACE> view)
{
  for(int i = 0; i < view.shape()[0]; i++)
  {
    for(int j = 0; j < view.shape()[1]; j++)
    {
      view(i, j) = j * j + i;
    }
  }
}

/*!
 * \brief Check that a 2D array can be modified/accessed from device code
 * \param [in] v the array to check.
 */
template <typename T, axom::MemorySpace SPACE>
void check_device_2D(Array<T, 2, SPACE>& v)
{
  const IndexType size = v.size();
  const IndexType M = v.shape()[0];
  const IndexType N = v.shape()[1];
  // Then assign to it via a raw device pointer
  assign_raw_2d<<<1, 1>>>(v.data(), M, N);

  // Check the contents of the array by assigning to a Dynamic array
  // The Umpire allocator should be Host, so we can access it from the CPU
  int host_alloc_id = axom::getDefaultAllocatorID();
  Array<T, 2> check_raw_array_dynamic(v, host_alloc_id);
  EXPECT_EQ(check_raw_array_dynamic.size(), size);
  EXPECT_EQ(check_raw_array_dynamic.shape(), v.shape());

  for(int i = 0; i < M; i++)
  {
    for(int j = 0; j < N; j++)
    {
      EXPECT_EQ(check_raw_array_dynamic(i, j), i * i + j);
    }
  }

  // Then check the contents by assigning to an explicitly Host array
  Array<T, 2, axom::MemorySpace::Host> check_raw_array_host = v;
  EXPECT_EQ(check_raw_array_host.size(), size);
  EXPECT_EQ(check_raw_array_host.shape(), v.shape());

  for(int i = 0; i < M; i++)
  {
    for(int j = 0; j < N; j++)
    {
      EXPECT_EQ(check_raw_array_host(i, j), i * i + j);
    }
  }

  // Then modify the underlying data via a view
  ArrayView<T, 2, SPACE> view(v);
  assign_view_2d<<<1, 1>>>(view);

  // Check the contents of the array by assigning to a Dynamic array
  // The Umpire allocator should be Host, so we can access it from the CPU
  Array<T, 2> check_view_array_dynamic(view, host_alloc_id);
  EXPECT_EQ(check_view_array_dynamic.size(), size);
  EXPECT_EQ(check_view_array_dynamic.shape(), v.shape());

  for(int i = 0; i < M; i++)
  {
    for(int j = 0; j < N; j++)
    {
      EXPECT_EQ(check_view_array_dynamic(i, j), j * j + i);
    }
  }

  // Then check the contents by assigning to an explicitly Host array
  Array<T, 2, axom::MemorySpace::Host> check_view_array_host = view;
  EXPECT_EQ(check_view_array_host.size(), size);
  EXPECT_EQ(check_view_array_host.shape(), v.shape());

  for(int i = 0; i < M; i++)
  {
    for(int j = 0; j < N; j++)
    {
      EXPECT_EQ(check_view_array_host(i, j), j * j + i);
    }
  }
}

#endif  // defined(AXOM_USE_GPU) && defined(AXOM_GPUCC) && defined(AXOM_USE_UMPIRE)

} /* end namespace internal */

//------------------------------------------------------------------------------
// UNIT TESTS
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
TEST(core_array, checkStorage)
{
  constexpr IndexType ZERO = 0;

  for(IndexType capacity = 2; capacity < 512; capacity *= 2)
  {
    Array<int> v_int(ZERO, capacity);
    internal::check_storage(v_int);

    Array<double> v_double(ZERO, capacity);
    internal::check_storage(v_double);
  }
}

//------------------------------------------------------------------------------
TEST(core_array, checkFill)
{
  for(IndexType capacity = 2; capacity < 512; capacity *= 2)
  {
    IndexType size = capacity / 2;
    Array<int> v_int(size, capacity);
    internal::check_fill(v_int);

    Array<double> v_double(size, capacity);
    internal::check_fill(v_double);
  }
}

//------------------------------------------------------------------------------
#if defined(AXOM_USE_GPU) && defined(AXOM_GPUCC) && defined(AXOM_USE_UMPIRE)
TEST(core_array, checkFillDevice)
{
  for(IndexType capacity = 2; capacity < 512; capacity *= 2)
  {
    IndexType size = capacity / 2;
    Array<int, 1, MemorySpace::Device> v_int(size, capacity);
    internal::check_fill(v_int);

    Array<double, 1, MemorySpace::Device> v_double(size, capacity);
    internal::check_fill(v_double);
  }
}
#endif

//------------------------------------------------------------------------------
TEST(core_array, checkSet)
{
  for(IndexType size = 2; size < 512; size *= 2)
  {
    Array<int> v_int(size);
    internal::check_set(v_int);

    Array<double> v_double(size);
    internal::check_set(v_double);
  }
}

//------------------------------------------------------------------------------
TEST(core_array, checkResize)
{
  constexpr IndexType ZERO = 0;

  for(double ratio = 1.0; ratio <= 2.0; ratio += 0.5)
  {
    for(IndexType capacity = 2; capacity <= 512; capacity *= 2)
    {
      Array<int> v_int(ZERO, capacity);
      v_int.setResizeRatio(ratio);
      internal::check_resize(v_int);

      Array<double> v_double(ZERO, capacity);
      v_double.setResizeRatio(ratio);
      internal::check_resize(v_double);
    }
  }
}

//------------------------------------------------------------------------------
TEST(core_array_DeathTest, checkResize)
{
  constexpr IndexType ZERO = 0;
  IndexType size = 100;

  /* Resizing isn't allowed with a ratio less than 1.0. */
  Array<int> v_int(ZERO, size);
  v_int.setResizeRatio(0.99);
  EXPECT_DEATH_IF_SUPPORTED(internal::check_resize(v_int), "");
}

//------------------------------------------------------------------------------
TEST(core_array, checkInsert)
{
  constexpr IndexType ZERO = 0;

  for(double ratio = 1.0; ratio <= 2.0; ratio += 0.5)
  {
    for(IndexType capacity = 2; capacity <= 512; capacity *= 2)
    {
      Array<int> v_int(ZERO, capacity);
      v_int.setResizeRatio(ratio);
      internal::check_insert(v_int);

      Array<double> v_double(ZERO, capacity);
      v_double.setResizeRatio(ratio);
      internal::check_insert(v_double);
    }
  }
}

//------------------------------------------------------------------------------
TEST(core_array, checkInsertIterator)
{
  constexpr IndexType ZERO = 0;

  for(double ratio = 1.0; ratio <= 2.0; ratio += 0.5)
  {
    for(IndexType capacity = 10; capacity <= 512; capacity *= 2)
    {
      Array<int> v_int(ZERO, capacity);
      v_int.setResizeRatio(ratio);
      internal::check_insert_iterator(v_int);

      Array<double> v_double(ZERO, capacity);
      v_double.setResizeRatio(ratio);
      internal::check_insert_iterator(v_double);
    }
  }
}

//------------------------------------------------------------------------------
TEST(core_array, checkEmplace)
{
  constexpr IndexType ZERO = 0;

  for(double ratio = 1.0; ratio <= 2.0; ratio += 0.5)
  {
    for(IndexType capacity = 10; capacity <= 512; capacity *= 2)
    {
      Array<int> v_int(ZERO, capacity);
      v_int.setResizeRatio(ratio);
      internal::check_emplace(v_int);

      Array<double> v_double(ZERO, capacity);
      v_double.setResizeRatio(ratio);
      internal::check_emplace(v_double);
    }
  }
}

//------------------------------------------------------------------------------
TEST(core_array, checkSwap)
{
  for(IndexType size = 10; size <= 512; size *= 2)
  {
    Array<int> v_int(size);
    internal::check_swap(v_int);

    Array<double> v_double(size);
    internal::check_swap(v_double);

    Array<int, 2> v_int_2d(size, size);
    internal::check_swap(v_int_2d);

    Array<double, 2> v_double_2d(size, size);
    internal::check_swap(v_double);
  }
}

//------------------------------------------------------------------------------
TEST(core_array, checkAlloc)
{
  std::vector<int> memory_locations
  {
#if defined(AXOM_USE_UMPIRE)
    axom::getUmpireResourceAllocatorID(umpire::resource::Host)
  #if defined(UMPIRE_ENABLE_DEVICE)
      ,
      axom::getUmpireResourceAllocatorID(umpire::resource::Device)
  #endif
  #if defined(UMPIRE_ENABLE_UM)
        ,
      axom::getUmpireResourceAllocatorID(umpire::resource::Unified)
  #endif
  #if defined(UMPIRE_ENABLE_CONST)
        ,
      axom::getUmpireResourceAllocatorID(umpire::resource::Constant)
  #endif
  #if defined(UMPIRE_ENABLE_PINNED)
        ,
      axom::getUmpireResourceAllocatorID(umpire::resource::Pinned)
  #endif
#endif
  };

  for(double ratio = 1.0; ratio <= 2.0; ratio += 0.5)
  {
    for(IndexType capacity = 4; capacity <= 512; capacity *= 2)
    {
      // First use the dynamic option
      for(int id : memory_locations)
      {
        Array<int, 1, axom::MemorySpace::Dynamic> v_int(capacity, capacity, id);
        internal::check_alloc(v_int, id);

        Array<double, 1, axom::MemorySpace::Dynamic> v_double(capacity,
                                                              capacity,
                                                              id);
        internal::check_alloc(v_double, id);
      }
// Then, if Umpire is available, we can use the space as an explicit template parameter
#ifdef AXOM_USE_UMPIRE
  #ifdef UMPIRE_ENABLE_DEVICE
      Array<int, 1, axom::MemorySpace::Device> v_int_device(capacity, capacity);
      internal::check_alloc(
        v_int_device,
        axom::getUmpireResourceAllocatorID(umpire::resource::Device));
      Array<double, 1, axom::MemorySpace::Device> v_double_device(capacity,
                                                                  capacity);
      internal::check_alloc(
        v_double_device,
        axom::getUmpireResourceAllocatorID(umpire::resource::Device));
  #endif
  #ifdef UMPIRE_ENABLE_UM
      Array<int, 1, axom::MemorySpace::Unified> v_int_unified(capacity, capacity);
      internal::check_alloc(
        v_int_unified,
        axom::getUmpireResourceAllocatorID(umpire::resource::Unified));
      Array<double, 1, axom::MemorySpace::Unified> v_double_unified(capacity,
                                                                    capacity);
      internal::check_alloc(
        v_double_unified,
        axom::getUmpireResourceAllocatorID(umpire::resource::Unified));
  #endif
  #ifdef UMPIRE_ENABLE_CONST
      Array<int, 1, axom::MemorySpace::Constant> v_int_const(capacity, capacity);
      internal::check_alloc(
        v_int_const,
        axom::getUmpireResourceAllocatorID(umpire::resource::Constant));
      Array<double, 1, axom::MemorySpace::Constant> v_double_const(capacity,
                                                                   capacity);
      internal::check_alloc(
        v_double_const,
        axom::getUmpireResourceAllocatorID(umpire::resource::Constant));
  #endif
  #ifdef UMPIRE_ENABLE_PINNED
      Array<int, 1, axom::MemorySpace::Pinned> v_int_pinned(capacity, capacity);
      internal::check_alloc(
        v_int_pinned,
        axom::getUmpireResourceAllocatorID(umpire::resource::Pinned));
      Array<double, 1, axom::MemorySpace::Pinned> v_double_pinned(capacity,
                                                                  capacity);
      internal::check_alloc(
        v_double_pinned,
        axom::getUmpireResourceAllocatorID(umpire::resource::Pinned));
  #endif
#endif
    }
  }
}

//------------------------------------------------------------------------------
TEST(core_array, checkExternalView)
{
  constexpr double MAGIC_NUM = 5683578.8;
  constexpr IndexType MAX_SIZE = 256;
  constexpr IndexType MAX_VALUES = MAX_SIZE;
  union DataBuffer
  {
    int ints[MAX_SIZE];
    double doubles[MAX_SIZE];
  };

  DataBuffer buffer;
  std::fill_n(buffer.doubles, MAX_VALUES, MAGIC_NUM);

  for(IndexType size = 16; size <= MAX_SIZE; size *= 2)
  {
    ArrayView<int> v_int_view(buffer.ints, size);
    EXPECT_EQ(v_int_view.data(), buffer.ints);
    internal::check_external_view(v_int_view);

    ArrayView<double> v_double_view(buffer.doubles, size);
    EXPECT_EQ(v_double_view.data(), buffer.doubles);
    internal::check_external_view(v_double_view);

    /* Set v_double's data to MAGIC_NUM */
    std::fill_n(v_double_view.data(), size, MAGIC_NUM);

    /* Check that the data still exists in the buffer */
    for(IndexType i = 0; i < MAX_VALUES; ++i)
    {
      EXPECT_EQ(buffer.doubles[i], MAGIC_NUM);
    }
  }
}

//------------------------------------------------------------------------------
TEST(core_array, checkIterator)
{
  constexpr int SIZE = 1000;
  axom::Array<int> v_int(SIZE);

  /* Push 0...999 elements */
  for(int i = 0; i < SIZE; i++)
  {
    v_int[i] = i;
  }

  EXPECT_EQ(*v_int.begin(), 0);
  EXPECT_EQ(v_int.front(), 0);
  EXPECT_EQ(*(v_int.end() - 1), SIZE - 1);
  EXPECT_EQ(v_int.back(), SIZE - 1);
  EXPECT_EQ(v_int.size(), SIZE);

  /* Erase nothing */
  axom::Array<int>::ArrayIterator ret1 =
    v_int.erase(v_int.begin() + SIZE / 2, v_int.begin() + SIZE / 2);

  EXPECT_EQ(ret1, v_int.begin() + SIZE / 2);
  EXPECT_EQ(v_int.size(), SIZE);

  /* Erase half the elements */
  axom::Array<int>::ArrayIterator ret2 =
    v_int.erase(v_int.begin(), v_int.begin() + SIZE / 2);

  EXPECT_EQ(ret2, v_int.begin());
  EXPECT_EQ(*v_int.begin(), SIZE / 2);
  EXPECT_EQ(v_int.front(), SIZE / 2);
  EXPECT_EQ(*(v_int.end() - 1), SIZE - 1);
  EXPECT_EQ(v_int.back(), SIZE - 1);
  EXPECT_EQ(v_int.size(), SIZE / 2);

  /* Erase first, last elements */
  axom::Array<int>::ArrayIterator ret3 = v_int.erase(v_int.begin());

  EXPECT_EQ(ret3, v_int.begin());
  EXPECT_EQ(*v_int.begin(), SIZE / 2 + 1);
  EXPECT_EQ(v_int.front(), SIZE / 2 + 1);

  axom::Array<int>::ArrayIterator ret4 = v_int.erase(v_int.end() - 1);

  EXPECT_EQ(ret4, v_int.end());
  EXPECT_EQ(*(v_int.end() - 1), SIZE - 2);
  EXPECT_EQ(v_int.back(), SIZE - 2);

  /* Clear the rest of the array */
  v_int.clear();
  EXPECT_EQ(v_int.size(), 0);
}

//------------------------------------------------------------------------------
#if defined(AXOM_USE_GPU) && defined(AXOM_GPUCC) && defined(AXOM_USE_UMPIRE)
void checkIteratorDeviceImpl()
{
  constexpr int SIZE = 1000;
  axom::Array<int, 1, axom::MemorySpace::Host> v_int_host(SIZE);
  axom::Array<int, 1, axom::MemorySpace::Device> v_int(SIZE);

  /* Push 0...999 elements */
  for(int i = 0; i < SIZE; i++)
  {
    v_int_host[i] = i;
  }
  v_int = v_int_host;

  EXPECT_EQ(*v_int_host.begin(), 0);
  EXPECT_EQ(*(v_int_host.end() - 1), SIZE - 1);
  EXPECT_EQ(v_int.size(), SIZE);

  /* Erase nothing */
  auto ret1 = v_int.erase(v_int.begin() + SIZE / 2, v_int.begin() + SIZE / 2);

  EXPECT_EQ(ret1, v_int.begin() + SIZE / 2);
  EXPECT_EQ(v_int.size(), SIZE);

  /* Erase half the elements */
  auto ret2 = v_int.erase(v_int.begin(), v_int.begin() + SIZE / 2);

  EXPECT_EQ(ret2, v_int.begin());
  EXPECT_EQ(v_int.size(), SIZE / 2);
  v_int_host = v_int;
  EXPECT_EQ(*v_int_host.begin(), SIZE / 2);
  EXPECT_EQ(*(v_int_host.end() - 1), SIZE - 1);

  /* Erase first, last elements */
  auto ret3 = v_int.erase(v_int.begin());

  EXPECT_EQ(ret3, v_int.begin());
  v_int_host = v_int;
  EXPECT_EQ(*v_int_host.begin(), SIZE / 2 + 1);

  auto ret4 = v_int.erase(v_int.end() - 1);

  EXPECT_EQ(ret4, v_int.end());
  v_int_host = v_int;
  EXPECT_EQ(*(v_int_host.end() - 1), SIZE - 2);

  /* Clear the rest of the array */
  v_int.clear();
  EXPECT_EQ(v_int.size(), 0);
}

TEST(core_array, checkIteratorDevice) { checkIteratorDeviceImpl(); }
#endif

//------------------------------------------------------------------------------
TEST(core_array, check_move_copy)
{
  constexpr int MAGIC_INT = 255;
  constexpr double MAGIC_DOUBLE = 5683578.8;

  for(IndexType capacity = 2; capacity < 512; capacity *= 2)
  {
    IndexType size = capacity;

    /* Check copy and move semantics for Array of ints */
    Array<int> v_int(size, capacity);
    v_int.fill(MAGIC_INT);

    Array<int> v_int_copy_ctor(v_int);
    Array<int> v_int_copy_assign;
    v_int_copy_assign = v_int;
    EXPECT_EQ(v_int, v_int_copy_ctor);
    EXPECT_EQ(v_int, v_int_copy_assign);

    Array<int> v_int_move_assign;
    v_int_move_assign = std::move(v_int_copy_assign);
    Array<int> v_int_move_ctor = std::move(v_int_copy_ctor);
    EXPECT_EQ(v_int, v_int_move_assign);
    EXPECT_EQ(v_int, v_int_move_ctor);
    EXPECT_EQ(v_int_copy_assign.data(), nullptr);
    EXPECT_EQ(v_int_copy_ctor.data(), nullptr);

    /* Check copy and move semantics for Array of doubles */
    Array<double> v_double(size, capacity);
    v_double.fill(MAGIC_DOUBLE);

    Array<double> v_double_copy_ctor(v_double);
    Array<double> v_double_copy_assign;
    v_double_copy_assign = v_double;
    EXPECT_EQ(v_double, v_double_copy_ctor);
    EXPECT_EQ(v_double, v_double_copy_assign);

    Array<double> v_double_move_assign;
    v_double_move_assign = std::move(v_double_copy_assign);
    Array<double> v_double_move_ctor = std::move(v_double_copy_ctor);
    EXPECT_EQ(v_double, v_double_move_assign);
    EXPECT_EQ(v_double, v_double_move_ctor);
    EXPECT_EQ(v_double_copy_assign.data(), nullptr);
    EXPECT_EQ(v_double_copy_ctor.data(), nullptr);
  }
}

//------------------------------------------------------------------------------
TEST(core_array, check_move_copy_multidimensional)
{
  constexpr int MAGIC_INT = 255;
  constexpr double MAGIC_DOUBLE = 5683578.8;

  for(IndexType capacity = 2; capacity < 512; capacity *= 2)
  {
    IndexType size = capacity;

    /* Check copy and move semantics for Array of ints */
    Array<int, 2> v_int(size, size);
    v_int.fill(MAGIC_INT);

    Array<int, 2> v_int_copy_ctor(v_int);
    Array<int, 2> v_int_copy_assign;
    v_int_copy_assign = v_int;
    EXPECT_EQ(v_int, v_int_copy_ctor);
    EXPECT_EQ(v_int, v_int_copy_assign);
    // operator== already checks the shape, but we should also check the strides
    EXPECT_EQ(v_int.strides(), v_int_copy_ctor.strides());
    EXPECT_EQ(v_int.strides(), v_int_copy_assign.strides());

    Array<int, 2> v_int_move_assign;
    v_int_move_assign = std::move(v_int_copy_assign);
    Array<int, 2> v_int_move_ctor = std::move(v_int_copy_ctor);
    EXPECT_EQ(v_int, v_int_move_assign);
    EXPECT_EQ(v_int, v_int_move_ctor);
    EXPECT_EQ(v_int.strides(), v_int_move_assign.strides());
    EXPECT_EQ(v_int.strides(), v_int_move_ctor.strides());
    EXPECT_EQ(v_int_copy_assign.data(), nullptr);
    EXPECT_EQ(v_int_copy_ctor.data(), nullptr);

    /* Check copy and move semantics for Array of doubles */
    Array<double, 2> v_double(size, size);
    v_double.fill(MAGIC_DOUBLE);

    Array<double, 2> v_double_copy_ctor(v_double);
    Array<double, 2> v_double_copy_assign;
    v_double_copy_assign = v_double;
    EXPECT_EQ(v_double, v_double_copy_ctor);
    EXPECT_EQ(v_double, v_double_copy_assign);
    // operator== already checks the shape, but we should also check the strides
    EXPECT_EQ(v_double.strides(), v_double_copy_ctor.strides());
    EXPECT_EQ(v_double.strides(), v_double_copy_assign.strides());

    Array<double, 2> v_double_move_assign;
    v_double_move_assign = std::move(v_double_copy_assign);
    Array<double, 2> v_double_move_ctor = std::move(v_double_copy_ctor);
    EXPECT_EQ(v_double, v_double_move_assign);
    EXPECT_EQ(v_double, v_double_move_ctor);
    EXPECT_EQ(v_double.strides(), v_double_move_assign.strides());
    EXPECT_EQ(v_double.strides(), v_double_move_ctor.strides());
    EXPECT_EQ(v_double_copy_assign.data(), nullptr);
    EXPECT_EQ(v_double_copy_ctor.data(), nullptr);
  }
}

//------------------------------------------------------------------------------
TEST(core_array, check_view_move_copy)
{
  constexpr int MAGIC_INT = 255;
  constexpr double MAGIC_DOUBLE = 5683578.8;

  for(IndexType capacity = 2; capacity < 512; capacity *= 2)
  {
    IndexType size = capacity;

    /* Check copy and move semantics for ArrayView of ints */
    std::vector<int> ints(size, MAGIC_INT);
    ArrayView<int> v_int_view(ints.data(), size);

    ArrayView<int> v_int_view_copy_ctor(v_int_view);
    ArrayView<int> v_int_view_copy_assign;
    v_int_view_copy_assign = v_int_view;
    EXPECT_EQ(v_int_view, v_int_view_copy_ctor);
    EXPECT_EQ(v_int_view, v_int_view_copy_assign);

    /* Check copy and move semantics for ArrayView of doubles */
    std::vector<double> doubles(size, MAGIC_DOUBLE);
    ArrayView<double> v_double_view(doubles.data(), size);

    ArrayView<double> v_double_view_copy_ctor(v_double_view);
    ArrayView<double> v_double_view_copy_assign;
    v_double_view_copy_assign = v_double_view;
    EXPECT_EQ(v_double_view, v_double_view_copy_ctor);
    EXPECT_EQ(v_double_view, v_double_view_copy_assign);
  }
}

//------------------------------------------------------------------------------
TEST(core_array, check_multidimensional)
{
  constexpr int MAGIC_INT = 255;
  constexpr double MAGIC_DOUBLE = 5683578.8;

  // First test multidimensional int arrays
  Array<int, 2> v_int;
  v_int.resize(2, 2);
  v_int.fill(MAGIC_INT);
  // Make sure the number of elements and contents are correct
  EXPECT_EQ(v_int.size(), 2 * 2);
  StackArray<IndexType, 2> expected_shape = {2, 2};
  EXPECT_EQ(v_int.shape(), expected_shape);
  for(const auto val : v_int)
  {
    EXPECT_EQ(val, MAGIC_INT);
  }
  // Then assign different values to each element
  v_int(0, 0) = 1;
  v_int(0, 1) = 2;
  v_int(1, 0) = 3;
  v_int(1, 1) = 4;

  // FIXME: Should we add a std::initializer_list ctor?
  Array<int> v_int_flat(4);
  v_int_flat[0] = 1;
  v_int_flat[1] = 2;
  v_int_flat[2] = 3;
  v_int_flat[3] = 4;
  StackArray<IndexType, 1> expected_flat_shape = {4};
  EXPECT_EQ(v_int_flat.shape(), expected_flat_shape);

  for(int i = 0; i < v_int_flat.size(); i++)
  {
    // For a multidim array, flatIndex(i) is a "flat" index into the raw data
    EXPECT_EQ(v_int.flatIndex(i), v_int_flat[i]);
  }

  Array<double, 3> v_double(4, 3, 2);
  v_double.fill(MAGIC_DOUBLE);
  EXPECT_EQ(v_double.size(), 4 * 3 * 2);
  StackArray<IndexType, 3> expected_double_shape = {4, 3, 2};
  EXPECT_EQ(v_double.shape(), expected_double_shape);
  for(const auto val : v_double)
  {
    EXPECT_EQ(val, MAGIC_DOUBLE);
  }

  Array<double> v_double_flat(4 * 3 * 2);
  int double_flat_idx = 0;
  for(int i = 0; i < v_double.shape()[0]; i++)
  {
    for(int j = 0; j < v_double.shape()[1]; j++)
    {
      for(int k = 0; k < v_double.shape()[2]; k++)
      {
        v_double(i, j, k) = (i * i) + (5 * j) + k;
        v_double_flat[double_flat_idx++] = (i * i) + (5 * j) + k;
      }
    }
  }

  for(int i = 0; i < v_double.size(); i++)
  {
    // For a multidim array, flatIndex(i) is a "flat" index into the raw data
    EXPECT_EQ(v_double.flatIndex(i), v_double_flat[i]);
  }

  for(int i = 0; i < v_double.shape()[0]; i++)
  {
    Array<double, 2> v_double_subarray_2d = v_double[i];
    for(int j = 0; j < v_double.shape()[1]; j++)
    {
      Array<double> v_double_subarray_1d = v_double(i, j);
      for(int k = 0; k < v_double.shape()[2]; k++)
      {
        EXPECT_EQ(v_double(i, j, k), v_double[i][j][k]);
        EXPECT_EQ(v_double(i, j, k), v_double_subarray_2d(j, k));
        EXPECT_EQ(v_double(i, j, k), v_double_subarray_1d[k]);
      }
    }
  }
}

//------------------------------------------------------------------------------
TEST(core_array, check_multidimensional_view)
{
  constexpr int MAGIC_INT = 255;
  constexpr double MAGIC_DOUBLE = 5683578.8;

  // First test multidimensional int arrays
  int v_int_arr[] = {MAGIC_INT, MAGIC_INT, MAGIC_INT, MAGIC_INT};
  ArrayView<int, 2> v_int_view(v_int_arr, 2, 2);
  // Make sure the number of elements and contents are correct
  EXPECT_EQ(v_int_view.size(), 2 * 2);
  StackArray<IndexType, 2> expected_shape = {2, 2};
  EXPECT_EQ(v_int_view.shape(), expected_shape);
  for(const auto val : v_int_view)
  {
    EXPECT_EQ(val, MAGIC_INT);
  }
  // Then assign different values to each element
  v_int_view(0, 0) = 1;
  v_int_view(0, 1) = 2;
  v_int_view(1, 0) = 3;
  v_int_view(1, 1) = 4;

  // FIXME: Should we add a std::initializer_list ctor?
  int v_int_flat_arr[] = {1, 2, 3, 4};
  ArrayView<int> v_int_flat_view(v_int_flat_arr, 4);
  StackArray<IndexType, 1> expected_flat_shape = {4};
  EXPECT_EQ(v_int_flat_view.shape(), expected_flat_shape);

  for(int i = 0; i < v_int_flat_view.size(); i++)
  {
    // For a multidim array, flatIndex(i) is a "flat" index into the raw data
    EXPECT_EQ(v_int_view.flatIndex(i), v_int_flat_view[i]);
  }

  double v_double_arr[4 * 3 * 2];
  std::fill_n(v_double_arr, 4 * 3 * 2, MAGIC_DOUBLE);
  ArrayView<double, 3> v_double_view(v_double_arr, 4, 3, 2);
  EXPECT_EQ(v_double_view.size(), 4 * 3 * 2);
  StackArray<IndexType, 3> expected_double_shape = {4, 3, 2};
  EXPECT_EQ(v_double_view.shape(), expected_double_shape);
  for(const auto val : v_double_view)
  {
    EXPECT_EQ(val, MAGIC_DOUBLE);
  }

  double v_double_flat_arr[4 * 3 * 2];
  ArrayView<double> v_double_flat_view(v_double_flat_arr, 4 * 3 * 2);
  int double_flat_idx = 0;
  for(int i = 0; i < v_double_view.shape()[0]; i++)
  {
    for(int j = 0; j < v_double_view.shape()[1]; j++)
    {
      for(int k = 0; k < v_double_view.shape()[2]; k++)
      {
        v_double_view(i, j, k) = (i * i) + (5 * j) + k;
        v_double_flat_view[double_flat_idx++] = (i * i) + (5 * j) + k;
      }
    }
  }

  for(int i = 0; i < v_double_view.size(); i++)
  {
    // For a multidim array, flatIndex(i) is a "flat" index into the raw data
    EXPECT_EQ(v_double_view.flatIndex(i), v_double_flat_view[i]);
  }

  for(int i = 0; i < v_double_view.shape()[0]; i++)
  {
    Array<double, 2> v_double_subarray_2d = v_double_view[i];
    for(int j = 0; j < v_double_view.shape()[1]; j++)
    {
      Array<double> v_double_subarray_1d = v_double_view(i, j);
      for(int k = 0; k < v_double_view.shape()[2]; k++)
      {
        EXPECT_EQ(v_double_view(i, j, k), v_double_view[i][j][k]);
        EXPECT_EQ(v_double_view(i, j, k), v_double_subarray_2d(j, k));
        EXPECT_EQ(v_double_view(i, j, k), v_double_subarray_1d[k]);
      }
    }
  }
}

struct Elem
{
  double earth, wind, fire;
};
//------------------------------------------------------------------------------
TEST(core_array, check_multidimensional_view_spacing)
{
  // NUM_COMPS is the spacing between consecutive values of the
  // same type (earth, wind or fire).
  constexpr int NUM_COMPS = sizeof(Elem) / sizeof(double);
  constexpr int NUM_DIMS = 3;

  // Initialize a multidimensional, multicomponent array.
  StackArray<IndexType, NUM_DIMS> shape {3, 2, 4};
  Array<Elem, 3> mdElemArray(shape[0], shape[1], shape[2]);
  for(int i = 0; i < shape[0]; ++i)
  {
    for(int j = 0; j < shape[1]; ++j)
    {
      for(int k = 0; k < shape[2]; ++k)
      {
        int pvalue = 1000 * i + 100 * j + 10 * k;
        mdElemArray(i, j, k).earth = pvalue + .1;
        mdElemArray(i, j, k).wind = pvalue + .2;
        mdElemArray(i, j, k).fire = pvalue + .3;
      }
    }
  }

  // Verify views of the 3 individual components in mdElemArray.
  IndexType spacing = NUM_COMPS;
  ArrayView<double, 3> earthOnly(&mdElemArray(0, 0, 0).earth, shape, spacing);
  ArrayView<double, 3> windOnly(&mdElemArray(0, 0, 0).wind, shape, spacing);
  ArrayView<double, 3> fireOnly(&mdElemArray(0, 0, 0).fire, shape, spacing);

  for(int i = 0; i < shape[0]; ++i)
  {
    for(int j = 0; j < shape[1]; ++j)
    {
      for(int k = 0; k < shape[2]; ++k)
      {
        int pvalue = 1000 * i + 100 * j + 10 * k;
        EXPECT_EQ(earthOnly(i, j, k), pvalue + .1);
        EXPECT_EQ(windOnly(i, j, k), pvalue + .2);
        EXPECT_EQ(fireOnly(i, j, k), pvalue + .3);

        EXPECT_EQ(&earthOnly(i, j, k), &mdElemArray(i, j, k).earth);
        EXPECT_EQ(&windOnly(i, j, k), &mdElemArray(i, j, k).wind);
        EXPECT_EQ(&fireOnly(i, j, k), &mdElemArray(i, j, k).fire);
      }
    }
  }
}

//------------------------------------------------------------------------------
TEST(core_array, checkDevice)
{
#if !defined(AXOM_USE_GPU) || !defined(AXOM_GPUCC) || \
  !defined(AXOM_USE_UMPIRE) || !defined(UMPIRE_ENABLE_DEVICE)
  GTEST_SKIP() << "CUDA or HIP is not available, skipping tests that use Array "
                  "in device code";
#else
  for(IndexType capacity = 2; capacity < 512; capacity *= 2)
  {
    // Allocate a Dynamic array in Device memory
    Array<int, 1, axom::MemorySpace::Dynamic> v_int_dynamic(
      capacity,
      capacity,
      axom::getUmpireResourceAllocatorID(umpire::resource::Device));

    internal::check_device(v_int_dynamic);

    Array<double, 1, axom::MemorySpace::Dynamic> v_double_dynamic(
      capacity,
      capacity,
      axom::getUmpireResourceAllocatorID(umpire::resource::Device));

    internal::check_device(v_double_dynamic);

    // Then allocate an explicitly Device array
    Array<int, 1, axom::MemorySpace::Device> v_int_device(capacity, capacity);
    internal::check_device(v_int_device);

    Array<double, 1, axom::MemorySpace::Device> v_double_device(capacity,
                                                                capacity);
    internal::check_device(v_double_device);
  }
#endif
}

//------------------------------------------------------------------------------
TEST(core_array, checkDevice2D)
{
#if !defined(AXOM_USE_GPU) || !defined(AXOM_GPUCC) || \
  !defined(AXOM_USE_UMPIRE) || !defined(UMPIRE_ENABLE_DEVICE)
  GTEST_SKIP() << "CUDA or HIP is not available, skipping tests that use Array "
                  "in device code";
#else
  for(IndexType capacity = 2; capacity < 512; capacity *= 2)
  {
    // Allocate an explicitly Device array
    Array<int, 2, axom::MemorySpace::Device> v_int_device(capacity, capacity);
    internal::check_device_2D(v_int_device);

    Array<double, 2, axom::MemorySpace::Device> v_double_device(capacity,
                                                                capacity);
    internal::check_device_2D(v_double_device);
  }
#endif
}

struct HasDefault
{
  int member = 255;

  bool operator==(const HasDefault& other) const
  {
    return (member == other.member);
  }
};

//------------------------------------------------------------------------------
TEST(core_array, checkDefaultInitialization)
{
  constexpr int MAGIC_INT = 255;
  for(IndexType capacity = 2; capacity < 512; capacity *= 2)
  {
    // Make sure the array gets zero-initialized
    Array<int> v_int(capacity);
    for(const auto ele : v_int)
    {
      EXPECT_EQ(ele, 0);
    }

    Array<int, 2> v_int_2d(capacity, capacity);
    for(const auto ele : v_int_2d)
    {
      EXPECT_EQ(ele, 0);
    }

    Array<HasDefault> v_has_default(capacity);
    for(const auto& ele : v_has_default)
    {
      EXPECT_EQ(ele.member, MAGIC_INT);
    }

    Array<HasDefault, 2> v_has_default_2d(capacity, capacity);
    for(const auto& ele : v_has_default_2d)
    {
      EXPECT_EQ(ele.member, MAGIC_INT);
    }
  }
}

//------------------------------------------------------------------------------
TEST(core_array, checkDefaultInitializationDevice)
{
#if !defined(AXOM_USE_GPU) || !defined(AXOM_GPUCC) || \
  !defined(AXOM_USE_UMPIRE) || !defined(UMPIRE_ENABLE_DEVICE)
  GTEST_SKIP() << "CUDA or HIP is not available, skipping tests that use Array "
                  "in device code";
#else
  constexpr int MAGIC_INT = 255;
  for(IndexType capacity = 2; capacity < 512; capacity *= 2)
  {
    // Allocate an explicitly Device array of ints (zero-initialized)
    Array<int, 1, axom::MemorySpace::Device> v_int(capacity);

    // Then copy it to the host
    Array<int, 1, axom::MemorySpace::Host> v_int_host(v_int);

    for(const auto ele : v_int_host)
    {
      EXPECT_EQ(ele, 0);
    }

    // Allocate an explicitly Device array of a default-constructible type
    Array<HasDefault, 1, axom::MemorySpace::Device> v_has_default_device(capacity);

    // Then copy it to the host
    Array<HasDefault, 1, axom::MemorySpace::Host> v_has_default_host(
      v_has_default_device);

    for(const auto& ele : v_has_default_host)
    {
      EXPECT_EQ(ele.member, MAGIC_INT);
    }
  }
#endif
}

//------------------------------------------------------------------------------

/**
 * A struct that fails if the default constructor is called.
 * It is in support of the checkUninitialized test for axom::Array
 */
struct FailsOnConstruction
{
  int member = 255;

  FailsOnConstruction() { EXPECT_TRUE(false); }

  bool operator==(const FailsOnConstruction& other) const
  {
    return member == other.member;
  }
};

TEST(core_array, checkUninitialized)
{
  for(IndexType capacity = 2; capacity < 512; capacity *= 2)
  {
    // Test uninitialized functionality with 1D Array using FailsOnConstruction type
    {
      Array<FailsOnConstruction> arr(ArrayOptions::Uninitialized {}, capacity);

      EXPECT_LE(capacity, arr.capacity());
      EXPECT_EQ(capacity, arr.size());

      arr.resize(ArrayOptions::Uninitialized {}, capacity * 2);

      EXPECT_LE(capacity * 2, arr.capacity());
      EXPECT_EQ(capacity * 2, arr.size());
    }

    // Test default 1D Array with trivially copyable HasDefault type
    // Note: this test will not fail if HasDefault is copied, but will at least
    // check that the code compiles and that we can copy and assign these arrays
    {
      Array<HasDefault> arr(ArrayOptions::Uninitialized {}, capacity);

      EXPECT_LE(capacity, arr.capacity());
      EXPECT_EQ(capacity, arr.size());

      Array<HasDefault> copied(arr);
      EXPECT_LE(arr.capacity(), copied.capacity());
      EXPECT_EQ(arr.size(), copied.size());
      EXPECT_EQ(arr, copied);

      Array<HasDefault> assigned;
      assigned = arr;
      EXPECT_LE(arr.capacity(), assigned.capacity());
      EXPECT_EQ(arr.size(), assigned.size());
      EXPECT_EQ(arr, assigned);
    }

    // Tests uninitialized with 2D Array
    {
      Array<FailsOnConstruction, 2> arr(ArrayOptions::Uninitialized {},
                                        capacity,
                                        capacity);

      EXPECT_LE(capacity * capacity, arr.capacity());
      EXPECT_EQ(capacity * capacity, arr.size());

      arr.resize(ArrayOptions::Uninitialized {}, capacity * 2, capacity * 2);

      EXPECT_LE(capacity * capacity * 4, arr.capacity());
      EXPECT_EQ(capacity * capacity * 4, arr.size());
    }

    // Tests uninitialized with 1D Array with user-supplied allocator
    {
      Array<FailsOnConstruction> arr(ArrayOptions::Uninitialized {},
                                     capacity,
                                     capacity,
                                     axom::getDefaultAllocatorID());

      EXPECT_EQ(capacity, arr.capacity());
      EXPECT_EQ(capacity, arr.size());
      EXPECT_EQ(axom::getDefaultAllocatorID(), arr.getAllocatorID());

      arr.resize(ArrayOptions::Uninitialized {}, capacity * 2);

      EXPECT_LE(capacity * 2, arr.capacity());
      EXPECT_EQ(capacity * 2, arr.size());
      EXPECT_EQ(axom::getDefaultAllocatorID(), arr.getAllocatorID());
    }
  }
}

//------------------------------------------------------------------------------
TEST(core_array, checkConstConversion)
{
  constexpr IndexType size = 10;
  Array<int> v_int(size);
  for(int i = 0; i < v_int.size(); i++)
  {
    v_int[i] = i;
  }
  const Array<int>& v_int_cref = v_int;
  ArrayView<const int> v_int_view = v_int_cref;
  EXPECT_EQ(v_int, v_int_view);
  ArrayView<const int> v_int_view_copy = v_int_view;
  EXPECT_EQ(v_int, v_int_view_copy);
  // ArrayView<int> v_int_view_copy_non_const = v_int_view; // Fails to compile as it should
  // v_int_view[1] = 12; // Fails to compile as it should

  // Check begin() const and end() const
  int idx = 0;
  for(const auto& ele : v_int_cref)
  {
    EXPECT_EQ(ele, idx++);
  }

  Array<double, 2> v_double_2d(size, size);
  for(int i = 0; i < size; i++)
  {
    for(int j = 0; j < size; j++)
    {
      v_double_2d(i, j) = 7.1 * i + j * 2.3;
    }
  }
  const Array<double, 2>& v_double_2d_cref = v_double_2d;
  ArrayView<const double, 2> v_double_2d_view = v_double_2d_cref;
  EXPECT_EQ(v_double_2d, v_double_2d_view);
  ArrayView<const double, 2> v_double_2d_view_copy = v_double_2d_view;
  EXPECT_EQ(v_double_2d, v_double_2d_view_copy);
  // v_double_2d_view_copy(0,0) = 1.1; // Fails to compile as it should
}

//------------------------------------------------------------------------------

TEST(core_array, checkVariadicCtors)
{
  int i = 5;
  size_t s = 6;

  Array<int, 2> arr1(i, i);
  Array<int, 2> arr2(i, s);
  Array<int, 2> arr3(s, i);
  Array<int, 2> arr4(s, s);

  Array<int, 3> arr5(i, i, i);
  Array<int, 3> arr6(i, i, s);
  Array<int, 3> arr7(i, s, i);
  Array<int, 3> arr8(i, s, s);
  Array<int, 3> arr9(s, i, i);
  Array<int, 3> arr10(s, i, s);
  Array<int, 3> arr11(s, s, i);
  Array<int, 3> arr12(s, s, s);
}

//------------------------------------------------------------------------------

TEST(core_array, check_subspan_range)
{
  int m = 10;
  int n = 3;
  int l = 5;
  Array<int> arr(m);

  ArrayView<int> arrv1(arr);
  EXPECT_EQ(arrv1.size(), arr.size());

  ArrayView<int> arrv2 = arrv1.subspan(n);
  EXPECT_EQ(arrv2.size() + n, arrv1.size());
  EXPECT_EQ(&arrv2[0], &arr[n]);
  EXPECT_EQ(&arrv2[arrv2.size() - 1], &arr[arr.size() - 1]);

  ArrayView<int> arrv3 = arrv1.subspan(n, l);
  EXPECT_EQ(arrv3.size(), l);
  EXPECT_EQ(&arrv3[0], &arr[n]);
  EXPECT_EQ(&arrv3[arrv3.size() - 1], &arr[n + l - 1]);
}

//------------------------------------------------------------------------------

template <typename DataType>
void test_resize_with_stackarray(DataType value)
{
  const int I_DIMS = 3;
  const int J_DIMS = 5;
  const int K_DIMS = 7;
  Array<DataType, 2> arr2;

  StackArray<axom::IndexType, 2> dims2 = {I_DIMS, J_DIMS};
  arr2.resize(dims2, value);
  EXPECT_EQ(arr2.size(), I_DIMS * J_DIMS);
  EXPECT_EQ(arr2.shape()[0], I_DIMS);
  EXPECT_EQ(arr2.shape()[1], J_DIMS);
  for(int i = 0; i < I_DIMS; i++)
  {
    for(int j = 0; j < J_DIMS; j++)
    {
      EXPECT_EQ(arr2[i][j], value);
    }
  }

  Array<DataType, 3> arr3;

  StackArray<axom::IndexType, 3> dims3 = {I_DIMS, J_DIMS, K_DIMS};
  arr3.resize(dims3, value);
  EXPECT_EQ(arr3.size(), I_DIMS * J_DIMS * K_DIMS);
  EXPECT_EQ(arr3.shape()[0], I_DIMS);
  EXPECT_EQ(arr3.shape()[1], J_DIMS);
  EXPECT_EQ(arr3.shape()[2], K_DIMS);
  for(int i = 0; i < I_DIMS; i++)
  {
    for(int j = 0; j < J_DIMS; j++)
    {
      for(int k = 0; k < K_DIMS; k++)
      {
        EXPECT_EQ(arr3[i][j][k], value);
      }
    }
  }
}

TEST(core_array, resize_stackarray)
{
  test_resize_with_stackarray<bool>(false);
  test_resize_with_stackarray<int>(-1);
}

} /* end namespace axom */
