/*
 * Copyright (c) 2015, Lawrence Livermore National Security, LLC.
 * Produced at the Lawrence Livermore National Laboratory.
 *
 * All rights reserved.
 *
 * This source code cannot be distributed without permission and
 * further review from Lawrence Livermore National Laboratory.
 */

/*!
 ******************************************************************************
 *
 * \file
 *
 * \brief   Header file containing definition of DataBuffer class.
 *
 ******************************************************************************
 */

#ifndef DATABUFFER_HPP_
#define DATABUFFER_HPP_

// Standard C++ headers
#include <vector>

// Other toolkit component headers
#include "common/CommonTypes.hpp"
#include "slic/slic.hpp"

// SiDRe project headers
#include "SidreTypes.hpp"

namespace asctoolkit
{
namespace sidre
{
// using directives to make Conduit usage easier and less visible
using conduit::Node;
using conduit::Schema;

class DataStore;
class DataView;

/*!
 * \class DataBuffer
 *
 * \brief DataBuffer holds a data object, which it owns (and allocates!)
 *
 * The DataBuffer class has the following properties:
 *
 *    - DataBuffer objects can only be created via the DataStore interface,
 *      not directly.
 *    - A DataBuffer object has a unique identifier within a DataStore,
 *      which is assigned by the DataStore when the buffer is created.
 *    - The data object owned by a DataBuffer is unique to that DataBuffer
 *      object; i.e.,  DataBuffers that own data do not share their data.
 *    - A DataBuffer may hold a pointer to externally-owned data. When this
 *      is the case, the buffer cannot be used to (re)allocate or deallocate
 *      the data. However, the external data can be desribed and accessed
 *      via the buffer object in a similar to data that is owned by a buffer.
 *    - Typical usage is to declare the data a DataBuffer will hold and then
 *      either allocate it by calling one of the DataBuffer allocate or
 *      reallocate methods, or set the buffer to reference externally-owned
 *      data by calling setExternalData().
 *    - A DataBuffer object maintains a collection of DataViews that
 *      refer to its data.
 *
 */
class DataBuffer
{

public:

  //
  // Friend declarations to constrain usage via controlled access to
  // private members.
  //
  friend class DataStore;
  friend class DataGroup;
  friend class DataView;

//@{
//!  @name Accessor methods

  /*!
   * \brief Return the unique index of this buffer object.
   */
  IndexType getIndex() const
  {
    return m_index;
  }

  /*!
   * \brief Return number of views attached to this buffer.
   */
  size_t getNumViews() const
  {
    return m_views.size();
  }

  /*!
   * \brief Return true if buffer holds externally-owned data, or
   * false if buffer owns the data it holds (default case).
   */
  bool isExternal() const
  {
    return m_is_data_external;
  }

  /*!
   * \brief Return void-pointer to data held by DataBuffer.
   */
  void * getVoidPtr()
  {
    return m_data;
  }

  /*!
   * \brief Returns data held by node (or pointer to data if array).
   */
  Node::Value getData()
  {
    return m_node.value();
  }

  //@}

  /*!
   * \brief Return type of data for this DataBuffer object.
   */
  TypeID getTypeID() const
  {
    return static_cast<TypeID>(m_schema.dtype().id());
  }

  /*!
   * \brief Return total number of elements allocated by this DataBuffer object.
   */
  size_t getNumElements() const
  {
    return m_schema.dtype().number_of_elements();
  }

  /*!
   * \brief Return total number of bytes associated with this DataBuffer object.
   */
  size_t getTotalBytes() const;

  /*
   * \brief Return true if DataBuffer has an associated DataView with given
   *        index; else false.
   */
  bool hasView( IndexType idx ) const
  {
    return ( 0 <= idx && static_cast<unsigned>(idx) < m_views.size() &&
             m_views[idx] != ATK_NULLPTR );
  }

  /*!
   * \brief Return (non-const) pointer to data view object with given index
   *        associated with buffer, or ATK_NULLPTR if none exists.
   */
  DataView * getView( IndexType idx);

//@}


//@{
//!  @name Data declaration and allocation methods

  /*!
   * \brief Declare a buffer with data given type and number of elements.
   *
   * To use the buffer, the data must be allocated by calling allocate()
   * or set to external data by calling setExternalData().
   *
   * If given number of elements is < 0, method does nothing.
   *
   * \return pointer to this DataBuffer object.
   */
  DataBuffer * declare(TypeID type, SidreLength num_elems);

