#ifndef YASM_X86GENERAL_H
#define YASM_X86GENERAL_H
//
// x86 general instruction header file
//
//  Copyright (C) 2001-2007  Peter Johnson
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
#include <memory>

#include "yasmx/Config/export.h"


namespace yasm
{

class BytecodeContainer;
class SourceLocation;
class Value;

namespace arch
{

class X86Common;
class X86EffAddr;
class X86Opcode;

// Postponed (from parsing to later binding) action options.
enum X86GeneralPostOp
{
    // None
    X86_POSTOP_NONE = 0,

    // Instructions that take a sign-extended imm8 as well as imm values
    // (eg, the arith instructions and a subset of the imul instructions)
    // should set this and put the imm8 form as the "normal" opcode (in
    // the first one or two bytes) and non-imm8 form in the second or
    // third byte of the opcode.
    X86_POSTOP_SIGNEXT_IMM8,

    // Could become a short opcode mov with bits=64 and a32 prefix.
    X86_POSTOP_SHORT_MOV,

    // Override any attempt at address-size override to 16 bits, and never
    // generate a prefix.  This is used for the ENTER opcode.
    X86_POSTOP_ADDRESS16,

    // Large imm64 that can become a sign-extended imm32.
    X86_POSTOP_SIMM32_AVAIL
};

YASM_STD_EXPORT
void AppendGeneral(BytecodeContainer& container,
                   const X86Common& common,
                   const X86Opcode& opcode,
                   std::auto_ptr<X86EffAddr> ea,
                   std::auto_ptr<Value> imm,
                   unsigned char special_prefix,
                   unsigned char rex,
                   X86GeneralPostOp postop,
                   bool default_rel,
                   SourceLocation source);

}} // namespace yasm::arch

#endif
