// Copyright (c) 2017-2024, Lawrence Livermore National Security, LLC and
// other Axom Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

/**
 * \file BivariateMap.hpp
 *
 * \brief Contains the BivariateMap class, a map for a BivariateSet
 *
 */

#ifndef SLAM_BIVARIATE_MAP_HPP_
#define SLAM_BIVARIATE_MAP_HPP_

#include "axom/slam/Map.hpp"
#include "axom/slam/Relation.hpp"
#include "axom/slam/BivariateSet.hpp"
#include "axom/slam/SubMap.hpp"
#include "axom/slam/Set.hpp"
#include "axom/slam/policies/StridePolicies.hpp"
#include "axom/slam/policies/PolicyTraits.hpp"

#include <cassert>
#include <typeinfo>

namespace axom
{
namespace slam
{
/**
 * \class BivariateMap
 * \brief A Map for BivariateSet. It associates a constant number of values to
 *        every element in a BivariateSet (as determined by StridePolicy).
 *
 * \detail Like BivariateSet, every value in BivariateMap is indexed by two
 *         indices. BivariateMap's `operator(i)` returns a SubMap of all entries
 *         whose first index is `i` in the BivariateSet.
 *
 * The different indexing systems (DenseIndex, SparseIndex, FlatIndex) are
 * explained in BivariateSet. Because BivariateMap can have more than one
 * component, FlatIndex is further divided into ComponentFlatIndex, where each
 * component in each element is indexed separately, and ElementFlatIndex,
 * an index that disregards the individual components. Hence, to access
 * each component, one would need to provide a component index as well.
 *
 * Example:
 * For a 2 x 2 sparse matrix with 3 components below:
 *     \code
 *         0    1
 *     0  abc
 *     1       def
 *     \endcode
 *
 *   Access the elements using ElementFlatIndex `(e)` would be...\n
 *   `(e = 0) = abc`\n
 *   `(e = 1) = def`\n
 *   To access each component, provide a component index (c)
 *   `(e = 0, c = 0) = a`\n
 *   `(e = 0, c = 1) = b`\n
 *   `(e = 0, c = 2) = c`\n
 *   `(e = 1, c = 0) = d`\n
 *   `(e = 1, c = 1) = e`\n
 *   `(e = 1, c = 2) = f`\n
 *
 *   To access using ComponentFlatIndex `(idx)`...\n
 *   `(idx = 0) = a`\n
 *   `(idx = 1) = b`\n
 *   `(idx = 2) = c`\n
 *   `(idx = 3) = d`\n
 *   `(idx = 4) = e`\n
 *   `(idx = 5) = f`\n
 *
 * \tparam DataType the data type of each value
 * \tparam StridePolicy A policy class for configuring the number of components
 *         associate with each element. There is a fixed \a stride between
 *         the data associated with each element of the set.
 * \see BivariateSet, SubMap
 */

template <typename T,
          typename BSet = BivariateSet<>,
          typename IndPol =
            policies::STLVectorIndirection<typename BSet::PositionType, T>,
          typename StrPol = policies::StrideOne<typename BSet::PositionType>,
          typename IfacePol = policies::VirtualInterface>
class BivariateMap
  : public policies::MapInterface<IfacePol, typename BSet::PositionType>,
    public StrPol
{
public:
  using DataType = T;
  using BivariateSetType = BSet;
  using IndirectionPolicy = IndPol;
  using StridePolicyType = StrPol;

  using SetPosition = typename BSet::PositionType;
  using SetElement = typename BSet::ElementType;

  using ElementShape = typename StridePolicyType::ShapeType;

  using SetType = typename slam::RangeSet<SetPosition, SetElement>::ConcreteSet;
  using MapType = Map<DataType, SetType, IndPol, StrPol, IfacePol>;
  using OrderedSetType = typename BSet::SubsetType;

  using ValueType = typename IndirectionPolicy::IndirectionResult;
  using ConstValueType = typename IndirectionPolicy::ConstIndirectionResult;
  using PointerType = std::remove_reference_t<ValueType>*;
  using ConstPointerType = std::remove_reference_t<ConstValueType>*;

  using BivariateMapType = BivariateMap<DataType, BSet, IndPol, StrPol, IfacePol>;

  template <bool Const>
  class FlatIterator;
  using iterator = FlatIterator<false>;
  using const_iterator = FlatIterator<true>;

  template <bool Const>
  class RangeIterator;
  using range_iterator = RangeIterator<false>;
  using const_range_iterator = RangeIterator<true>;

  using SubMapIndirection =
    typename policies::ToViewIndirection<IndirectionPolicy>::Type;
  using SubMapConstIndirection =
    typename policies::ToViewIndirection<IndirectionPolicy>::ConstType;
  using SubMapType = Map<T, OrderedSetType, SubMapIndirection, StrPol>;
  using ConstSubMapType = Map<T, OrderedSetType, SubMapConstIndirection, StrPol>;

  using SubMapIterator = typename SubMapType::iterator;
  using ConstSubMapIterator = typename ConstSubMapType::iterator;
  using SubMapRangeIterator = typename SubMapType::range_iterator;
  using ConstSubMapRangeIterator = typename ConstSubMapType::range_iterator;

  using NullBivariateSetType =
    NullBivariateSet<typename BSet::FirstSetType, typename BSet::SecondSetType>;

private:
  static const NullBivariateSetType s_nullBiSet;

  template <typename USet = BivariateSetType,
            bool HasValue = !std::is_abstract<USet>::value>
  struct BSetContainer;

  template <typename USet>
  struct BSetContainer<USet, false>
  {
    BSetContainer(const USet* set) : m_pSet(set) { }

    AXOM_HOST_DEVICE const USet* get() const { return m_pSet; }

    const USet* m_pSet;
  };

  template <typename USet>
  struct BSetContainer<USet, true>
  {
    BSetContainer(const USet* set) : m_pSet(set) { }
    BSetContainer(const USet& set) : m_set(set) { }

    AXOM_HOST_DEVICE const USet* get() const
    {
      if(m_pSet)
      {
        return m_pSet;
      }
      else
      {
        return &m_set;
      }
    }

    const USet* m_pSet {nullptr};
    USet m_set;
  };

public:
  using ConcreteMap =
    BivariateMap<T, BSet, IndPol, StrPol, policies::ConcreteInterface>;
  using VirtualMap =
    BivariateMap<T, BSet, IndPol, StrPol, policies::VirtualInterface>;

public:
  /**
   * \brief Constructor for a BivariateMap
   *
   * \param bSet          (Optional) Pointer to the BivariateSet.
   * \param defaultValue  (Optional) The default value used to initialize the
   *                      entries of the map.
   * \param shape         (Optional) The number of components in the map.
   *
   * \note  When using a compile time StridePolicy, \a stride must be equal to
   *        \a StridePolicy::stride(), when provided.
   */
  BivariateMap(const BivariateSetType* bSet = &s_nullBiSet,
               DataType defaultValue = DataType(),
               ElementShape shape = StridePolicyType::DefaultSize(),
               int allocatorID = axom::getDefaultAllocatorID())
    : StridePolicyType(shape)
    , m_bset(bSet)
    , m_map(SetType(bSet->size()), defaultValue, shape, allocatorID)
  { }

  /// \overload
  template <typename UBSet,
            typename Enable = typename std::enable_if<
              !std::is_abstract<BivariateSetType>::value &&
              std::is_base_of<BivariateSetType, UBSet>::value>::type>
  BivariateMap(const UBSet& bSet,
               DataType defaultValue = DataType(),
               ElementShape shape = StridePolicyType::DefaultSize(),
               int allocatorID = axom::getDefaultAllocatorID())
    : StridePolicyType(shape)
    , m_bset(bSet)
    , m_map(SetType(bSet->size()), defaultValue, shape, allocatorID)
  {
    static_assert(std::is_same<BivariateSetType, UBSet>::value,
                  "Argument set is of a more-derived type than the Map's set "
                  "type. This may lead to object slicing. Use Map's pointer "
                  "constructor instead to store polymorphic sets.");
  }

  /**
   * \brief Constructor for BivariateMap using a BivariateSet passed by-value
   *        and data passed in by-value.
   *
   * \param bSet    A reference to the map's associated bivariate set
   * \param data    The data buffer to set the map's data to.
   * \param shape   (Optional) The number of DataType that each element in the
   *                set will be mapped to.
   *                When using a \a RuntimeStridePolicy, the default is 1.
   * \note  When using a compile time StridePolicy, \a stride must be equal to
   *        \a stride(), when provided.
   */
  BivariateMap(const BivariateSetType* bSet,
               typename MapType::OrderedMap data,
               ElementShape shape = StridePolicyType::DefaultSize())
    : StridePolicyType(shape)
    , m_bset(bSet)
    , m_map(SetType(bSet->size()), data, shape)
  { }

  /**
   * \brief Constructor for BivariateMap using a BivariateSet passed by-value
   *        and data passed in by-value.
   *
   * \param bSet    A reference to the map's associated bivariate set
   * \param data    The data buffer to set the map's data to.
   * \param shape   (Optional) The number of DataType that each element in the
   *                set will be mapped to.
   *                When using a \a RuntimeStridePolicy, the default is 1.
   * \note  When using a compile time StridePolicy, \a stride must be equal to
   *        \a stride(), when provided.
   */
  template <typename UBSet,
            typename TBSet = BivariateSetType,
            typename Enable =
              typename std::enable_if<!std::is_abstract<TBSet>::value &&
                                      std::is_base_of<TBSet, UBSet>::value>::type>
  BivariateMap(const UBSet& bSet,
               typename MapType::OrderedMap data,
               ElementShape shape = StridePolicyType::DefaultSize())
    : StridePolicyType(shape)
    , m_bset(bSet)
    , m_map(SetType(bSet.size()), data, shape)
  {
    static_assert(std::is_same<BivariateSetType, UBSet>::value,
                  "Argument set is of a more-derived type than the Map's set "
                  "type. This may lead to object slicing. Use Map's pointer "
                  "constructor instead to store polymorphic sets.");
  }

  // (KW) Problem -- does not work with RelationSet
  template <typename BivariateSetRetType, typename RelType = void>
  typename std::enable_if<!traits::has_relation_ptr<BivariateSetRetType>::value,
                          BivariateSetRetType>::type
  getBivariateSet() const
  {
    using OuterSet = const typename BivariateSetRetType::FirstSetType;
    using InnerSet = const typename BivariateSetRetType::SecondSetType;
    OuterSet* outer = dynamic_cast<OuterSet*>(set()->getFirstSet());
    InnerSet* inner = dynamic_cast<InnerSet*>(set()->getSecondSet());

    return BivariateSetRetType(outer, inner);
  }

  template <typename BivariateSetRetType, typename RelType>
  typename std::enable_if<traits::has_relation_ptr<BivariateSetRetType>::value,
                          BivariateSetRetType>::type
  getBivariateSet() const
  {
    auto* rel = dynamic_cast<const RelType*>(m_bset)->getRelation();
    SLIC_ASSERT(rel != nullptr);

    return BivariateSetRetType(rel);
  }

  /// \name BivariateMap value access functions
  /// @{
  ///

  /**
   * \brief  Access the value in the map using a FlatIndex in the range of 0 to
   *         `size()*numComp()`
   *
   * \return The value for the j<sup>th</sup> component of the i<sup>th</sup>
   *         element, where `setIndex = i * numComp() + j`.
   * \pre    0 <= setIndex < size() * numComp()
   */
  AXOM_HOST_DEVICE ConstValueType operator[](SetPosition setIndex) const
  {
    return m_map[setIndex];
  }
  AXOM_HOST_DEVICE ValueType operator[](SetPosition setIndex)
  {
    return m_map[setIndex];
  }

public:
  /**
   * \brief Returns a SubMap containing the subset of the BivariateMap given the
   *        first set index
   * \pre 0 <= firstIdx < size(firstIdx)
   */
  AXOM_HOST_DEVICE ConstSubMapType operator()(SetPosition firstIdx) const
  {
#ifndef AXOM_DEVICE_CODE
    verifyFirstSetIndex(firstIdx);
#endif
    using ArrayViewType = typename SubMapConstIndirection::IndirectionBufferType;
    // Construct an ArrayView with the subset data.
    auto elemRange = set()->elementRangeSet(firstIdx);
    SetPosition dataOffset = elemRange.offset() * StrPol::stride();
    SetPosition dataSize = elemRange.size() * StrPol::stride();
    ArrayViewType submapView(m_map.data().data() + dataOffset, dataSize);

    auto s = set()->getElements(firstIdx);
    return ConstSubMapType(s, submapView, StrPol::shape());
  }

  AXOM_HOST_DEVICE SubMapType operator()(SetPosition firstIdx)
  {
#ifndef AXOM_DEVICE_CODE
    verifyFirstSetIndex(firstIdx);
#endif
    using ArrayViewType = typename SubMapIndirection::IndirectionBufferType;
    // Construct an ArrayView with the subset data.
    auto elemRange = set()->elementRangeSet(firstIdx);
    SetPosition dataOffset = elemRange.offset() * StrPol::stride();
    SetPosition dataSize = elemRange.size() * StrPol::stride();
    ArrayViewType submapView(m_map.data().data() + dataOffset, dataSize);

    auto s = set()->getElements(firstIdx);
    return SubMapType(s, submapView, StrPol::shape());
  }

  /**
   * \brief Access the value associated with the given SparseIndex into the
   *        BivariateSet and the component index.
   *
   * \pre `0 <= s1 < firstSetSize()`
   * \pre `0 <= s2 < size(s1)`
   * \pre `0 <= comp < numComp()`
   */
  template <typename... ComponentIndex>
  AXOM_HOST_DEVICE ConstValueType operator()(SetPosition s1,
                                             SetPosition s2,
                                             ComponentIndex... comp) const
  {
    auto idx = flatIndex(s1, s2);
    return flatValue(idx, comp...);
  }

  template <typename... ComponentIndex>
  AXOM_HOST_DEVICE ValueType operator()(SetPosition s1,
                                        SetPosition s2,
                                        ComponentIndex... comp)
  {
    auto idx = flatIndex(s1, s2);
    return flatValue(idx, comp...);
  }

  /**
   * \brief Access the value associated with the given FlatIndex into the
   *        BivariateSet and the component index.
   *
   * \pre `0 <= flatIndex < size()`
   * \pre `0 <= comp < numComp()`
   */
  template <typename... ComponentIndex>
  AXOM_HOST_DEVICE ConstValueType flatValue(SetPosition flatIndex,
                                            ComponentIndex... comp) const
  {
    return m_map(flatIndex, comp...);
  }

  template <typename... ComponentIndex>
  AXOM_HOST_DEVICE ValueType flatValue(SetPosition flatIndex,
                                       ComponentIndex... comp)
  {
    return m_map(flatIndex, comp...);
  }

  /**
   * \brief Access the value associated with the given DenseIndex into the
   *        BivariateSet and the component index.
   *
   * \pre `0 <= s1 < firstSetSize()`
   * \pre `0 <= s2 < secondSetSize()`
   * \pre `0 <= comp < numComp()`
   *
   * \return a DataType pointer to the value associated with the given index,
   *         or nullptr if there is no value for the given index.
   * \warning For sparse BivariateSet type, this function may have to do a
   *          linear search and can be slow.
   */
  template <typename... ComponentIndex>
  AXOM_HOST_DEVICE ConstPointerType findValue(SetPosition s1,
                                              SetPosition s2,
                                              ComponentIndex... comp) const
  {
    SetPosition i = set()->findElementFlatIndex(s1, s2);
    if(i == BivariateSetType::INVALID_POS)
    {
      //the BivariateSet does not contain this index pair
      return nullptr;
    }
    return &(m_map(i, comp...));
  }

  template <typename... ComponentIndex>
  AXOM_HOST_DEVICE PointerType findValue(SetPosition s1,
                                         SetPosition s2,
                                         ComponentIndex... comp)
  {
    SetPosition i = set()->findElementFlatIndex(s1, s2);
    if(i == BivariateSetType::INVALID_POS)
    {
      //the BivariateSet does not contain this index pair
      return nullptr;
    }
    return &(m_map(i, comp...));
  }

  /// @}

  /// \name BivariateMap index access functions
  /// @{
  ///

  /**
   * \brief Returns the SparseIndex of the element given the DenseIndex
   *
   * \return The SparseIndex of the element, or BivariateSet::INVALID_POS if
   *         the set doesn't contain the given DenseIndex
   */
  SetPosition index(SetPosition s1, SetPosition s2) const
  {
    return set()->findElementIndex(s1, s2);
  }

  /**
   * \brief Return a set of DenseIndex associated to the given first set index
   *
   * \param s1 the first set index
   * \return OrderedSet containing the elements
   */
  OrderedSetType indexSet(SetPosition s1) const
  {
    return set()->getElements(s1);
  }

  /**
   * \brief Search for the FlatIndex of an element given its DenseIndex in the
   *        BivariateSet.
   */
  AXOM_HOST_DEVICE inline SetPosition flatIndex(SetPosition s1, SetPosition s2) const
  {
    return set()->findElementFlatIndex(s1, s2);
  }

  /// @}

protected:
  /**
   * \brief Utility function to determine if submaps should use indirection
   * when finding the set indices of their elements.
   *
   * This test distinguishes between ProductSet whose second set do not use
   * indirection and other BivariateSet types
   */
  AXOM_HOST_DEVICE constexpr bool submapIndicesHaveIndirection() const
  {
    return traits::indices_use_indirection<BivariateSetType>::value;
    //      || (set()->getSecondSet()->at(0) != 0);
  }

public:
  /** BivariateMap iterator functions */
  AXOM_HOST_DEVICE iterator begin() { return iterator(this, 0); }
  AXOM_HOST_DEVICE iterator end()
  {
    return iterator(this, totalSize() * numComp());
  }
  AXOM_HOST_DEVICE const_iterator begin() const
  {
    return const_iterator(this, 0);
  }
  AXOM_HOST_DEVICE const_iterator end() const
  {
    return const_iterator(this, totalSize() * numComp());
  }
  AXOM_HOST_DEVICE range_iterator set_begin()
  {
    return range_iterator(this, 0);
  }
  AXOM_HOST_DEVICE range_iterator set_end()
  {
    return range_iterator(this, totalSize());
  }
  AXOM_HOST_DEVICE const_range_iterator set_begin() const
  {
    return const_range_iterator(this, 0);
  }
  AXOM_HOST_DEVICE const_range_iterator set_end() const
  {
    return const_range_iterator(this, totalSize());
  }

  /** Iterator via Submap */
  AXOM_HOST_DEVICE SubMapIterator begin(int i) { return (*this)(i).begin(); }
  AXOM_HOST_DEVICE SubMapIterator end(int i) { return (*this)(i).end(); }
  AXOM_HOST_DEVICE ConstSubMapIterator begin(int i) const
  {
    return (*this)(i).begin();
  }
  AXOM_HOST_DEVICE ConstSubMapIterator end(int i) const
  {
    return (*this)(i).end();
  }
  AXOM_HOST_DEVICE SubMapRangeIterator set_begin(int i)
  {
    return (*this)(i).set_begin();
  }
  AXOM_HOST_DEVICE SubMapRangeIterator set_end(int i)
  {
    return (*this)(i).set_end();
  }
  AXOM_HOST_DEVICE ConstSubMapRangeIterator set_begin(int i) const
  {
    return (*this)(i).set_begin();
  }
  AXOM_HOST_DEVICE ConstSubMapRangeIterator set_end(int i) const
  {
    return (*this)(i).set_end();
  }

public:
  AXOM_HOST_DEVICE const BivariateSetType* set() const { return m_bset.get(); }
  const MapType* getMap() const { return &m_map; }
  MapType* getMap() { return &m_map; }

  bool isValid(bool verboseOutput = false) const
  {
    return set()->isValid(verboseOutput) && m_map.isValid(verboseOutput);
  }

  /// \name BivariateMap cardinality functions
  /// @{
  ///

  /** \brief Returns the BivariateSet size. */
  AXOM_HOST_DEVICE SetPosition size() const { return set()->size(); }
  /** \brief Returns the BivariateSet size. */
  SetPosition totalSize() const { return set()->size(); }

  SetPosition firstSetSize() const { return set()->firstSetSize(); }
  AXOM_HOST_DEVICE SetPosition secondSetSize() const
  {
    return set()->secondSetSize();
  }
  /** \brief Returns the number of the BivariateSet ordered pairs with
   *         the given first set index. */
  SetPosition size(SetPosition s) const { return set()->size(s); }
  /** \brief Return the number of components of the map  */
  SetPosition numComp() const { return StrPol::stride(); }

  /// @}

  /**
   * \brief Given a DataType array of size `totalSize()*numComp()`, copy
   *        the data into the BivariateMap storage.
   *
   * \param data_arr The array of DataType that contains the data to be copied.
   */
  void copy(const DataType* data_arr)
  {
    for(int i = 0; i < m_map.size() * StrPol::stride(); i++)
    {
      m_map[i] = data_arr[i];
    }
  }

  /** \brief replace all elements in the Map with the default DataType */
  void clear() { m_map.clear(); }

private:
  /** \brief Check the indices (DenseIndex) are valid   */
  void verifyPosition(SetPosition s1, SetPosition s2) const
  {
    set()->verifyPosition(s1, s2);
  }

  /** \brief Check the given ElementFlatIndex is valid.  */
  void verifyPosition(SetPosition AXOM_DEBUG_PARAM(pos)) const
  {
    SLIC_ASSERT_MSG(pos >= 0 && pos < SetPosition(m_map.size()),
                    "Attempted to access element "
                      << pos << " but BivariateMap's data has size "
                      << m_map.size());
  }

  void verifyFirstSetIndex(SetPosition AXOM_DEBUG_PARAM(firstIdx)) const
  {
    SLIC_ASSERT_MSG(firstIdx >= 0 && firstIdx < firstSetSize(),
                    "Attempted to access elements with first set index "
                      << firstIdx << ", but BivariateMap's first set has size "
                      << firstSetSize());
  }

private:
  BSetContainer<> m_bset;
  MapType m_map;
};  //end BivariateMap

template <typename T, typename BSet, typename IndPol, typename StrPol, typename IfacePol>
typename BivariateMap<T, BSet, IndPol, StrPol, IfacePol>::NullBivariateSetType const
  BivariateMap<T, BSet, IndPol, StrPol, IfacePol>::s_nullBiSet;

/**
 * \class BivariateMapIterator
 * \brief An iterator type for a BivariateMap, iterating via its
 *        ElementFlatIndex.
 *
 *  This iterator class iterates over all elements in the associated map.
 */
template <typename T, typename BSet, typename IndPol, typename StrPol, typename IfacePol>
template <bool Const>
class BivariateMap<T, BSet, IndPol, StrPol, IfacePol>::FlatIterator
{
private:
  using iterator_category = std::forward_iterator_tag;
  using value_type = DataType;
  using difference_type = SetPosition;

