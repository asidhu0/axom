#include "multimat/multimat.hpp" 

#include <iostream>
#include <sstream>
#include <iterator>
#include <algorithm>

#include "slic/slic.hpp"
#include <cassert>

#include "multimat.hpp"


using namespace std;
using namespace axom::multimat;


MultiMat::MultiMat():
  m_cellMatRel(nullptr),
  m_cellMatNZSet(nullptr),
  m_cellMatProdSet(nullptr)
{
  m_ncells = m_nmats = 0;
  m_dataLayout = DataLayout::CELL_CENTRIC;
  m_sparcityLayout = SparcityLayout::SPARSE;
}

MultiMat::MultiMat(DataLayout d, SparcityLayout s): MultiMat() {
  m_dataLayout = d;
  m_sparcityLayout = s;
}

MultiMat::~MultiMat()
{
  for (auto mapPtr : m_mapVec) {
    delete mapPtr;
  }
  delete m_cellMatProdSet;
  delete m_cellMatNZSet;
  delete m_cellMatRel;
}

template<typename T>
MultiMat::MapBaseType* MultiMat::helperfun_copyField(const MultiMat& mm, int map_i)
{
  MapBaseType* other_map_ptr = mm.m_mapVec[map_i];
  if (mm.getFieldMapping(map_i) == FieldMapping::PER_CELL_MAT)
  {
    BivariateSetType* biSetPtr = dynamic_cast<BivariateSetType*>(get_mapped_set(getFieldMapping(map_i)));

    MultiMat::Field2D<T>* typed_ptr = dynamic_cast<MultiMat::Field2D<T>*>(other_map_ptr);
    MultiMat::Field2D<T>* new_ptr = new MultiMat::Field2D<T>(biSetPtr, T(), typed_ptr->stride());
    new_ptr->copy(typed_ptr->getMap()->data().data());
    return new_ptr;
  }
  else
  {
    SetType* setPtr = get_mapped_set(getFieldMapping(map_i));
    MultiMat::Field1D<T>* typed_ptr = dynamic_cast<MultiMat::Field1D<T>*>(other_map_ptr);
    MultiMat::Field1D<T>* new_ptr = new MultiMat::Field1D<T>(setPtr, T(), typed_ptr->stride());
    new_ptr->copy(*typed_ptr);
    return new_ptr;
  }
}


MultiMat::MultiMat(const MultiMat& other) :
  m_ncells(other.m_ncells),
  m_nmats(other.m_nmats),
  m_dataLayout( other.m_dataLayout),
  m_sparcityLayout( other.m_sparcityLayout),
  m_cellSet(0,other.m_ncells),
  m_matSet(0, other.m_nmats),
  m_cellMatRel_beginsVec(other.m_cellMatRel_beginsVec),
  m_cellMatRel_indicesVec(other.m_cellMatRel_indicesVec),
  m_cellMatRel(nullptr),
  m_cellMatNZSet(nullptr),
  m_cellMatProdSet(nullptr),
  m_arrNameVec(other.m_arrNameVec),
  m_fieldMappingVec(other.m_fieldMappingVec),
  m_dataTypeVec(other.m_dataTypeVec)
{
  RangeSetType& set1 = (m_dataLayout == DataLayout::CELL_CENTRIC ? m_cellSet : m_matSet);
  RangeSetType& set2 = (m_dataLayout == DataLayout::CELL_CENTRIC ? m_matSet : m_cellSet);
  m_cellMatRel = new StaticVariableRelationType(&set1, &set2);
  m_cellMatRel->bindBeginOffsets(set1.size(), &m_cellMatRel_beginsVec);
  m_cellMatRel->bindIndices(m_cellMatRel_indicesVec.size(), &m_cellMatRel_indicesVec);
  m_cellMatNZSet = new RelationSetType(m_cellMatRel);
  m_cellMatProdSet = new ProductSetType(&set1, &set2);

  for (unsigned int map_i = 0; map_i < other.m_mapVec.size(); ++map_i)
  {
    MapBaseType* new_map_ptr = nullptr;
    if (m_dataTypeVec[map_i] == DataTypeSupported::TypeDouble) {
      new_map_ptr = helperfun_copyField<double>(other, map_i);
    }
    else if (m_dataTypeVec[map_i] == DataTypeSupported::TypeFloat) {
      new_map_ptr = helperfun_copyField<float>(other, map_i);
    }
    else if (m_dataTypeVec[map_i] == DataTypeSupported::TypeInt) {
      new_map_ptr = helperfun_copyField<int>(other, map_i);
    }
    else if (m_dataTypeVec[map_i] == DataTypeSupported::TypeUnsignChar) {
      new_map_ptr = helperfun_copyField<unsigned char>(other, map_i);
    }
    else assert(false); //TODO
    
    SLIC_ASSERT(new_map_ptr != nullptr);
    m_mapVec.push_back(new_map_ptr);
  }
}

