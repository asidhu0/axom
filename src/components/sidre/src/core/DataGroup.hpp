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
 * \brief   Header file containing definition of DataGroup class.
 *
 ******************************************************************************
 */

#ifndef DATAGROUP_HPP_
#define DATAGROUP_HPP_

#include "common/config.hpp"    // defines ATK_USE_CXX11
#include "common/ATKMacros.hpp"

// Standard C++ headers
#include <memory>
#include <map>
#include <string>
#include <vector>
#include <set>
#include <cstring>

// third party lib headers
#include "hdf5.h"

#ifndef USE_NEW_MAP_COLLECTION
#define USE_NEW_MAP_COLLECTION
#endif

#if defined(ATK_USE_UNORDERED_MAP)
//STL or Boost unordered_map, depending on
#include <unordered_map>
#endif

#if defined(ATK_USE_SPARSEHASH)
#include <sparsehash/dense_hash_map>
#endif

// Other CS Toolkit headers
#include "slic/slic.hpp"
#include "common/CommonTypes.hpp"

// SiDRe project headers
#include "SidreTypes.hpp"
#include "Collections.hpp"
#include "DataView.hpp"


namespace asctoolkit
{
namespace sidre
{

class DataBuffer;
class DataGroup;
class DataStore;

/*!
 * \class DataGroup
 *
 * \brief DataGroup holds a collection of DataViews and (child) DataGroups.
 *
 * The DataGroup class has the following properties:
 *
 *    - DataGroups can be organized into a (tree) hierachy by creating
 *      child Groups from the root Group owned by a DataStore object.
 *    - A DataGroup object can only be created by another DataGroup; the
 *      DataGroup ctor is not visible externally. A DataGroup is owned
 *      by the DataGroup that creates it (its parent) and becomes a
 *      (direct) child Group of the parent. DataGroups in the subtree
 *      rooted at an ancestor DataGroup are that Group's descendants.
 *    - A DataGroup object has a unique name (string) within its parent
 *      DataGroup.
 *    - A DataGroup object maintains a pointer to its parent DataGroup.
 *    - A DataGroup object can be moved or copied to another DataGroup.
 *    - DataGroup objects can create DataView objects within them. The
 *      DataGroup that creates a DataView owns it.
 *    - A DataView object has a unique name (string) within the DataGroup
 *      that owns it.
 *    - A DataView object can be moved or copied to another DataGroup.
 *
 * Note that DataViews and child DataGroups within a Group can be accessed
 * by name or index.
 *
 * Note that certain methods for querying, creating, retrieving, and
 * deleting DataGroups and DataViews take a string with path syntax,
 * while others take the name of a direct child of the current DataGroup.
 * Methods that require the name of a direct child are marked with
 * "Child", for example hasChildView() and hasChildGroup().  When a path
 * string is passed to a method that accepts path syntax, the last item in
 * the path indicates the item to be created, accessed, etc.  For example,
 *
 * \verbatim
 *
 *    DataView* view = group->createView("foo/bar/baz");
 *
 *    is equivalent to:
 *
 *    DataView* view =
 *      group->createGroup("foo")->createGroup("bar")->createView("baz");
 *
 * \endverbatim
 *
 * In particular, intermediate Groups "foo" and "bar" will be created in
 * this case if they don't already exist.
 *
 * Methods that access Views or Groups by index work with the direct
 * children of the current Group because an index has no meaning outside
 * of the indexed group.  None of these methods is marked with "Child".
 *
 * IMPORTANT: when Views or Groups are created, destroyed, copied, or moved,
 * indices of other Views and Groups in associated DataGroup objects may
 * become invalid. This is analogous to iterator invalidation for STL
 * containers when the container contents change.
 *
 */
class DataGroup
{
public:

  //
  // Friend declarations to constrain usage via controlled access to
  // private members.
  //
  friend class DataStore;


//@{
//!  @name Basic query and accessor methods.

  /*!
   * \brief Return the path delimiter
   */
  char getPathDelimiter() const
  {
    return s_path_delimiter;
  }

  /*!
   * \brief Return const reference to name of Group object.
   *
   * \sa getPath(), getPathName()
   */
  const std::string& getName() const
  {
    return m_name;
  }

  /*!
   * \brief Return path of Group object, not including its name.
   *
   * \sa getName(), getPathName()
   */
  std::string getPath() const;

  /*!
   * \brief Return full path of Group object, including its name.
   *
   * If a DataStore contains a DataGroup tree structure a/b/c/d/e, with
   * group d owning a view v, the following results are expected:
   *
   * Method Call      | Result
   * -----------------|----------
   * d->getName()     | d
   * d->getPath()     | a/b/c
   * d->getPathName() | a/b/c/d
   *
   * \sa getName(), getPath(), DataView::getPathName()
   */
  std::string getPathName() const
  {
    if (getPath().length() < 1)
    {
      return getName();
    }

    return getPath() + getPathDelimiter() + getName();
  }