  using iter = FlatIterator;

public:
  using DataRefType = std::conditional_t<Const, const DataType&, DataType&>;
  using BivariateMapPtr =
    std::conditional_t<Const, const BivariateMap*, BivariateMap*>;

  using PositionType = SetPosition;
  static constexpr PositionType INVALID_POS = -2;

public:
  /**
     * \brief Construct a new BivariateMap Iterator given an ElementFlatIndex
     */
  FlatIterator(BivariateMapPtr sMap, PositionType pos)
    : m_map(sMap)
    , m_bsetIterator(m_map->set(), pos / m_map->numComp())
    , m_compIndex(pos % m_map->numComp())
  { }

  FlatIterator& operator++()
  {
    m_compIndex++;
    if(m_compIndex == m_map->numComp())
    {
      m_bsetIterator++;
      m_compIndex = 0;
    }
    return *this;
  }

  FlatIterator operator++(int)
  {
    FlatIterator next = *this;
    ++(*this);
    return next;
  }

  bool operator==(const iter& other) const
  {
    return (m_map == other.m_map) && (m_bsetIterator == other.m_bsetIterator) &&
      (m_compIndex == other.m_compIndex);
  }
  bool operator!=(const iter& other) const { return !operator==(other); }

  /**
     * \brief Returns the current map element pointed to by the iterator.
     */
  DataRefType operator*()
  {
    return m_map->flatValue(m_bsetIterator.flatIndex(), m_compIndex);
  }

