// Copyright (c) 2017-2020, Lawrence Livermore National Security, LLC and
// other Axom Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

#ifndef AXOM_SPIN_BVH_H_
#define AXOM_SPIN_BVH_H_

// axom core includes
#include "axom/config.hpp"                 // for Axom compile-time definitions
#include "axom/core/Macros.hpp"            // for Axom macros
#include "axom/core/memory_management.hpp" // for memory functions
#include "axom/core/Types.hpp"             // for fixed bitwidth types

#include "axom/core/execution/execution_space.hpp" // for execution spaces
#include "axom/core/execution/for_all.hpp"         // for generic for_all()

// slic includes
#include "axom/slic/interface/slic.hpp"    // for SLIC macros

// spin includes
#include "axom/spin/internal/linear_bvh/aabb.hpp"
#include "axom/spin/internal/linear_bvh/build_radix_tree.hpp"
#include "axom/spin/internal/linear_bvh/bvh_traverse.hpp"
#include "axom/spin/internal/linear_bvh/bvh_vtkio.hpp"
#include "axom/spin/internal/linear_bvh/BVHData.hpp"
#include "axom/spin/internal/linear_bvh/emit_bvh.hpp"
#include "axom/spin/internal/linear_bvh/QueryAccessor.hpp"
#include "axom/spin/internal/linear_bvh/TraversalPredicates.hpp"
#include "axom/spin/internal/linear_bvh/vec.hpp"

// C/C++ includes
#include <fstream>  // for std::ofstream
#include <sstream>  // for std::ostringstream
#include <string>   // for std::string
#include <cstring>  // for memcpy

#if !defined(AXOM_USE_RAJA) || !defined(AXOM_USE_UMPIRE)
#error *** The spin::BVH class requires RAJA and Umpire ***
#endif

// RAJA includes
#include "RAJA/RAJA.hpp"

namespace axom
{
namespace spin
{

/*!
 * \brief Enumerates the list of return codes for various BVH operations.
 */
enum BVHReturnCodes
{
  BVH_BUILD_FAILED=-1, //!< indicates that generation of the BVH failed
  BVH_BUILD_OK,        //!< indicates that the BVH was generated successfully
};


/*!
 * \class BVH
 *
 * \brief Defines a Bounding Volume Hierarchy (BVH) spatial acceleration
 *  data structure over a set of geometric entities.
 *
 * The BVH class provides functionality for generating a hierarchical spatial
 * partitioning over a set of geometric entities. Each entity in the BVH is
 * represented by a bounding volume, in this case an axis-aligned bounding box.
 * Once the BVH structure is generated, it is used to accelerate various spatial
 * queries, such as, collision detection, ray tracing, etc., by reducing the
 * search space for a given operation to an abbreviated list of candidate
 * geometric entities to check for a particular query.
 *
 * \tparam NDIMS the number of dimensions, e.g., 2 or 3.
 * \tparam ExecSpace the execution space to use, e.g. SEQ_EXEC, CUDA_EXEC, etc.
 * \tparam FloatType floating precision, e.g., `double` or `float`. Optional.
 *
 * \note The last template parameter is optional. Defaults to double precision
 *  if not specified.
 *
 * \pre The spin::BVH class requires RAJA and Umpire. For a CPU-only, sequential
 *  implementation, see the spin::BVHTree class.
 *
 * \note The Execution Space, supplied as the 2nd template argument, specifies
 *
 *  1. Where and how the BVH is generated and stored
 *  2. Where and how subsequent queries are performed
 *  3. The default memory space, bound to the corresponding execution space
 *
 * \see axom::execution_space for more details.
 *
 *  A simple example illustrating how to use the BVH class is given below:
 *  \code
 *
 *     namespace spin = axom::spin;
 *     constexpr int DIMENSION = 3;
 *
 *     // get a list of axis-aligned bounding boxes in a flat array
 *     const double* aabbs = ...
 *
 *     // create a 3D BVH instance in parallel on the CPU using OpenMP
 *     spin::BVH< DIMENSION, axom::OMP_EXEC > bvh( aabbs, numItems );
 *     bvh.build();
 *
 *     // query points supplied in arrays, qx, qy, qz,
 *     const axom::IndexType numPoints = ...
 *     const double* qx = ...
 *     const double* qy = ...
 *     const double* qz = ...
 *
 *     // output array buffers
 *     axom::IndexType* offsets    = axom::allocate< IndexType >( numPoints );
 *     axom::IndexType* counts     = axom::allocate< IndexType >( numPoints );
 *     axom::IndexType* candidates = nullptr;
 *
 *     // find candidates in parallel, allocates and populates the supplied
 *     // candidates array
 *     bvh.find( offsets, counts, candidates, numPoints, qx, qy, qz );
 *     SLIC_ASSERT( candidates != nullptr );
 *
 *     ...
 *
 *     // caller is responsible for properly de-allocating the candidates array
 *     axom::deallocate( candidates );
 *
 *  \endcode
 *
 */
template < int NDIMS, typename ExecSpace, typename FloatType = double >
class BVH
{
public:

