/*============================================================================
  CMake - Cross Platform Makefile Generator
  Copyright 2000-2009 Kitware, Inc., Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#include "cmRemoveDefinitionsCommand.h"

// cmRemoveDefinitionsCommand
bool cmRemoveDefinitionsCommand::InitialPass(
  std::vector<std::string> const& args, cmExecutionStatus&)
{
  // it is OK to have no arguments
  if (args.empty()) {
    return true;
  }

  for (std::vector<std::string>::const_iterator i = args.begin();
       i != args.end(); ++i) {
    this->Makefile->RemoveDefineFlag(i->c_str());
  }
  return true;
}
