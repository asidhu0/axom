// Copyright (c) 2017-2021, Lawrence Livermore National Security, LLC and
// other Axom Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

#ifndef SLAM_IA_IMPL_H_
#define SLAM_IA_IMPL_H_

/*
 * \file IA_impl.hpp
 *
 * \brief Contains the implementation of the IAMesh class and helper functions
 */

#include "axom/core/Macros.hpp"
#include "axom/slam/policies/SizePolicies.hpp"
#include "axom/slam/ModularInt.hpp"

#include "axom/fmt.hpp"

#include <vector>
#include <map>

namespace axom
{
namespace slam
{
/**
 * Helper function on std::vectors
 */

namespace /*anonymous*/
{
// Checks if \a v is in the list \a s
template <typename T, typename IterableT>
bool is_subset(T v, const IterableT& iterable)
{
  for(auto item : iterable)
  {
    if(item == v)
    {
      return true;
    }
  }
  return false;
}

// Formatted output of a relation or map to an array of strings
// helper function for IAMesh::print_all
template <typename RelOrMap, typename SetType>
std::vector<std::string> entries_as_vec(const RelOrMap& outer, const SetType& s)
{
  const int sz = outer.size();
  std::vector<std::string> strs(sz);
  for(auto pos : s.positions())
  {
    strs[pos] = s.isValidEntry(pos)
      ? axom::fmt::format("{}: {}", pos, outer[pos])
      : axom::fmt::format("{}: {{}}", pos);
  }

  return strs;
}

}  //end anonymous namespace

template <unsigned int TDIM, unsigned int SDIM, typename P>
typename IAMesh<TDIM, SDIM, P>::ElementAndFaceIdxType
IAMesh<TDIM, SDIM, P>::ElemNbrFinder(V2EMapType& vertpair_to_elem_map,
                                     IndexType element_i,
                                     IndexType side_i)
{
  // NOTE: V2EMapType maps a sorted tuple of vertex IDs to a face on a given
  //       mesh element. It is used to find the element index of the opposite
  //       face within the mesh

  IndexArray vlist = getElementFace(element_i, side_i);
  std::sort(vlist.begin(), vlist.end());

  ElementAndFaceIdxType zs_pair(element_i, side_i);

  auto map_ret2 = vertpair_to_elem_map.insert(std::make_pair(vlist, zs_pair));
  if(!map_ret2.second)  //if this pair is in the map, we've found our match
  {
    auto orig_pair = map_ret2.first->second;
    vertpair_to_elem_map.erase(map_ret2.first);
    return orig_pair;
  }

  //No matching pair is found. Return an invalid pair
  return ElementAndFaceIdxType(-1, -1);
}

template <unsigned int TDIM, unsigned int SDIM, typename P>
void IAMesh<TDIM, SDIM, P>::print_all() const
{
  axom::fmt::memory_buffer out;
  axom::fmt::format_to(out,
                       "IA mesh: {} mesh in {}d with {} valid vertices (of {}) "
                       "and {} valid elements (of {})\n",
                       (TDIM == 2 ? "triangle" : "tetrahedral"),
                       SDIM,
                       vertex_set.numberOfValidEntries(),
                       vertex_set.size(),
                       element_set.numberOfValidEntries(),
                       element_set.size());

  //print out the element and vertex sets
  axom::fmt::format_to(out,
                       "  element_set ({}/{}): [{}]\n",
                       element_set.numberOfValidEntries(),
                       element_set.size(),
                       axom::fmt::join(element_set, ", "));

  axom::fmt::format_to(out,
                       "  vertex_set ({}/{}): [{}]\n",
                       vertex_set.numberOfValidEntries(),
                       vertex_set.size(),
                       axom::fmt::join(vertex_set, ", "));

  //print out the relations on the sets (ev, ve and ee)
  axom::fmt::format_to(
    out,
    "  ev_rel ({}/{}): [{}]\n",
    ev_rel.numberOfValidEntries(),
    ev_rel.size(),
    axom::fmt::join(entries_as_vec(ev_rel, element_set), "; "));

  axom::fmt::format_to(
    out,
    "  ve_rel ({}/{}): [{}]\n",
    ve_rel.numberOfValidEntries(),
    ve_rel.size(),
    axom::fmt::join(entries_as_vec(ve_rel, vertex_set), "; "));

  axom::fmt::format_to(
    out,
    "  ee_rel ({}/{}): [{}]\n",
    ee_rel.numberOfValidEntries(),
    ee_rel.size(),
    axom::fmt::join(entries_as_vec(ee_rel, element_set), "; "));

  //print out the coordinate map (i.e the positions)
  axom::fmt::format_to(
    out,
    "  vertex coord ({}/{}): [{}]\n",
    vcoord_map.numberOfValidEntries(),
    vcoord_map.size(),
    axom::fmt::join(entries_as_vec(vcoord_map, vertex_set), "; "));

  SLIC_INFO(axom::fmt::to_string(out));
}

//-----------------------------------------------------------------------------

template <unsigned int TDIM, unsigned int SDIM, typename P>
IAMesh<TDIM, SDIM, P>::IAMesh()
  : vertex_set(0)
  , element_set(0)
  , ev_rel(&element_set, &vertex_set)
  , ve_rel(&vertex_set, &element_set)
  , ee_rel(&element_set, &element_set)
  , vcoord_map(&vertex_set)
{ }

template <unsigned int TDIM, unsigned int SDIM, typename P>
IAMesh<TDIM, SDIM, P>::IAMesh(const IAMesh& m)
{
  operator=(m);
}

template <unsigned int TDIM, unsigned int SDIM, typename P>
IAMesh<TDIM, SDIM, P>& IAMesh<TDIM, SDIM, P>::operator=(const IAMesh& m)
{
  if(&m != this)
  {
    vertex_set = m.vertex_set;
    element_set = m.element_set;
    ev_rel = ElementBoundaryRelation(&element_set, &vertex_set);
    ve_rel = VertexCoboundaryRelation(&vertex_set, &element_set);
    ee_rel = ElementAdjacencyRelation(&element_set, &element_set);
    vcoord_map = PositionMap(&vertex_set);

    ev_rel.data() = m.ev_rel.data();
    ve_rel.data() = m.ve_rel.data();
    ee_rel.data() = m.ee_rel.data();
    vcoord_map.data() = m.vcoord_map.data();
  }

  return *this;
}

template <unsigned int TDIM, unsigned int SDIM, typename P>
IAMesh<TDIM, SDIM, P>::IAMesh(std::vector<double>& points,
                              std::vector<IndexType>& tri)
  : vertex_set(points.size() / COORDS_PER_VERT)
  , element_set(tri.size() / VERTS_PER_ELEM)
  , ev_rel(&element_set, &vertex_set)
  , ve_rel(&vertex_set, &element_set)
  , ee_rel(&element_set, &element_set)
  , vcoord_map(&vertex_set)
{
  // Relation, element to vertex boundary relation
  for(IndexType idx = 0; idx < element_set.size() * VERTS_PER_ELEM; ++idx)
  {
    ev_rel.insert(idx / VERTS_PER_ELEM, tri[idx]);
  }
  SLIC_ASSERT_MSG(
    ev_rel.isValid(),
    "Error creating (dynamic) relation from elements to vertices!");

  // The map, vertex to coordinates
  for(IndexType idx = 0; idx < vertex_set.size(); ++idx)
  {
    vcoord_map[idx] = Point(&(points[idx * COORDS_PER_VERT]));
  }
  SLIC_ASSERT_MSG(vcoord_map.isValid(true),
                  "Error creating map from vertex to coords!");

  //Vertex element relation. 1->1 mapping only 1 element per vertex.
  for(IndexType zIdx = 0; zIdx < element_set.size(); ++zIdx)
  {
    for(IndexType idx = 0; idx < (int)ev_rel[zIdx].size(); ++idx)
    {
      ve_rel.modify(ev_rel[zIdx][idx], 0, zIdx);
    }
  }
  SLIC_ASSERT_MSG(
    ve_rel.isValid(true),
    "Error creating (dynamic) relation from vertices to elements!\n");

  //Before making element to element relation, construct the data.
  // For every cell, find the union of triangles for each pair of vertices
  IndexArray element_element_vec(element_set.size() * VERTS_PER_ELEM,
                                 ElementBoundaryRelation::INVALID_INDEX);

  V2EMapType vertpair_to_elem_map;

  for(IndexType element_i : element_set)
  {
    for(IndexType side_i = 0; side_i < VERTS_PER_ELEM; side_i++)
    {
      ElementAndFaceIdxType nst =
        ElemNbrFinder(vertpair_to_elem_map, element_i, side_i);

      IndexType other_element_idx = nst.first;
      IndexType other_side_idx = nst.second;

      if(element_set.isValidEntry(other_element_idx))
      {
        int idx0 = element_i * VERTS_PER_ELEM + side_i;
        element_element_vec[idx0] = other_element_idx;

        int idx1 = other_element_idx * VERTS_PER_ELEM + other_side_idx;
        element_element_vec[idx1] = element_i;
      }
    }
  }

  //Element adjacency relation along facets
  for(IndexType i : element_set)
  {
    for(int j = 0; j < VERTS_PER_ELEM; j++)
    {
      ee_rel.modify(i, j, element_element_vec[i * VERTS_PER_ELEM + j]);
    }
  }
  SLIC_ASSERT_MSG(
    ee_rel.isValid(true),
    "Error creating (dynamic) relation from elements to elements!");
}

template <unsigned int TDIM, unsigned int SDIM, typename P>
typename IAMesh<TDIM, SDIM, P>::IndexArray
IAMesh<TDIM, SDIM, P>::getVerticesInElement(IndexType element_idx) const
{
  IndexArray ret;
  if(!ev_rel.isValidEntry(element_idx))
  {
    SLIC_WARNING("Attempting to retrieve data with an invalid element");
    return ret;
  }

  typename ElementBoundaryRelation::RelationSubset rvec = ev_rel[element_idx];
  ret.resize(rvec.size());
  for(int i = 0; i < rvec.size(); i++)
  {
    ret[i] = rvec[i];
  }

  return ret;
}

template <unsigned int TDIM, unsigned int SDIM, typename P>
typename IAMesh<TDIM, SDIM, P>::IndexArray
IAMesh<TDIM, SDIM, P>::getElementsWithVertex(IndexType vertex_idx) const
{
  // reasonable expected size of vertex star in triangle and tet meshes
  constexpr int EXP_SZ = (TDIM == 2) ? 8 : 32;

  IndexArray ret;
  ret.reserve(EXP_SZ);

  if(!ve_rel.isValidEntry(vertex_idx))
  {
    //this vertex is not connected to any elements
    SLIC_WARNING_IF(
      !vertex_set.isValidEntry(vertex_idx),
      "Attempting to retrieve data with an invalid vertex id: " << vertex_idx);
    return ret;
  }

  IndexType starting_element_idx = ve_rel[vertex_idx][0];

  ret.push_back(starting_element_idx);
  IndexArray element_traverse_queue;
  element_traverse_queue.push_back(starting_element_idx);

  while(!element_traverse_queue.empty())
  {
    IndexType element_i = element_traverse_queue.back();
    element_traverse_queue.pop_back();

    for(auto nbr : ee_rel[element_i])
    {
      // If nbr is valid, has not already been found and contains the vertex in question,
      // add it and enqueue to check neighbors.
      if(element_set.isValidEntry(nbr)  //
         && !is_subset(nbr, ret)        //
         && is_subset(vertex_idx, ev_rel[nbr]))
      {
        ret.push_back(nbr);
        element_traverse_queue.push_back(nbr);
      }
    }
  }

  return ret;
}

template <unsigned int TDIM, unsigned int SDIM, typename P>
typename IAMesh<TDIM, SDIM, P>::IndexArray IAMesh<TDIM, SDIM, P>::getElementFace(
  IndexType element_idx,
  IndexType face_idx) const
{
  constexpr int VERTS_PER_FACET = VERTS_PER_ELEM - 1;

  using CTSize = slam::policies::CompileTimeSize<IndexType, VERTS_PER_ELEM>;
  slam::ModularInt<CTSize> mod_face(face_idx);

  IndexArray ret;
  ret.reserve(VERTS_PER_FACET);

  if(!element_set.isValidEntry(element_idx))
  {
    SLIC_WARNING(
      "Attempting to retrieve data with an invalid element: " << element_idx);

    return ret;
  }

  SLIC_ASSERT_MSG(0 <= face_idx && face_idx < VERTS_PER_ELEM,
                  "Face index is invalid.");

  auto ev = ev_rel[element_idx];
  for(int i = 0; i < VERTS_PER_FACET; ++i)
  {
    ret.push_back(ev[mod_face + i]);
  }

  return ret;
}

template <unsigned int TDIM, unsigned int SDIM, typename P>
typename IAMesh<TDIM, SDIM, P>::IndexArray
IAMesh<TDIM, SDIM, P>::getElementNeighbors(IndexType element_idx) const
{
  IndexArray ret;

  if(!ee_rel.isValidEntry(element_idx))
  {
    //this element is invalid
    SLIC_WARNING("Attempting to retrieve data with an invalid element.");
    return ret;
  }

  auto rvec = ee_rel[element_idx];
  ret.resize(rvec.size());
  for(int i = 0; i < rvec.size(); i++)
  {
    ret[i] = rvec[i];
  }

  return ret;
}

template <unsigned int TDIM, unsigned int SDIM, typename P>
const typename IAMesh<TDIM, SDIM, P>::Point& IAMesh<TDIM, SDIM, P>::getVertexPoint(
  IndexType vertex_idx) const
{
  SLIC_ASSERT(isValidVertexEntry(vertex_idx));

  return vcoord_map[vertex_idx];
}

template <unsigned int TDIM, unsigned int SDIM, typename P>
void IAMesh<TDIM, SDIM, P>::removeVertex(IndexType vertex_idx)
{
  if(!vertex_set.isValidEntry(vertex_idx))
  {
    SLIC_WARNING("Attempting to remove an invalid vertex");
    return;
  }

  //check if any element uses this vertex. If so, remove them too.
  for(auto attached_element : getElementsWithVertex(vertex_idx))
  {
    removeElement(attached_element);
  }

  vertex_set.remove(vertex_idx);
  ve_rel.remove(vertex_idx);
  //Note: once the set entry is removed, its corresponding
  // map entry is assumed to be invalid
}

template <unsigned int TDIM, unsigned int SDIM, typename P>
void IAMesh<TDIM, SDIM, P>::removeElement(IndexType element_idx)
{
  if(!element_set.isValidEntry(element_idx))
  {
    SLIC_WARNING("Attempting to remove an invalid element");
    return;
  }

  // update vertex coboundary relation for vertices of the removed cell (when necessary)
  for(auto vertex_i : ev_rel[element_idx])
  {
    // update VE relation for vertex_i when it points to deleted element
    if(ve_rel[vertex_i][0] == element_idx)
    {
      IndexType new_elem = ElementSet::INVALID_ENTRY;
      for(auto nbr : ee_rel[element_idx])
      {
        // update to a valid neighbor that is incident in vertex_i
        if(element_set.isValidEntry(nbr) && is_subset(vertex_i, ev_rel[nbr]))
        {
          new_elem = nbr;
          break;
        }
      }
      ve_rel.modify(vertex_i, 0, new_elem);
    }
  }

  //erase this element and it boundary relation
  element_set.remove(element_idx);
  ev_rel.remove(element_idx);

  //erase neighbor element's adjacency data pointing to deleted element
  for(auto nbr : ee_rel[element_idx])
  {
    if(!element_set.isValidEntry(nbr)) continue;
    for(auto it = ee_rel[nbr].begin(); it != ee_rel[nbr].end(); ++it)
    {
      if(*it == element_idx)
      {
        ee_rel.modify(nbr, it.index(), ElementBoundaryRelation::INVALID_INDEX);
        break;
      }
    }
  }
  ee_rel.remove(element_idx);
}

template <unsigned int TDIM, unsigned int SDIM, typename P>
typename IAMesh<TDIM, SDIM, P>::IndexType IAMesh<TDIM, SDIM, P>::addVertex(
  const Point& p)
{
  IndexType vertex_idx = vertex_set.insert();
  vcoord_map.insert(vertex_idx, p);
  ve_rel.insert(vertex_idx, VertexCoboundaryRelation::INVALID_INDEX);

  return vertex_idx;
}

template <unsigned int TDIM, unsigned int SDIM, typename P>
typename IAMesh<TDIM, SDIM, P>::IndexType IAMesh<TDIM, SDIM, P>::addElement(
  IndexType v0,
  IndexType v1,
  IndexType v2,
  IndexType v3)
{
  SLIC_ASSERT(VERTS_PER_ELEM <= 4);
  IndexType vlist[] = {v0, v1, v2, v3};
  return addElement(vlist);
}

template <unsigned int TDIM, unsigned int SDIM, typename P>
typename IAMesh<TDIM, SDIM, P>::IndexType IAMesh<TDIM, SDIM, P>::addElement(
  const IndexType* vlist)
{
  // Implementation note:
  //   This function reconstructs the vertex-element relation
  //    on each vertex ID of the new element
  // Can we optimize this function?

  for(int i = 0; i < VERTS_PER_ELEM; i++)
  {
    if(!vertex_set.isValidEntry(vlist[i]))
      SLIC_WARNING(
        "Trying to add an element with invalid vertex index:" << vlist[i]);
  }

  IndexType element_idx = element_set.insert();

  for(int i = 0; i < VERTS_PER_ELEM; ++i)
  {
    ev_rel.insert(element_idx, vlist[i]);
  }

  //make sure the space is allocated in ee_rel
  ee_rel.insert(element_idx, ElementAdjacencyRelation::INVALID_INDEX);

  V2EMapType vertpair_to_elem_map;

  //First add each face of this new element into the map
  for(int side_i = 0; side_i < VERTS_PER_ELEM; ++side_i)
  {
    ElementAndFaceIdxType zs_pair =
      ElemNbrFinder(vertpair_to_elem_map, element_idx, side_i);
    SLIC_ASSERT(zs_pair.first == -1);
    AXOM_UNUSED_VAR(zs_pair);
  }

  //Make a list of elements that shares at least 1 vertex of the new element
  std::set<IndexType> elem_list;
  for(int n = 0; n < VERTS_PER_ELEM; ++n)
  {
    IndexArray ele_list_short = getElementsWithVertex(vlist[n]);
    for(unsigned int i = 0; i < ele_list_short.size(); ++i)
    {
      elem_list.insert(ele_list_short[i]);
    }
  }

  //Check if any of the elements share a face with the new element.
  // If so, modify ee_rel to reflect that.
  for(auto it = elem_list.begin(); it != elem_list.end(); ++it)
  {
    IndexType otherElementIdx = *it;
    if(otherElementIdx < 0 || otherElementIdx == element_idx) continue;
    for(IndexType s = 0; s < VERTS_PER_ELEM; s++)
    {
      IndexType otherSideIdx = s;
      // insert the pair
      ElementAndFaceIdxType zs_pair =
        ElemNbrFinder(vertpair_to_elem_map, otherElementIdx, otherSideIdx);

      //If zs_pair returned is the new element, save this nbr to set later
      IndexType foundElementIdx = zs_pair.first;
      IndexType foundSideIdx = zs_pair.second;

      if(foundElementIdx == element_idx)
      {
        // if there is already a neighbor on the save list, this mesh is not a
        // manifold.  Example: Having an edge with 3 faces...

        SLIC_ASSERT(ee_rel[otherElementIdx][otherSideIdx] < 0);

        ee_rel.modify(foundElementIdx, foundSideIdx, otherElementIdx);
        ee_rel.modify(otherElementIdx, otherSideIdx, foundElementIdx);

        //put new element pair back in queue to check if mesh is manifold
        ElemNbrFinder(vertpair_to_elem_map, foundElementIdx, foundSideIdx);
      }
    }
  }

  //update ve_rel
  for(int i = 0; i < VERTS_PER_ELEM; i++)
  {
    if(!ve_rel.isValidEntry(vlist[i]))
    {
      ve_rel.modify(vlist[i], 0, element_idx);
    }
  }

  return element_idx;
}

template <unsigned int TDIM, unsigned int SDIM, typename P>
typename IAMesh<TDIM, SDIM, P>::IndexType IAMesh<TDIM, SDIM, P>::addElement(
  const IndexType* vlist,
  const IndexType* neighbors)
{
  for(int i = 0; i < VERTS_PER_ELEM; ++i)
  {
    if(!vertex_set.isValidEntry(vlist[i]))
    {
      SLIC_WARNING(
        "Trying to add an element with invalid vertex index:" << vlist[i]);
    }
  }

  IndexType element_idx = element_set.insert();

  // set the vertices in this element's ev relation
  for(int i = 0; i < VERTS_PER_ELEM; ++i)
  {
    ev_rel.modify(element_idx, i, vlist[i]);
  }

  // set the neighbor elements in this element's ee relation
  for(int i = 0; i < VERTS_PER_ELEM; ++i)
  {
    ee_rel.modify(element_idx, i, neighbors[i]);
  }

  // update ve relation of this element's vertices, if necessary
  for(int i = 0; i < VERTS_PER_ELEM; ++i)
  {
    const IndexType v = vlist[i];
    const IndexType e = ve_rel[v][0];

    if(!element_set.isValidEntry(e))
    {
      ve_rel[v][0] = element_idx;
    }
  }

  return element_idx;
}

template <unsigned int TDIM, unsigned int SDIM, typename P>
void IAMesh<TDIM, SDIM, P>::fixVertexNeighborhood(
  IndexType vertex_idx,
  const std::vector<IndexType>& new_elements)
{
  using IndexPairType = std::pair<IndexType, IndexType>;
  using FaceVertMapType = std::map<IndexArray, IndexPairType>;
  using FaceVertPairType = std::pair<IndexArray, IndexPairType>;
  FaceVertMapType fv_map;

  for(auto el : new_elements)
  {
    for(int face_i = 0; face_i < VERTS_PER_ELEM; ++face_i)
    {
      IndexArray fv_list = getElementFace(el, face_i);

      //sort vertices on this face
      std::sort(fv_list.begin(), fv_list.end());

      if(!is_subset(vertex_idx, fv_list))  // Update boundary facet of star
      {
        // update neighbor along this facet to point to current element
        const IndexType nbr = ee_rel[el][face_i];

        if(element_set.isValidEntry(nbr))
        {
          // figure out which face this is on the neighbor
          // TODO: Make this more efficient
          auto nbr_ee = ee_rel[nbr];
          for(int face_j = 0; face_j < VERTS_PER_ELEM; ++face_j)
          {
            if(!element_set.isValidEntry(nbr_ee[face_j]))
            {
              IndexArray nbr_facet_verts = getElementFace(nbr, face_j);
              std::sort(nbr_facet_verts.begin(), nbr_facet_verts.end());
              if(nbr_facet_verts == fv_list)
              {
                ee_rel.modify(nbr, face_j, el);
              }
            }
          }
        }
      }
      else  // update internal facet of star
      {
        std::pair<FaceVertMapType::iterator, bool> ret =
          fv_map.insert(FaceVertPairType(fv_list, IndexPairType(el, face_i)));

        if(!ret.second)  //found a matching face
        {
          const IndexType nbr_elem = ret.first->second.first;
          const IndexType nbr_face_i = ret.first->second.second;

          ee_rel.modify(el, face_i, nbr_elem);
          ee_rel.modify(nbr_elem, nbr_face_i, el);
        }
      }
    }
  }
}

/* Remove all the invalid entries in the IA structure*/
template <unsigned int TDIM, unsigned int SDIM, typename P>
void IAMesh<TDIM, SDIM, P>::compact()
{
  constexpr IndexType INVALID_VERTEX = VertexSet::INVALID_ENTRY;
  constexpr IndexType INVALID_ELEMENT = ElementSet::INVALID_ENTRY;

  //Construct an array that maps original set indices to new compacted indices
  IndexArray vertex_set_map(vertex_set.size(), INVALID_VERTEX);
  IndexArray element_set_map(element_set.size(), INVALID_ELEMENT);

  int v_count = 0;
  for(auto v : vertex_set.positions())
  {
    if(vertex_set.isValidEntry(v))
    {
      vertex_set_map[v] = v_count++;
    }
  }

  int e_count = 0;
  for(auto e : element_set.positions())
  {
    if(element_set.isValidEntry(e))
    {
      element_set_map[e] = e_count++;
    }
  }

  //update the EV boundary relation
  for(auto e : element_set.positions())
  {
    const auto new_e = element_set_map[e];
    if(new_e != INVALID_ELEMENT)
    {
      const auto ev_old = ev_rel[e];
      auto ev_new = ev_rel[new_e];
      for(auto i : ev_new.positions())
      {
        const auto old = ev_old[i];
        ev_new[i] =
          (old != INVALID_VERTEX) ? vertex_set_map[old] : INVALID_VERTEX;
      }
    }
  }
  ev_rel.data().resize(e_count * VERTS_PER_ELEM);

  //update the VE coboundary relation
  for(auto v : vertex_set.positions())
  {
    const auto new_v = vertex_set_map[v];
    if(new_v != INVALID_VERTEX)
    {
      // cardinality of VE relation is 1
      const auto old = ve_rel[v][0];
      ve_rel[new_v][0] =
        (old != INVALID_ELEMENT) ? element_set_map[old] : INVALID_ELEMENT;
    }
  }
  ve_rel.data().resize(v_count);

  //update the EE adjacency relation
  for(auto e : element_set.positions())
  {
    int new_e = element_set_map[e];
    if(new_e != INVALID_ELEMENT)
    {
      const auto ee_old = ee_rel[e];
      auto ee_new = ee_rel[new_e];
      for(auto i : ee_new.positions())
      {
        const auto old = ee_old[i];
        ee_new[i] =
          (old != INVALID_ELEMENT) ? element_set_map[old] : INVALID_ELEMENT;
      }
    }
  }
  ee_rel.data().resize(e_count * VERTS_PER_ELEM);

  //Update the coordinate positions map
  for(auto v : vertex_set.positions())
  {
    int new_entry_index = vertex_set_map[v];
    if(new_entry_index != INVALID_VERTEX)
    {
      vcoord_map[new_entry_index] = vcoord_map[v];
    }
  }
  vcoord_map.resize(v_count);

  //update the sets
  vertex_set.reset(v_count);
  element_set.reset(e_count);
}

template <unsigned int TDIM, unsigned int SDIM, typename P>
bool IAMesh<TDIM, SDIM, P>::isEmpty() const
{
  return vertex_set.size() == 0;
}

template <unsigned int TDIM, unsigned int SDIM, typename P>
bool IAMesh<TDIM, SDIM, P>::isManifold(bool verboseOutput) const
{
  /* TODO check that...
   * - Each valid non-boundary entry in the ee_rel should have a counterpart
   *   that points to it.
   * - [More expensive] That we can reconstruct the star of every vertex and
   *   that that the star is either a ball or a half-ball
   * (alternatively, that the link is a sphere or a disk)
   * - Other things that ensures the mesh is manifold?
   */

  std::stringstream errSstr;

  bool bValid = true;

  if(!isValid(verboseOutput)) return false;

  // Each valid vertex should have a valid entry in ve_rel.
  for(IndexType i = 0; i < vertex_set.size(); ++i)
  {
    if(vertex_set.isValidEntry(i) && !ve_rel.isValidEntry(i))
    {
      if(verboseOutput)
      {
        errSstr << "\n\t vertex " << i
                << " is not connected to any elements.\n\t";
      }
      bValid = false;
    }
  }

  if(verboseOutput && !bValid)
  {
    SLIC_DEBUG(errSstr.str());
  }

  return bValid;
}

template <unsigned int TDIM, unsigned int SDIM, typename P>
bool IAMesh<TDIM, SDIM, P>::isValid(bool verboseOutput) const
{
  std::stringstream errSstr;

  bool bValid = true;

  bValid &= vertex_set.isValid(verboseOutput);
  bValid &= element_set.isValid(verboseOutput);
  bValid &= ev_rel.isValid(verboseOutput);
  bValid &= ve_rel.isValid(verboseOutput);
  bValid &= ee_rel.isValid(verboseOutput);
  bValid &= vcoord_map.isValid(verboseOutput);

  //Check that sizes for vertices match
  if(vertex_set.size() != ve_rel.size() || vertex_set.size() != vcoord_map.size())
  {
    if(verboseOutput)
    {
      errSstr << "\n\t vertex set and relation size don't match.\n\t";
      errSstr << "vertex size: " << vertex_set.size() << "\n\t"
              << "ve_rel size: " << ve_rel.size() << "\n\t"
              << "vcoord size: " << vcoord_map.size();
    }
    bValid = false;
  }

  //Check that sizes for elements match
  if(element_set.size() != ev_rel.size() || element_set.size() != ee_rel.size())
  {
    if(verboseOutput)
    {
      errSstr << "\n\t element set and relation size don't match.";
      errSstr << "element_set size: " << element_set.size() << "\n\t"
              << "ev_rel size: " << ev_rel.size() << "\n\t"
              << "ee_rel size: " << ee_rel.size();
    }
    bValid = false;
  }

  //Check that all ev_rel are valid if the element_set is valid.
  for(IndexType pos = 0; pos < element_set.size(); ++pos)
  {
    if(element_set.isValidEntry(pos))
    {
      for(IndexType rpos = 0; rpos < ev_rel[pos].size(); rpos++)
      {
        if(ev_rel[pos][rpos] == ElementBoundaryRelation::INVALID_INDEX)
        {
          if(verboseOutput)
          {
            errSstr
              << "\n\t* Element->Vertex relation contains an invalid entry"
              << " for a valid element \n\t pos: " << pos << ", entry: " << rpos
              << ".";
          }
          bValid = false;
        }
      }
    }
  }

  //Check that valid entries in relation/map map to valid entries in set
  for(IndexType pos = 0; pos < vertex_set.size(); ++pos)
  {
    if(ve_rel.isValidEntry(pos) && !vertex_set.isValidEntry(pos))
    {
      if(verboseOutput)
      {
        errSstr
          << "\n\t * Relation contains a valid entry with an invalid set entry"
          << " at pos " << pos << ".";
      }
      bValid = false;
    }
  }

  if(verboseOutput && !bValid)
  {
    SLIC_DEBUG(errSstr.str());
  }

  return bValid;
}

}  // end namespace slam
}  // end namespace axom

#endif  // SLAM_IA_IMPL_H_