  /*!
   * \brief Return pointer to non-const parent Group of a Group.
   *
   * Note that if this method is called on the root Group in a
   * DataStore, ATK_NULLPTR is returned.
   */
  DataGroup * getParent()
  {
    return m_parent;
  }

  /*!
   * \brief Return pointer to const parent Group of a Group.
   *
   * Note that if this method is called on the root Group in a
   * DataStore, ATK_NULLPTR is returned.
   */
  const DataGroup * getParent() const
  {
    return m_parent;
  }

  /*!
   * \brief Return number of child Groups in a Group object.
   */
  size_t getNumGroups() const
  {
    return m_group_coll.getNumItems();
  }

  /*!
   * \brief Return number of Views owned by a Group object.
   */
  size_t getNumViews() const
  {
    return m_view_coll.getNumItems();
  }

  /*!
   * \brief Return pointer to non-const DataStore object that owns this
   * object.
   */
  DataStore * getDataStore()
  {
    return m_datastore;
  }

  /*!
   * \brief Return pointer to const DataStore object that owns this
   * object.
   */
  const DataStore * getDataStore() const
  {
    return m_datastore;
  }

//@}


//@{
//!  @name View query methods.

  /*!
   * \brief Return true if Group includes a descendant View with
   * given name or path; else false.
   */
  bool hasView( const std::string& path ) const;

  /*!
   * \brief Return true if this Group owns a View with given name (not path);
   * else false.
   */
  bool hasChildView( const std::string& name ) const
  {
    return m_view_coll.hasItem(name);
  }

  /*!
   * \brief Return true if this Group owns a View with given index; else false.
   */
  bool hasView( IndexType idx ) const
  {
    return m_view_coll.hasItem(idx);
  }

  /*!
   * \brief Return index of View with given name owned by this Group object.
   *
   *        If no such View exists, return sidre::InvalidIndex;
   */
  IndexType getViewIndex(const std::string& name) const
  {
    SLIC_CHECK_MSG(hasChildView(name),
                   "Group " << this->getName() <<
                   " has no View with name '" << name << "'");

    return m_view_coll.getItemIndex(name);
  }

  /*!
   * \brief Return name of View with given index owned by Group object.
   *
   *        If no such View exists, return sidre::InvalidName.
   */
  const std::string& getViewName(IndexType idx) const
  {
    SLIC_CHECK_MSG(hasView(idx),
                   "Group " << this->getName() <<
                   " has no View with index " << idx);

    return m_view_coll.getItemName(idx);
  }

//@}


//@{
//!  @name View access and iteration methods.

  /*!

   * \brief Return pointer to non-const View with given name or path.
   *
   * This method requires that all groups in the path exist if a path is given.
   *
   * If no such View exists, ATK_NULLPTR is returned.
   */
  DataView * getView( const std::string& path );

  /*!
   * \brief Return pointer to const View with given name or path.
   *
   * This method requires that all Groups in the path exist if a path is given.
   *
   * If no such View exists, ATK_NULLPTR is returned.
   */
  const DataView * getView( const std::string& path ) const;

  /*!
   * \brief Return pointer to non-const View with given index.
   *
   * If no such View exists, ATK_NULLPTR is returned.
   */
  DataView * getView( IndexType idx )
  {
    SLIC_CHECK_MSG( hasView(idx),
                    "Group " << this->getName()
                             << " has no View with index " << idx);

    return m_view_coll.getItem(idx);
  }

  /*!
   * \brief Return pointer to const View with given index.
   *
   * If no such View exists, ATK_NULLPTR is returned.
   */
  const DataView * getView( IndexType idx ) const
  {
    SLIC_CHECK_MSG( hasView(idx),
                    "Group " << this->getName()
                             << " has no View with index " << idx);

    return m_view_coll.getItem(idx);
  }

  /*!
   * \brief Return first valid View index in Group object
   *        (i.e., smallest index over all Views).
   *
   * sidre::InvalidIndex is returned if Group has no Views.
   */
  IndexType getFirstValidViewIndex() const
  {
    return m_view_coll.getFirstValidIndex();
  }

  /*!
   * \brief Return next valid View index in Group object after given index
   *        (i.e., smallest index over all View indices larger than given one).
   *
   * sidre::InvalidIndex is returned if there is no valid index greater
   * than given one.
   */
  IndexType getNextValidViewIndex(IndexType idx) const
  {
    return m_view_coll.getNextValidIndex(idx);
  }

//@}


//@{
//!  @name Methods to create a View that has no associated data.
//!
//! IMPORTANT: These methods do not allocate data or associate a View
//! with data. Thus, to do anything useful with a View created by one
//! of these methods, the View should be allocated, attached to a Buffer
//! or attached to externally-owned data.
//!
//! Each of these methods is a no-op if the given View name is an
//! empty string or the Group already has a View with given name or path.
//!
//! Additional conditions under which a method can be a no-op are described
//! for each method.

