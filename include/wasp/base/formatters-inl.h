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

#include "wasp/base/macros.h"

namespace fmt {

template <typename T, std::size_t SIZE, typename Allocator>
string_view to_string_view(const basic_memory_buffer<T, SIZE, Allocator>& buf) {
  return string_view{buf.data(), buf.size()};
}

template <typename T>
template <typename Ctx>
typename Ctx::iterator formatter<::wasp::At<T>>::format(
    const ::wasp::At<T>& self,
    Ctx& ctx) {
  return formatter<T>::format(*self, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::string_view>::format(
    const ::wasp::string_view& self,
    Ctx& ctx) {
  return formatter<string_view>::format(string_view(self.data(), self.size()),
                                        ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::SpanU8>::format(
    const ::wasp::SpanU8& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "\"");
  for (auto x : self) {
    format_to(buf, "\\{:02x}", x);
  }
  format_to(buf, "\"");
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename T>
template <typename Ctx>
typename Ctx::iterator formatter<::wasp::span<const T>>::format(
    ::wasp::span<const T> self,
    Ctx& ctx) {
  memory_buffer buf;
  string_view space = "";
  format_to(buf, "[");
  for (const auto& x : self) {
    format_to(buf, "{}{}", space, x);
    space = " ";
  }
  format_to(buf, "]");
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename T>
template <typename Ctx>
typename Ctx::iterator formatter<std::vector<T>>::format(
    const std::vector<T>& self,
    Ctx& ctx) {
  return formatter<::wasp::span<const T>>::format(::wasp::span<const T>{self},
                                                  ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::v128>::format(::wasp::v128 self,
                                                       Ctx& ctx) {
  memory_buffer buf;
  string_view space = "";
  for (const auto& x : self.as<::wasp::u32x4>()) {
    format_to(buf, "{}{:#x}", space, x);
    space = " ";
  }
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::Opcode>::format(
    const ::wasp::Opcode& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
#define WASP_V(prefix, val, Name, str, ...) \
  case ::wasp::Opcode::Name:                \
    result = str;                           \
    break;
#define WASP_FEATURE_V(...) WASP_V(__VA_ARGS__)
#define WASP_PREFIX_V(...) WASP_V(__VA_ARGS__)
#include "wasp/base/def/opcode.def"
#undef WASP_V
#undef WASP_FEATURE_V
#undef WASP_PREFIX_V
    default: {
      // Special case for opcodes with unknown ids.
      memory_buffer buf;
      format_to(buf, "<unknown:{}>", static_cast<::wasp::u32>(self));
      return formatter<string_view>::format(to_string_view(buf), ctx);
    }
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::ValueType>::format(
    const ::wasp::ValueType& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
#define WASP_V(val, Name, str, ...) \
  case ::wasp::ValueType::Name:     \
    result = str;                   \
    break;
#define WASP_FEATURE_V(...) WASP_V(__VA_ARGS__)
#include "wasp/base/def/value_type.def"
#undef WASP_V
#undef WASP_FEATURE_V
    default:
      WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

}  // namespace fmt
