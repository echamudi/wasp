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

#ifndef WASP_BINARY_TYPES_H_
#define WASP_BINARY_TYPES_H_

#include <string>
#include <vector>

#include "src/base/types.h"

namespace wasp {
namespace binary {

struct BorrowedTraits {
  using Name = string_view;
  using Buffer = SpanU8;

  static Name ToName(string_view x) { return x; }
  static Buffer ToBuffer(SpanU8 x) { return x; }
};

struct OwnedTraits {
  using Name = std::string;
  using Buffer = std::vector<u8>;

  static Name ToName(std::string x) { return x; }
  static Name ToName(string_view x) { return Name{x}; }
  static Buffer ToBuffer(std::vector<u8> x) { return x; }
  static Buffer ToBuffer(SpanU8 x) { return Buffer{x.begin(), x.end()}; }
};

enum class ValType {
  I32,
  I64,
  F32,
  F64,
  Anyfunc,
  Func,
  Void,
};

enum class ExternalKind {
  Func,
  Table,
  Memory,
  Global,
};

enum class Mutability {
  Const,
  Var,
};

struct MemArg {
  MemArg(u32 align_log2, u32 offset) : align_log2(align_log2), offset(offset) {}

  u32 align_log2;
  u32 offset;
};

struct Limits {
  explicit Limits(u32 min) : min(min) {}
  Limits(u32 min, u32 max) : min(min), max(max) {}

  u32 min;
  optional<u32> max;
};

struct LocalDecl {
  LocalDecl(Index count, ValType type) : count(count), type(type) {}

  Index count;
  ValType type;
};

template <typename Traits = BorrowedTraits>
struct KnownSection {
  KnownSection(u32 id, SpanU8 data) : id(id), data(Traits::ToBuffer(data)) {}

  u32 id;
  typename Traits::Buffer data;
};

template <typename Traits = BorrowedTraits>
struct CustomSection {
  CustomSection(string_view name, SpanU8 data)
      : name(Traits::ToName(name)), data(Traits::ToBuffer(data)) {}

  typename Traits::Name name;
  typename Traits::Buffer data;
};

template <typename Traits = BorrowedTraits>
struct Section {
  template <typename T>
  explicit Section(T&& contents) : contents(std::move(contents)) {}

  bool is_known() const { return contents.index() == 0; }
  bool is_custom() const { return contents.index() == 1; }

  KnownSection<Traits>& known() { return absl::get<0>(contents); }
  const KnownSection<Traits>& known() const { return absl::get<0>(contents); }
  CustomSection<Traits>& custom() { return absl::get<1>(contents); }
  const CustomSection<Traits>& custom() const { return absl::get<1>(contents); }

  variant<KnownSection<Traits>, CustomSection<Traits>> contents;
};

using ValTypes = std::vector<ValType>;

struct FuncType {
  FuncType(ValTypes&& param_types, ValTypes&& result_types)
      : param_types(std::move(param_types)),
        result_types(std::move(result_types)) {}

  ValTypes param_types;
  ValTypes result_types;
};

struct TypeEntry {
  TypeEntry(ValType form, FuncType&& type)
      : form(form), type(std::move(type)) {}

  ValType form;
  FuncType type;
};

struct TableType {
  TableType(Limits limits, ValType elemtype)
      : limits(limits), elemtype(elemtype) {}

  Limits limits;
  ValType elemtype;
};

struct MemoryType {
  explicit MemoryType(Limits limits) : limits(limits) {}

  Limits limits;
};

struct GlobalType {
  GlobalType(ValType valtype, Mutability mut) : valtype(valtype), mut(mut) {}

  ValType valtype;
  Mutability mut;
};

template <typename Traits = BorrowedTraits>
struct Import {
  template <typename T>
  Import(string_view module, string_view name, T&& desc)
      : module(Traits::ToName(module)),
        name(Traits::ToName(name)),
        desc(std::move(desc)) {}

  ExternalKind kind() const { return static_cast<ExternalKind>(desc.index()); }

