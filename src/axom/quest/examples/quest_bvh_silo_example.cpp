// Copyright (c) 2017-2023, Lawrence Livermore National Security, LLC and
// other Axom Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

//-----------------------------------------------------------------------------
///
/// file: device_spatial_indexes.cpp
///
/// This example uses a spatial index, the linear BVH tree from Axom's spin
/// component, in addition to RAJA and Umpire based kernels for a highly
//  efficient performance-portable self-intersection algorithm.
//-----------------------------------------------------------------------------

#include "axom/config.hpp"
#include "axom/core.hpp"
#include "axom/slic.hpp"

#ifdef AXOM_USE_RAJA
  #include "RAJA/RAJA.hpp"
#else
  #error This example requires axom to be configured with RAJA support
#endif

#ifdef AXOM_USE_UMPIRE
  #include "umpire/Umpire.hpp"
#else
  #error This example requires axom to be configured with Umpire support
#endif

#ifdef AXOM_USE_CONDUIT
  #include "conduit_relay.hpp"
  #include "conduit_relay_io_silo.hpp"
#else
  #error This example requires axom to be configured with Conduit support
#endif

#include "axom/mint.hpp"
#include "axom/primal.hpp"
#include "axom/spin.hpp"
#include "axom/quest.hpp"

#include "axom/fmt.hpp"
#include "axom/CLI11.hpp"

#include <memory>
#include <array>

using seq_exec = axom::SEQ_EXEC;

using UMesh = axom::mint::UnstructuredMesh<axom::mint::SINGLE_SHAPE>;

// clang-format off
#if defined(AXOM_USE_OPENMP)
  using omp_exec = axom::OMP_EXEC;
#else
  using omp_exec = seq_exec;
#endif

#if defined(AXOM_USE_CUDA)
  constexpr int BLK_SZ = 256;
  using cuda_exec = axom::CUDA_EXEC<BLK_SZ>;
#else
  using cuda_exec = seq_exec;
#endif
// clang-format on

//-----------------------------------------------------------------------------
/// Basic RAII utility class for initializing and finalizing slic logger
//-----------------------------------------------------------------------------
struct BasicLogger
{
  BasicLogger()
  {
    namespace slic = axom::slic;

    // Initialize the SLIC logger
    slic::initialize();
    slic::setLoggingMsgLevel(slic::message::Debug);

    // Customize logging levels and formatting
    const std::string slicFormatStr = "[<LEVEL>] <MESSAGE> \n";

    slic::addStreamToMsgLevel(new slic::GenericOutputStream(&std::cerr),
                              slic::message::Error);
    slic::addStreamToMsgLevel(
      new slic::GenericOutputStream(&std::cerr, slicFormatStr),
      slic::message::Warning);

    auto* compactStream =
      new slic::GenericOutputStream(&std::cout, slicFormatStr);
    slic::addStreamToMsgLevel(compactStream, slic::message::Info);
    slic::addStreamToMsgLevel(compactStream, slic::message::Debug);
  }

  ~BasicLogger() { axom::slic::finalize(); }
};

//-----------------------------------------------------------------------------
/// Struct to help with parsing and storing command line args
//-----------------------------------------------------------------------------
enum class RuntimePolicy
{
  raja_seq = 1,
  raja_omp = 2,
  raja_cuda = 3
};

struct Input
{
  static const std::map<std::string, RuntimePolicy> s_validPolicies;

  std::string mesh_file_first {""};
  std::string mesh_file_second {""};
  bool verboseOutput {false};
  double intersectionThreshold {1e-08};
  RuntimePolicy policy {RuntimePolicy::raja_seq};

  void parse(int argc, char** argv, axom::CLI::App& app);
  bool isVerbose() const { return verboseOutput; }
};

