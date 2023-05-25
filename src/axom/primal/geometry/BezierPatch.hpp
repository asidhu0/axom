// Copyright (c) 2017-2023, Lawrence Livermore National Security, LLC and
// other Axom Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

/*!
 * \file BezierPatch.hpp
 *
 * \brief A BezierPatch primitive
 */

#ifndef AXOM_PRIMAL_BEZIERPATCH_HPP_
#define AXOM_PRIMAL_BEZIERPATCH_HPP_

#include "axom/core.hpp"
#include "axom/slic.hpp"

#include "axom/primal/geometry/NumericArray.hpp"
#include "axom/primal/geometry/Point.hpp"
#include "axom/primal/geometry/Vector.hpp"
#include "axom/primal/geometry/Segment.hpp"
#include "axom/primal/geometry/BezierCurve.hpp"
#include "axom/primal/geometry/BoundingBox.hpp"
#include "axom/primal/geometry/OrientedBoundingBox.hpp"

#include "axom/primal/operators/squared_distance.hpp"

#include <vector>
#include <ostream>

namespace axom
{
namespace primal
{
// Forward declare the templated classes and operator functions
template <typename T, int NDIMS>
class BezierPatch;

/*! \brief Overloaded output operator for Bezier Patches*/
template <typename T, int NDIMS>
std::ostream& operator<<(std::ostream& os, const BezierPatch<T, NDIMS>& bCurve);

/*!
 * \class BezierPatch
 *
 * \brief Represents a 3D Bezier patch defined by a 2D array of control points
 * \tparam T the coordinate type, e.g., double, float, etc.
 * \tparam NDIMS the number of dimensions
 *
 * The order of a Bezier patch with (N+1)(M+1) control points is (N, M).
 * The patch is approximated by the control points,
 * parametrized from t=0 to t=1 and s=0 to s=1.
 * 
 * Contains a 2D array of positive weights to represent a rational Bezier patch.
 * Nonrational Bezier patches are identified by an empty weights array.
 * Algorithms for Rational Bezier curves derived from 
 * < Not yet known. Probably the same Farin book? >
 */
template <typename T, int NDIMS = 3>
class BezierPatch
{
public:
  using PointType = Point<T, NDIMS>;
  using VectorType = Vector<T, NDIMS>;
  using NumArrayType = NumericArray<T, NDIMS>;
  using PlaneType = Plane<T, NDIMS>;
  using CoordsVec = axom::Array<PointType>;
  using CoordsMat = axom::Array<PointType, 2>;
  using BoundingBoxType = BoundingBox<T, NDIMS>;
  using OrientedBoundingBoxType = OrientedBoundingBox<T, NDIMS>;
  using BezierCurveType = primal::BezierCurve<T, NDIMS>;

  // TODO: Unsure if this behavior is captured by the templating statement above
  AXOM_STATIC_ASSERT_MSG(NDIMS == 3,
                         "A Bezier Patch object must be defined in 3-D");
  AXOM_STATIC_ASSERT_MSG(
    std::is_arithmetic<T>::value,
    "A Bezier Curve must be defined using an arithmetic type");

public:
  /*!
   * \brief Constructor for a Bezier Patch that reserves space for
   *  the given order of the surface
   *
   * \param [in] order the order of the resulting Bezier curve
   * \pre order is greater than or equal to -1.
   */
  BezierPatch(int ord_u = -1, int ord_v = -1)
  {
    SLIC_ASSERT(ord_u >= -1 && ord_v >= -1);

    const int sz_u = utilities::max(0, ord_u + 1);
    const int sz_v = utilities::max(0, ord_v + 1);

    m_controlPoints.resize(sz_u, sz_v);

    makeNonrational();
  }

  /*!
   * \brief Constructor for a Bezier Patch from a 2D array of coordinates
   *
   * \param [in] pts an array with ord_u+1 arrays with ord_m+1 control points
   * \param [in] ord_u The patches's polynomial order on the first axis
   * \param [in] ord_v The patches's polynomial order on the second axis
   * \pre order in both directions is greater than or equal to zero
   *
   */
  BezierPatch(PointType* pts, int ord_u, int ord_v)
  {
    SLIC_ASSERT(pts != nullptr);
    SLIC_ASSERT(ord_u >= 0 && ord_v >= 0);

    const int sz_u = utilities::max(0, ord_u + 1);
    const int sz_v = utilities::max(0, ord_v + 1);

    m_controlPoints.resize(sz_u, sz_v);

    for(int t = 0; t < sz_u * sz_v; ++t)
      m_controlPoints(t / sz_v, t % sz_v) = pts[t];

    makeNonrational();
  }