  AXOM_STATIC_ASSERT_MSG( ( (NDIMS==2) || (NDIMS==3) ),
                          "The BVH class may be used only in 2D or 3D." );
  AXOM_STATIC_ASSERT_MSG( std::is_floating_point< FloatType >::value,
                          "A valid FloatingType must be used for the BVH." );
  AXOM_STATIC_ASSERT_MSG( axom::execution_space< ExecSpace >::valid(),
      "A valid execution space must be supplied to the BVH." );


  /*!
   * \brief Default constructor. Disabled.
   */
  BVH() = delete;

  /*!
   * \brief Creates a BVH instance, of specified dimension, over a given set
   *  of geometric entities, each represented by its corresponding axis-aligned
   *  bounding box.
   *
   * \param [in] boxes buffer consisting of bounding boxes for each entity.
   * \param [in] numItems the total number of items to store in the BVH.
   *
   * \note boxes is an array of length 2*dimension*numItems, that stores the
   *  two corners of the axis-aligned bounding box corresponding to a given
   *  geometric entity. For example, in 3D, the two corners of the ith bounding
   *  box are given by:
   *  \code
   *    const int offset = i*6;
   *
   *    double xmin = boxes[ offset   ];
   *    double ymin = boxes[ offset+1 ];
   *    double zmin = boxes[ offset+2 ];
   *
   *    double xmax = boxes[ offset+3 ];
   *    double ymax = boxes[ offset+4 ];
   *    double zmax = boxes[ offset+5 ];
   *  \endcode
   *
   * \warning The supplied boxes array must point to a buffer in a memory space
   *  that is compatible with the execution space. For example, when using
   *  CUDA_EXEC, boxes must be in unified memory or GPU memory. The code
   *  currently does not check for that.
   *
   * \pre boxes != nullptr
   * \pre numItems > 0
   */
  BVH( const FloatType* boxes, IndexType numItems );

  /*!
   * \brief Destructor.
   */
  ~BVH();

  /*!
   * \brief Sets the scale factor for scaling the supplied bounding boxes.
   * \param [in] scale_factor the scale factor
   *
   * \note The default scale factor is set to 1.001
   */
  void setScaleFactor( FloatType scale_factor )
  { m_scaleFactor = scale_factor; };

  /*!
   * \brief Generates the BVH
   * \return status set to BVH_BUILD_OK on success.
   */
  int build( );

  /*!
   * \brief Returns the bounds of the BVH, given by the the root bounding box.
   *
   * \param [out] min buffer to store the lower corner of the root bounding box.
   * \param [out] max buffer to store the upper corner of the root bounding box.
   *
   * \note min/max point to arrays that are at least NDIMS long.
   *
   * \pre min != nullptr
   * \pre max != nullptr
   */
  void getBounds( FloatType* min, FloatType* max ) const;

  /*!
   * \brief Finds the candidate geometric entities that contain each of the
   *  given query points.
   *
   * \param [out] offsets offset to the candidates array for each query point
   * \param [out] counts stores the number of candidates per query point
   * \param [out] candidates array of the candidate IDs for each query point
   * \param [in]  numPts the total number of query points supplied
   * \param [in]  x array of x-coordinates
   * \param [in]  y array of y-coordinates
   * \param [in]  z array of z-coordinates, may be nullptr if 2D
   *
   * \note offsets and counts are pointers to arrays of size numPts that are
   *  pre-allocated by the caller before calling find().
   *
   * \note The candidates array is allocated internally by the method and
   *  ownership of the memory is transferred to the caller. Consequently, the
   *  caller is responsible for properly deallocating the candidates buffer.
   *
   * \note Upon completion, the ith query point has:
   *  * counts[ i ] candidates
   *  * Stored in the candidates array in the following range:
   *    [ offsets[ i ], offsets[ i ]+counts[ i ] ]
   *
   * \pre offsets != nullptr
   * \pre counts  != nullptr
   * \pre candidates == nullptr
   * \pre x != nullptr
   * \pre y != nullptr if dimension==2 || dimension==3
   * \pre z != nullptr if dimension==3
   */
  void find( IndexType* offsets, IndexType* counts,
             IndexType*& candidates, IndexType numPts,
             const FloatType* x,
             const FloatType* y,
             const FloatType* z=nullptr ) const;