void Input::parse(int argc, char** argv, axom::CLI::App& app)
{
  app.add_option("-i, --infile", mesh_file_first)
    ->description("The first input silo mesh file to insert into BVH")
    ->required()
    ->check(axom::CLI::ExistingFile);

  app.add_option("-q, --queryfile", mesh_file_second)
    ->description("The second input silo mesh file to query BVH")
    ->required()
    ->check(axom::CLI::ExistingFile);

  app.add_flag("-v,--verbose", verboseOutput)
    ->description("Increase logging verbosity?")
    ->capture_default_str();

  app.add_option("--intersection-threshold", intersectionThreshold)
    ->description("Threshold to use when testing for intersecting hexes")
    ->capture_default_str();

  app.add_option("-p, --policy", policy)
    ->description(
      "Execution policy."
      "\nSet to 'raja_seq' or 1 to use the RAJA sequential policy."
#ifdef AXOM_USE_OPENMP
      "\nSet to 'raja_omp' or 2 to use the RAJA openmp policy."
#endif
#ifdef AXOM_USE_OPENMP
      "\nSet to 'raja_cuda' or 3 to use the RAJA cuda policy."
#endif
      )
    ->capture_default_str()
    ->transform(axom::CLI::CheckedTransformer(Input::s_validPolicies));

  app.get_formatter()->column_width(40);

  app.parse(argc, argv);  // Could throw an exception

  // Output parsed information
  SLIC_INFO(axom::fmt::format(
    R"(
     Parsed parameters:
      * First Silo mesh to insert into BVH: '{}'
      * Second Silo mesh to query BVH: '{}'
      * Threshold for intersections: {}
      * Verbose logging: {}
      * Runtime execution policy: '{}'
      )",
    mesh_file_first,
    mesh_file_second,
    intersectionThreshold,
    verboseOutput,
    policy == RuntimePolicy::raja_omp
      ? "raja_omp"
      : (policy == RuntimePolicy::raja_cuda) ? "raja_cuda" : "raja_seq"));
}

const std::map<std::string, RuntimePolicy> Input::s_validPolicies(
  {{"raja_seq", RuntimePolicy::raja_seq}
#ifdef AXOM_USE_OPENMP
   ,
   {"raja_omp", RuntimePolicy::raja_omp}
#endif
#ifdef AXOM_USE_CUDA
   ,
   {"raja_cuda", RuntimePolicy::raja_cuda}
#endif
  });

//-----------------------------------------------------------------------------
/// Basic hexahedron mesh to be used in our application
//-----------------------------------------------------------------------------
struct HexMesh
{
  using Point = axom::primal::Point<double, 3>;
  using Hexahedron = axom::primal::Hexahedron<double, 3>;
  using BoundingBox = axom::primal::BoundingBox<double, 3>;

  axom::IndexType numHexes() const { return m_hexes.size(); }
  axom::Array<Hexahedron>& hexes() { return m_hexes; }
  const axom::Array<Hexahedron>& hexes() const { return m_hexes; }

  BoundingBox& meshBoundingBox() { return m_meshBoundingBox; }
  const BoundingBox& meshBoundingBox() const { return m_meshBoundingBox; }

  axom::Array<BoundingBox>& hexBoundingBoxes() { return m_hexBoundingBoxes; }
  const axom::Array<BoundingBox>& hexBoundingBoxes() const
  {
    return m_hexBoundingBoxes;
  }

  axom::Array<Hexahedron> m_hexes;
  axom::Array<BoundingBox> m_hexBoundingBoxes;
  BoundingBox m_meshBoundingBox;
};