  /*!
   * \brief Constructor for a Rational Bezier Path from an array of coordinates and weights
   *
   * \param [in] pts a matrix with (ord_u+1) * (ord_v+1) control points
   * \param [in] weights a matrix with (ord_u+1) * (ord_v+1) positive weights
   * \param [in] ord_u The patches's polynomial order on the first axis
   * \param [in] ord_v The patches's polynomial order on the second axis
   * \pre order is greater than or equal to zero in each direction
   *
   */
  BezierPatch(PointType* pts, T* weights, int ord_u, int ord_v)
  {
    SLIC_ASSERT(pts != nullptr);
    SLIC_ASSERT(ord_u >= 0 && ord_v >= 0);

    const int sz_u = utilities::max(0, ord_u + 1);
    const int sz_v = utilities::max(0, ord_v + 1);

    m_controlPoints.resize(sz_u, sz_v);

    for(int t = 0; t < sz_u * sz_v; ++t)
      m_controlPoints(t / sz_v, t % sz_v) = pts[t];

    if(weights == nullptr)
    {
      m_weights.resize(0, 0);
    }
    else
    {
      m_weights.resize(sz_u, sz_v);

      for(int t = 0; t < sz_u * sz_v; ++t)
        m_weights(t / sz_v, t % sz_v) = weights[t];

      SLIC_ASSERT(isValidRational());
    }
  }

  /*!
   * \brief Constructor for a Bezier Patch from an matrix of coordinates
   *
   * \param [in] pts a vector with (ord_u+1) * (ord_v+1) control points
   * \param [in] ord_u The patch's polynomial order on the first axis
   * \param [in] ord_v The patch's polynomial order on the second axis
   * \pre order is greater than or equal to zero in each direction
   *
   */
  BezierPatch(const CoordsMat& pts, int ord_u, int ord_v)
  {
    SLIC_ASSERT((ord_u >= 0) && (ord_v >= 0));

    const int sz_u = utilities::max(0, ord_u + 1);
    const int sz_v = utilities::max(0, ord_v + 1);

    m_controlPoints.resize(sz_u, sz_v);
    m_controlPoints = pts;

    makeNonrational();
  }

  /*!
   * \brief Constructor for a Rational Bezier Patch from a matrix
   * of coordinates and weights
   *
   * \param [in] pts a vector with (ord_u+1) * (ord_v+1) control points
   * \param [in] ord_u The patch's polynomial order on the first axis
   * \param [in] ord_v The patch's polynomial order on the second axis
   * \pre order is greater than or equal to zero in each direction
   */
  BezierPatch(const CoordsMat& pts,
              const axom::Array<axom::Array<T>>& weights,
              int ord_u,
              int ord_v)
  {
    SLIC_ASSERT(ord_u >= 0 && ord_v >= 0);
    SLIC_ASSERT(pts.shape()[0] == weights.shape()[0]);
    SLIC_ASSERT(pts.shape()[1] == weights.shape()[1]);

    const int sz_u = utilities::max(0, ord_u + 1);
    const int sz_v = utilities::max(0, ord_v + 1);

    m_controlPoints.resize(sz_u, sz_v);
    m_controlPoints = pts;

    m_weights.resize(sz_u, sz_v);
    m_weights = weights;

    SLIC_ASSERT(isValidRational());
  }

  /// Sets the order of the Bezier Curve
  void setOrder(int ord_u, int ord_v)
  {
    m_controlPoints.resize(ord_u + 1, ord_v + 1);
  }

  /// Returns the order of the Bezier Curve on the first axis
  int getOrder_u() const
  {
    return static_cast<int>(m_controlPoints.shape()[0]) - 1;
  }