  /*!
   * \brief Create an undescribed (i.e., empty) View object with given name
   * or path in this Group.
   *
   * \return pointer to new View object or ATK_NULLPTR if one is not created.
   */
  DataView * createView( const std::string& path );

  /*!
   * \brief Create View object with given name or path in this Group that
   *  has a data description with data type and number of elements.
   *
   * If given data type is undefined, or given number of elements is < 0,
   * method is a no-op.
   *
   * \return pointer to new View object or ATK_NULLPTR if one is not created.
   */
  DataView * createView( const std::string& path,
                         TypeID type,
                         SidreLength num_elems );

  /*!
   * \brief Create View object with given name or path in this Group that
   *  has a data description with data type and shape.
   *
   * If given data type is undefined, or given number of dimensions is < 0,
   * or given shape ptr is null, method is a no-op.
   *
   * \return pointer to new View object or ATK_NULLPTR if one is not created.
   */
  DataView * createView( const std::string& path,
                         TypeID type,
                         int ndims,
                         SidreLength * shape );

  /*!
   * \brief Create View object with given name or path in this Group that
   *  is described by a Conduit DataType object.
   *
   * \return pointer to new View object or ATK_NULLPTR if one is not created.
   */
  DataView * createView( const std::string& path,
                         const DataType& dtype);

//@}


//@{
//!  @name Methods to create a View with a Buffer attached.
//!
//! IMPORTANT: The Buffer passed to each of these methods may or may not
//! be allocated. Thus, to do anything useful with a View created by one
//! of these methods, the Buffer must be allocated and it must be compatible
//! with the View data description.
//!
//! Each of these methods is a no-op if the given View name is an
//! empty string or the Group already has a View with given name or path.
//!
//! Also, calling one of these methods with a null Buffer pointer is
//! similar to creating a View with no data association.
//!
//! Additional conditions under which a method can be a no-op are described
//! for each method.

  /*!
   * \brief Create an undescribed View object with given name or path in
   * this Group and attach given Buffer to it.
   *
   * IMPORTANT: The View cannot be used to access data in Buffer until it
   * is described by calling a DataView::apply() method.
   *
   * This method is equivalent to:
   * group->createView(name)->attachBuffer(buff).
   *
   * \return pointer to new View object or ATK_NULLPTR if one is not created.
   *
   * \sa DataView::attachBuffer
   */
  DataView * createView( const std::string& path,
                         DataBuffer * buff );

  /*!
   * \brief Create View object with given name or path in this Group that
   * has a data description with data type and number of elements and
   * attach given Buffer to it.
   *
   * If given data type is undefined, or given number of elements is < 0,
   * method is a no-op.
   *
   * This method is equivalent to:
   * group->createView(name, type, num_elems)->attachBuffer(buff), or
   * group->createView(name)->attachBuffer(buff)->apply(type, num_elems).
   *
   * \return pointer to new View object or ATK_NULLPTR if one is not created.
   *
   * \sa DataView::attachBuffer
   */
  DataView * createView( const std::string& path,
                         TypeID type,
                         SidreLength num_elems,
                         DataBuffer * buff );

  /*!
   * \brief Create View object with given name or path in this Group that
   * has a data description with data type and shape and attach given
   * Buffer to it.
   *
   * If given data type is undefined, or given number of dimensions is < 0,
   * or given shape ptr is null, method is a no-op.
   *
   * This method is equivalent to:
   * group->createView(name, type, ndims, shape)->attachBuffer(buff), or
   * group->createView(name)->attachBuffer(buff)->apply(type, ndims, shape).
   *
   * \return pointer to new View object or ATK_NULLPTR if one is not created.
   *
   * \sa DataView::attachBuffer
   */
  DataView * createView( const std::string& path,
                         TypeID type,
                         int ndims,
                         SidreLength * shape,
                         DataBuffer * buff );

  /*!
   * \brief Create View object with given name or path in this Group that
   *  is described by a Conduit DataType object and attach given Buffer to it.
   *
   * This method is equivalent to:
   * group->createView(name, dtype)->attachBuffer(buff), or
   * group->createView(name)->attachBuffer(buff)->apply(dtype).
   *
   * \return pointer to new View object or ATK_NULLPTR if one is not created.
   *
   * \sa DataView::attachBuffer
   */
  DataView * createView( const std::string& path,
                         const DataType& dtype,
                         DataBuffer * buff );

//@}


//@{
//!  @name Methods to create a View with externally-owned data attached.
//!
//! IMPORTANT: To do anything useful with a View created by one of these
//! methods, the external data must be allocated and compatible with the
//! View description.
//!
//! Each of these methods is a no-op if the given View name is an
//! empty string or the Group already has a View with given name or path.
//!
//! Additional conditions under which a method can be a no-op are described
//! for each method.