HexMesh loadSiloHexMesh(const std::string& mesh_path)
{
  HexMesh hexMesh;

  axom::utilities::Timer timer(true);

  // Load silo mesh into Conduit node
  conduit::Node n_load;
  conduit::relay::io::silo::load_mesh(mesh_path, n_load);

  // Get unstructured topology for the cell connectivity
  conduit::Node unstruct_topo;
  conduit::Node unstruct_coords;

  conduit::blueprint::mesh::topology::structured::to_unstructured(
    n_load[0]["topologies/MMESH"],
    unstruct_topo,
    unstruct_coords);

  // Verify this is a hexahedral mesh
  std::string shape = unstruct_topo["elements/shape"].as_string();
  if(shape != "hex")
  {
    SLIC_ERROR("A hex mesh was expected!");
  }

  UMesh* mesh = new UMesh(3, axom::mint::HEX);

  int* connectivity = unstruct_topo["elements/connectivity"].value();

  const int HEX_OFFSET = 8;
  int num_nodes = (unstruct_coords["values/x"]).dtype().number_of_elements();
  double* x_vals = unstruct_coords["values/x"].value();
  double* y_vals = unstruct_coords["values/y"].value();
  double* z_vals = unstruct_coords["values/z"].value();

  int connectivity_size =
    (unstruct_topo["elements/connectivity"]).dtype().number_of_elements();

  // Sanity check for number of cells
  int cell_calc_from_nodes =
    std::round(std::pow(std::pow(num_nodes, 1.0 / 3.0) - 1, 3));
  int cell_calc_from_connectivity = connectivity_size / HEX_OFFSET;
  if(cell_calc_from_nodes != cell_calc_from_connectivity)
  {
    SLIC_ERROR("Number of connectivity elements is not expected!\n"
               << "First calculation is " << cell_calc_from_nodes
               << " and second calculation is " << cell_calc_from_connectivity);
  }

  // Append mesh nodes
  for(int i = 0; i < num_nodes; i++)
  {
    mesh->appendNode(x_vals[i], y_vals[i], z_vals[i]);
  }

  // Append mesh cells
  for(int i = 0; i < connectivity_size / HEX_OFFSET; i++)
  {
    const axom::IndexType cell[] = {
      connectivity[i * HEX_OFFSET],
      connectivity[(i * HEX_OFFSET) + 1],
      connectivity[(i * HEX_OFFSET) + 2],
      connectivity[(i * HEX_OFFSET) + 3],
      connectivity[(i * HEX_OFFSET) + 4],
      connectivity[(i * HEX_OFFSET) + 5],
      connectivity[(i * HEX_OFFSET) + 6],
      connectivity[(i * HEX_OFFSET) + 7],
    };

    mesh->appendCell(cell);
  }

  timer.stop();
  SLIC_INFO(axom::fmt::format("Loading the mesh took {:4.3} seconds.",
                              timer.elapsedTimeInSec()));

  // // Write out to vtk for test viewing
  // axom::mint::write_vtk(mesh, "test.vtk");

  // extract hexes into an axom::Array
  const int numCells = mesh->getNumberOfCells();
  hexMesh.m_hexes.reserve(numCells);
  {
    HexMesh::Hexahedron hex;
    std::array<axom::IndexType, 8> hexCell;
    for(int i = 0; i < numCells; ++i)
    {
      mesh->getCellNodeIDs(i, hexCell.data());
      mesh->getNode(hexCell[0], hex[0].data());
      mesh->getNode(hexCell[1], hex[1].data());
      mesh->getNode(hexCell[2], hex[2].data());
      mesh->getNode(hexCell[3], hex[3].data());
      mesh->getNode(hexCell[4], hex[4].data());
      mesh->getNode(hexCell[5], hex[5].data());
      mesh->getNode(hexCell[6], hex[6].data());
      mesh->getNode(hexCell[7], hex[7].data());

      hexMesh.m_hexes.emplace_back(hex);
    }
  }

  delete mesh;
  mesh = nullptr;

  // compute and store hex bounding boxes and mesh bounding box
  hexMesh.m_hexBoundingBoxes.reserve(numCells);
  for(const auto& hex : hexMesh.hexes())
  {
    hexMesh.m_hexBoundingBoxes.emplace_back(
      axom::primal::compute_bounding_box(hex));
    hexMesh.m_meshBoundingBox.addBox(hexMesh.m_hexBoundingBoxes.back());
  }

  SLIC_INFO(
    axom::fmt::format("Mesh bounding box is {}.\n", hexMesh.meshBoundingBox()));

  return hexMesh;
}

using IndexPair = std::pair<axom::IndexType, axom::IndexType>;