  /// Returns the order of the Bezier Curve
  int getOrder_v() const
  {
    return static_cast<int>(m_controlPoints.shape()[1]) - 1;
  }

  /// Make trivially rational. If already rational, do nothing
  void makeRational()
  {
    if(!isRational())
    {
      const int ord_u = getOrder_u();
      const int ord_v = getOrder_v();

      m_weights.resize(ord_u + 1, ord_v + 1);

      for(int i = 0; i <= ord_u; ++i)
        for(int j = 0; j <= ord_v; ++j) m_weights[i][j] = 1.0;
    }
  }

  /// Make nonrational by shrinking array of weights
  void makeNonrational() { m_weights.resize(0, 0); }

  /// Use array size as flag for rationality
  bool isRational() const { return (m_weights.size() != 0); }

  /// Clears the list of control points, make nonrational
  void clear()
  {
    const int ord_u = getOrder_u();
    const int ord_v = getOrder_v();

    for(int p = 0; p <= ord_u; ++p)
      for(int q = 0; q <= ord_v; ++q) m_controlPoints(p, q) = PointType();

    makeNonrational();
  }

  /// Retrieves the control point at index \a (idx_p, idx_q)
  PointType& operator()(int ui, int vi) { return m_controlPoints(ui, vi); }

  /// Retrieves the vector of control points at index \a idx
  const PointType& operator()(int ui, int vi) const
  {
    return m_controlPoints(ui, vi);
  }

  /*!
   * \brief Get a specific weight
   *
   * \param [in] ui The index of the weight on the first axis
   * \param [in] vi The index of the weight on the second axis
   * \pre Requires that the surface be rational
   */
  const T& getWeight(int ui, int vi) const
  {
    SLIC_ASSERT(isRational());
    return m_weights(ui, vi);
  }

  /*!
   * \brief Set the weight at a specific index
   *
   * \param [in] ui The index of the weight in on the first axis
   * \param [in] vi The index of the weight in on the second axis
   * \param [in] weight The updated value of the weight
   * \pre Requires that the surface be rational
   * \pre Requires that the weight be positive
   */
  void setWeight(int ui, int vi, T weight)
  {
    SLIC_ASSERT(isRational());
    SLIC_ASSERT(weight > 0);

    m_weights(ui, vi) = weight;
  };

  /// Checks equality of two Bezier Patches
  friend inline bool operator==(const BezierPatch<T, NDIMS>& lhs,
                                const BezierPatch<T, NDIMS>& rhs)
  {
    return (lhs.m_controlPoints == rhs.m_controlPoints) &&
      (lhs.m_weights == rhs.m_weights);
  }

  friend inline bool operator!=(const BezierPatch<T, NDIMS>& lhs,
                                const BezierPatch<T, NDIMS>& rhs)
  {
    return !(lhs == rhs);
  }

  /// Returns a copy of the Bezier curve's control points
  CoordsMat getControlPoints() const { return m_controlPoints; }

  /// Returns a copy of the Bezier curve's weights
  axom::Array<T, 2> getWeights() const { return m_weights; }

  /*!
   * \brief Reverses the order of one direction of the Bezier patch's control points and weights
   *
   * \param [in] axis orientation of curve. 0 to reverse in u, 1 for reverse in v
   */
  void reverseOrientation(int axis)
  {
    const int ord_u = getOrder_u();
    const int mid_u = (ord_u + 1) / 2;

    const int ord_v = getOrder_v();
    const int mid_v = (ord_v + 1) / 2;

    if(axis == 0)
    {
      for(int q = 0; q <= ord_v; ++q)
      {
        for(int i = 0; i < mid_u; ++i)
          axom::utilities::swap(m_controlPoints(i, q),
                                m_controlPoints(ord_u - i, q));

        if(isRational())
          for(int i = 0; i < mid_u; ++i)
            axom::utilities::swap(m_weights(i, q), m_weights(ord_u - i, q));
      }
    }
    else
    {
      for(int p = 0; p <= ord_u; ++p)
      {
        for(int i = 0; i < mid_v; ++i)
          axom::utilities::swap(m_controlPoints(p, i),
                                m_controlPoints(p, ord_v - i));

        if(isRational())
          for(int i = 0; i < mid_v; ++i)
            axom::utilities::swap(m_weights(p, i), m_weights(p, ord_v - i));
      }
    }
  }

