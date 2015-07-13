/*
 * Copyright (c) 2015, Lawrence Livermore National Security, LLC.
 * Produced at the Lawrence Livermore National Laboratory.
 *
 * All rights reserved.
 *
 * This source code cannot be distributed without permission and
 * further review from Lawrence Livermore National Laboratory.
 */

#include "gtest/gtest.h"

#include "sidre/sidre.hpp"

using asctoolkit::sidre::DataBuffer;
using asctoolkit::sidre::DataGroup;
using asctoolkit::sidre::DataStore;
using asctoolkit::sidre::DataView;
using asctoolkit::sidre::DataType;

//------------------------------------------------------------------------------
// Some simple types and functions used in tests
// (included in namespace to prevent clashes)
//------------------------------------------------------------------------------
namespace dsopaquetest
{

enum Centering { _Zone_,
                 _Node_,
                 _UnknownCentering_};

enum DType { _Double_,
             _Int_,
             _UnknownType_};


class Extent
{
public:

  Extent(int lo, int hi)
    : m_ilo(lo), m_ihi(hi) {; }

  int getNumPts(Centering cent) const
  {
    int retval = 0;

    switch ( cent )
    {

    case _Zone_: {
      retval = (m_ihi - m_ilo + 1);
      break;
    }

    case _Node_: {
      retval = (m_ihi - m_ilo + 2);
      break;
    }

    default: {
      retval = -1;         // I know magic numbers are bad. So sue me.
    }

    }   // switch on centering

    return retval;
  }

  int m_ilo;
  int m_ihi;
};


class MeshVar
{
public:

  MeshVar(Centering cent, DType type, int depth = 1)
    : m_cent(cent), m_type(type), m_depth(depth) {; }

  int getNumVals(const Extent * ext) const
  {
    return ( ext->getNumPts(m_cent) * m_depth );
  }

  Centering m_cent;
  DType m_type;
  int m_depth;
};


}  // closing brace for dsopaquetest namespace

//------------------------------------------------------------------------------
//
// Simple test that adds an opaque data object, retrieves it and checks if
// the retrieved object is in the expected state.
//
TEST(sidre_opaque,inout)
{
  using namespace dsopaquetest;

  const int ihi_val = 9;

  DataStore * ds   = new DataStore();
  DataGroup * root = ds->getRoot();

  DataGroup * problem_gp = root->createGroup("problem");

  Extent * ext = new Extent(0, ihi_val);

  DataView * ext_view = problem_gp->createOpaqueView("ext", ext);
#if 1
//  problem_gp->CreateViewAndBuffer("ext");
//  problem_gp->CreateOpaqueView("ext", ext);
//  problem_gp->CreateView("ext", 0);
//  problem_gp->MoveView(0);
//  problem_gp->MoveView(problem_gp->GetView("ext"));
//  problem_gp->CopyView(0);
//  problem_gp->CopyView(problem_gp->GetView("ext"));
//  problem_gp->AttachView(0);
//  problem_gp->CopyView(problem_gp->GetView("ext"));
//  Can't do following: method is private...
//  DataView* v = problem_gp->DetachView("ext");
//  std::cout << "view name = " << v->GetName() << std::endl;
//  problem_gp->DestroyView("foo");
//  root->MoveGroup(problem_gp);
//  root->CopyGroup(problem_gp);
//  Can't do following: method is private...
//  root->DetachGroup("bar");
//  root->DestroyGroup("bar");
//  problem_gp->getView(2);
#endif

  bool test_opaque = ext_view->isOpaque();
  EXPECT_EQ(test_opaque, true);

  Extent * test_extent =
    static_cast<Extent *>(ext_view->getOpaque());
  int test_ihi = test_extent->m_ihi;

  EXPECT_EQ(test_ihi, ihi_val);

  // clean up...
  delete ext;
  delete ds;
}

