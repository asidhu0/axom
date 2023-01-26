// wrapBuffer.cpp
// This file is generated by Shroud 0.12.2. Do not edit.
//
// Copyright (c) 2017-2023, Lawrence Livermore National Security, LLC and
// other Axom Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
#include "wrapBuffer.h"
#include "axom/sidre/core/Buffer.hpp"

// splicer begin class.Buffer.CXX_definitions
// splicer end class.Buffer.CXX_definitions

extern "C" {

// splicer begin class.Buffer.C_definitions
// splicer end class.Buffer.C_definitions

SIDRE_IndexType SIDRE_Buffer_get_index(const SIDRE_Buffer *self)
{
  const axom::sidre::Buffer *SH_this =
    static_cast<const axom::sidre::Buffer *>(self->addr);
  // splicer begin class.Buffer.method.get_index
  axom::sidre::IndexType SHC_rv = SH_this->getIndex();
  return SHC_rv;
  // splicer end class.Buffer.method.get_index
}

size_t SIDRE_Buffer_get_num_views(const SIDRE_Buffer *self)
{
  const axom::sidre::Buffer *SH_this =
    static_cast<const axom::sidre::Buffer *>(self->addr);
  // splicer begin class.Buffer.method.get_num_views
  size_t SHC_rv = SH_this->getNumViews();
  return SHC_rv;
  // splicer end class.Buffer.method.get_num_views
}

void *SIDRE_Buffer_get_void_ptr(SIDRE_Buffer *self)
{
  axom::sidre::Buffer *SH_this = static_cast<axom::sidre::Buffer *>(self->addr);
  // splicer begin class.Buffer.method.get_void_ptr
  void *SHC_rv = SH_this->getVoidPtr();
  return SHC_rv;
  // splicer end class.Buffer.method.get_void_ptr
}

SIDRE_TypeIDint SIDRE_Buffer_get_type_id(const SIDRE_Buffer *self)
{
  const axom::sidre::Buffer *SH_this =
    static_cast<const axom::sidre::Buffer *>(self->addr);
  // splicer begin class.Buffer.method.get_type_id
  axom::sidre::TypeID SHCXX_rv = SH_this->getTypeID();
  SIDRE_TypeIDint SHC_rv = static_cast<SIDRE_TypeIDint>(SHCXX_rv);
  return SHC_rv;
  // splicer end class.Buffer.method.get_type_id
}

size_t SIDRE_Buffer_get_num_elements(const SIDRE_Buffer *self)
{
  const axom::sidre::Buffer *SH_this =
    static_cast<const axom::sidre::Buffer *>(self->addr);
  // splicer begin class.Buffer.method.get_num_elements
  size_t SHC_rv = SH_this->getNumElements();
  return SHC_rv;
  // splicer end class.Buffer.method.get_num_elements
}

size_t SIDRE_Buffer_get_total_bytes(const SIDRE_Buffer *self)
{
  const axom::sidre::Buffer *SH_this =
    static_cast<const axom::sidre::Buffer *>(self->addr);
  // splicer begin class.Buffer.method.get_total_bytes
  size_t SHC_rv = SH_this->getTotalBytes();
  return SHC_rv;
  // splicer end class.Buffer.method.get_total_bytes
}

size_t SIDRE_Buffer_get_bytes_per_element(const SIDRE_Buffer *self)
{
  const axom::sidre::Buffer *SH_this =
    static_cast<const axom::sidre::Buffer *>(self->addr);
  // splicer begin class.Buffer.method.get_bytes_per_element
  size_t SHC_rv = SH_this->getBytesPerElement();
  return SHC_rv;
  // splicer end class.Buffer.method.get_bytes_per_element
}

void SIDRE_Buffer_describe(SIDRE_Buffer *self,
                           SIDRE_TypeID type,
                           SIDRE_IndexType num_elems)
{
  axom::sidre::Buffer *SH_this = static_cast<axom::sidre::Buffer *>(self->addr);
  // splicer begin class.Buffer.method.describe
  axom::sidre::TypeID SHCXX_type = static_cast<axom::sidre::TypeID>(type);
  SH_this->describe(SHCXX_type, num_elems);
  // splicer end class.Buffer.method.describe
}

void SIDRE_Buffer_allocate_existing(SIDRE_Buffer *self)
{
  axom::sidre::Buffer *SH_this = static_cast<axom::sidre::Buffer *>(self->addr);
  // splicer begin class.Buffer.method.allocate_existing
  SH_this->allocate();
  // splicer end class.Buffer.method.allocate_existing
}

void SIDRE_Buffer_allocate_from_type(SIDRE_Buffer *self,
                                     SIDRE_TypeID type,
                                     SIDRE_IndexType num_elems)
{
  axom::sidre::Buffer *SH_this = static_cast<axom::sidre::Buffer *>(self->addr);
  // splicer begin class.Buffer.method.allocate_from_type
  axom::sidre::TypeID SHCXX_type = static_cast<axom::sidre::TypeID>(type);
  SH_this->allocate(SHCXX_type, num_elems);
  // splicer end class.Buffer.method.allocate_from_type
}

void SIDRE_Buffer_reallocate(SIDRE_Buffer *self, SIDRE_IndexType num_elems)
{
  axom::sidre::Buffer *SH_this = static_cast<axom::sidre::Buffer *>(self->addr);
  // splicer begin class.Buffer.method.reallocate
  SH_this->reallocate(num_elems);
  // splicer end class.Buffer.method.reallocate
}

void SIDRE_Buffer_print(const SIDRE_Buffer *self)
{
  const axom::sidre::Buffer *SH_this =
    static_cast<const axom::sidre::Buffer *>(self->addr);
  // splicer begin class.Buffer.method.print
  SH_this->print();
  // splicer end class.Buffer.method.print
}

}  // extern "C"