  typename Traits::Name module;
  typename Traits::Name name;
  variant<Index, TableType, MemoryType, GlobalType> desc;
};

template <typename Traits = BorrowedTraits>
struct Expr {
  explicit Expr(SpanU8 data) : data(Traits::ToBuffer(data)) {}
  template <typename U>
  Expr(const Expr<U>& other) : data(Traits::ToBuffer(other.data)) {}

  typename Traits::Buffer data;
};

struct Opcode {
  explicit Opcode(u8 code) : code(code) {}
  Opcode(u8 prefix, u32 code) : prefix(prefix), code(code) {}

  optional<u8> prefix;
  u32 code;
};

struct EmptyImmediate {};

struct CallIndirectImmediate {
  CallIndirectImmediate(Index index, u8 reserved)
      : index(index), reserved(reserved) {}

  Index index;
  u8 reserved;
};

struct BrTableImmediate {
  BrTableImmediate(std::vector<Index>&& targets, Index default_target)
      : targets(std::move(targets)), default_target(default_target) {}

  std::vector<Index> targets;
  Index default_target;
};

struct Instr {
  explicit Instr(Opcode opcode) : opcode(opcode), immediate(EmptyImmediate{}) {}
  template <typename T>
  Instr(Opcode opcode, T&& value)
      : opcode(opcode), immediate(std::move(value)) {}

  Opcode opcode;
  variant<EmptyImmediate,
          ValType,
          Index,
          CallIndirectImmediate,
          BrTableImmediate,
          u8,
          MemArg,
          s32,
          s64,
          f32,
          f64>
      immediate;
};

using Instrs = std::vector<Instr>;

struct Func {
  explicit Func(Index type_index) : type_index(type_index) {}

  Index type_index;
};

struct Table {
  explicit Table(TableType table_type) : table_type(table_type) {}

  TableType table_type;
};

struct Memory {
  explicit Memory(MemoryType memory_type) : memory_type(memory_type) {}

  MemoryType memory_type;
};

template <typename Traits = BorrowedTraits>
struct Global {
  Global(GlobalType global_type, Expr<> init_expr)
      : global_type(global_type), init_expr(init_expr) {}

  GlobalType global_type;
  Expr<Traits> init_expr;
};

template <typename Traits = BorrowedTraits>
struct Export {
  Export(ExternalKind kind, string_view name, Index index) :
    kind(kind), name(name), index(index) {}

  ExternalKind kind;
  typename Traits::Name name;
  Index index;
};

struct Start {
  explicit Start(Index func_index) : func_index(func_index) {}

  Index func_index;
};

template <typename Traits = BorrowedTraits>
struct ElementSegment {
  ElementSegment(Index table_index, Expr<> offset, std::vector<Index>&& init)
      : table_index(table_index), offset(offset), init(std::move(init)) {}

  Index table_index;
  Expr<Traits> offset;
  std::vector<Index> init;
};

template <typename Traits = BorrowedTraits>
struct Code {
  Code(std::vector<LocalDecl>&& local_decls, Expr<> body)
      : local_decls(std::move(local_decls)), body(body) {}

  std::vector<LocalDecl> local_decls;
  Expr<Traits> body;
};

template <typename Traits = BorrowedTraits>
struct DataSegment {
  DataSegment(Index memory_index, Expr<> offset, SpanU8 init)
      : memory_index(memory_index),
        offset(offset),
        init(Traits::ToBuffer(init)) {}

  Index memory_index;
  Expr<Traits> offset;
  typename Traits::Buffer init;
};

template <typename Traits = BorrowedTraits>
struct Module {
  std::vector<FuncType> types;
  std::vector<Import<Traits>> imports;
  std::vector<Func> funcs;
  std::vector<Table> tables;
  std::vector<Memory> memories;
  std::vector<Global<Traits>> globals;
  std::vector<Export<Traits>> exports;
  optional<Start> start;
  std::vector<ElementSegment<Traits>> element_segments;
  std::vector<Code<Traits>> codes;
  std::vector<DataSegment<Traits>> data_segments;
  std::vector<CustomSection<Traits>> custom_sections;
};

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_TYPES_H_
