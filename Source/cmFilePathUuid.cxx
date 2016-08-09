/*============================================================================
  CMake - Cross Platform Makefile Generator
  Copyright 2016 Sebastian Holtermann (sebholt@xwmw.org)

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/

#include "cmFilePathUuid.h"

#include "cmCryptoHash.h"
#include "cmMakefile.h"
#include "cmSystemTools.h"
#include "cmsys/Base64.h"

cmFilePathUuid::cmFilePathUuid(cmMakefile* makefile)
{
  initParentDirs(makefile->GetCurrentSourceDirectory(),
                 makefile->GetCurrentBinaryDirectory(),
                 makefile->GetHomeDirectory(),
                 makefile->GetHomeOutputDirectory());
}

cmFilePathUuid::cmFilePathUuid(const std::string& currentSrcDir,
                               const std::string& currentBinDir,
                               const std::string& projectSrcDir,
                               const std::string& projectBinDir)
{
  initParentDirs(currentSrcDir, currentBinDir, projectSrcDir, projectBinDir);
}

void cmFilePathUuid::initParentDirs(const std::string& currentSrcDir,
                                    const std::string& currentBinDir,
                                    const std::string& projectSrcDir,
                                    const std::string& projectBinDir)
{
  parentDirs[0].first = cmsys::SystemTools::GetRealPath(currentSrcDir);
  parentDirs[1].first = cmsys::SystemTools::GetRealPath(currentBinDir);
  parentDirs[2].first = cmsys::SystemTools::GetRealPath(projectSrcDir);
  parentDirs[3].first = cmsys::SystemTools::GetRealPath(projectBinDir);

  parentDirs[0].second = "CurrentSource";
  parentDirs[1].second = "CurrentBinary";
  parentDirs[2].second = "ProjectSource";
  parentDirs[3].second = "ProjectBinary";
}

std::string cmFilePathUuid::get(const std::string& filePath,
                                const char* outputPrefix,
                                const char* outputSuffix)
{
  std::string sourceFilename = cmsys::SystemTools::GetFilenameName(filePath);
  std::string sourceBasename =
    cmsys::SystemTools::GetFilenameWithoutLastExtension(sourceFilename);

  // Acquire checksum string
  std::string checksum;
  {
    std::string sourceRelPath;
    std::string sourceRelSeed;
    GetRelPathSeed(filePath, sourceRelPath, sourceRelSeed);
    checksum = GetChecksumString(sourceFilename, sourceRelPath, sourceRelSeed);
  }

  // Compose the file name
  std::string uuid;
  if (outputPrefix) {
    uuid += outputPrefix;
  }
  uuid += sourceBasename.substr(0, partLengthName);
  uuid += "_";
  uuid += checksum.substr(0, partLengthCheckSum);
  if (outputSuffix) {
    uuid += outputSuffix;
  }
  return uuid;
}

void cmFilePathUuid::GetRelPathSeed(const std::string& filePath,
                                    std::string& sourceRelPath,
                                    std::string& sourceRelSeed)
{
  const std::string sourceNameReal = cmsys::SystemTools::GetRealPath(filePath);
  std::string parentDirectory;
  // Find closest project parent directory
  for (size_t ii = 0; ii != numParentDirs; ++ii) {
    const std::string& pDir = parentDirs[ii].first;
    if (!pDir.empty() &&
        cmsys::SystemTools::IsSubDirectory(sourceNameReal, pDir)) {
      sourceRelSeed = parentDirs[ii].second;
      parentDirectory = pDir;
      break;
    }
  }
  // Check if the file path is below a known project directory
  if (parentDirectory.empty()) {
    // Use file syste root as fallback parent directory
    sourceRelSeed = "FileSystemRoot";
    cmsys::SystemTools::SplitPathRootComponent(sourceNameReal,
                                               &parentDirectory);
  }
  sourceRelPath = cmsys::SystemTools::RelativePath(
    parentDirectory, cmsys::SystemTools::GetParentDirectory(sourceNameReal));
}

std::string cmFilePathUuid::GetChecksumString(
  const std::string& sourceFilename, const std::string& sourceRelPath,
  const std::string& sourceRelSeed)
{
  // Calculate the file ( seed + relative path + name ) checksum
  std::string checksumBase64;

  std::vector<unsigned char> hashBytes;
  {
    // Acquire hash in a hex value string
    std::string hexHash = cmCryptoHash::New("SHA256")->HashString(
      (sourceRelSeed + sourceRelPath + sourceFilename).c_str());
    // Convert hex value string to bytes
    hashBytes.resize(hexHash.size() / 2);
    for (unsigned int ii = 0; ii != hashBytes.size(); ++ii) {
      unsigned char hbyte[2] = { 0, 0 };
      for (unsigned int jj = 0; jj != 2; ++jj) {
        const unsigned char nibble = hexHash[ii * 2 + jj];
        if ('0' <= nibble && nibble <= '9') {
          hbyte[jj] = static_cast<unsigned char>(nibble - '0');
        } else if ('a' <= nibble && nibble <= 'f') {
          hbyte[jj] = static_cast<unsigned char>(nibble - 'a' + 10);
        } else if ('A' <= nibble && nibble <= 'f') {
          hbyte[jj] = static_cast<unsigned char>(nibble - 'A' + 10);
        } else {
          // Unexpected non hex character
          std::cerr << "Unexpected non hex character in checksum string";
          exit(-1);
        }
      }
      hashBytes[ii] = static_cast<unsigned char>(hbyte[1] | (hbyte[0] << 4));
    }
  }
  // Convert hash bytes to Base64 text string
  {
    std::vector<unsigned char> base64Bytes(hashBytes.size() * 2, 0);
    cmsysBase64_Encode(&hashBytes[0], hashBytes.size(), &base64Bytes[0], 0);
    checksumBase64 = reinterpret_cast<const char*>(&base64Bytes[0]);
    // Base64 allows '+' and '/' characters.
    // Both are problematic when used in file names.
    // Replace them with safer alternatives.
    std::replace(checksumBase64.begin(), checksumBase64.end(), '+', '_');
    std::replace(checksumBase64.begin(), checksumBase64.end(), '/', '-');
  }

  return checksumBase64;
}
