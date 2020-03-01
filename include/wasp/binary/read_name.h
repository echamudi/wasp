//
// Copyright 2020 WebAssembly Community Group participants
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

#ifndef WASP_BINARY_READ_NAME_H_
#define WASP_BINARY_READ_NAME_H_

#include "wasp/binary/read.h"
#include "wasp/binary/types_name.h"

namespace wasp {
namespace binary {

struct Context;

auto Read(SpanU8*, Context&, Tag<IndirectNameAssoc>)
    -> OptAt<IndirectNameAssoc>;
auto Read(SpanU8*, Context&, Tag<NameAssoc>) -> OptAt<NameAssoc>;
auto Read(SpanU8*, Context&, Tag<NameSubsection>) -> OptAt<NameSubsection>;
auto Read(SpanU8*, Context&, Tag<NameSubsectionId>) -> OptAt<NameSubsectionId>;

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_NAME_H_