MultiMat& MultiMat::operator=(const MultiMat& other)
{
  if (this == &other) return *this;

  SLIC_ASSERT(false); //TODO

  return *this;
}


void MultiMat::setNumberOfMat(int n)
{
  assert(n > 0);
  m_nmats = n;

  m_matSet = RangeSetType(0, m_nmats);
  assert(m_matSet.isValid());
}

void MultiMat::setNumberOfCell(int c)
{
  assert(c > 0);
  m_ncells = c;

  m_cellSet = RangeSetType(0, m_ncells);
  assert(m_cellSet.isValid());
}

void MultiMat::setCellMatRel(vector<bool>& vecarr)
{
  //Setup the SLAM cell to mat relation
  //This step is necessary if the volfrac field is sparse

  SLIC_ASSERT(vecarr.size() == m_ncells * m_nmats); //This should be a dense matrix
  SLIC_ASSERT(m_cellMatRel == nullptr); //cellmatRel has not been set before

  RangeSetType& set1 = (m_dataLayout == DataLayout::CELL_CENTRIC ? m_cellSet : m_matSet);
  RangeSetType& set2 = (m_dataLayout == DataLayout::CELL_CENTRIC ? m_matSet : m_cellSet);

  //count the non-zeros
  int nz_count = 0;
  for (bool b : vecarr)
    nz_count += b;

  //Set-up the cell/mat relation
  m_cellMatRel_beginsVec.resize(set1.size() + 1, -1);
  m_cellMatRel_indicesVec.resize(nz_count);

  SetPosType curIdx = SetPosType();
  for (SetPosType i = 0; i < set1.size(); ++i)
  {
    m_cellMatRel_beginsVec[i] = curIdx;
    for (SetPosType j = 0; j < set2.size(); ++j)
    {
      if (vecarr[i*set2.size() + j]) {
        m_cellMatRel_indicesVec[curIdx] = j;
        ++curIdx;
      }
    }
  }
  m_cellMatRel_beginsVec[set1.size()] = curIdx;

  m_cellMatRel = new StaticVariableRelationType(&set1, &set2);
  m_cellMatRel->bindBeginOffsets(set1.size(), &m_cellMatRel_beginsVec);
  m_cellMatRel->bindIndices(m_cellMatRel_indicesVec.size(), &m_cellMatRel_indicesVec);

  assert(m_cellMatRel->isValid());
  
  //Set-up both dense and sparse bivariate sets.
  m_cellMatNZSet = new RelationSetType(m_cellMatRel);
  m_cellMatProdSet = new ProductSetType(&set1, &set2);
  
  //Create a field for VolFrac as the 0th field
  m_mapVec.push_back(nullptr);
  m_arrNameVec.push_back("Volfrac");
  m_fieldMappingVec.push_back(FieldMapping::PER_CELL_MAT);
  m_dataTypeVec.push_back(DataTypeSupported::TypeDouble);
  assert(m_mapVec.size() == 1);
  assert(m_arrNameVec.size() == 1);
  assert(m_fieldMappingVec.size() == 1);
  assert(m_dataTypeVec.size() == 1);

}


