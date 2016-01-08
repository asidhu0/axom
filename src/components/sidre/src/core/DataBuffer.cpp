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
 * \brief   Implementation file for DataBuffer class.
 *
 ******************************************************************************
 */


// Associated header file
#include "DataBuffer.hpp"

// Standard C++ headers
#include <algorithm>

// Other CS Toolkit headers
#include "common/CommonTypes.hpp"
#include "slic/slic.hpp"

// SiDRe project headers
#include "DataGroup.hpp"
#include "DataView.hpp"
#include "SidreTypes.hpp"


namespace asctoolkit
{
namespace sidre
{

/*
 *************************************************************************
 *
 * Return total number of bytes associated with this DataBuffer object.
 *
 *************************************************************************
 */
size_t DataBuffer::getTotalBytes() const
{
  return m_schema.total_bytes();
}

/*
 *************************************************************************
 *
 * Return non-cost pointer to view with given index or null ptr.
 *
 *************************************************************************
 */
DataView * DataBuffer::getView( IndexType idx )
{
  SLIC_CHECK_MSG(hasView(idx), "no view exists with index == " << idx);

  if ( hasView(idx) )
  {
    return m_views[idx];
  }
  else
  {
    return ATK_NULLPTR;
  }
}


/*
 *************************************************************************
 *
 * Declare buffer to OWN data of given type and number of elements.
 *
 *************************************************************************
 */
DataBuffer * DataBuffer::declare(TypeID type, SidreLength num_elems)
{
  SLIC_ASSERT_MSG(num_elems >= 0, "Must declare number of elements >=0");

  if ( num_elems >= 0 )
  {
    m_type = type;

    DataType dtype = conduit::DataType::default_dtype(type);
    dtype.set_number_of_elements(num_elems);
    m_schema.set(dtype);
  }
  return this;
}

/*
 *************************************************************************
 *
 * Allocate data previously declared.
 *
 *************************************************************************
 */
DataBuffer * DataBuffer::allocate()
{
  SLIC_ASSERT_MSG( !m_is_data_external,
                   "Attempting to allocate buffer holding external data");

  if ( !m_is_data_external )
  {
    // cleanup old data
    cleanup();
    std::size_t alloc_size = getTotalBytes();
    m_data = allocateBytes(alloc_size);
    m_node.set_external(m_schema, m_data);
  }

  return this;
}

/*
 *************************************************************************
 *
 * Declare and allocate data described by type and num elements.
 *
 *************************************************************************
 */
DataBuffer * DataBuffer::allocate(TypeID type, SidreLength num_elems)
{
  SLIC_ASSERT_MSG(num_elems >= 0, "Must allocate number of elements >=0");
  SLIC_ASSERT_MSG( !m_is_data_external,
                   "Attempting to allocate buffer holding external data");

  if ( num_elems >= 0 && !m_is_data_external )
  {
    declare(type, num_elems);
    allocate();
  }

  return this;
}

/*
 *************************************************************************
 *
 * Reallocate data to given number of elements.
 *
 *************************************************************************
 */
DataBuffer * DataBuffer::reallocate( SidreLength num_elems)
{
  SLIC_ASSERT_MSG(num_elems >= 0, "Must re-allocate number of elements >=0");
  SLIC_ASSERT_MSG( !m_is_data_external,
                   "Attempting to re-allocate buffer holding external data");
  SLIC_ASSERT_MSG( m_data != ATK_NULLPTR,
                   "Attempting to reallocate an unallocated buffer");

  std::size_t old_size = getTotalBytes();
  // update the buffer's Conduit Node
  DataType dtype = conduit::DataType::default_dtype(m_type);
  dtype.set_number_of_elements( num_elems );
  m_schema.set(dtype);

  std::size_t new_size = getTotalBytes();

  void * realloc_data = allocateBytes(new_size);

  memcpy(realloc_data, m_data, std::min(old_size, new_size));

  // cleanup old data
  cleanup();

  // let the buffer hold the new data
  m_data = realloc_data;

  // update the conduit node data pointer
  m_node.set_external(m_schema, m_data);

  return this;
}

/*
 *************************************************************************
 *
 * Update contents of buffer from src and which is nbytes long.
 *
 *************************************************************************
 */
DataBuffer * DataBuffer::update(const void * src, size_t nbytes)
{
  size_t buff_nbytes = getTotalBytes();
  SLIC_ASSERT_MSG(nbytes <= buff_nbytes,
                  "Must allocate number of elements >=0");

  if ( src != ATK_NULLPTR && nbytes <= buff_nbytes)
  {
    memcpy(m_data, src, nbytes);
  }

  return this;
}

/*
 *************************************************************************
 *
 * Set buffer to externally-owned data.
 *
 *************************************************************************
 */
DataBuffer * DataBuffer::setExternalData(void * external_data)
{
  SLIC_ASSERT_MSG( external_data != ATK_NULLPTR,
                   "Attempting to set buffer to external data given null pointer" );

  if ( external_data != ATK_NULLPTR )
  {
    m_data = external_data;
    m_node.set_external(m_schema, m_data);
    m_is_data_external = true;
  }
  return this;
}

/*
 *************************************************************************
 *
 * Copy data buffer description to given Conduit node.
 *
 *************************************************************************
 */
void DataBuffer::info(Node &n) const
{
  n["index"].set(m_index);
  n["is_data_external"].set(m_is_data_external);
  n["schema"].set(m_schema.to_json());
  n["node"].set(m_node.to_json());
}

/*
 *************************************************************************
 *
 * Print JSON description of data buffer to stdout.
 *
 *************************************************************************
 */
void DataBuffer::print() const
{
  print(std::cout);
}

/*
 *************************************************************************
 *
 * Print JSON description of data buffer to an ostream.
 *
 *************************************************************************
 */
void DataBuffer::print(std::ostream& os) const
{
  Node n;
  info(n);
  n.to_json_stream(os);
}



/*
 *************************************************************************
 *
 * PRIVATE ctor taking unique index.
 *
 *************************************************************************
 */
DataBuffer::DataBuffer( IndexType index )
  : m_index(index),
  m_views(),
  m_type(EMPTY_ID),
  m_data(ATK_NULLPTR),
  m_node(),
  m_schema(),
  m_is_data_external(false)
{}


/*
 *************************************************************************
 *
 * PRIVATE copy ctor.
 *
 *************************************************************************
 */
DataBuffer::DataBuffer(const DataBuffer& source )
  : m_index(source.m_index),
  m_views(source.m_views),
  m_type(EMPTY_ID),
  m_data(source.m_data),
  m_node(source.m_node),
  m_schema(source.m_schema),
  m_is_data_external(source.m_is_data_external)
{
// disallow?
}


/*
 *************************************************************************
 *
 * PRIVATE dtor.
 *
 *************************************************************************
 */
DataBuffer::~DataBuffer()
{
  cleanup();
}


/*
 *************************************************************************
 *
 * PRIVATE method to attach data view.
 *
 *************************************************************************
 */
void DataBuffer::attachView( DataView * view )
{
  m_views.push_back( view );
}


/*
 *************************************************************************
 *
 * PRIVATE method to detach data view.
 *
 *************************************************************************
 */
void DataBuffer::detachView( DataView * view )
{
  //Find new end iterator
  std::vector<DataView *>::iterator pos = std::remove(m_views.begin(),
                                                      m_views.end(),
                                                      view);
  // check if pos is ok?
  //Erase the "removed" elements.
  m_views.erase(pos, m_views.end());
}

/*
 *************************************************************************
 *
 * PRIVATE cleanup
 *
 *************************************************************************
 */
void DataBuffer::cleanup()
{
  // cleanup allocated data
  if ( m_data != ATK_NULLPTR )
  {
    if (!m_is_data_external )
    {
      releaseBytes(m_data);
    }
  }
}

/*
 *************************************************************************
 *
 * PRIVATE allocateBytes
 *
 *************************************************************************
 */
void * DataBuffer::allocateBytes(std::size_t num_bytes)
{
  SLIC_ASSERT_MSG(num_bytes > 0,
                  "Attempting to allocate 0 bytes");

  char * data = new char[num_bytes];
  return ((void *)data);
}

/*
 *************************************************************************
 *
 * PRIVATE releaseBytes
 *
 *************************************************************************
 */
void DataBuffer::releaseBytes(void * ptr)
{
  if ( !m_is_data_external )
  {
    delete [] ((char *)ptr);
    m_data = ATK_NULLPTR;
  }
}


} /* end namespace sidre */
} /* end namespace asctoolkit */