  /*!
   * \brief Create View object with given name with given name or path in
   * this Group and attach external data ptr to it.
   *
   * IMPORTANT: Note that the View is "opaque" (it has no knowledge of
   * the type or structure of the data) until a DataView::apply() method
   * is called.
   *
   * This method is equivalent to:
   * group->createView(name)->setExternalDataPtr(external_ptr).
   *
   * \return pointer to new View object or ATK_NULLPTR if one is not created.
   *
   * \sa DataView::setExternalDataPtr
   */
  DataView * createView( const std::string& path,
                         void * external_ptr );

  /*!
   * \brief Create View object with given name or path in this Group that
   * has a data description with data type and number of elements and
   * attach externally-owned data to it.
   *
   * If given data type is undefined, or given number of elements is < 0,
   * method is a no-op.
   *
   * This method is equivalent to:
   * group->createView(name, type, num_elems)->setExternalDataPtr(external_ptr),
   * or group->createView(name)->setExternalDataPtr(external_ptr)->
   *           apply(type, num_elems).
   *
   * \return pointer to new View object or ATK_NULLPTR if one is not created.
   *
   * \sa DataView::setExternalDataPtr
   */
  DataView * createView( const std::string& path,
                         TypeID type,
                         SidreLength num_elems,
                         void * external_ptr );


  /*!
   * \brief Create View object with given name or path in this Group that
   * has a data description with data type and shape and attach
   * externally-owned data to it.
   *
   * If given data type is undefined, or given number of dimensions is < 0,
   * or given shape ptr is null, method is a no-op.
   *
   * This method is equivalent to:
   * group->createView(name, type, ndims, shape)->
   *        setExternalDataPtr(external_ptr), or
   * group->createView(name)->setExternalDataPtr(external_ptr)->
   *        apply(type, ndims, shape).
   *
   * \return pointer to new View object or ATK_NULLPTR if one is not created.
   *
   * \sa DataView::setExternalDataPtr
   */
  DataView * createView( const std::string& path,
                         TypeID type,
                         int ndims,
                         SidreLength * shape,
                         void * external_ptr );
  /*!
   * \brief Create View object with given name or path in this Group that
   * is described by a Conduit DataType object and attach externally-owned
   * data to it.
   *
   * This method is equivalent to:
   * group->createView(name, dtype)->setExternalDataPtr(external_ptr), or
   * group->createView(name)->setExternalDataPtr(external_ptr)->apply(dtype).
   *
   * \return pointer to new View object or ATK_NULLPTR if one is not created.
   *
   * \sa DataView::attachBuffer
   */
  DataView * createView( const std::string& path,
                         const DataType& dtype,
                         void * external_ptr );

//@}


//@{
//!  @name Methods to create a View and allocate data for it.
//!
//! Each of these methods is a no-op if the given View name is an
//! empty string or the Group already has a View with given name or path.
//!
//! Additional conditions under which a method can be a no-op are described
//! for each method.

  /*!
   * \brief Create View object with given name or path in this Group that
   * has a data description with data type and number of elements and
   * allocate data for it.
   *
   * If given data type is undefined, or given number of elements is < 0,
   * method is a no-op.
   *
   * This is equivalent to: createView(name)->allocate(type, num_elems), or
   * createView(name, type, num_elems)->allocate()
   *
   * \return pointer to new View object or ATK_NULLPTR if one is not created.
   *
   * \sa DataView::allocate
   */
  DataView * createViewAndAllocate( const std::string& path,
                                    TypeID type,
                                    SidreLength num_elems );

  /*!
   * \brief Create View object with given name or path in this Group that
   * has a data description with data type and shape and allocate data for it.
   *
   * If given data type is undefined, or given number of dimensions is < 0,
   * or given shape ptr is null, method is a no-op.
   *
   * This method is equivalent to:
   * group->createView(name)->allocate(type, ndims, shape), or
   * createView(name, type, ndims, shape)->allocate().
   *
   * \return pointer to new View object or ATK_NULLPTR if one is not created.
   *
   * \sa DataView::allocate
   */
  DataView * createViewAndAllocate( const std::string& path,
                                    TypeID type,
                                    int ndims,
                                    SidreLength * shape );

  /*!
   * \brief Create View object with given name or path in this Group that
   * is described by a Conduit DataType object and allocate data for it.
   *
   * This method is equivalent to:
   * group->createView(name)->allocate(dtype), or
   * group->createView(name, dtype)->allocate().
   *
   * If given data type object is empty, data will not be allocated.
   *
   * \return pointer to new View object or ATK_NULLPTR if one is not created.
   *
   * \sa DataView::allocate
   */
  DataView * createViewAndAllocate( const std::string& path,
                                    const DataType& dtype);

