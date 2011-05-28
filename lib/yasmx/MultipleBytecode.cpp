///
/// Multiple bytecode wrapper and container
///
///  Copyright (C) 2001-2007  Peter Johnson
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions
/// are met:
/// 1. Redistributions of source code must retain the above copyright
///    notice, this list of conditions and the following disclaimer.
/// 2. Redistributions in binary form must reproduce the above copyright
///    notice, this list of conditions and the following disclaimer in the
///    documentation and/or other materials provided with the distribution.
///
/// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
/// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
/// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
/// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
/// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
/// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
/// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
/// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
/// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
/// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
/// POSSIBILITY OF SUCH DAMAGE.
///
#include "yasmx/BytecodeContainer.h"

#include "yasmx/Bytecode.h"
#include "yasmx/BytecodeOutput.h"
#include "yasmx/Expr.h"
#include "yasmx/IntNum.h"
#include "yasmx/Location_util.h"


using namespace yasm;

namespace {
class Multiple
{
public:
    Multiple(std::auto_ptr<Expr> e);
    ~Multiple();

    /// Finalizes after parsing.
    bool Finalize(SourceLocation source, Diagnostic& diags);

    /// Calculates the minimum size.
    bool CalcLen(Bytecode& bc,
                 const Bytecode::AddSpanFunc& add_span,
                 Diagnostic& diags);

    /// Calculate for output.
    bool CalcForOutput(SourceLocation source, Diagnostic& diags);

    /// Write an XML representation.  For debugging purposes.
    pugi::xml_node Write(pugi::xml_node out) const;

    void setInt(long val) { m_int = val; }
    long getInt() const { return m_int; }

private:
    /// Number of times contents is repeated.
    Expr m_expr;

    /// Number of times contents is repeated, integer version.
    long m_int;
};

class MultipleBytecode : public Bytecode::Contents
{
public:
    MultipleBytecode(std::auto_ptr<Expr> e);
    ~MultipleBytecode();

    /// Finalizes the bytecode after parsing.
    bool Finalize(Bytecode& bc, Diagnostic& diags);

    /// Calculates the minimum size of a bytecode.
    bool CalcLen(Bytecode& bc,
                 /*@out@*/ unsigned long* len,
                 const Bytecode::AddSpanFunc& add_span,
                 Diagnostic& diags);

    /// Recalculates the bytecode's length based on an expanded span
    /// length.
    bool Expand(Bytecode& bc,
                unsigned long* len,
                int span,
                long old_val,
                long new_val,
                bool* keep,
                /*@out@*/ long* neg_thres,
                /*@out@*/ long* pos_thres,
                Diagnostic& diags);

    /// Convert a bytecode into its byte representation.
    bool Output(Bytecode& bc, BytecodeOutput& bc_out);

    llvm::StringRef getType() const;

    MultipleBytecode* clone() const;

    /// Write an XML representation.  For debugging purposes.
    pugi::xml_node Write(pugi::xml_node out) const;

    BytecodeContainer& getContents() { return m_contents; }

private:
    /// Number of times contents is repeated.
    Multiple m_multiple;

    /// Contents to be repeated.
    BytecodeContainer m_contents;
};

class FillBytecode : public Bytecode::Contents
{
public:
    FillBytecode(std::auto_ptr<Expr> multiple, unsigned int size);
    FillBytecode(std::auto_ptr<Expr> multiple,
                 unsigned int size,
                 std::auto_ptr<Expr> value,
                 SourceLocation source);
    ~FillBytecode();

    /// Finalizes the bytecode after parsing.
    bool Finalize(Bytecode& bc, Diagnostic& diags);

    /// Calculates the minimum size of a bytecode.
    bool CalcLen(Bytecode& bc,
                 /*@out@*/ unsigned long* len,
                 const Bytecode::AddSpanFunc& add_span,
                 Diagnostic& diags);

    /// Recalculates the bytecode's length based on an expanded span
    /// length.
    bool Expand(Bytecode& bc,
                unsigned long* len,
                int span,
                long old_val,
                long new_val,
                bool* keep,
                /*@out@*/ long* neg_thres,
                /*@out@*/ long* pos_thres,
                Diagnostic& diags);

    /// Convert a bytecode into its byte representation.
    bool Output(Bytecode& bc, BytecodeOutput& bc_out);

    llvm::StringRef getType() const;

    FillBytecode* clone() const;

    /// Write an XML representation.  For debugging purposes.
    pugi::xml_node Write(pugi::xml_node out) const;

private:
    /// Number of times contents is repeated.
    Multiple m_multiple;

    /// Fill value.
    Value m_value;

    /// True if skip instead of value output.
    bool m_skip;
};
} // anonymous namespace

Multiple::Multiple(std::auto_ptr<Expr> e)
    : m_int(0)
{
    m_expr.swap(*e);
}

Multiple::~Multiple()
{
}