  /**
     * \brief return the current iterator's first index into the BivariateSet
     */
  PositionType firstIndex() const { return m_bsetIterator.firstIndex(); }

  /**
     * \brief return the current iterator's second index (DenseIndex)
     *        into the BivariateSet
     */
  PositionType secondIndex() const { return m_bsetIterator.secondIndex(); }

  /// \brief return the current iterator's component index
  PositionType compIndex() const { return m_compIndex; }

  /** \brief Returns the number of components per element in the map. */
  PositionType numComp() const { return m_map->numComp(); }

private:
  BivariateMapPtr m_map;
  typename BivariateSetType::IteratorType m_bsetIterator;
  IndexType m_compIndex;
};

/**
 * \class BivariateMap::RangeIterator
 *
 * \brief An iterator type for a BivariateMap, iterating over elements in an
 *  associated BivariateSet.
 *
 *  Unlike the FlatIterator, which iterates over all map elements, the
 *  RangeIterator may point to a range of elements in the case of non-unit
 *  stride.
 */
template <typename T, typename BSet, typename IndPol, typename StrPol, typename IfacePol>
template <bool Const>
class BivariateMap<T, BSet, IndPol, StrPol, IfacePol>::RangeIterator
{
private:
  using MapIterator = typename MapType::template MapRangeIterator<Const>;