  /*!
   * \brief Create View object with given name or path in this Group
   * set its data to given scalar value.
   *
   * This is equivalent to: createView(name)->setScalar(value);
   *
   * If given data type object is empty, data will not be allocated.
   *
   * \return pointer to new View object or ATK_NULLPTR if one is not created.
   *
   * \sa DataView::setScalar
   */
  template<typename ScalarType>
  DataView * createViewScalar( const std::string& path, ScalarType value)
  {
    DataView * view = createView(path);
    if (view != ATK_NULLPTR)
    {
      view->setScalar(value);
    }

    return view;
  }

  /*!
   * \brief Create View object with given name or path in this Group
   * set its data to given string.
   *
   * This is equivalent to: createView(name)->setString(value);
   *
   * If given data type object is empty, data will not be allocated.
   *
   * \return pointer to new View object or ATK_NULLPTR if one is not created.
   *
   * \sa DataView::setString
   */
  DataView * createViewString( const std::string& path,
                               const std::string& value);

//@}


//@{
//!  @name View destruction methods.
//!
//! Each of these methods is a no-op if the specified View does not exist.

  /*!
   * \brief Destroy View with given name or path owned by this Group, but leave
   * its data intect.
   */
  void destroyView(const std::string& path);

  /*!
   * \brief Destroy View with given index owned by this Group, but leave
   * its data intect.
   */
  void destroyView(IndexType idx);

  /*!
   * \brief Destroy all Views owned by this Group, but leave all their
   *        data intact.
   */
  void destroyViews();

  /*!
   * \brief Destroy View with given name or path owned by this Group and deallocate
   * its data if it's the only View associated with that data.
   */
  void destroyViewAndData(const std::string& path);

  /*!
   * \brief Destroy View with given index owned by this Group and deallocate
   * its data if it's the only View associated with that data.
   */
  void destroyViewAndData(IndexType idx);

  /*!
   * \brief Destroy all Views owned by this Group and deallocate
   * data for each View when it's the only View associated with that data.
   */
  void destroyViewsAndData();

//@}


//@{
//!  @name View move and copy methods.

  /*!
   * \brief Remove given View object from its owning Group and move it
   *        to this Group.
   *
   * If given View pointer is null or Group already has a View with
   * same name as given View, method is a no-op.
   *
   * \return pointer to given argument View object or ATK_NULLPTR if View
   * is not moved into this Group.
   */
  DataView * moveView(DataView * view);

  /*!
   * \brief Create a copy of given View object and add it to this Group.
   *
   * Note that View copying is a "shallow" copy; the data associated with
   * the View is not copied. The new View object is associated with
   * the same data as the original.
   *
   * If given Group pointer is null or Group already has a child Group with
   * same name as given Group, method is a no-op.
   *
   * \return pointer to given argument Group object or ATK_NULLPTR if Group
   * is not moved into this Group.
   */
  DataView * copyView(DataView * view);

//@}


//@{
//!  @name Child Group query methods.

  /*!
   * \brief Return true if this Group has a descendant Group with given
   * name or path; else false.
   */
  bool hasGroup( const std::string& path ) const;

  /*!
   * \brief Return true if this Group has a child Group with given
   * name; else false.
   */
  bool hasChildGroup( const std::string& name ) const
  {
    return m_group_coll.hasItem(name);
  }

  /*!
   * \brief Return true if Group has an immediate child Group
   *        with given index; else false.
   */
  bool hasGroup( IndexType idx ) const
  {
    return m_group_coll.hasItem(idx);
  }

  /*!
   * \brief Return the index of immediate child Group with given name.
   *
   *        If no such child Group exists, return sidre::InvalidIndex;
   */
  IndexType getGroupIndex(const std::string& name) const
  {
    SLIC_CHECK_MSG(hasChildGroup(name),
                   "Group " << this->getName() <<
                   " has no child Group with name '" << name << "'");

    return m_group_coll.getItemIndex(name);
  }

  /*!
   * \brief Return the name of immediate child Group with given index.
   *
   *        If no such child Group exists, return sidre::InvalidName.
   */
  const std::string& getGroupName(IndexType idx) const
  {
    SLIC_CHECK_MSG(hasGroup(idx),
                   "Group " << this->getName() <<
                   " has no child Group with index " << idx);

    return m_group_coll.getItemName(idx);
  }

//@}


//@{
//!  @name Group access and iteration methods.

  /*!
   * \brief Return pointer to non-const child Group with given name or path.
   *
   * This method requires that all Groups in the path exist if a path is given.
   *
   * If no such Group exists, ATK_NULLPTR is returned.
   */
  DataGroup * getGroup( const std::string& path );

  /*!
   * \brief Return pointer to const child Group with given name or path.
   *
   * This method requires that all Groups in the path exist if a path is given.
   *
   * If no such Group exists, ATK_NULLPTR is returned.
   */
  DataGroup const * getGroup( const std::string& path ) const;

  /*!
   * \brief Return pointer to non-const immediate child Group with given index.
   *
   * If no such Group exists, ATK_NULLPTR is returned.
   */
  DataGroup * getGroup( IndexType idx )
  {
    SLIC_CHECK_MSG(hasGroup(idx),
                   "Group " << this->getName() <<
                   " has no child Group with index " << idx);

    return m_group_coll.getItem(idx);
  }

