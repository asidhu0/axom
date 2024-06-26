// Copyright (c) 2017-2024, Lawrence Livermore National Security, LLC and
// other Axom Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

#ifndef AXOM_MIR_STRUCTURED_INDEXING_HPP_
#define AXOM_MIR_STRUCTURED_INDEXING_HPP_

#include "axom/core/StackArray.hpp"
#include "axom/core/ArrayView.hpp"
#include "axom/primal/geometry/Point.hpp"

namespace axom
{
namespace mir
{
namespace views
{

/**
 * \brief This class encapsulates a structured mesh size and contains methods to
 *        help with indexing into it.
 *
 * \tparam NDIMS The number of dimensions.
 */
template <typename IndexT, int NDIMS = 3>
struct StructuredIndexing
{
  using IndexType = IndexT;
  using LogicalIndex = axom::StackArray<axom::IndexType, NDIMS>;

  AXOM_HOST_DEVICE constexpr static int dimensions() { return NDIMS; }

  /**
   * \brief constructor
   *
   * \param dims The dimensions we're indexing.
   */
  AXOM_HOST_DEVICE
  StructuredIndexing() : m_dimensions()
  {
  }

  AXOM_HOST_DEVICE
  StructuredIndexing(const LogicalIndex &dims) : m_dimensions(dims)
  {
  }

  /**
   * \brief Return the number of points in the coordset.
   *
   * \return The number of points in the coordset.
   */
  AXOM_HOST_DEVICE
  IndexType size() const
  {
     IndexType sz = 1;
     for(int i = 0; i < NDIMS; i++)
       sz *= m_dimensions[i];
     return sz;
  }

  /**
   * \brief Return the j stride.
   *
   * \return The j stride to move up a row.
   */
  template <size_t _ndims = NDIMS>
  AXOM_HOST_DEVICE
  typename std::enable_if<_ndims >= 2, IndexType>::type
  jStride() const
  {
    return m_dimensions[0];
  }

  /**
   * \brief Return the k stride.
   *
   * \return The k stride to move forward a "page".
   */
  template <size_t _ndims = NDIMS>
  AXOM_HOST_DEVICE
  typename std::enable_if<_ndims == 3, IndexType>::type
  kStride() const
  {
    return m_dimensions[0] * m_dimensions[1];
  }

  /**
   * \brief Turn an index into a logical index.
   *
   * \param index The index to convert.
   *
   * \return The logical index that corresponds to the \a index.
   */
  /// @{

  template <size_t _ndims = NDIMS>
  AXOM_HOST_DEVICE
  typename std::enable_if<_ndims == 1, LogicalIndex>::type
  IndexToLogicalIndex(IndexType index) const
  {
    LogicalIndex logical;
    logical[0] = index;
    return logical;
  }

  template <size_t _ndims = NDIMS>
  AXOM_HOST_DEVICE
  typename std::enable_if<_ndims == 2, LogicalIndex>::type
  IndexToLogicalIndex(IndexType index) const
  {
    LogicalIndex logical;
    const auto nx = m_dimensions[0];
    logical[0] = index % nx;
    logical[1] = index / nx;
    return logical;
  }

  template <size_t _ndims = NDIMS>
  AXOM_HOST_DEVICE
  typename std::enable_if<_ndims == 3, LogicalIndex>::type
  IndexToLogicalIndex(IndexType index) const
  {
    LogicalIndex logical;
    const auto nx = m_dimensions[0];
    const auto nxy = nx * m_dimensions[1];
    logical[0] = index % nx;
    logical[1] = (index % nxy) / nx;
    logical[2] = index / nxy;
    return logical;
  }

  /// @}

  /**
   * \brief Turn a logical index into a flat index.
   *
   * \param logical The logical indexto convert to a flat index.
   *
   * \return The index that corresponds to the \a logical index.
   */
  /// @{
  template <size_t _ndims = NDIMS>
  AXOM_HOST_DEVICE
  typename std::enable_if<_ndims == 1, IndexType>::type
  LogicalIndexToIndex(const LogicalIndex &logical) const
  {
    return logical[0];
  }

  template <size_t _ndims = NDIMS>
  AXOM_HOST_DEVICE
  typename std::enable_if<_ndims == 2, IndexType>::type
  LogicalIndexToIndex(const LogicalIndex &logical) const
  {
    return logical[1] * m_dimensions[0] + logical[0];
  }

  template <size_t _ndims = NDIMS>
  AXOM_HOST_DEVICE
  typename std::enable_if<_ndims == 3, IndexType>::type
  LogicalIndexToIndex(const LogicalIndex &logical) const
  {
    return (logical[2] * m_dimensions[1] * m_dimensions[0]) + (logical[1] * m_dimensions[0]) + logical[0];
  }

  /// @}

  LogicalIndex m_dimensions{1};
};

} // end namespace views
} // end namespace mir
} // end namespace axom

#endif