template <typename ExecSpace>
axom::Array<IndexPair> findIntersectionsBVH(const HexMesh& insertMesh,
                                            const HexMesh& queryMesh,
                                            double tol,
                                            bool verboseOutput = false)
{
  SLIC_INFO("Running BVH intersection algorithm in execution Space: "
            << axom::execution_space<ExecSpace>::name());

  using HexArray = axom::Array<typename HexMesh::Hexahedron>;
  using BBoxArray = axom::Array<typename HexMesh::BoundingBox>;
  using IndexArray = axom::Array<axom::IndexType>;
  constexpr bool on_device = axom::execution_space<ExecSpace>::onDevice();

  using ATOMIC_POL = typename axom::execution_space<ExecSpace>::atomic_policy;

  axom::Array<IndexPair> intersectionPairs;

  // Get ids of necessary allocators
  const int host_allocator =
    axom::getUmpireResourceAllocatorID(umpire::resource::Host);
  const int kernel_allocator = on_device
    ? axom::getUmpireResourceAllocatorID(umpire::resource::Device)
    : axom::execution_space<ExecSpace>::allocatorID();

  // Copy the insert-BVH hexes to the device, if necessary
  // Either way, insert_hexes_v will be a view w/ data in the correct space
  auto& insert_hexes_h = insertMesh.hexes();
  HexArray insert_hexes_d =
    on_device ? HexArray(insert_hexes_h, kernel_allocator) : HexArray();
  auto insert_hexes_v = on_device ? insert_hexes_d.view() : insert_hexes_h.view();

  // Copy the insert-BVH bboxes to the device, if necessary
  // Either way, insert_bbox_v will be a view w/ data in the correct space
  auto& insert_bbox_h = insertMesh.hexBoundingBoxes();
  BBoxArray insert_bbox_d =
    on_device ? BBoxArray(insert_bbox_h, kernel_allocator) : BBoxArray();
  auto insert_bbox_v = on_device ? insert_bbox_d.view() : insert_bbox_h.view();

  // Copy the query-BVH hexes to the device, if necessary
  // Either way, query_hexes_v will be a view w/ data in the correct space
  auto& query_hexes_h = queryMesh.hexes();
  HexArray query_hexes_d =
    on_device ? HexArray(query_hexes_h, kernel_allocator) : HexArray();
  auto query_hexes_v = on_device ? query_hexes_d.view() : query_hexes_h.view();

  // Copy the query-BVH bboxes to the device, if necessary
  // Either way, bbox_v will be a view w/ data in the correct space
  auto& query_bbox_h = queryMesh.hexBoundingBoxes();
  BBoxArray query_bbox_d =
    on_device ? BBoxArray(query_bbox_h, kernel_allocator) : BBoxArray();
  auto query_bbox_v = on_device ? query_bbox_d.view() : query_bbox_h.view();

  axom::utilities::Timer timer;

  // Initialize a BVH tree over the insert mesh bounding boxes
  timer.start();
  axom::spin::BVH<3, ExecSpace, double> bvh;
  bvh.setAllocatorID(kernel_allocator);
  bvh.initialize(insert_bbox_v, insert_bbox_v.size());
  timer.stop();
  SLIC_INFO_IF(verboseOutput,
               axom::fmt::format("0: Initializing BVH took {:4.3} seconds.",
                                 timer.elapsedTimeInSec()));

  // Search for intersecting bounding boxes of hexes to query;
  // result is returned as CSR arrays for candidate data
  timer.start();
  IndexArray offsets_d(query_bbox_v.size(), query_bbox_v.size(), kernel_allocator);
  IndexArray counts_d(query_bbox_v.size(), query_bbox_v.size(), kernel_allocator);
  IndexArray candidates_d(0, 0, kernel_allocator);

  auto offsets_v = offsets_d.view();
  auto counts_v = counts_d.view();
  bvh.findBoundingBoxes(offsets_v,
                        counts_v,
                        candidates_d,
                        query_bbox_v.size(),
                        query_bbox_v);

  timer.stop();
  SLIC_INFO_IF(verboseOutput,
               axom::fmt::format(
                 "1: Querying candidate bounding boxes took {:4.3} seconds.",
                 timer.elapsedTimeInSec()));

  // Initialize query indices and bvh candidate indices
  IndexArray indices_d(axom::ArrayOptions::Uninitialized {},
                       candidates_d.size(),
                       kernel_allocator);

  IndexArray validCandidates_d(axom::ArrayOptions::Uninitialized {},
                               candidates_d.size(),
                               kernel_allocator);

  axom::IndexType numCandidates {};
  timer.start();
  {
    const int totalQueryHexes = queryMesh.numHexes();

    IndexArray numValidCandidates_d(1, 1, kernel_allocator);
    numValidCandidates_d.fill(0);
    auto* numValidCandidates_p = numValidCandidates_d.data();

    auto indices_v = indices_d.view();
    auto validCandidates_v = validCandidates_d.view();
    auto candidates_v = candidates_d.view();

    // Initialize pairs of query and candidate indices
    axom::for_all<ExecSpace>(
      totalQueryHexes,
      AXOM_LAMBDA(axom::IndexType i) {
        for(int j = 0; j < counts_v[i]; j++)
        {
          const axom::IndexType potential = candidates_v[offsets_v[i] + j];
          const auto idx = RAJA::atomicAdd<ATOMIC_POL>(numValidCandidates_p,
                                                       axom::IndexType {1});
          indices_v[idx] = i;
          validCandidates_v[idx] = potential;
        }
      });

    axom::copy(&numCandidates, numValidCandidates_p, sizeof(axom::IndexType));
  }
  timer.stop();
  SLIC_INFO_IF(verboseOutput,
               axom::fmt::format("2: Linearizing query indices and bvh "
                                 "candidate indices took {:4.3} seconds.",
                                 timer.elapsedTimeInSec()));

  IndexArray intersect_d[2] = {IndexArray(axom::ArrayOptions::Uninitialized {},
                                          numCandidates,
                                          kernel_allocator),
                               IndexArray(axom::ArrayOptions::Uninitialized {},
                                          numCandidates,
                                          kernel_allocator)};
  axom::IndexType numIntersections {};
  timer.start();
  {
    auto intersect1_v = intersect_d[0].view();
    auto intersect2_v = intersect_d[1].view();

    IndexArray numIntersections_d(1, 1, kernel_allocator);
    auto* numIntersections_p = numIntersections_d.data();

    auto indices_v = indices_d.view();
    auto validCandidates_v = validCandidates_d.view();

    // Perform hex-hex tests
    axom::for_all<ExecSpace>(
      numCandidates,
      AXOM_LAMBDA(axom::IndexType i) {
        constexpr bool includeBoundaries = false;
        const auto index = indices_v[i];
        const auto candidate = validCandidates_v[i];
        // For now, using bbox-bbox intersection, because well, have to implement
        // the hex-hex intersection routine first.
        // if(axom::primal::intersect(query_hexes_v[index],
        //                            insert_hexes_v[candidate],
        //                            includeBoundaries,
        //                            tol))
        if(axom::primal::intersect(query_bbox_v[index], insert_bbox_v[candidate]))
        {
          const auto idx =
            RAJA::atomicAdd<ATOMIC_POL>(numIntersections_p, axom::IndexType {1});
          intersect1_v[idx] = index;
          intersect2_v[idx] = candidate;
        }
      });

    axom::copy(&numIntersections, numIntersections_p, sizeof(axom::IndexType));
  }
  intersect_d[0].resize(numIntersections);
  intersect_d[1].resize(numIntersections);

  timer.stop();
  SLIC_INFO_IF(
    verboseOutput,
    axom::fmt::format("3: Finding actual intersections took {:4.3} seconds.",
                      timer.elapsedTimeInSec()));

  SLIC_INFO_IF(verboseOutput,
               axom::fmt::format(axom::utilities::locale(),
                                 R"(Stats for query
    -- Number of insert-BVH mesh hexes {:L}
    -- Number of query mesh hexes {:L}
    -- Total possible candidates {:L}
    -- Candidates from BVH query {:L}
    -- Potential candidates after linearizing {:L}
    -- Actual intersections {:L}
    )",
                                 insertMesh.numHexes(),
                                 queryMesh.numHexes(),
                                 insertMesh.numHexes() * queryMesh.numHexes(),
                                 candidates_d.size(),
                                 numCandidates,
                                 numIntersections));

  // copy results back to host and into return vector
  IndexArray intersect_h[2] = {
    on_device ? IndexArray(intersect_d[0], host_allocator) : IndexArray(),
    on_device ? IndexArray(intersect_d[1], host_allocator) : IndexArray()};

  auto intersect1_h_v = on_device ? intersect_h[0].view() : intersect_d[0].view();
  auto intersect2_h_v = on_device ? intersect_h[1].view() : intersect_d[1].view();

  for(axom::IndexType idx = 0; idx < numIntersections; ++idx)
  {
    intersectionPairs.emplace_back(
      std::make_pair(intersect1_h_v[idx], intersect2_h_v[idx]));
  }

  return intersectionPairs;
}  // end of findIntersectionsBVH for Silo Meshes

