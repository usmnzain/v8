// Copyright 2021 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_BASELINE_RISCV32_BASELINE_COMPILER_RISCV32_INL_H_
#define V8_BASELINE_RISCV32_BASELINE_COMPILER_RISCV32_INL_H_

#include "src/baseline/baseline-compiler.h"

namespace v8 {
namespace internal {
namespace baseline {

#define __ basm_.

void BaselineCompiler::Prologue() {
  ASM_CODE_COMMENT(&masm_);
  // Enter the frame here, since CallBuiltin will override lr.
  __ masm()->EnterFrame(StackFrame::BASELINE);
  DCHECK_EQ(kJSFunctionRegister, kJavaScriptCallTargetRegister);
  int max_frame_size =
      bytecode_->frame_size() + max_call_args_ * kSystemPointerSize;
  CallBuiltin<Builtin::kBaselineOutOfLinePrologue>(
      kContextRegister, kJSFunctionRegister, kJavaScriptCallArgCountRegister,
      max_frame_size, kJavaScriptCallNewTargetRegister, bytecode_);
  PrologueFillFrame();
}

void BaselineCompiler::PrologueFillFrame() {
  ASM_CODE_COMMENT(&masm_);
  // Inlined register frame fill
  interpreter::Register new_target_or_generator_register =
      bytecode_->incoming_new_target_or_generator_register();
  __ LoadRoot(kInterpreterAccumulatorRegister, RootIndex::kUndefinedValue);
  int register_count = bytecode_->register_count();
  // Magic value
  const int kLoopUnrollSize = 8;
  const int new_target_index = new_target_or_generator_register.index();
  const bool has_new_target = new_target_index != kMaxInt;
  if (has_new_target) {
    DCHECK_LE(new_target_index, register_count);
    __ masm()->Add(sp, sp, Operand(-(kPointerSize * new_target_index)));
    for (int i = 0; i < new_target_index; i++) {
      __ masm()->Sw(kInterpreterAccumulatorRegister,
                    MemOperand(sp, i * kSystemPointerSize));
    }
    // Push new_target_or_generator.
    __ Push(kJavaScriptCallNewTargetRegister);
    register_count -= new_target_index + 1;
  }
  if (register_count < 2 * kLoopUnrollSize) {
    // If the frame is small enough, just unroll the frame fill completely.
    __ masm()->Add(sp, sp, Operand(-(kPointerSize * register_count)));
    for (int i = 0; i < register_count; ++i) {
      __ masm()->Sw(kInterpreterAccumulatorRegister,
                    MemOperand(sp, i * kSystemPointerSize));
    }
  } else {
    __ masm()->Add(sp, sp, Operand(-(kPointerSize * register_count)));
    for (int i = 0; i < register_count; ++i) {
      __ masm()->Sw(kInterpreterAccumulatorRegister,
                    MemOperand(sp, i * kSystemPointerSize));
    }
  }
}

void BaselineCompiler::VerifyFrameSize() {
  ASM_CODE_COMMENT(&masm_);
  __ masm()->Add(kScratchReg, sp,
                 Operand(InterpreterFrameConstants::kFixedFrameSizeFromFp +
                         bytecode_->frame_size()));
  __ masm()->Assert(eq, AbortReason::kUnexpectedStackPointer, kScratchReg,
                    Operand(fp));
}

#undef __

}  // namespace baseline
}  // namespace internal
}  // namespace v8

#endif  // V8_BASELINE_RISCV32_BASELINE_COMPILER_RISCV32_INL_H_
