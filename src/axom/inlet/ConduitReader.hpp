// Copyright (c) 2017-2020, Lawrence Livermore National Security, LLC and
// other Axom Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

/*!
 *******************************************************************************
 * \file ConduitReader.hpp
 *
 * \brief This file contains the class definition of the ConduitReader.
 *******************************************************************************
 */

#ifndef INLET_CONDUITREADER_HPP
#define INLET_CONDUITREADER_HPP

#include "axom/inlet/Reader.hpp"

#include "conduit.hpp"

namespace axom
{
namespace inlet
{
/*!
 *******************************************************************************
 * \class ConduitReader
 *
 * \brief A Reader that is able to read variables from a file compatible with
 * one of Conduit's protocols.
 *
 * \see Reader
 * 
 * \note This class should not be used directly, instead, use the YAMLReader
 * and JSONReader derived classes
 *******************************************************************************
 */
class ConduitReader : public Reader
{
public:
  /*!
   *****************************************************************************
   * \brief Constructs a reader with the selected Conduit protocol
   * \param [in] protocol The Conduit protocol, currently only "yaml" and "json"
   * are supported
   *****************************************************************************
   */
  ConduitReader(const std::string& protocol);

  bool parseFile(const std::string& filePath) override;

  bool parseString(const std::string& stringToRead) override;

  bool getBool(const std::string& id, bool& value) override;

  bool getDouble(const std::string& id, double& value) override;

  bool getInt(const std::string& id, int& value) override;

  bool getString(const std::string& id, std::string& value) override;

  bool getIntMap(const std::string& id,
                 std::unordered_map<int, int>& values) override;
  bool getIntMap(const std::string& id,
                 std::unordered_map<std::string, int>& values) override;

  bool getDoubleMap(const std::string& id,
                    std::unordered_map<int, double>& values) override;
  bool getDoubleMap(const std::string& id,
                    std::unordered_map<std::string, double>& values) override;

  bool getBoolMap(const std::string& id,
                  std::unordered_map<int, bool>& values) override;
  bool getBoolMap(const std::string& id,
                  std::unordered_map<std::string, bool>& values) override;

  bool getStringMap(const std::string& id,
                    std::unordered_map<int, std::string>& values) override;
  bool getStringMap(const std::string& id,
                    std::unordered_map<std::string, std::string>& values) override;

  bool getIndices(const std::string& id, std::vector<int>& indices) override;
  bool getIndices(const std::string& id,
                  std::vector<std::string>& indices) override;

  /*!
   *****************************************************************************
   * \brief Get a function from the input file
   *
   * \param [in]  id    The identifier to the function that will be retrieved
   * \param [in]  ret_type    The return type of the function
   * \param [in]  arg_types    The argument types of the function
   *
   * \return The function, compares false if not found
   * \note Conduit does not support functions - calling this function will result
   * in a SLIC_ERROR
   *****************************************************************************
   */
  FunctionVariant getFunction(const std::string& id,
                              const FunctionType ret_type,
                              const std::vector<FunctionType>& arg_types) override;

  /*!
   *****************************************************************************
   * \brief The base index for Conduit arrays
   *****************************************************************************
   */
  static const int baseIndex = 0;

private:
  bool getValue(const conduit::Node& node, int& value);
  bool getValue(const conduit::Node& node, std::string& value);
  bool getValue(const conduit::Node& node, double& value);
  bool getValue(const conduit::Node& node, bool& value);

  template <typename T>
  bool getDictionary(const std::string& id,
                     std::unordered_map<std::string, T>& values);

  template <typename T>
  bool getArray(const std::string& id, std::unordered_map<int, T>& values);
  conduit::Node m_root;
  const std::string m_protocol;
};

}  // end namespace inlet
}  // end namespace axom

#endif