  /*!
   * \brief Writes the BVH to the specified VTK file for visualization.
   * \param [in] fileName the name of VTK file.
   * \note Primarily used for debugging.
   */
  void writeVtkFile( const std::string& fileName ) const;

private:

/// \name Private Members
/// @{

  FloatType m_scaleFactor;
  IndexType m_numItems;
  const FloatType* m_boxes;
  internal::linear_bvh::BVHData< FloatType,NDIMS > m_bvh;

  static constexpr FloatType DEFAULT_SCALE_FACTOR = 1.001;
/// @}

  DISABLE_COPY_AND_ASSIGNMENT(BVH);
  DISABLE_MOVE_AND_ASSIGNMENT(BVH);
};

//------------------------------------------------------------------------------
//  BVH IMPLEMENTATION
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  PRIVATE HELPER METHOD IMPLEMENTATION
//------------------------------------------------------------------------------
namespace
{

/*!
 * \brief Performs a traversal to count the candidates for each query point.
 *
 * \param [in] leftCheck functor for left bin predicate check.
 * \param [in] rightCheck functor for right bin predicate check.
 * \param [in] inner_nodes array of vec4s for the BVH inner nodes.
 * \param [in] leaf_nodes array of BVH leaf node indices
 * \param [in] N the number of user-supplied query points
 * \param [out] counts array of candidate counts for each query point.
 * \param [in] x user-supplied array of x-coordinates
 * \param [in] y user-supplied array of y-coordinates
 * \param [in] z user-supplied array of z-coordinates
 *
 * \return total_count the total count of candidates for all query points.
 */
template < int NDIMS, typename ExecSpace,
           typename LeftPredicate,
           typename RightPredicate,
           typename FloatType >
IndexType bvh_get_counts(
                   LeftPredicate&& leftCheck,
                   RightPredicate&& rightCheck,
                   const internal::linear_bvh::Vec< FloatType,4 >* inner_nodes,
                   const int32* leaf_nodes,
                   IndexType N,
                   IndexType* counts,
                   const FloatType* x,
                   const FloatType* y,
                   const FloatType* z ) noexcept
{
  // sanity checks
  SLIC_ASSERT( inner_nodes != nullptr );
  SLIC_ASSERT( leaf_nodes != nullptr );
  SLIC_ERROR_IF( counts == nullptr, "supplied null pointer for counts!" );
  SLIC_ERROR_IF( x == nullptr, "supplied null pointer for x-coordinates!" );
  SLIC_ERROR_IF( y == nullptr, "supplied null pointer for y-coordinates!" );
  SLIC_ERROR_IF( (z==nullptr && NDIMS==3),
                 "supplied null pointer for z-coordinates!" );

  namespace bvh = internal::linear_bvh;
  using point_t = bvh::Vec< FloatType, NDIMS >;

  // STEP 1: count number of candidates for each query point
  using reduce_pol = typename axom::execution_space< ExecSpace >::reduce_policy;
  RAJA::ReduceSum< reduce_pol, IndexType > total_count( 0 );

  using QueryAccessor = bvh::QueryAccessor< NDIMS, FloatType >;
  for_all< ExecSpace >( N, AXOM_LAMBDA(IndexType i)
  {
    int32 count = 0;
    point_t point;
    QueryAccessor::getPoint( point, i, x, y, z );


    auto leafAction = [&]( int32 AXOM_NOT_USED(current_node),
                      const int32* AXOM_NOT_USED(leaf_nodes) ) -> void {
      count++;
    };

    bvh::bvh_traverse( inner_nodes,
                       leaf_nodes,
                       point,
                       leftCheck,
                       rightCheck,
                       leafAction );

    counts[ i ]  = count;
    total_count += count;

  } );

  return ( total_count.get() );
}

} /* end anonymous namespace */

//------------------------------------------------------------------------------
//  PUBLIC API IMPLEMENTATION
//------------------------------------------------------------------------------

template< int NDIMS, typename ExecSpace, typename FloatType >
BVH< NDIMS, ExecSpace, FloatType >::BVH( const FloatType* boxes,
                                         IndexType numItems ) :
  m_scaleFactor( DEFAULT_SCALE_FACTOR ),
  m_numItems( numItems ),
  m_boxes( boxes )
{

}

//------------------------------------------------------------------------------
template< int NDIMS, typename ExecSpace, typename FloatType >
BVH< NDIMS, ExecSpace, FloatType >::~BVH()
{
  m_bvh.deallocate();
}

//------------------------------------------------------------------------------
template< int NDIMS, typename ExecSpace, typename FloatType >
int BVH< NDIMS, ExecSpace, FloatType >::build()
{
  // STEP 0: set the default memory allocator to use for the execution space.
  const int currentAllocatorID = axom::getDefaultAllocatorID();
  const int allocatorID = axom::execution_space< ExecSpace >::allocatorID();
  axom::setDefaultAllocator( allocatorID );

  // STEP 1: Handle case when user supplied a single bounding box
  int numBoxes        = m_numItems;
  FloatType* boxesptr = nullptr;
  if ( m_numItems == 1 )
  {
    numBoxes          = 2;
    constexpr int32 M = NDIMS * 2;      // number of entries for one box
    const int N       = numBoxes * M;   // number of entries for N boxes
    boxesptr          = axom::allocate< FloatType >( N );

    const FloatType* myboxes = m_boxes;

    // copy first box and add a fake 2nd box
    for_all< ExecSpace >( N, AXOM_LAMBDA(IndexType i)
    {
      boxesptr[ i ] = ( i < M ) ? myboxes[ i ] : 0.0;
    } );

  } // END if single item
  else
  {
    boxesptr = const_cast< FloatType* >( m_boxes );
  }

  // STEP 2: Build a RadixTree consisting of the bounding boxes, sorted
  // by their corresponding morton code.
  internal::linear_bvh::RadixTree< FloatType, NDIMS > radix_tree;
  internal::linear_bvh::AABB< FloatType, NDIMS > global_bounds;
  internal::linear_bvh::build_radix_tree< ExecSpace >(
      boxesptr, numBoxes, global_bounds, radix_tree, m_scaleFactor );

  // STEP 3: emit the BVH data-structure from the radix tree
  m_bvh.m_bounds = global_bounds;
  m_bvh.allocate( numBoxes );

  // STEP 4: emit the BVH
  internal::linear_bvh::emit_bvh< ExecSpace >( radix_tree, m_bvh );

  radix_tree.deallocate();

  // STEP 5: deallocate boxesptr if user supplied a single box
  if ( m_numItems == 1 )
  {
    SLIC_ASSERT( boxesptr != m_boxes );
    axom::deallocate( boxesptr );
  }

  // STEP 6: restore default allocator
  axom::setDefaultAllocator( currentAllocatorID );
  return BVH_BUILD_OK;
}

//------------------------------------------------------------------------------
template< int NDIMS, typename ExecSpace, typename FloatType >
void BVH< NDIMS, ExecSpace, FloatType >::getBounds( FloatType* min,
                                                    FloatType* max ) const
{
  SLIC_ASSERT( min != nullptr );
  SLIC_ASSERT( max != nullptr );
  m_bvh.m_bounds.min( min );
  m_bvh.m_bounds.max( max );
}

//------------------------------------------------------------------------------
template< int NDIMS, typename ExecSpace, typename FloatType >
void BVH< NDIMS, ExecSpace, FloatType >::find( IndexType* offsets,
                                               IndexType* counts,
                                               IndexType*& candidates,
                                               IndexType numPts,
                                               const FloatType* x,
                                               const FloatType* y,
                                               const FloatType* z ) const
{
  SLIC_ASSERT( offsets != nullptr );
  SLIC_ASSERT( counts != nullptr );
  SLIC_ASSERT( candidates == nullptr );
  SLIC_ASSERT( x != nullptr );
  SLIC_ASSERT( y != nullptr );

  // STEP 0: set the default memory allocator to use for the execution space.
  const int currentAllocatorID = axom::getDefaultAllocatorID();
  const int allocatorID = axom::execution_space< ExecSpace >::allocatorID();
  axom::setDefaultAllocator( allocatorID );

  namespace bvh = internal::linear_bvh;
  using vec4_t  = bvh::Vec< FloatType, 4 >;
  using point_t = bvh::Vec< FloatType, NDIMS >;
  using TraversalPredicates = bvh::TraversalPredicates< NDIMS, FloatType >;
  using QueryAccessor       = bvh::QueryAccessor< NDIMS, FloatType >;

  // STEP 1: count number of candidates for each query point
  const vec4_t* inner_nodes = m_bvh.m_inner_nodes;
  const int32*  leaf_nodes  = m_bvh.m_leaf_nodes;
  SLIC_ASSERT( inner_nodes != nullptr );
  SLIC_ASSERT( leaf_nodes != nullptr );

  // STEP 2: define traversal predicates
  auto leftPredicate = [] AXOM_HOST_DEVICE ( const point_t& p,
                                             const vec4_t& s1,
                                             const vec4_t& s2 ) -> bool
  { return TraversalPredicates::pointInLeftBin( p, s1, s2 ); };

  auto rightPredicate = [] AXOM_HOST_DEVICE ( const point_t& p,
                                              const vec4_t& s2,
                                              const vec4_t& s3 ) -> bool
  { return TraversalPredicates::pointInRightBin( p, s2, s3 ); };

  // STEP 3: get counts
  int total_count = bvh_get_counts< NDIMS,ExecSpace >(
      leftPredicate, rightPredicate, inner_nodes, leaf_nodes,
      numPts, counts, x, y, z );

  using exec_policy = typename axom::execution_space< ExecSpace >::loop_policy;
  RAJA::exclusive_scan< exec_policy >(
      counts, counts+numPts, offsets, RAJA::operators::plus<IndexType>{} );

  IndexType total_candidates = static_cast< IndexType >( total_count );
  candidates = axom::allocate< IndexType >( total_candidates);

  // STEP 4: fill in candidates for each point
  for_all< ExecSpace >( numPts, AXOM_LAMBDA (IndexType i)
  {
    int32 offset = offsets[ i ];

    point_t point;
    QueryAccessor::getPoint( point, i, x, y, z );

    auto leafAction = [=,&offset]( int32 current_node,
                                   const int32* leaf_nodes ) -> void {
      candidates[offset] = leaf_nodes[current_node];
      offset++;
    };


    bvh::bvh_traverse( inner_nodes,
                       leaf_nodes,
                       point,
                       leftPredicate,
                       rightPredicate,
                       leafAction );

  } );

  // STEP 3: restore default allocator
  axom::setDefaultAllocator( currentAllocatorID );
}

//------------------------------------------------------------------------------
template < int NDIMS, typename ExecSpace, typename FloatType >
void BVH< NDIMS, ExecSpace, FloatType >::writeVtkFile(
    const std::string& fileName ) const
{
  std::ostringstream nodes;
  std::ostringstream cells;
  std::ostringstream levels;

  // STEP 0: Write VTK header
  std::ofstream ofs;
  ofs.open( fileName.c_str() );
  ofs << "# vtk DataFile Version 3.0\n";
  ofs << " BVHTree \n";
  ofs << "ASCII\n";
  ofs << "DATASET UNSTRUCTURED_GRID\n";

  // STEP 1: write root
  int32 numPoints = 0;
  int32 numBins   = 0;
  internal::linear_bvh::write_root(
      m_bvh.m_bounds, numPoints, numBins,nodes,cells,levels );


  // STEP 2: traverse the BVH and dump each bin
  constexpr int32 ROOT = 0;
  internal::linear_bvh::write_recursive< FloatType, NDIMS >(
      m_bvh.m_inner_nodes, ROOT, 1, numPoints, numBins, nodes, cells, levels );

  // STEP 3: write nodes
  ofs << "POINTS " << numPoints << " double\n";
  ofs << nodes.str() << std::endl;

  // STEP 4: write cells
  const int32 nnodes = (NDIMS==2) ? 4 : 8;
  ofs << "CELLS " << numBins << " " << numBins*(nnodes+1) << std::endl;
  ofs << cells.str() << std::endl;

  // STEP 5: write cell types
  ofs << "CELL_TYPES " << numBins << std::endl;
  const int32 cellType = (NDIMS==2) ? 9 : 12;
  for ( int32 i=0; i < numBins; ++i )
  {
    ofs << cellType << std::endl;
  }

  // STEP 6: dump level information
  ofs << "CELL_DATA " << numBins << std::endl;
  ofs << "SCALARS level int\n";
  ofs << "LOOKUP_TABLE default\n";
  ofs << levels.str() << std::endl;
  ofs << std::endl;

  // STEP 7: close file
  ofs.close();
}

} /* namespace primal */
} /* namespace axom */


#endif /* AXOM_PRIMAL_BVH_H_ */