  /// Swap the axes such that s(u, v) becomes s(v, u)
  void swapAxes()
  {
    const int ord_u = getOrder_u();
    const int ord_v = getOrder_v();

    CoordsMat new_controlPoints(ord_v + 1, ord_u + 1);

    for(int p = 0; p <= ord_u; ++p)
      for(int q = 0; q <= ord_v; ++q)
        new_controlPoints(q, p) = m_controlPoints(p, q);

    m_controlPoints = new_controlPoints;

    if(isRational())
    {
      axom::Array<T, 2> new_weights(ord_v + 1, ord_u + 1);
      for(int p = 0; p <= ord_u; ++p)
        for(int q = 0; q <= ord_v; ++q) new_weights(q, p) = m_weights(p, q);

      m_weights = new_weights;
    }
  }

  /// Returns an axis-aligned bounding box containing the Bezier curve
  BoundingBoxType boundingBox() const
  {
    return BoundingBoxType(m_controlPoints.data(),
                           static_cast<int>(m_controlPoints.size()));
  }

  /// Returns an oriented bounding box containing the Bezier curve
  OrientedBoundingBoxType orientedBoundingBox() const
  {
    return OrientedBoundingBoxType(m_controlPoints.data(),
                                   static_cast<int>(m_controlPoints.size()));
  }

  /*!
   * \brief Evaluates a slice Bezier patch for a fixed parameter value of \a u or \a v
   *
   * \param [in] u parameter value at which to evaluate the first axis
   * \param [in] v parameter value at which to evaluate the second axis
   * \param [in] axis orientation of curve. 0 for fixed u, 1 for fixed v
   * \return p the value of the Bezier patch at (u, v)
   *
   * \note We typically evaluate the curve at \a u or \a v between 0 and 1
   */
  BezierCurveType isocurve(T uv, int axis = 0) const
  {
    SLIC_ASSERT((axis == 0) || (axis == 1));

    using axom::utilities::lerp;

    const int ord_u = getOrder_u();
    const int ord_v = getOrder_v();

    BezierCurveType c(-1);

    if(isRational())
    {
      if(axis == 0)  // Keeping a fixed value of t
      {
        c.setOrder(ord_v);
        c.makeRational();

        axom::Array<T> dCarray(ord_u + 1);
        axom::Array<T> dWarray(ord_u + 1);

        // Run de Casteljau algorithm on each row of control nodes and each dimension
        for(int q = 0; q <= ord_v; ++q)
          for(int i = 0; i < NDIMS; ++i)
          {
            for(int p = 0; p <= ord_u; ++p)
            {
              dCarray[p] = m_controlPoints(p, q)[i] * m_weights(p, q);
              dWarray[p] = m_weights(p, q);
            }

            for(int p = 1; p <= ord_u; ++p)
            {
              const int end = ord_u - p;
              for(int k = 0; k <= end; ++k)
              {
                dCarray[k] = lerp(dCarray[k], dCarray[k + 1], uv);
                dWarray[k] = lerp(dWarray[k], dWarray[k + 1], uv);
              }
            }

            c[q][i] = dCarray[0] / dWarray[0];
            c.setWeight(q, dWarray[0]);
          }
      }
      // Run de Casteljau algorithm on each column of control nodes and each dimension
      else  // Keeping a fixed value of s
      {
        c.setOrder(ord_u);
        c.makeRational();

        axom::Array<T> dCarray(ord_v + 1);
        axom::Array<T> dWarray(ord_v + 1);

        for(int p = 0; p <= ord_u; ++p)
          for(int i = 0; i < NDIMS; ++i)
          {
            for(int q = 0; q <= ord_v; ++q)
            {
              dCarray[q] = m_controlPoints(p, q)[i] * m_weights(p, q);
              dWarray[q] = m_weights(p, q);
            }

            for(int q = 1; q <= ord_v; ++q)
            {
              const int end = ord_v - q;
              for(int k = 0; k <= end; ++k)
              {
                dCarray[k] = lerp(dCarray[k], dCarray[k + 1], uv);
                dWarray[k] = lerp(dWarray[k], dWarray[k + 1], uv);
              }
            }
            c[p][i] = dCarray[0] / dWarray[0];
            c.setWeight(p, dWarray[0]);
          }
      }
    }
    else
    {
      if(axis == 0)  // Keeping a fixed value of t
      {
        c.setOrder(ord_v);
        axom::Array<T> dCarray(ord_u + 1);  // Temp array

        // Run de Casteljau algorithm on each row of control nodes and each dimension
        for(int q = 0; q <= ord_v; ++q)
          for(int i = 0; i < NDIMS; ++i)
          {
            for(int p = 0; p <= ord_u; ++p)
              dCarray[p] = m_controlPoints(p, q)[i];

            for(int p = 1; p <= ord_u; ++p)
            {
              const int end = ord_u - p;
              for(int k = 0; k <= end; ++k)
                dCarray[k] = lerp(dCarray[k], dCarray[k + 1], uv);
            }
            c[q][i] = dCarray[0];
          }
      }
      // Run de Casteljau algorithm on each column of control nodes and each dimension
      else  // Keeping a fixed value of s
      {
        c.setOrder(ord_u);
        axom::Array<T> dCarray(ord_v + 1);  // Temp array

        for(int p = 0; p <= ord_u; ++p)
          for(int i = 0; i < NDIMS; ++i)
          {
            for(int q = 0; q <= ord_v; ++q)
              dCarray[q] = m_controlPoints(p, q)[i];

            for(int q = 1; q <= ord_v; ++q)
            {
              const int end = ord_v - q;
              for(int k = 0; k <= end; ++k)
              {
                dCarray[k] = lerp(dCarray[k], dCarray[k + 1], uv);
              }
            }
            c[p][i] = dCarray[0];
          }
      }
    }

    return c;
  }