int MultiMat::setVolfracField(double* arr)
{
  //Assumes this is to CellxMat mapping, named "Volfrac", and is stride 1.
  int arr_i = addFieldArray_impl<double>("Volfrac", FieldMapping::PER_CELL_MAT, arr, 1);

  //checks volfrac values sums to 1
  auto& map = *dynamic_cast<Field2D<double>*>(m_mapVec[arr_i]);
  double tol = 10e-9;
  if (m_dataLayout == DataLayout::CELL_CENTRIC) 
  {
    for (int i = 0; i < map.firstSetSize(); ++i)
    {
      double sum = 0.0;
      for (auto iter = map.begin(i); iter != map.end(i); ++iter)
      {
        sum += iter.value();
      }

      SLIC_ASSERT(abs(sum - 1.0) < tol);
    }
  }
  else //material centric layout
  {
    std::vector<double> sum_vec(m_cellSet.size(), 0);
    for (int i = 0; i < map.firstSetSize(); ++i)
    {
      for (auto iter = map.begin(i); iter != map.end(i); ++iter)
      {
        sum_vec[iter.index()] += iter.value();
      }
    }

    for(unsigned int i=0; i<sum_vec.size(); ++i)
      SLIC_ASSERT(abs(sum_vec[i] - 1.0) < tol);
  }

  //move the data to the first one (index 0) in the list
  std::iter_swap(m_mapVec.begin(), m_mapVec.begin() + arr_i);
  std::iter_swap(m_dataTypeVec.begin(), m_dataTypeVec.begin() + arr_i);

  //remove the new entry...
  int nfield = m_mapVec.size() - 1;
  m_mapVec.resize(nfield); //TODO if delete is needed
  m_fieldMappingVec.resize(nfield);
  m_arrNameVec.resize(nfield);
  m_dataTypeVec.resize(nfield);

  return 0;
}


MultiMat::Field2D<double>& MultiMat::getVolfracField()
{
  return *dynamic_cast<Field2D<double>*>(m_mapVec[0]);
}


int MultiMat::getFieldIdx(const std::string& field_name) const
{
  for (unsigned int i = 0; i < m_arrNameVec.size(); i++)
  {
    if (m_arrNameVec[i] == field_name)
      return i;
  }

  return -1;
}


MultiMat::IdSet MultiMat::getMatInCell(int c)
{
  SLIC_ASSERT(m_dataLayout == DataLayout::CELL_CENTRIC);
  SLIC_ASSERT(m_cellMatRel != nullptr);

  if (m_sparcityLayout == SparcityLayout::SPARSE) {
    return (*m_cellMatRel)[c]; //returns a RelationSet / OrderedSet with STLindirection
  }
  else if (m_sparcityLayout == SparcityLayout::DENSE)
    return (*m_cellMatRel)[c]; //since the relation is currently only stored sparse, return the same thing.
  else assert(false);
}


MultiMat::IndexSet MultiMat::getIndexingSetOfCell(int c)
{
  assert(m_dataLayout == DataLayout::CELL_CENTRIC);
  assert(0 <= c && c < (int)m_ncells);

  if (m_sparcityLayout == SparcityLayout::SPARSE) {
    int start_idx = m_cellMatRel_beginsVec[c];
    int end_idx = m_cellMatRel_beginsVec[c + 1];
    return RangeSetType::SetBuilder().range(start_idx, end_idx);
  }
  else if (m_sparcityLayout == SparcityLayout::DENSE) {
    //return m_cellMatProdSet.getRow(c);
    int size2 = m_cellMatProdSet->secondSetSize();
    return RangeSetType::SetBuilder().range(c*size2, (c + 1)*size2 - 1);
  }
  else assert(false);
}

void MultiMat::convertLayoutToCellDominant()
{
  if (m_dataLayout == DataLayout::CELL_CENTRIC) return;
  transposeData();
}

void MultiMat::convertLayoutToMaterialDominant() 
{ 
  if (m_dataLayout == DataLayout::MAT_CENTRIC) return;
  transposeData();
}