  /*!
   * \brief Return pointer to const immediate child Group with given index.
   *
   * If no such Group exists, ATK_NULLPTR is returned.
   */
  const DataGroup * getGroup( IndexType idx ) const
  {
    SLIC_CHECK_MSG(hasGroup(idx),
                   "Group " << this->getName() <<
                   " has no child Group with index " << idx);

    return m_group_coll.getItem(idx);
  }

  /*!
   * \brief Return first valid child Group index (i.e., smallest
   *        index over all child Groups).
   *
   * sidre::InvalidIndex is returned if Group has no child Groups.
   */
  IndexType getFirstValidGroupIndex() const
  {
    return m_group_coll.getFirstValidIndex();
  }

  /*!
   * \brief Return next valid child Group index after given index
   *        (i.e., smallest index over all child Group indices larger
   *        than given one).
   *
   * sidre::InvalidIndex is returned if there is no valid index greater
   * than given one.
   */
  IndexType getNextValidGroupIndex(IndexType idx) const
  {
    return m_group_coll.getNextValidIndex(idx);
  }

//@}


//@{
//!  @name Child Group creation and destruction methods.

  /*!
   * \brief Create a child Group within this Group with given name or path.
   *
   * If name is an empty string or Group already has a child Group with
   * given name or path, method is a no-op.
   *
   * \return pointer to created DataGroup object or ATK_NULLPTR if new
   * Group is not created.
   */
  DataGroup * createGroup( const std::string& path );

  /*!
   * \brief Destroy child Group in this Group with given name or path.
   *
   * If no such Group exists, method is a no-op.
   */
  void destroyGroup(const std::string& path);

  /*!
   * \brief Destroy child Group within this Group with given index.
   *
   * If no such Group exists, method is a no-op.
   */
  void destroyGroup(IndexType idx);

  /*!
   * \brief Destroy all child Groups in this Group.
   *
   * Note that this will recursively destroy entire Group sub-tree below
   * this Group.
   */
  void destroyGroups();

//@}


//@{
//!  @name Group move and copy methods.

  /*!
   * \brief Remove given Group object from its parent Group and make it
   *        a child of this Group.
   *
   * If given Group pointer is null or Group already has a child Group with
   * same name as given Group, method is a no-op.
   *
   * \return pointer to given argument Group object or ATK_NULLPTR if Group
   * is not moved into this Group.
   */
  DataGroup * moveGroup(DataGroup * group);

  /*!
   * \brief Create a copy of Group hierarchy rooted at given Group and make it
   *        a child of this Group.
   *
   * Note that all Views in the Group hierarchy are copied as well.
   *
   * Note that Group copying is a "shallow" copy; the data associated
   * with Views in a Group are not copied. In particular, the new Group
   * hierachy and all its Views is associated with the same data as the
   * given Group.
   *
   * If given Group pointer is null or Group already has a child Group with
   * same name as given Group, method is a no-op.
   *
   * \return pointer to given argument Group object or ATK_NULLPTR if Group
   * is not moved into this Group.
   */
  DataGroup * copyGroup(DataGroup * group);

//@}


//@{
//!  @name Group print methods.

  /*!
   * \brief Print JSON description of data Group to stdout.
   *
   * Note that this will recursively print entire Group sub-tree
   * starting at this Group object.
   */
  void print() const;

  /*!
   * \brief Print JSON description of data Group to an ostream.
   *
   * Note that this will recursively print entire Group sub-tree
   * starting at this Group object.
   */
  void print(std::ostream& os) const;


  /*!
   * \brief Print given number of levels of Group sub-tree
   *        starting at this Group object to an output stream.
   */
  void printTree( const int nlevels, std::ostream& os ) const;

//@}

  /*!
   * \brief Copy description of Group hierarchy rooted at this Group to
   * given Conduit node.
   *
   * The description includes Views of this Group and all of its children
   * recursively.
   */
  void copyToConduitNode(Node& n) const;

  /*!
   * \brief Copy data Group native layout to given Conduit node.
   *
   * The native layout is a Conduit Node hierarchy that maps the Conduit Node data
   * externally to the Sidre View data so that it can be filled in from the data
   * in the file (independent of file format) and can be accessed as a Conduit tree.
   */
  void createNativeLayout(Node& n) const;

  /*!
   * \brief Copy data Group native layout to given Conduit node.
   *
   * The native layout is a Conduit Node hierarchy that maps the Conduit Node data
   * externally to the Sidre View data so that it can be filled in from the data
   * in the file (independent of file format) and can be accessed as a Conduit tree.
   *
   * Only the Views which have external data are added to the node.
   */
  void createExternalLayout(Node& n) const;