  /*!
   * \brief Evaluates a Bezier patch at a particular parameter value (\a u, \a v)
   *
   * \param [in] u parameter value at which to evaluate on the first axis
   * \param [in] v parameter value at which to evaluate on the second axis
   * \return p the value of the Bezier patch at (u, v)
   *
   * \note We typically evaluate the curve at \a u and \a v between 0 and 1
   */
  PointType evaluate(T u, T v) const
  {
    if(getOrder_u() >= getOrder_v())
      return isocurve(u, 0).evaluate(v);
    else
      return isocurve(v, 1).evaluate(u);
  }

  /*!
   * \brief Computes a tangent of a Bezier patch at a particular parameter value (\a u, \a v) along \a axis
   *
   * \param [in] u parameter value at which to evaluate on the first axis
   * \param [in] v parameter value at which to evaluate on the second axis
   * \param [in] axis orientation of vector. 0 for fixed u, 1 for fixed v
   * \return v a tangent vector of the Bezier curve at (u, v)
   *
   * \note We typically find the tangent of the curve at \a u and \a v between 0 and 1
   */
  VectorType dt(T u, T v, int axis) const
  {
    SLIC_ASSERT((axis == 0) || (axis == 1));
    if(axis == 0)  // Get isocurve at fixed v
      return isocurve(v, 1).dt(u);
    else  // Get isocurve at fixed u
      return isocurve(u, 0).dt(v);
  }

  /*!
   * \brief Computes the normal vector of a Bezier patch at a particular parameter value (\a u, \a v)
   *
   * \param [in] u parameter value at which to evaluate on the first axis
   * \param [in] v parameter value at which to evaluate on the second axis
   * \return vec the normal vector of the Bezier curve at (u, v)
   *
   * \note We typically find the normal of the curve at \a u and \a v between 0 and 1
   */
  VectorType normal(T u, T v) const
  {
    VectorType tangent_t = dt(u, v, 0);
    VectorType tangent_s = dt(u, v, 1);
    return VectorType::cross_product(tangent_t, tangent_s);
  }