  /*!
   * \brief Allocate data previously declared using a declare() method.
   *
   * It is the responsibility of the caller to make sure that the buffer
   * object was previously declared.  If the the buffer is already
   * holding data that it owns, that data will be deallocated and new data
   * will be allocated according to the current declared state.
   *
   * If buffer is already set to externally-owned data, this method
   * does nothing.
   *
   * \return pointer to this DataBuffer object.
   */
  DataBuffer * allocate();

  /*!
   * \brief Declare and allocate data described by type and number of elements.
   *
   * This is equivalent to calling declare(type, num_elems), then allocate().
   * on this DataBuffer object.
   *
   * If buffer is already set to externally-owned data, this method
   * does nothing.
   *
   * \return pointer to this DataBuffer object.
   */
  DataBuffer * allocate(TypeID type, SidreLength num_elems);

  /*!
   * \brief Reallocate data to given number of elements.
   *
   *        Equivalent to calling declare(type), then allocate().
   *
   * If buffer is already set to externally-owned data or given
   * number of elements < 0, this method does nothing.
   *
   * \return pointer to this DataBuffer object.
   */
  DataBuffer * reallocate(SidreLength num_elems);

  /*!
   * \brief Update contents of buffer memory.
   *
   * This will copy nbytes of data into the buffer.  nbytes must be greater
   * than 0 and less than getTotalBytes().
   *
   * If given pointer is null, this method does nothing.
   *
   * \return pointer to this DataBuffer object.
   */
  DataBuffer * update(const void * src, size_t nbytes);

  /*!
   * \brief Set buffer to external data.
   *
   * It is the responsibility of the caller to make sure that the buffer
   * object was previously declared, that the data pointer is consistent
   * with how the buffer was declared, and that the buffer is not already
   * holding data that it owns.
   *
   * If given pointer is null, this method does nothing.
   *
   * \return pointer to this DataBuffer object.
   */
  DataBuffer * setExternalData(void * external_data);

//@}


  /*!
   * \brief Copy data buffer description to given Conduit node.
   */
  void info(Node& n) const;

  /*!
   * \brief Print JSON description of data buffer to stdout.
   */
  void print() const;

  /*!
   * \brief Print JSON description of data buffer to an ostream.
   */
  void print(std::ostream& os) const;


private:

  /*!
   *  \brief Private ctor that assigns unique id.
   */
  DataBuffer( IndexType uid );

  /*!
   * \brief Private copy ctor.
   */
  DataBuffer(const DataBuffer& source );

  /*!
   * \brief Private dtor.
   */
  ~DataBuffer();

  /*!
   * \brief Private methods to attach/detach data view to buffer.
   */
  void attachView( DataView * dataView );
  ///
  void detachView( DataView * dataView );

  /*!
   * \brief Private methods allocate and deallocate data owned by buffer.
   */
  void cleanup();
  ///
  void * allocateBytes(std::size_t num_bytes);
  ///
  void  releaseBytes(void * );

#ifdef ATK_ENABLE_FORTRAN
  /*!
   * \brief Set as Fortran allocatable.
   *
   * If given pointer is null, this method does nothing.
   *
   * \return pointer to this DataBuffer object.
   */
  DataBuffer * setFortranAllocatable(void * array, TypeID type, int rank);
#endif

  /// Index Identifier - unique within a dataStore.
  IndexType m_index;

  /// Container of DataViews attached to this buffer.
  std::vector<DataView *> m_views;

  // Type of data pointed to by m_data
  TypeID m_type;

  /// Pointer to the data owned by DataBuffer.
  void * m_data;

  /// Conduit Node that holds buffer data.
  Node m_node;

  /// Conduit Schema that describes buffer data.
  Schema m_schema;

  /// Is buffer holding externally-owned data?
  bool m_is_data_external;

  /*!
   *  Unimplemented ctors and copy-assignment operators.
   */
#ifdef USE_CXX11
  DataBuffer() = delete;
  DataBuffer( DataBuffer&& ) = delete;

  DataBuffer& operator=( const DataBuffer& ) = delete;
  DataBuffer& operator=( DataBuffer&& ) = delete;
#else
  DataBuffer();
  DataBuffer& operator=( const DataBuffer& );
#endif




};


} /* end namespace sidre */
} /* end namespace asctoolkit */

#endif /* DATABUFFER_HPP_ */