  /*!
   * \brief Return true if this Group is equivalent to given Group; else false.
   *
   * Two Groups are equivalent if they are the root Groups of identical
   * Group hierarchy structures with the same names for all Views and
   * Groups in the hierarchy, and the Views are also equivalent.
   *
   * \sa DataView::isEquivalentTo
   */
  bool isEquivalentTo(const DataGroup * other) const;


  /*!
   *@{
   * @name    Group I/O methods
   *   These methods save and load Group trees to and from files.
   *   This includes the views and buffers used in by groups in the tree.
   *   We provide several "protocol" options:
   *
   *   protocols:
   *    sidre_hdf5 (default)
   *    sidre_conduit_json
   *    sidre_json
   *
   *    conduit_hdf5
   *    conduit_bin
   *    conduit_json
   *    json
   *
   *   There are two overloaded versions for each of save, load, and
   *   loadExternalData.  The first of each takes a file path and is intended
   *   for use in a serial context and can be called directly using any
   *   of the supported protocols.  The second takes an hdf5 handle that
   *   has previously been created by the calling code.  These mainly exist
   *   to handle parallel I/O calls from the SPIO component.  They can only
   *   take the sidre_hdf5 or conduit_hdf5 protocols.
   */

  /*!
   * \brief Save the Group to a file.
   *
   *  Saves the tree starting at this group and the buffers used by the views
   *  in this tree.
   *
   *  \param path      file path
   *  \param protocol  I/O protocol
   */
  void save( const std::string& path,
             const std::string& protocol = "sidre_hdf5") const;

  /*!
   * \brief Save the Group to an hdf5 handle.
   *
   * \param h5_id      hdf5 handle
   * \param protocol   I/O protocol sidre_hdf5 or conduit_hdf5
   */
  void save( const hid_t& h5_id,
             const std::string &protocol = "sidre_hdf5") const;


  /*!
   * \brief Load the Group from a file.
   *
   * \param path      file path
   * \param protocol  I/O protocol
   */
  void load(const std::string& path,
            const std::string& protocol = "sidre_hdf5");

  /*!
   * \brief Load the Group from an hdf5 handle.
   * \param h5_id      hdf5 handle
   * \param protocol   I/O protocol sidre_hdf5 or conduit_hdf5
   */
  void load( const hid_t& h5_id,
             const std::string &protocol = "sidre_hdf5");


  /*!
   * \brief Load data into the Group's external views from a file.
   *
   * No protocol argument is needed, as this only is used with the sidre_hdf5
   * protocol.
   *
   * \param path      file path
   */
  void loadExternalData(const std::string& path);

  /*!
   * \brief Load data into the Group's external views from a hdf5 handle.
   *
   * No protocol argument is needed, as this only is used with the sidre_hdf5
   * protocol.
   *
   * \param h5_id      hdf5 handle
   */
  void loadExternalData(const hid_t& h5_id);


private:
  DISABLE_DEFAULT_CTOR(DataGroup); 
  DISABLE_COPY_AND_ASSIGNMENT(DataGroup); 
  DISABLE_MOVE_AND_ASSIGNMENT(DataGroup); 

//@{
//!  @name Private Group ctors and dtors
//!        (callable only by DataStore and DataGroup methods).

  /*!
   *  \brief Private ctor that creates a Group with given name
   *         in given parent Group.
   */
  DataGroup(const std::string& name, DataGroup * parent);

  /*!
   *  \brief Private ctor that creates a Group with given name
   *         in the given DataStore root Group.
   */
  DataGroup(const std::string& name, DataStore * datastore);

  /*!
   * \brief Destructor destroys all Views and child Groups.
   */
  ~DataGroup();

//@}


//@{
//!  @name Private Group View manipulation methods.

  /*!
   * \brief Attach View object to this Group.
   */
  DataView * attachView(DataView * view);

  /*!
   * \brief Detach View object from this Group.
   */
  DataView * detachView(const DataView * view)
  {
    return detachView(view->getName());
  }

  /*!
   * \brief Detach View with given name from this Group.
   */
  DataView * detachView(const std::string& name);

  /*!
   * \brief Detach View with given index from this Group.
   */
  DataView * detachView(IndexType idx);

  /*!
   * \brief Destroy View and its data if its data is not shared with any
   * other View.
   *
   * Data will not be destroyed as long as a View still exists that
   * references it.
   *
   * IMPORTANT: this method assumes View is owned by this Group.
   */
  void destroyViewAndData( DataView * view );

//@}


//@{
//!  @name Private (child) Group manipulation methods.

  /*!
   * \brief Attach Group to this Group as a child.
   */
  DataGroup * attachGroup(DataGroup * group);

  /*!
   * \brief Detaich child Group with given name from this Group.
   */
  DataGroup * detachGroup(const std::string& name);

  /*!
   * \brief Detaich child Group with given index from this Group.
   */
  DataGroup * detachGroup(IndexType idx);

//@}


//@{
//!  @name Private DataGroup methods for interacting with Conduit Nodes.