  using iterator_category = std::forward_iterator_tag;
  using value_type = typename MapIterator::value_type;
  using difference_type = SetPosition;

public:
  using DataRefType = typename MapIterator::DataRefType;
  using BivariateMapPtr =
    std::conditional_t<Const, const BivariateMap*, BivariateMap*>;

  using PositionType = SetPosition;
  static constexpr PositionType INVALID_POS = -2;

public:
  /**
     * \brief Construct a new BivariateMap Iterator given an ElementFlatIndex
     */
  RangeIterator(BivariateMapPtr sMap, PositionType pos)
    : m_map(sMap)
    , m_mapIterator(m_map->getMap()->set_begin() + pos)
    , m_bsetIterator(m_map->set(), pos)
  { }

  RangeIterator& operator++()
  {
    m_bsetIterator++;
    m_mapIterator++;
    return *this;
  }

  RangeIterator operator++(int)
  {
    RangeIterator next = *this;
    ++(*this);
    return next;
  }

  bool operator==(const RangeIterator& other) const
  {
    return (m_map == other.m_map) && (m_bsetIterator == other.m_bsetIterator);
  }
  bool operator!=(const RangeIterator& other) const
  {
    return !operator==(other);
  }

  /**
     * \brief Returns the current iterator value. If the BivariateMap has
     *        multiple components, this will return the first component.
     *        To access the other components, use iter(comp)
     */
  value_type operator*() { return *m_mapIterator; }

