//
// Copyright 2018 WebAssembly Community Group participants
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "wasp/base/file.h"

#include <fstream>
#include <string>

namespace wasp {

optional<Buffer> ReadFile(string_view filename) {
  std::ifstream stream{std::string{filename}, std::ios::in | std::ios::binary};
  if (!stream) {
    return nullopt;
  }

  Buffer buffer;
  stream.seekg(0, std::ios::end);
  buffer.resize(stream.tellg());
  stream.seekg(0, std::ios::beg);
  stream.read(reinterpret_cast<char*>(&buffer[0]), buffer.size());
  if (stream.fail()) {
    return nullopt;
  }

  return buffer;
}

}  // namespace wasp