  /*!
   * \brief Splits a Bezier patch into two Bezier patches
   *
   * \param [in] uv parameter value between 0 and 1 at which to bisect the patch
   * \param [in] axis orientation of split. 0 for fixed u, 1 for fixed v
   * \param [out] p1 First output Bezier patch
   * \param [out] p2 Second output Bezier patch
   *
   * \pre Parameter \a uv must be between 0 and 1
   */
  void split(T uv, int axis, BezierPatch& p1, BezierPatch& p2) const
  {
    SLIC_ASSERT((axis == 0) || (axis == 1));

    using axom::utilities::lerp;

    const int ord_u = getOrder_u();
    const int ord_v = getOrder_v();

    SLIC_ASSERT((ord_u >= 0) && (ord_v >= 0));

    // Note: The second patch's control points are computed inline
    //       as we find the first patch's control points
    p2 = *this;

    p1.setOrder(ord_u, ord_v);
    if(isRational())
    {
      p1.makeRational();  // p2 already rational
      if(axis == 0)       // Split across a fixed value of t
      {
        // Run algorithm across each row of control nodes
        for(int q = 0; q <= ord_v; ++q)
        {
          // Do the rational de Casteljau algorithm
          p1(0, q) = p2(0, q);
          p1.setWeight(0, q, p2.getWeight(0, q));

          for(int p = 1; p <= ord_u; ++p)
          {
            const int end = ord_u - p;

            for(int k = 0; k <= end; ++k)
            {
              double temp_weight =
                lerp(p2.getWeight(k, q), p2.getWeight(k + 1, q), uv);

              for(int i = 0; i < NDIMS; ++i)
              {
                p2(k, q)[i] = lerp(p2.getWeight(k, q) * p2(k, q)[i],
                                   p2.getWeight(k + 1, q) * p2(k + 1, q)[i],
                                   uv) /
                  temp_weight;
              }

              p2.setWeight(k, q, temp_weight);
            }

            p1(p, q) = p2(0, q);
            p1.setWeight(p, q, p2.getWeight(0, q));
          }
        }
      }
      else
      {
        // Run algorithm across each column of control nodes
        for(int p = 0; p <= ord_u; ++p)
        {
          // Do the rational de Casteljau algorithm
          p1(p, 0) = p2(p, 0);
          p1.setWeight(p, 0, p2.getWeight(p, 0));

          for(int q = 1; q <= ord_v; ++q)
          {
            const int end = ord_v - q;

            for(int k = 0; k <= end; ++k)
            {
              double temp_weight =
                lerp(p2.getWeight(p, k), p2.getWeight(p, k + 1), uv);

              for(int i = 0; i < NDIMS; ++i)
              {
                p2(p, k)[i] = lerp(p2.getWeight(p, k) * p2(p, k)[i],
                                   p2.getWeight(p, k + 1) * p2(p, k + 1)[i],
                                   uv) /
                  temp_weight;
              }

              p2.setWeight(p, k, temp_weight);
            }

            p1(p, q) = p2(p, 0);
            p1.setWeight(p, q, p2.getWeight(p, 0));
          }
        }
      }
    }
    else
    {
      if(axis == 0)  // Split across a fixed value of t
      {
        // Do the split for each row of control nodes
        for(int q = 0; q <= ord_v; ++q)
        {
          p1(0, q) = m_controlPoints(0, q);

          for(int i = 0; i < NDIMS; ++i)
            for(int p = 1; p <= ord_u; ++p)
            {
              const int end = ord_u - p;
              for(int k = 0; k <= end; ++k)
                p2(k, q)[i] = lerp(p2(k, q)[i], p2(k + 1, q)[i], uv);

              p1(p, q)[i] = p2(0, q)[i];
            }
        }
      }
      else
      {
        // Do the split for each column of control nodes
        for(int p = 0; p <= ord_u; ++p)
        {
          p1(p, 0) = m_controlPoints(p, 0);

          for(int i = 0; i < NDIMS; ++i)
            for(int q = 1; q <= ord_v; ++q)
            {
              const int end = ord_v - q;
              for(int k = 0; k <= end; ++k)
                p2(p, k)[i] = lerp(p2(p, k)[i], p2(p, k + 1)[i], uv);

              p1(p, q)[i] = p2(p, 0)[i];
            }
        }
      }
    }
  }