  /**
     * \brief Returns the iterator's value at the specified component.
     *        Returns the first component if comp_idx is not specified.
     * \param comp_idx  (Optional) Zero-based index of the component.
     */
  DataRefType operator()(PositionType comp_idx = 0) { return value(comp_idx); }

  /** \brief Returns the first component value after n increments.  */
  DataRefType operator[](PositionType n) { return *(this->operator+(n)); }

  /**
     * \brief Return the value at the iterator's position. Same as operator()
     */
  DataRefType value(PositionType comp = 0) { return m_mapIterator(comp); }

  /**
     * \brief return the current iterator's first index into the BivariateSet
     */
  PositionType firstIndex() const { return m_bsetIterator.firstIndex(); }

  /**
     * \brief return the current iterator's second index (DenseIndex)
     *        into the BivariateSet
     */
  PositionType secondIndex() const { return m_bsetIterator.secondIndex(); }

  /** \brief Returns the number of components per element in the map. */
  PositionType numComp() const { return m_map->numComp(); }

private:
  BivariateMapPtr m_map;
  typename MapType::template MapRangeIterator<Const> m_mapIterator;
  typename BivariateSetType::IteratorType m_bsetIterator;
};

}  // end namespace slam
}  // end namespace axom

#endif  // SLAM_BIVARIATE_MAP_HPP_
