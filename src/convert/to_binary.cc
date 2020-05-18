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

#include "wasp/convert/to_binary.h"

#include <cassert>

#include "wasp/binary/encoding.h"
#include "wasp/binary/write.h"

namespace wasp {
namespace convert {

string_view Context::Add(std::string str) {
  strings.push_back(std::make_unique<std::string>(str));
  return string_view{*strings.back()};
}

SpanU8 Context::Add(Buffer buffer) {
  buffers.push_back(std::make_unique<Buffer>(std::move(buffer)));
  return SpanU8{*buffers.back()};
}

auto ToBinary(Context& context, const At<text::Text>& value)
    -> At<string_view> {
  return MakeAt(value.loc(), context.Add(value->ToString()));
}

auto ToBinary(Context& context, const At<text::Var>& value) -> At<Index> {
  assert(holds_alternative<Index>(*value));
  return MakeAt(value.loc(), std::get<Index>(*value));
}

auto ToBinary(Context& context, const OptAt<text::Var>& var) -> At<Index> {
  assert(var.has_value());
  return ToBinary(context, var.value());
}

auto ToBinary(Context& context,
              const OptAt<text::Var>& var,
              Index default_index) -> At<Index> {
  if (var.has_value()) {
    return ToBinary(context, var.value());
  } else {
    return MakeAt(default_index);
  }
}

auto ToBinary(Context& context, const text::VarList& vars)
    -> binary::IndexList {
  binary::IndexList result;
  for (auto var : vars) {
    result.push_back(ToBinary(context, var));
  }
  return result;
}

// Section 1: Type
auto ToBinary(Context& context, const At<text::FunctionType>& value)
    -> At<binary::FunctionType> {
  return MakeAt(value.loc(),
                binary::FunctionType{value->params, value->results});
}

auto ToBinary(Context& context, const text::BoundValueTypeList& values)
    -> ValueTypeList {
  ValueTypeList result;
  for (auto& value : values) {
    result.push_back(value->type);
  }
  return result;
}

auto ToBinary(Context& context, const At<text::TypeEntry>& value)
    -> At<binary::TypeEntry> {
  return MakeAt(value.loc(),
                binary::TypeEntry{MakeAt(
                    value->type.loc(),
                    binary::FunctionType{ToBinary(context, value->type->params),
                                         value->type->results})});
}

// Section 2: Import
auto ToBinary(Context& context, const At<text::Import>& value)
    -> At<binary::Import> {
  auto module = MakeAt(value->module.loc(), ToBinary(context, value->module));
  auto name = MakeAt(value->name.loc(), ToBinary(context, value->name));

  switch (value->desc.index()) {
    case 0: { // Function
      auto& desc = std::get<text::FunctionDesc>(value->desc);
      return MakeAt(value.loc(),
                    binary::Import{module, name,
                                   MakeAt(desc.type_use->loc(),
                                          ToBinary(context, desc.type_use))});
    }

    case 1: { // Table
      auto& desc = std::get<text::TableDesc>(value->desc);
      return MakeAt(
          value.loc(),
          binary::Import{module, name, MakeAt(desc.type.loc(), desc.type)});
    }

    case 2: { // Memory
      auto& desc = std::get<text::MemoryDesc>(value->desc);
      return MakeAt(
          value.loc(),
          binary::Import{module, name, MakeAt(desc.type.loc(), desc.type)});
    }

    case 3: { // Global
      auto& desc = std::get<text::GlobalDesc>(value->desc);
      return MakeAt(
          value.loc(),
          binary::Import{module, name, MakeAt(desc.type.loc(), desc.type)});
    }

    case 4: { // Event
      auto& desc = std::get<text::EventDesc>(value->desc);
      return MakeAt(value.loc(),
                    binary::Import{
                        module, name,
                        MakeAt(desc.type.loc(), ToBinary(context, desc.type))});
    }

    default:
      WASP_UNREACHABLE();
  }
}

// Section 2: Function
auto ToBinary(Context& context, const At<text::Function>& value)
    -> OptAt<binary::Function> {
  if (value->import) {
    return nullopt;
  }
  return MakeAt(value.loc(),
                binary::Function{ToBinary(context, value->desc.type_use)});
}

// Section 4: Table
auto ToBinary(Context& context, const At<text::Table>& value)
    -> OptAt<binary::Table> {
  if (value->import) {
    return nullopt;
  }
  return MakeAt(value.loc(), binary::Table{value->desc.type});
}

// Section 5: Memory
auto ToBinary(Context& context, const At<text::Memory>& value)
    -> OptAt<binary::Memory> {
  if (value->import) {
    return nullopt;
  }
  return MakeAt(value.loc(), binary::Memory{value->desc.type});
}

// Section 6: Global
auto ToBinary(Context& context, const At<text::ConstantExpression>& value)
    -> At<binary::ConstantExpression> {
  return MakeAt(value.loc(), binary::ConstantExpression{
                                 ToBinary(context, value->instructions)});
}

auto ToBinary(Context& context, const At<text::Global>& value)
    -> OptAt<binary::Global> {
  if (value->import) {
    return nullopt;
  }
  assert(value->init.has_value());
  return MakeAt(value.loc(), binary::Global{value->desc.type,
                                            ToBinary(context, *value->init)});
}

// Section 7: Export
auto ToBinary(Context& context, const At<text::Export>& value)
    -> At<binary::Export> {
  return MakeAt(value.loc(),
                binary::Export{value->kind, ToBinary(context, value->name),
                               ToBinary(context, value->var)});
}

// Section 8: Start
auto ToBinary(Context& context, const At<text::Start>& value)
    -> At<binary::Start> {
  return MakeAt(value.loc(), binary::Start{ToBinary(context, value->var)});
}

auto ToBinary(Context& context, const At<text::ElementExpression>& value)
    -> At<binary::ElementExpression> {
  return MakeAt(value.loc(), binary::ElementExpression{
                                 ToBinary(context, value->instructions)});
}

auto ToBinary(Context& context, const text::ElementExpressionList& value)
    -> binary::ElementExpressionList {
  binary::ElementExpressionList result;
  for (auto&& elemexpr : value) {
    result.push_back(ToBinary(context, elemexpr));
  }
  return result;
}

auto ToBinary(Context& context, const text::ElementList& value)
    -> binary::ElementList {
  if (holds_alternative<text::ElementListWithExpressions>(value)) {
    auto&& elements = get<text::ElementListWithExpressions>(value);
    return binary::ElementListWithExpressions{elements.elemtype,
                                              ToBinary(context, elements.list)};
  } else {
    auto&& elements = get<text::ElementListWithVars>(value);
    return binary::ElementListWithIndexes{elements.kind,
                                          ToBinary(context, elements.list)};
  }
}

auto ToBinary(Context& context, const At<text::ElementSegment>& value)
    -> At<binary::ElementSegment> {
  if (value->type == SegmentType::Active) {
    return MakeAt(value.loc(),
                  binary::ElementSegment{ToBinary(context, value->table, 0),
                                         ToBinary(context, *value->offset),
                                         ToBinary(context, value->elements)});
  } else {
    return MakeAt(value.loc(),
                  binary::ElementSegment{value->type,
                                         ToBinary(context, value->elements)});
  }
}

// Section 10: Code
auto ToBinary(Context& context, const At<text::BlockImmediate>& value)
    -> At<binary::BlockType> {
  if (value->type.IsInlineType()) {
    // TODO: This is really nasty; find a nicer way.
    auto inline_type = value->type.GetInlineType();
    if (!inline_type) {
      return MakeAt(value.loc(), binary::BlockType::Void);
    }
    return MakeAt(value.loc(),
                  binary::BlockType(binary::EncodeValueTypeAsBlockType(
                      binary::encoding::ValueType::Encode(*inline_type))));
  } else {
    Index block_type_index = ToBinary(context, value->type.type_use);
    assert(block_type_index < 0x8000'0000);
    return MakeAt(value.loc(), binary::BlockType(block_type_index));
  }
}

auto ToBinary(Context& context, const At<text::BrOnExnImmediate>& value)
    -> At<binary::BrOnExnImmediate> {
  return MakeAt(value.loc(),
                binary::BrOnExnImmediate{ToBinary(context, value->target),
                                         ToBinary(context, value->event)});
}

auto ToBinary(Context& context, const At<text::BrTableImmediate>& value)
    -> At<binary::BrTableImmediate> {
  return MakeAt(value.loc(), binary::BrTableImmediate{
                                 ToBinary(context, value->targets),
                                 ToBinary(context, value->default_target)});
}

auto ToBinary(Context& context, const At<text::CallIndirectImmediate>& value)
    -> At<binary::CallIndirectImmediate> {
  return MakeAt(value.loc(), binary::CallIndirectImmediate{
                                 ToBinary(context, value->type.type_use),
                                 ToBinary(context, value->table, 0)});
}

auto ToBinary(Context& context, const At<text::CopyImmediate>& value)
    -> At<binary::CopyImmediate> {
  return MakeAt(value.loc(),
                binary::CopyImmediate{ToBinary(context, value->dst, 0),
                                      ToBinary(context, value->src, 0)});
}

auto ToBinary(Context& context, const At<text::InitImmediate>& value)
    -> At<binary::InitImmediate> {
  return MakeAt(value.loc(),
                binary::InitImmediate{ToBinary(context, value->segment),
                                      ToBinary(context, value->dst, 0)});
}

u32 GetNaturalAlignment(Opcode opcode) {
  switch (opcode) {
    case Opcode::I32AtomicLoad8U:
    case Opcode::I32AtomicRmw8AddU:
    case Opcode::I32AtomicRmw8AndU:
    case Opcode::I32AtomicRmw8CmpxchgU:
    case Opcode::I32AtomicRmw8OrU:
    case Opcode::I32AtomicRmw8SubU:
    case Opcode::I32AtomicRmw8XchgU:
    case Opcode::I32AtomicRmw8XorU:
    case Opcode::I32AtomicStore8:
    case Opcode::I32Load8S:
    case Opcode::I32Load8U:
    case Opcode::I32Store8:
    case Opcode::I64AtomicLoad8U:
    case Opcode::I64AtomicRmw8AddU:
    case Opcode::I64AtomicRmw8AndU:
    case Opcode::I64AtomicRmw8CmpxchgU:
    case Opcode::I64AtomicRmw8OrU:
    case Opcode::I64AtomicRmw8SubU:
    case Opcode::I64AtomicRmw8XchgU:
    case Opcode::I64AtomicRmw8XorU:
    case Opcode::I64AtomicStore8:
    case Opcode::I64Load8S:
    case Opcode::I64Load8U:
    case Opcode::I64Store8:
    case Opcode::V8X16LoadSplat:
      return 1;

    case Opcode::I32AtomicLoad16U:
    case Opcode::I32AtomicRmw16AddU:
    case Opcode::I32AtomicRmw16AndU:
    case Opcode::I32AtomicRmw16CmpxchgU:
    case Opcode::I32AtomicRmw16OrU:
    case Opcode::I32AtomicRmw16SubU:
    case Opcode::I32AtomicRmw16XchgU:
    case Opcode::I32AtomicRmw16XorU:
    case Opcode::I32AtomicStore16:
    case Opcode::I32Load16S:
    case Opcode::I32Load16U:
    case Opcode::I32Store16:
    case Opcode::I64AtomicLoad16U:
    case Opcode::I64AtomicRmw16AddU:
    case Opcode::I64AtomicRmw16AndU:
    case Opcode::I64AtomicRmw16CmpxchgU:
    case Opcode::I64AtomicRmw16OrU:
    case Opcode::I64AtomicRmw16SubU:
    case Opcode::I64AtomicRmw16XchgU:
    case Opcode::I64AtomicRmw16XorU:
    case Opcode::I64AtomicStore16:
    case Opcode::I64Load16S:
    case Opcode::I64Load16U:
    case Opcode::I64Store16:
    case Opcode::V16X8LoadSplat:
      return 2;

    case Opcode::F32Load:
    case Opcode::F32Store:
    case Opcode::I32AtomicLoad:
    case Opcode::I32AtomicRmwAdd:
    case Opcode::I32AtomicRmwAnd:
    case Opcode::I32AtomicRmwCmpxchg:
    case Opcode::I32AtomicRmwOr:
    case Opcode::I32AtomicRmwSub:
    case Opcode::I32AtomicRmwXchg:
    case Opcode::I32AtomicRmwXor:
    case Opcode::I32AtomicStore:
    case Opcode::I32Load:
    case Opcode::I32Store:
    case Opcode::I64AtomicLoad32U:
    case Opcode::I64AtomicRmw32AddU:
    case Opcode::I64AtomicRmw32AndU:
    case Opcode::I64AtomicRmw32CmpxchgU:
    case Opcode::I64AtomicRmw32OrU:
    case Opcode::I64AtomicRmw32SubU:
    case Opcode::I64AtomicRmw32XchgU:
    case Opcode::I64AtomicRmw32XorU:
    case Opcode::I64AtomicStore32:
    case Opcode::I64Load32S:
    case Opcode::I64Load32U:
    case Opcode::I64Store32:
    case Opcode::MemoryAtomicNotify:
    case Opcode::MemoryAtomicWait32:
    case Opcode::V32X4LoadSplat:
      return 4;

    case Opcode::F64Load:
    case Opcode::F64Store:
    case Opcode::I16X8Load8X8S:
    case Opcode::I16X8Load8X8U:
    case Opcode::I32X4Load16X4S:
    case Opcode::I32X4Load16X4U:
    case Opcode::I64AtomicLoad:
    case Opcode::I64AtomicRmwAdd:
    case Opcode::I64AtomicRmwAnd:
    case Opcode::I64AtomicRmwCmpxchg:
    case Opcode::I64AtomicRmwOr:
    case Opcode::I64AtomicRmwSub:
    case Opcode::I64AtomicRmwXchg:
    case Opcode::I64AtomicRmwXor:
    case Opcode::I64AtomicStore:
    case Opcode::I64Load:
    case Opcode::I64Store:
    case Opcode::I64X2Load32X2S:
    case Opcode::I64X2Load32X2U:
    case Opcode::MemoryAtomicWait64:
    case Opcode::V64X2LoadSplat:
      return 8;

    case Opcode::V128Load:
    case Opcode::V128Store:
      return 16;

    default:
      return 0;
  }
}

u32 GetAlignLog2(u32 align) {
  // Must be power-of-two.
  assert(align != 0 && (align & (align - 1)) == 0);
  u32 align_log2 = 0;
  while (align != 0) {
    align >>= 1;
    align_log2++;
  }
  return align_log2 - 1;
}

auto ToBinary(Context& context,
              const At<text::MemArgImmediate>& value,
              u32 natural_align) -> At<binary::MemArgImmediate> {
  auto align = value->align
                   ? MakeAt(value->align->loc(), GetAlignLog2(*value->align))
                   : MakeAt(GetAlignLog2(natural_align));
  auto offset = value->offset ? *value->offset : MakeAt(u32{0});
  return MakeAt(value.loc(), binary::MemArgImmediate{align, offset});
}

auto ToBinary(Context& context, const At<text::Instruction>& value)
    -> At<binary::Instruction> {
  switch (value->immediate.index()) {
    case 0: // monostate
      return MakeAt(value.loc(), binary::Instruction{value->opcode});

    case 1: // s32
      return MakeAt(
          value.loc(),
          binary::Instruction{value->opcode, get<At<s32>>(value->immediate)});

    case 2: // s64
      return MakeAt(
          value.loc(),
          binary::Instruction{value->opcode, get<At<s64>>(value->immediate)});

    case 3: // f32
      return MakeAt(
          value.loc(),
          binary::Instruction{value->opcode, get<At<f32>>(value->immediate)});

    case 4: // f64
      return MakeAt(
          value.loc(),
          binary::Instruction{value->opcode, get<At<f64>>(value->immediate)});

    case 5: // v128
      return MakeAt(
          value.loc(),
          binary::Instruction{value->opcode, get<At<v128>>(value->immediate)});

    case 6: // Var
      return MakeAt(
          value.loc(),
          binary::Instruction{
              value->opcode,
              ToBinary(context, get<At<text::Var>>(value->immediate))});

    case 7: // BlockImmediate
      return MakeAt(
          value.loc(),
          binary::Instruction{value->opcode,
                              ToBinary(context, get<At<text::BlockImmediate>>(
                                                    value->immediate))});

    case 8: // BrOnExnImmediate
      return MakeAt(
          value.loc(),
          binary::Instruction{value->opcode,
                              ToBinary(context, get<At<text::BrOnExnImmediate>>(
                                                    value->immediate))});

    case 9: // BrTableImmediate
      return MakeAt(
          value.loc(),
          binary::Instruction{value->opcode,
                              ToBinary(context, get<At<text::BrTableImmediate>>(
                                                    value->immediate))});

    case 10: // CallIndirectImmediate
      return MakeAt(value.loc(),
                    binary::Instruction{
                        value->opcode,
                        ToBinary(context, get<At<text::CallIndirectImmediate>>(
                                              value->immediate))});

    case 11: // CopyImmediate
      return MakeAt(
          value.loc(),
          binary::Instruction{value->opcode,
                              ToBinary(context, get<At<text::CopyImmediate>>(
                                                    value->immediate))});

    case 12: // InitImmediate
      return MakeAt(
          value.loc(),
          binary::Instruction{value->opcode,
                              ToBinary(context, get<At<text::InitImmediate>>(
                                                    value->immediate))});

    case 13: // MemArgImmediate
      return MakeAt(
          value.loc(),
          binary::Instruction{
              value->opcode,
              ToBinary(context,
                       get<At<text::MemArgImmediate>>(value->immediate),
                       GetNaturalAlignment(*value->opcode))});

    case 14: // ReferenceType
      return MakeAt(value.loc(), binary::Instruction{
                                     value->opcode,
                                     get<At<ReferenceType>>(value->immediate)});

    case 15: // SelectImmediate
      return MakeAt(
          value.loc(),
          binary::Instruction{
              value->opcode, get<At<text::SelectImmediate>>(value->immediate)});

    case 16: // ShuffleImmediate
      return MakeAt(
          value.loc(),
          binary::Instruction{value->opcode,
                              get<At<ShuffleImmediate>>(value->immediate)});

    case 17: // SimdLaneImmediate
      return MakeAt(value.loc(),
                    binary::Instruction{
                        value->opcode,
                        get<At<text::SimdLaneImmediate>>(value->immediate)});

    default:
      WASP_UNREACHABLE();
  }
}

auto ToBinary(Context& context, const text::InstructionList& value)
    -> binary::InstructionList {
  binary::InstructionList result;
  for (auto&& instr : value) {
    result.push_back(ToBinary(context, instr));
  }
  return result;
}

auto ToBinaryExpression(Context& context,
                        const At<text::InstructionList>& value)
    -> At<binary::Expression> {
  Buffer buffer;
  auto out = std::back_inserter(buffer);
  for (auto&& instr : *value) {
    out = binary::Write(ToBinary(context, instr), out);
  }
  // Write implicit 'end' instruction.
  out = binary::Write(binary::Instruction{Opcode::End}, out);
  return MakeAt(value.loc(),
                binary::Expression{context.Add(std::move(buffer))});
}

auto ToBinaryLocalsList(Context& context,
                        const At<text::BoundValueTypeList>& value)
    -> At<binary::LocalsList> {
  binary::LocalsList result;
  optional<ValueType> local_type;
  for (auto&& bound_local : *value) {
    if (local_type && bound_local->type == *local_type) {
      assert(!result.empty());
      // TODO: Combine locations?
      (*result.back()->count)++;
    } else {
      result.push_back(binary::Locals{1, bound_local->type});
      local_type = bound_local->type;
    }
  }
  return MakeAt(value.loc(), result);
}

auto ToBinaryCode(Context& context, const At<text::Function>& value)
    -> OptAt<binary::Code> {
  if (value->import) {
    return nullopt;
  }

  return MakeAt(value.loc(),
                binary::Code{ToBinaryLocalsList(context, value->locals),
                             ToBinaryExpression(context, value->instructions)});
}

// Section 11: Data
auto ToBinary(Context& context, const At<text::TextList>& value) -> SpanU8 {
  Buffer buffer;
  for (auto&& text : *value) {
    text->ToBuffer(buffer);
  }
  return context.Add(buffer);
}

auto ToBinary(Context& context, const At<text::DataSegment>& value)
    -> At<binary::DataSegment> {
  if (value->type == SegmentType::Active) {
    return MakeAt(value.loc(),
                  binary::DataSegment{ToBinary(context, value->memory, 0),
                                      ToBinary(context, *value->offset),
                                      ToBinary(context, value->data)});
  } else {
    return MakeAt(value.loc(),
                  binary::DataSegment{ToBinary(context, value->data)});
  }
}

// Section 12: DataCount

// Section 13: Event
auto ToBinary(Context& context, const At<text::EventType>& value)
    -> At<binary::EventType> {
  return MakeAt(value.loc(),
                binary::EventType{value->attribute,
                                  ToBinary(context, value->type.type_use)});
}

auto ToBinary(Context& context, const At<text::Event>& value)
    -> OptAt<binary::Event> {
  if (value->import) {
    return nullopt;
  }
  return MakeAt(value.loc(),
                binary::Event{ToBinary(context, value->desc.type)});
}

// Module
auto ToBinary(Context& context, const At<text::Module>& value)
    -> At<binary::Module> {
  binary::Module result;

  auto push_back_opt = [](auto& vec, auto&& item) {
    if (item) {
      vec.push_back(*item);
    }
  };

  for (auto&& item : *value) {
    switch (item.index()) {
      case 0:  // TypeEntry.
        result.types.push_back(
            ToBinary(context, get<At<text::TypeEntry>>(item)));
        break;

      case 1:  // Import.
        result.imports.push_back(
            ToBinary(context, get<At<text::Import>>(item)));
        break;

      case 2: { // Function
        auto&& function = get<At<text::Function>>(item);
        push_back_opt(result.functions, ToBinary(context, function));
        push_back_opt(result.codes, ToBinaryCode(context, function));
        break;
      }

      case 3:  // Table
        push_back_opt(result.tables,
                      ToBinary(context, get<At<text::Table>>(item)));
        break;

      case 4:  // Memory
        push_back_opt(result.memories,
                      ToBinary(context, get<At<text::Memory>>(item)));
        break;

      case 5:  // Global
        push_back_opt(result.globals,
                      ToBinary(context, get<At<text::Global>>(item)));
        break;

      case 6:  // Export
        result.exports.push_back(
            ToBinary(context, get<At<text::Export>>(item)));
        break;

      case 7:  // Start
        // This will overwrite an existing Start section, if any. That
        // shouldn't happen, since reading multiple start sections means the
        // text is malformed.
        result.start = ToBinary(context, get<At<text::Start>>(item));
        break;

      case 8:  // ElementSegment
        result.element_segments.push_back(
            ToBinary(context, get<At<text::ElementSegment>>(item)));
        break;

      case 9:  // DataSegment
        result.data_segments.push_back(
            ToBinary(context, get<At<text::DataSegment>>(item)));
        break;

      case 10:   // Event
        push_back_opt(result.events,
                      ToBinary(context, get<At<text::Event>>(item)));
        break;
    }
  }
  return MakeAt(value.loc(), result);
}

}  // namespace convert
}  // namespace wasp