//------------------------------------------------------------------------------
//
// Test that adds "MeshVars" as opaque data objects, creates views for their
// data on each of two domains, allocates their data (based on centering,
// domain size, and depth), and then checks to if the allocated data
// lengths match the expected values.
//
TEST(sidre_opaque,meshvar)
{
  using namespace dsopaquetest;

  const int ilo_val[] = {0, 10};
  const int ihi_val[] = {9, 21};
  const std::string dom_name[] = { std::string("domain0"),
                                   std::string("domain1") };

  const int zone_var_depth = 1;
  const int node_var_depth = 2;

  DataStore * ds   = new DataStore();
  DataGroup * root = ds->getRoot();

  DataGroup * problem_gp = root->createGroup("problem");

  // Add two different mesh vars to mesh var group
  DataGroup * meshvar_gp = problem_gp->createGroup("mesh_var");
  MeshVar * zone_mv = new MeshVar(_Zone_, _Int_, zone_var_depth);
  DataView * zone_mv_view = meshvar_gp->createOpaqueView("zone_mv", zone_mv);
  MeshVar * node_mv = new MeshVar(_Node_, _Double_, node_var_depth);
  DataView * node_mv_view = meshvar_gp->createOpaqueView("node_mv", node_mv);

  //
  // Create domain groups, add extents
  // Create data views for mesh var data on domains and allocate
  //
  for (int idom = 0 ; idom < 2 ; ++idom)
  {

    DataGroup * dom_gp = problem_gp->createGroup(dom_name[idom]);
    Extent * dom_ext = new Extent(ilo_val[idom], ihi_val[idom]);
    dom_gp->createOpaqueView("ext", dom_ext);

    MeshVar * zonemv = static_cast<MeshVar *>( zone_mv_view->getOpaque() );
    DataView * dom_zone_view = dom_gp->createViewAndBuffer("zone_data");
    dom_zone_view->allocate( DataType::c_int(zonemv->getNumVals(dom_ext)) );

    MeshVar * nodemv = static_cast<MeshVar *>( node_mv_view->getOpaque() );
    DataView * dom_node_view = dom_gp->createViewAndBuffer("node_data");
    dom_node_view->allocate( DataType::c_double(nodemv->getNumVals(dom_ext)) );

  }

//
//  Print datastore contents to see what's going on.
//
//  ds->print();


  //
  // Check data lengths
  //
  for (int idom = 0 ; idom < 2 ; ++idom)
  {

    DataGroup * dom_gp = problem_gp->getGroup(dom_name[idom]);
    Extent * dom_ext = static_cast<Extent *>(
      dom_gp->getView("ext")->getOpaque() );

    MeshVar * zonemv = static_cast<MeshVar *>( zone_mv_view->getOpaque() );
    MeshVar * nodemv = static_cast<MeshVar *>( node_mv_view->getOpaque() );

    int num_zone_vals = zonemv->getNumVals(dom_ext);
    int test_num_zone_vals = dom_gp->getView("zone_data")->getNumberOfElements();
    EXPECT_EQ(num_zone_vals, test_num_zone_vals);

    int num_node_vals = nodemv->getNumVals(dom_ext);
    int test_num_node_vals = dom_gp->getView("node_data")->getNumberOfElements();
    EXPECT_EQ(num_node_vals, test_num_node_vals);

  }

  // clean up...
  delete zone_mv;
  delete node_mv;
  for (int idom = 0 ; idom < 2 ; ++idom)
  {
    delete static_cast<Extent *>(
      problem_gp->getGroup(dom_name[idom])->getView("ext")->getOpaque() );
  }
  delete ds;
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
#include "slic/UnitTestLogger.hpp"
using asctoolkit::slic::UnitTestLogger;

int main(int argc, char * argv[])
{
  int result = 0;

  ::testing::InitGoogleTest(&argc, argv);

  UnitTestLogger logger;   // create & initialize test logger,
  // finalized when exiting main scope

  result = RUN_ALL_TESTS();

  return result;
}