void MultiMat::transposeData()
{
  //create the new relation to pass into helper...
  RangeSetType& set1 = *(m_cellMatRel->fromSet());
  RangeSetType& set2 = *(m_cellMatRel->toSet());
  
  if(isCellDom()) SLIC_ASSERT(&set1 == &m_cellSet);
  else SLIC_ASSERT(&set1 == &m_matSet);

  auto nz_count = m_cellMatRel_indicesVec.size();
  StaticVariableRelationType* new_cellMatRel = nullptr;
  std::vector<SetPosType> new_cellMatRel_beginsVec(set2.size() + 1, 0); 
                //initialized to 0 becuase it will be used for counting
  std::vector<SetPosType> new_cellMatRel_indicesVec(nz_count, -1);
  RelationSetType* new_cellMatNZSet = nullptr;
  ProductSetType* new_cellMatProdSet = nullptr;
  std::vector<SetPosType> move_indices(nz_count, -1); //map from old to new loc

  //construct the new transposed relation
  //count the non-zero in each rows
  for (auto idx1 = 0; idx1 < m_cellMatRel->fromSetSize(); ++idx1)
  {
    IdSet relSubset = (*m_cellMatRel)[idx1];
    for (auto j = 0; j < relSubset.size(); ++j)
    {
      auto idx2 = relSubset[j];
      new_cellMatRel_beginsVec[idx2] += 1;
    }
  }
  //add them to make this the end index
  {
    unsigned int i;
    for( i = 1; i < new_cellMatRel_beginsVec.size() - 1; i++)
    {
      new_cellMatRel_beginsVec[i] += new_cellMatRel_beginsVec[i - 1];
    }
    new_cellMatRel_beginsVec[i] = new_cellMatRel_beginsVec[i - 1];
  }
  //fill in the indicesVec and the move_indices backward
  for (auto idx1 = m_cellMatRel->fromSetSize() - 1; idx1 >= 0; --idx1)
  {
    IdSet relSubset = (*m_cellMatRel)[idx1];
    for (auto j = relSubset.size()-1; j >= 0; --j)
    {
      auto idx2 = relSubset[j];
      auto compress_idx = --new_cellMatRel_beginsVec[idx2];
      new_cellMatRel_indicesVec[compress_idx] = idx1;
      move_indices[ m_cellMatRel_beginsVec[idx1] + j ] = compress_idx;
    }
  }

  new_cellMatRel = new StaticVariableRelationType(&set2, &set1);
  new_cellMatRel->bindBeginOffsets(set2.size(), &new_cellMatRel_beginsVec);
  new_cellMatRel->bindIndices(new_cellMatRel_indicesVec.size(), &new_cellMatRel_indicesVec);


  new_cellMatNZSet = new RelationSetType(new_cellMatRel);
  new_cellMatProdSet = new ProductSetType(&set2, &set1);
  
  for (unsigned int map_i = 0; map_i < m_fieldMappingVec.size(); map_i++)
  {
    //no conversion needed unless the field is PER_CELL_MAT
    if (m_fieldMappingVec[map_i] != FieldMapping::PER_CELL_MAT)
      continue;

    if (m_dataTypeVec[map_i] == DataTypeSupported::TypeDouble) {
      transposeData_helper<double>(map_i, new_cellMatNZSet, new_cellMatProdSet, move_indices);
    }
    else if (m_dataTypeVec[map_i] == DataTypeSupported::TypeFloat) {
      transposeData_helper<float>(map_i, new_cellMatNZSet, new_cellMatProdSet, move_indices);
    }
    else if (m_dataTypeVec[map_i] == DataTypeSupported::TypeInt) {
      transposeData_helper<int>(map_i, new_cellMatNZSet, new_cellMatProdSet, move_indices);
    }
    else if (m_dataTypeVec[map_i] == DataTypeSupported::TypeUnsignChar) {
      transposeData_helper<unsigned char>(map_i, new_cellMatNZSet, new_cellMatProdSet, move_indices);
    }
    else assert(false); //TODO
  }

  if(m_dataLayout == DataLayout::MAT_CENTRIC)
    m_dataLayout = DataLayout::CELL_CENTRIC;
  else
    m_dataLayout = DataLayout::MAT_CENTRIC;

  //switch vectors and rebind relation begin offset
  new_cellMatRel_beginsVec.swap(m_cellMatRel_beginsVec);
  new_cellMatRel_indicesVec.swap(m_cellMatRel_indicesVec);
  new_cellMatRel->bindBeginOffsets(set2.size(), &m_cellMatRel_beginsVec);
  new_cellMatRel->bindIndices(m_cellMatRel_indicesVec.size(), &m_cellMatRel_indicesVec);

  //delete old relation and biSets
  delete m_cellMatNZSet;
  delete m_cellMatRel;
  delete m_cellMatProdSet;
  m_cellMatRel = new_cellMatRel;
  m_cellMatNZSet = new_cellMatNZSet;
  m_cellMatProdSet = new_cellMatProdSet;
}