  /*!
   * \brief Splits a Bezier patch into four Bezier patches
   *
   * \param [in] u parameter value between 0 and 1 at which to bisect on the first axis
   * \param [in] v parameter value between 0 and 1 at which to bisect on the second axis
   * \param [out] p1 First output Bezier patch
   * \param [out] p2 Second output Bezier patch
   * \param [out] p3 Third output Bezier patch
   * \param [out] p4 Fourth output Bezier patch
   *
   *   v = 1
   *   ----------------------
   *   |         |          |
   *   |   p3    |    p4    |
   *   |         |          |
   *   --------(u,v)---------
   *   |         |          |
   *   |   p1    |    p2    |
   *   |         |          |
   *   ---------------------- u = 1
   *
   * \pre Parameter \a u and \a v must be between 0 and 1
   */
  void split(T u,
             T v,
             BezierPatch& p1,
             BezierPatch& p2,
             BezierPatch& p3,
             BezierPatch& p4) const
  {
    // Bisect the patch along the u direction
    split(u, 0, p1, p2);

    // Temporarily store the result in each half and split again
    BezierPatch p0(p1);
    p0.split(v, 1, p1, p3);

    p0 = p2;
    p0.split(v, 1, p2, p4);
  }

  /*!
   * \brief Predicate to check if the Bezier patch is approximately planar
   *
   * This function checks if all control points of the BezierPatch
   * are approximately on the plane defined by its four corners
   *
   * \param [in] tol Threshold for sum of squared distances
   * \return True if c1 is near-planar
   */
  bool isPlanar(double tol = 1E-8) const
  {
    const int ord_u = getOrder_u();
    const int ord_v = getOrder_v();

    if(ord_u <= 0 && ord_v <= 0) return true;
    if(ord_u == 1 && ord_v == 0) return true;
    if(ord_u == 0 && ord_v == 1) return true;

    PlaneType the_plane = make_plane(m_controlPoints(0, 0),
                                     m_controlPoints(0, ord_v),
                                     m_controlPoints(ord_u, 0));

    double sqDist = 0.0;

    // Check all control points for simplicity
    for(int p = 0; p <= ord_u && sqDist <= tol; ++p)
      for(int q = 0; q <= ord_v && sqDist <= tol; ++q)
      {
        double signedDist = the_plane.signedDistance(m_controlPoints(p, q));
        sqDist += signedDist * signedDist;
      }
    return (sqDist <= tol);
  }

  /*!
   * \brief Simple formatted print of a Bezier Curve instance
   *
   * \param os The output stream to write to
   * \return A reference to the modified ostream
   */
  std::ostream& print(std::ostream& os) const
  {
    const int ord_u = getOrder_u();
    const int ord_v = getOrder_v();

    os << "{ order (" << ord_u << ',' << ord_v << ") Bezier Patch ";

    for(int p = 0; p <= ord_u; ++p)
      for(int q = 0; q <= ord_v; ++q)
        os << m_controlPoints(p, q) << ((p < ord_u || q < ord_v) ? "," : "");

    if(isRational())
    {
      os << ", weights [";
      for(int p = 0; p <= ord_u; ++p)
        for(int q = 0; q <= ord_v; ++q)
          os << m_weights(p, q) << ((p < ord_u || q < ord_v) ? "," : "");
    }
    os << "}";

    return os;
  }

private:
  /// Check that the weights used are positive, and
  ///  that there is one for each control node
  bool isValidRational() const
  {
    if(!isRational()) return true;

    const int ord_u = getOrder_u();
    const int ord_v = getOrder_v();

    if(m_weights.shape()[0] != (ord_u + 1) || m_weights.shape()[1] != (ord_v + 1))
      return false;

    for(int p = 0; p <= ord_u; ++p)
      for(int q = 0; q <= ord_v; ++q)
        if(m_weights(p, q) <= 0) return false;

    return true;
  }

  CoordsMat m_controlPoints;
  axom::Array<T, 2> m_weights;
};

//------------------------------------------------------------------------------
/// Free functions related to BezierCurve
//------------------------------------------------------------------------------
template <typename T, int NDIMS>
std::ostream& operator<<(std::ostream& os, const BezierPatch<T, NDIMS>& bPatch)
{
  bPatch.print(os);
  return os;
}

}  // namespace primal
}  // namespace axom

#endif  // AXOM_PRIMAL_BEZIERPATCH_HPP_