bool
Multiple::Finalize(SourceLocation source, Diagnostic& diags)
{
    Value val(0, std::auto_ptr<Expr>(m_expr.clone()));

    if (!val.Finalize(diags, diag::err_multiple_too_complex))
        return false;
    if (val.isRelative())
    {
        diags.Report(source, diag::err_multiple_not_absolute);
        return false;
    }
    // Finalize creates NULL output if value=0, but expr is NULL
    // if value=1 (this difference is to make the common case small).
    // However, this means we need to set expr explicitly to 0
    // here if val.abs is NULL.
    if (Expr* e = val.getAbs())
        m_expr.swap(*e);
    else
        m_expr = 0;
    return true;
}

bool
Multiple::CalcLen(Bytecode& bc,
                  const Bytecode::AddSpanFunc& add_span,
                  Diagnostic& diags)
{
    // Calculate multiple value as an integer
    m_int = 1;
    if (m_expr.isIntNum())
    {
        IntNum num = m_expr.getIntNum();
        if (num.getSign() < 0)
        {
            m_int = 0;
            diags.Report(bc.getSource(), diag::err_multiple_negative);
            return false;
        }
        else
            m_int = num.getInt();
    }
    else
    {
        if (m_expr.Contains(ExprTerm::FLOAT))
        {
            m_int = 0;
            diags.Report(bc.getSource(), diag::err_expr_contains_float);
            return false;
        }
        else
        {
            Value value(0, Expr::Ptr(m_expr.clone()));
            add_span(bc, 0, value, 0, 0);
            m_int = 0;      // assume 0 to start
        }
    }
    return true;
}

bool
Multiple::CalcForOutput(SourceLocation source, Diagnostic& diags)
{
    SimplifyCalcDist(m_expr, diags);
    if (!m_expr.isIntNum())
    {
        diags.Report(source, diag::err_multiple_unknown);
        return false;
    }
    IntNum num = m_expr.getIntNum();
    if (num.getSign() < 0)
    {
        diags.Report(source, diag::err_multiple_negative);
        return false;
    }
    assert(m_int == num.getInt() && "multiple changed after optimize");
    m_int = num.getInt();
    return true;
}

pugi::xml_node
Multiple::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("Multiple");
    append_data(root, m_expr);
    root.append_attribute("int") = m_int;
    return root;
}

MultipleBytecode::MultipleBytecode(std::auto_ptr<Expr> e)
    : m_multiple(e)
{
}

MultipleBytecode::~MultipleBytecode()
{
}

bool
MultipleBytecode::Finalize(Bytecode& bc, Diagnostic& diags)
{
    if (!m_multiple.Finalize(bc.getSource(), diags))
        return false;

    for (BytecodeContainer::bc_iterator i = m_contents.bytecodes_begin(),
         end = m_contents.bytecodes_end(); i != end; ++i)
    {
        if (i->getSpecial() == Bytecode::Contents::SPECIAL_OFFSET)
        {
            diags.Report(bc.getSource(), diag::err_multiple_setpos);
            return false;
        }
        if (!i->Finalize(diags))
            return false;
    }
    return true;
}

bool
MultipleBytecode::CalcLen(Bytecode& bc,
                          /*@out@*/ unsigned long* len,
                          const Bytecode::AddSpanFunc& add_span,
                          Diagnostic& diags)
{
    if (!m_multiple.CalcLen(bc, add_span, diags))
        return false;

    unsigned long ilen = 0;
    for (BytecodeContainer::bc_iterator i = m_contents.bytecodes_begin(),
         end = m_contents.bytecodes_end(); i != end; ++i)
    {
        if (!i->CalcLen(add_span, diags))
            return false;
        ilen += i->getTotalLen();
    }

    *len = ilen * m_multiple.getInt();
    return true;
}

bool
MultipleBytecode::Expand(Bytecode& bc,
                         unsigned long* len,
                         int span,
                         long old_val,
                         long new_val,
                         bool* keep,
                         /*@out@*/ long* neg_thres,
                         /*@out@*/ long* pos_thres,
                         Diagnostic& diags)
{
    // XXX: support more than one bytecode here
    if (span == 0)
    {
        m_multiple.setInt(new_val);
        *keep = true;
    }
    else
    {
        if (!m_contents.bytecodes_front().Expand(span, old_val, new_val, keep,
                                                 neg_thres, pos_thres, diags))
            return false;
    }
    *len = m_contents.bytecodes_front().getTotalLen() * m_multiple.getInt();
    return true;
}

bool
MultipleBytecode::Output(Bytecode& bc, BytecodeOutput& bc_out)
{
    if (!m_multiple.CalcForOutput(bc.getSource(), bc_out.getDiagnostics()))
        return false;

    unsigned long total_len = 0;
    unsigned long pos = 0;
    for (long mult=0, multend=m_multiple.getInt(); mult<multend;
         mult++, pos += total_len)
    {
        for (BytecodeContainer::bc_iterator i = m_contents.bytecodes_begin(),
             end = m_contents.bytecodes_end(); i != end; ++i)
        {
            if (!i->Output(bc_out))
                return false;
        }
    }
    return true;
}