void MultiMat::convertLayoutToSparse() 
{ 
  if (m_sparcityLayout == SparcityLayout::SPARSE) return;
  
  for (unsigned int map_i = 0; map_i < m_fieldMappingVec.size(); map_i++)
  {
    //no conversion needed unless the field is PER_CELL_MAT
    if (m_fieldMappingVec[map_i] != FieldMapping::PER_CELL_MAT)
      continue;

    if (m_dataTypeVec[map_i] == DataTypeSupported::TypeDouble) {
      convertToSparse_helper<double>(map_i);
    }
    else if (m_dataTypeVec[map_i] == DataTypeSupported::TypeFloat) {
      convertToSparse_helper<float>(map_i);
    }
    else if (m_dataTypeVec[map_i] == DataTypeSupported::TypeInt) {
      convertToSparse_helper<int>(map_i);
    }
    else if (m_dataTypeVec[map_i] == DataTypeSupported::TypeUnsignChar) {
      convertToSparse_helper<unsigned char>(map_i);
    }
    else assert(false); //TODO
  }
  m_sparcityLayout = SparcityLayout::SPARSE;
}


void MultiMat::convertLayoutToDense() 
{ 
  if(m_sparcityLayout == SparcityLayout::DENSE) return;

  for (unsigned int map_i = 0; map_i < m_fieldMappingVec.size(); map_i++)
  {
    //no conversion needed unless the field is PER_CELL_MAT
    if (m_fieldMappingVec[map_i] != FieldMapping::PER_CELL_MAT)
      continue;

    if (m_dataTypeVec[map_i] == DataTypeSupported::TypeDouble) {
      convertToDense_helper<double>(map_i);
    }
    else if (m_dataTypeVec[map_i] == DataTypeSupported::TypeFloat) {
      convertToDense_helper<float>(map_i);
    }
    else if (m_dataTypeVec[map_i] == DataTypeSupported::TypeInt) {
      convertToDense_helper<int>(map_i);
    }
    else if (m_dataTypeVec[map_i] == DataTypeSupported::TypeUnsignChar) {
      convertToDense_helper<unsigned char>(map_i);
    }
    else assert(false); //TODO
  }
  m_sparcityLayout = SparcityLayout::DENSE;
}


void MultiMat::convertLayout(DataLayout new_layout, SparcityLayout new_sparcity)
{
  if (new_layout == m_dataLayout && new_sparcity == m_sparcityLayout)
    return;

  //sparse/dense conversion
  if (m_sparcityLayout == SparcityLayout::DENSE && new_sparcity == SparcityLayout::SPARSE)
  {
    convertLayoutToSparse();
  }
  else if(m_sparcityLayout == SparcityLayout::SPARSE && new_sparcity == SparcityLayout::DENSE)
  {
    convertLayoutToDense();
  }

  //cell/mat centric conversion
  if (m_dataLayout == DataLayout::CELL_CENTRIC && new_layout == DataLayout::MAT_CENTRIC)
  {
    convertLayoutToMaterialDominant();
  }
  else if(m_dataLayout == DataLayout::MAT_CENTRIC && new_layout == DataLayout::CELL_CENTRIC)
  {
    convertLayoutToCellDominant();
  }
}


std::string MultiMat::getDataLayoutAsString() const
{
  if(isCellDom())
    return "Cell-Centric";
  else if(isMatDom())
    return "Material-Centric";
  else 
    SLIC_ASSERT(false);
  return "";
}

std::string MultiMat::getSparcityLayoutAsString() const
{
  if(isSparse())
    return "Sparse";
  else if(isDense())
    return "Dense";
  else
    SLIC_ASSERT(false);
  return "";
}