int main(int argc, char** argv)
{
  // Initialize logger; use RAII so it will finalize at the end of the application
  BasicLogger logger;

  // Parse the command line arguments
  Input params;
  {
    axom::CLI::App app {"Silo Hex BVH mesh intersection tester"};
    try
    {
      params.parse(argc, argv, app);
    }
    catch(const axom::CLI::ParseError& e)
    {
      return app.exit(e);
    }
  }

  // Update the logging level based on verbosity flag
  axom::slic::setLoggingMsgLevel(params.isVerbose() ? axom::slic::message::Debug
                                                    : axom::slic::message::Info);

  // Load Silo mesh into BVH
  SLIC_INFO(axom::fmt::format("Reading silo file to insert into BVH: '{}'...\n",
                              params.mesh_file_first));

  HexMesh insert_mesh = loadSiloHexMesh(params.mesh_file_first);

  // Load Silo mesh for querying BVH
  SLIC_INFO(axom::fmt::format("Reading silo file to query BVH: '{}'...\n",
                              params.mesh_file_second));

  HexMesh query_mesh = loadSiloHexMesh(params.mesh_file_second);

  // Check for self-intersections; results are returned as an array of index pairs
  axom::Array<IndexPair> intersectionPairs;
  axom::utilities::Timer timer(true);
  switch(params.policy)
  {
  case RuntimePolicy::raja_omp:
#ifdef AXOM_USE_OPENMP
    intersectionPairs =
      findIntersectionsBVH<omp_exec>(insert_mesh,
                                     query_mesh,
                                     params.intersectionThreshold,
                                     params.isVerbose());
#endif
    break;
  case RuntimePolicy::raja_cuda:
#ifdef AXOM_USE_CUDA
    intersectionPairs =
      findIntersectionsBVH<cuda_exec>(insert_mesh,
                                      query_mesh,
                                      params.intersectionThreshold,
                                      params.isVerbose());
#endif
    break;
  default:  // RuntimePolicy::raja_seq
    intersectionPairs =
      findIntersectionsBVH<seq_exec>(insert_mesh,
                                     query_mesh,
                                     params.intersectionThreshold,
                                     params.isVerbose());
    break;
  }
  timer.stop();

  SLIC_INFO(axom::fmt::format("Computing intersections {} took {:4.3} seconds.",
                              "with a BVH tree",
                              timer.elapsedTimeInSec()));
  SLIC_INFO(axom::fmt::format(axom::utilities::locale(),
                              "Mesh had {:L} intersection pairs",
                              intersectionPairs.size()));

  // print first few pairs
  const int numIntersections = intersectionPairs.size();
  if(numIntersections > 0 && params.isVerbose())
  {
    constexpr int MAX_PRINT = 20;
    if(numIntersections > MAX_PRINT)
    {
      intersectionPairs.resize(MAX_PRINT);
      SLIC_INFO(axom::fmt::format("First {} intersection pairs: {} ...\n",
                                  MAX_PRINT,
                                  axom::fmt::join(intersectionPairs, ", ")));
    }
    else
    {
      SLIC_INFO(axom::fmt::format("Intersection pairs: {}\n",
                                  axom::fmt::join(intersectionPairs, ", ")));
    }
  }

  return 0;
}