  /*!
   * \brief Private method to copy DataGroup to Conduit Node.
   *
   * Note: This is for the "sidre_hdf5" protocol.
   */
  void exportTo(conduit::Node& result) const;

  /*!
   * \brief Private method to copy DataGroup to Conduit Node.
   *
   * \param buffer_indices Used to track what Buffers are referenced
   * by the Views in this Group and Groups in the sub-tree below it.
   */
  void exportTo(conduit::Node& data_holder,
                std::set<IndexType>& buffer_indices) const;

  /*!
   * \brief Private method to build a Group hierarchy from Conduit Node.
   *
   * Note: This is for the "sidre_{zzz}" protocols.
   */
  void importFrom(conduit::Node& node);

  /*!
   * \brief Private method to copy DataGroup from Conduit Node.
   *
   * Map of Buffer indices tracks old Buffer ids in the file to the
   * new Buffer ids in the datastore.  Buffer ids are not guaranteed
   * to remain the same when a tree is restored.
   *
   */
  void importFrom(conduit::Node& node,
                  const std::map<IndexType, IndexType>& buffer_id_map);


  /*!
   * \brief Private method to build a Group hierarchy from Conduit Node.
   *
   * Note: This is for the "conduit_{zzz}" protocols.
   */
  void importConduitTree(conduit::Node& node);


//@}

  /*!
   * \brief Private method that returns the Group that is the next-to-last
   * entry in a slash-deliminated ("/") path string.
   *
   * The string before the last "/" character, if there is one, is the
   * next-to-last path entry. In this case, the return value is that Group
   * in the path.
   *
   * If there is no "/" in the given path, the entire string is considered
   * the next-to-last path entry. In this case, the return value is this
   * Group.
   *
   * The path argument is modified while walking the path. Its value when
   * the method returns is the last entry in the path, either the string
   * following the last "/" in the input (if there is one) or the entire
   * input path string if it contains no "/".
   */
  DataGroup * walkPath(std::string& path, bool create_groups_in_path );

  /*!
   * \brief Const private method that returns the Group that is the
   * next-to-last entry in a delimited path string.
   *
   * The path argument is modified while walking the path. Its value when
   * the method returns is the last entry in the path, either the string
   * following the last "/" in the input (if there is one) or the entire
   * input path string if it contains no "/".
   */
  const DataGroup * walkPath(std::string& path ) const;

  /// Name of this DataGroup object.
  std::string m_name;

  /// Parent DataGroup of this DataGroup object.
  DataGroup * m_parent;

  /// This DataGroup object lives in the tree of this DataStore object.
  DataStore * m_datastore;

  /// Character used to denote a path string passed to get/create calls.
  static const char s_path_delimiter;

  ///
  /// Typedefs for View and child Group containers. They are here to
  /// avoid propagating specific type names in the DataGroup class
  /// implementation when we experiment with different containers.
  ///
  ///////////////////////////////////////////////////////////////////
  //
  // Associative container options
  //
  // To try a different container, set the "MapType" typedef to
  // what you want.  Note: only one typedef should be active!!!
  //
  // Current options are std::unordered_map and google::dense_hash_map
  // from Sparsehash library
  ///
  // typedef std::map<std::string, IndexType> MapType;
  ///
#if defined(ATK_USE_UNORDERED_MAP)
  typedef std::unordered_map<std::string, IndexType> MapType;
#else
#if defined(ATK_USE_SPARSEHASH)
  typedef google::dense_hash_map<std::string, IndexType> MapType;
#endif
#endif

  //
  ///////////////////////////////////////////////////////////////////

  ///////////////////////////////////////////////////////////////////
  // Collection options (API between DataGroup and assoc. container)
  ///////////////////////////////////////////////////////////////////
  //
#if defined(USE_NEW_MAP_COLLECTION)
  ///////////////////////////////////////////////////////////////////
  // Improved implementation (index-item association constant as long
  // as item is in collection, but holes in index sequence)
  ///////////////////////////////////////////////////////////////////
  //
  typedef NewMapCollection<DataView, MapType> DataViewCollection;
  //
  typedef NewMapCollection<DataGroup, MapType> DataGroupCollection;
  ///////////////////////////////////////////////////////////////////
#else
  ///////////////////////////////////////////////////////////////////
  // Original implementation (no holes in index sequence)
  ///////////////////////////////////////////////////////////////////
  //
  typedef MapCollection<DataView, MapType> DataViewCollection;
  //
  typedef MapCollection<DataGroup, MapType> DataGroupCollection;
  //
  ///////////////////////////////////////////////////////////////////
  //
#endif

  /// Collection of Views
  DataViewCollection m_view_coll;

  /// Collection of child Groups
  DataGroupCollection m_group_coll;

};


} /* end namespace sidre */
} /* end namespace asctoolkit */

#endif /* DATAGROUP_HPP_ */