void MultiMat::print() const
{
  std::stringstream sstr;

  sstr << "  Multimat Object Details:";
  sstr << "\nNumber of materials: " << m_nmats;
  sstr << "\nNumber of cells:     "<< m_ncells;
  sstr << "\nData layout:     " << getDataLayoutAsString();
  sstr << "\nSparcity layout: " << getSparcityLayoutAsString();
  
  sstr << "\n\n Number of fields: " << m_mapVec.size() << "\n";
  for (unsigned int i = 0; i < m_mapVec.size(); i++)
  {
    sstr << "Field " << i << ": " << m_arrNameVec[i].c_str();
    sstr << "  Mapping per ";
    switch (m_fieldMappingVec[i]) {
    case FieldMapping::PER_CELL: sstr << "cell"; break;
    case FieldMapping::PER_MAT: sstr << "material"; break;
    case FieldMapping::PER_CELL_MAT: sstr << "cellXmaterial"; break;
    }
  }
  sstr << "\n\n";

  cout << sstr.str() << endl;
}


bool MultiMat::isValid(bool verboseOutput) const
{
  bool bValid = true;
  std::stringstream errStr;

  if (m_cellSet.size() > 0 && m_matSet.size() > 0)
  {
    //It's a non-empty MultiMat object. 
    //Check Volfrac exists
    if (m_mapVec[0] == nullptr)
    {
      errStr << "\n\t*No Volfrac field added.";
      bValid = false;
    }
    else
    {
      //Check Volfrac values match the relation value (if dense) 
      const Field2D<double>& volfrac_map = *static_cast<Field2D<double>*>(m_mapVec[0]);
      std::vector<double> volfrac_sum(m_cellSet.size(), 0.0);
      if (isDense())
      {
        for (int i = 0; i < volfrac_map.firstSetSize(); ++i)
        {
          auto rel_iter = m_cellMatRel->begin(i);
          for (int j = 0; j < volfrac_map.secondSetSize(); ++j)
          {
            bool zero_volfrac = volfrac_map[i* volfrac_map.secondSetSize() + j] == 0.0; //exact comp?
            if (*rel_iter == j)
            { //cellmat rel is present
              if (zero_volfrac)
              {
                errStr << "\n\t*Volume fraction is zero for a cellmat with relation"; //TODO reword
                bValid = false;
              }
              ++rel_iter;
            }
            else
            {
              if (!zero_volfrac)
              {
                errStr << "\n\t*Volume fraction is non-zero for an empty cellmat relation";
                bValid = false;
              }
            }
          }
        }
      }

      //Check Volfrac sums to up 1.0
      for (int i = 0; i < volfrac_map.firstSetSize(); ++i)
      {
        auto& submap = volfrac_map(i);
        for (int j = 0; j < submap.size(); ++j)
        {
          if (isCellDom())
            volfrac_sum[i] += submap.value(j);
          else
            volfrac_sum[submap.index(j)] += submap.value(j);
        }
      }
      for (unsigned int i = 0; i < volfrac_sum.size(); ++i)
      {
        if (abs(volfrac_sum[i] - 1.0) > 10e-9)
        {
          errStr << "\n\t*Volfrac does not sum to 1.0 in cell " << i;
          bValid = false;
        }
      }

    }

  }

  if (verboseOutput)
  {
    if (bValid)
    {
      errStr << "MultiMat data was valid";
    }

    std::cout << errStr.str() << std::endl;
  }


  return bValid;
}

MultiMat::SetType* MultiMat::get_mapped_set(FieldMapping fm)
{
  SetType* set_ptr = nullptr;
  switch (fm)
  {
  case FieldMapping::PER_CELL:
    set_ptr = &m_cellSet;
    break;
  case FieldMapping::PER_MAT:
    set_ptr = &m_matSet;
    break;
  case FieldMapping::PER_CELL_MAT:
    set_ptr = dynamic_cast<SetType*>(get_mapped_biSet());
    break;
  default:
    assert(false);
    return nullptr;
  }
  return set_ptr;
}

MultiMat::BivariateSetType* MultiMat::get_mapped_biSet()
{
  BivariateSetType* set_ptr = nullptr;
  if (m_sparcityLayout == SparcityLayout::SPARSE)
    set_ptr = m_cellMatNZSet;
  else if (m_sparcityLayout == SparcityLayout::DENSE)
    set_ptr = m_cellMatProdSet;
  
  SLIC_ASSERT(set_ptr != nullptr);
  return set_ptr;
}