llvm::StringRef
MultipleBytecode::getType() const
{
    return "yasm::MultipleBytecode";
}

MultipleBytecode*
MultipleBytecode::clone() const
{
    // TODO: cloning
    assert(false);
    return 0;
}

pugi::xml_node
MultipleBytecode::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("MultipleBytecode");
    append_child(root, "Multiple", m_multiple);
    append_child(root, "Contents", m_contents);
    return root;
}

FillBytecode::FillBytecode(std::auto_ptr<Expr> multiple, unsigned int size)
    : m_multiple(multiple)
    , m_value(size*8, SymbolRef(0))
    , m_skip(true)
{
}

FillBytecode::FillBytecode(std::auto_ptr<Expr> multiple,
                           unsigned int size,
                           std::auto_ptr<Expr> value,
                           SourceLocation source)
    : m_multiple(multiple)
    , m_value(size*8, value)
    , m_skip(false)
{
    m_value.setSource(source);
}

FillBytecode::~FillBytecode()
{
}

bool
FillBytecode::Finalize(Bytecode& bc, Diagnostic& diags)
{
    if (!m_multiple.Finalize(bc.getSource(), diags))
        return false;

    if (!m_skip && !m_value.Finalize(diags))
        return false;

    return true;
}

bool
FillBytecode::CalcLen(Bytecode& bc,
                      /*@out@*/ unsigned long* len,
                      const Bytecode::AddSpanFunc& add_span,
                      Diagnostic& diags)
{
    if (!m_multiple.CalcLen(bc, add_span, diags))
        return false;

    *len = m_value.getSize()/8 * m_multiple.getInt();
    return true;
}

bool
FillBytecode::Expand(Bytecode& bc,
                     unsigned long* len,
                     int span,
                     long old_val,
                     long new_val,
                     bool* keep,
                     /*@out@*/ long* neg_thres,
                     /*@out@*/ long* pos_thres,
                     Diagnostic& diags)
{
    if (span == 0)
    {
        m_multiple.setInt(new_val);
        *keep = true;
    }
    *len = m_value.getSize()/8 * m_multiple.getInt();
    return true;
}

bool
FillBytecode::Output(Bytecode& bc, BytecodeOutput& bc_out)
{
    SourceLocation source = bc.getSource();

    if (!m_multiple.CalcForOutput(source, bc_out.getDiagnostics()))
        return false;

    if (m_skip)
    {
        bc_out.OutputGap(m_value.getSize()/8 * m_multiple.getInt(), source);
        return true;
    }

    Bytes& bytes = bc_out.getScratch();
    bytes.resize(m_value.getSize()/8);

    NumericOutput num_out(bytes);
    m_value.ConfigureOutput(&num_out);

    Location loc = {&bc, 0};
    if (!bc_out.ConvertValueToBytes(m_value, loc, num_out))
        return false;
    num_out.EmitWarnings(bc_out.getDiagnostics());
    num_out.ClearWarnings();

    for (long mult=0, multend=m_multiple.getInt(); mult<multend; mult++)
        bc_out.OutputBytes(bytes, source);

    return true;
}

llvm::StringRef
FillBytecode::getType() const
{
    return "yasm::FillBytecode";
}

FillBytecode*
FillBytecode::clone() const
{
    return new FillBytecode(*this);
}

pugi::xml_node
FillBytecode::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("Fill");
    append_data(root, m_multiple);
    append_data(root, m_value);
    if (m_skip)
        root.append_attribute("skip") = true;
    return root;
}

BytecodeContainer&
yasm::AppendMultiple(BytecodeContainer& container,
                     std::auto_ptr<Expr> multiple,
                     SourceLocation source)
{
    Bytecode& bc = container.FreshBytecode();
    MultipleBytecode* multbc(new MultipleBytecode(multiple));
    BytecodeContainer& retval = multbc->getContents();
    bc.Transform(Bytecode::Contents::Ptr(multbc));
    bc.setSource(source);
    return retval;
}

void
yasm::AppendSkip(BytecodeContainer& container,
                 std::auto_ptr<Expr> multiple,
                 unsigned int size,
                 SourceLocation source)
{
    Bytecode& bc = container.FreshBytecode();
    FillBytecode* fillbc(new FillBytecode(multiple, size));
    bc.Transform(Bytecode::Contents::Ptr(fillbc));
    bc.setSource(source);
}

void
yasm::AppendFill(BytecodeContainer& container,
                 std::auto_ptr<Expr> multiple,
                 unsigned int size,
                 std::auto_ptr<Expr> value,
                 SourceLocation source)
{
    Bytecode& bc = container.FreshBytecode();
    FillBytecode* fillbc(new FillBytecode(multiple, size, value, source));
    bc.Transform(Bytecode::Contents::Ptr(fillbc));
    bc.setSource(source);
}
