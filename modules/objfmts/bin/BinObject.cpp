//
// Flat-format binary object format
//
//  Copyright (C) 2002-2007  Peter Johnson
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
#include "BinObject.h"

#include "util.h"

#include "clang/Basic/SourceLocation.h"
#include "llvm/ADT/Twine.h"
#include "yasmx/Support/bitcount.h"
#include "yasmx/Support/Compose.h"
#include "yasmx/Support/errwarn.h"
#include "yasmx/Support/registry.h"
#include "yasmx/BytecodeOutput.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Bytes.h"
#include "yasmx/Diagnostic.h"
#include "yasmx/Directive.h"
#include "yasmx/DirHelpers.h"
#include "yasmx/Errwarns.h"
#include "yasmx/Expr.h"
#include "yasmx/IntNum.h"
#include "yasmx/Object.h"
#include "yasmx/NameValue.h"
#include "yasmx/Section.h"
#include "yasmx/Symbol.h"
#include "yasmx/Value.h"

#include "BinLink.h"
#include "BinMapOutput.h"
#include "BinSection.h"
#include "BinSymbol.h"


using namespace yasm;
using namespace yasm::objfmt;

BinObject::BinObject(const ObjectFormatModule& module, Object& object)
    : ObjectFormat(module, object), m_map_flags(NO_MAP), m_org(0)
{
}

BinObject::~BinObject()
{
}

void
BinObject::OutputMap(const IntNum& origin,
                     const BinGroups& groups,
                     Diagnostic& diags) const
{
    int map_flags = m_map_flags;

    if (map_flags == NO_MAP)
        return;

    if (map_flags == MAP_NONE)
        map_flags = MAP_BRIEF;          // default to brief

    std::string err;
    llvm::raw_fd_ostream os
        (m_map_filename.empty() ? "-" : m_map_filename.c_str(), err);
    if (!err.empty())
    {
        diags.Report(clang::SourceLocation(),
                     diags.getCustomDiagID(Diagnostic::Warning,
                         N_("unable to open map file '%0': %1")))
            << m_map_filename << err;
        return;
    }

    BinMapOutput out(os, m_object, origin, groups);
    out.OutputHeader();
    out.OutputOrigin();

    if (map_flags & MAP_BRIEF)
        out.OutputSectionsSummary();

    if (map_flags & MAP_SECTIONS)
        out.OutputSectionsDetail();

    if (map_flags & MAP_SYMBOLS)
        out.OutputSectionsSymbols();
}

namespace {
class BinOutput : public BytecodeStreamOutput
{
public:
    BinOutput(llvm::raw_fd_ostream& os, Object& object, Diagnostic& diags);
    ~BinOutput();

    void OutputSection(Section& sect,
                       const IntNum& origin,
                       Diagnostic& diags);

    // OutputBytecode overrides
    bool ConvertValueToBytes(Value& value,
                             Bytes& bytes,
                             Location loc,
                             int warn);

private:
    Object& m_object;
    llvm::raw_fd_ostream& m_fd_os;
    BytecodeNoOutput m_no_output;
};
} // anonymous namespace

BinOutput::BinOutput(llvm::raw_fd_ostream& os,
                     Object& object,
                     Diagnostic& diags)
    : BytecodeStreamOutput(os, diags),
      m_object(object),
      m_fd_os(os),
      m_no_output(diags)
{
}

BinOutput::~BinOutput()
{
}

void
BinOutput::OutputSection(Section& sect,
                         const IntNum& origin,
                         Diagnostic& diags)
{
    BytecodeOutput* outputter;

    if (sect.isBSS())
    {
        outputter = &m_no_output;
    }
    else
    {
        IntNum file_start = sect.getLMA();
        file_start -= origin;
        if (file_start.getSign() < 0)
        {
            diags.Report(clang::SourceLocation(),
                         diags.getCustomDiagID(Diagnostic::Error,
                             N_("section '%0' starts before origin (ORG)")))
                << sect.getName();
            return;
        }
        if (!file_start.isOkSize(sizeof(unsigned long)*8, 0, 0))
        {
            diags.Report(clang::SourceLocation(),
                         diags.getCustomDiagID(Diagnostic::Error,
                             N_("section '%0' start value too large")))
                << sect.getName();
            return;
        }
        m_fd_os.seek(file_start.getUInt());
        if (m_os.has_error())
        {
            diags.Report(clang::SourceLocation(), diag::err_file_output_seek);
            return;
        }

        outputter = this;
    }

    for (Section::bc_iterator i=sect.bytecodes_begin(),
         end=sect.bytecodes_end(); i != end; ++i)
    {
        i->Output(*outputter, diags);
    }
}

bool
BinOutput::ConvertValueToBytes(Value& value,
                               Bytes& bytes,
                               Location loc,
                               int warn)
{
    // Binary objects we need to resolve against object, not against section.
    if (value.isRelative())
    {
        Location label_loc;
        IntNum ssymval;
        Expr syme;
        SymbolRef rel = value.getRelative();

        if (rel->isAbsoluteSymbol())
            syme = Expr(0);
        else if (rel->getLabel(&label_loc) && label_loc.bc->getContainer())
            syme = Expr(rel);
        else if (getBinSSymValue(*rel, &ssymval))
            syme = Expr(ssymval);
        else
            goto done;

        // Handle PC-relative
        if (value.getSubLocation(&label_loc) && label_loc.bc->getContainer())
            syme -= label_loc;

        if (value.getRShift() > 0)
            syme >>= IntNum(value.getRShift());

        // Add into absolute portion
        value.AddAbs(syme);
        value.ClearRelative();
    }
done:
    // Simplify absolute portion of value, transforming symrecs
    if (Expr* abs = value.getAbs())
    {
        BinSimplify(*abs);
        abs->Simplify();
    }

    // Output
    IntNum intn;
    if (value.OutputBasic(bytes, &intn, warn, *m_object.getArch()))
        return true;

    // Couldn't output, assume it contains an external reference.
    Diag(value.getSource().getBegin(),
         getDiagnostics().getCustomDiagID(Diagnostic::Error,
             N_("binary object format does not support external references")));
    return false;
}

static void
CheckSymbol(const Symbol& sym, Diagnostic& diags)
{
    int vis = sym.getVisibility();

    // Don't check internally-generated symbols.  Only internally generated
    // symbols have symrec data, so simply check for its presence.
    if (sym.getAssocData<BinSymbol>())
        return;

    if (vis & Symbol::EXTERN)
    {
        diags.Report(sym.getDeclSource(),
                     diags.getCustomDiagID(Diagnostic::Warning,
            N_("binary object format does not support extern variables")));
    }
    else if (vis & Symbol::GLOBAL)
    {
        diags.Report(sym.getDeclSource(),
                     diags.getCustomDiagID(Diagnostic::Warning,
            N_("binary object format does not support global variables")));
    }
    else if (vis & Symbol::COMMON)
    {
        diags.Report(sym.getDeclSource(),
                     diags.getCustomDiagID(Diagnostic::Error,
            N_("binary object format does not support common variables")));
    }
}

void
BinObject::Output(llvm::raw_fd_ostream& os, bool all_syms, Errwarns& errwarns,
                  Diagnostic& diags)
{
    // Set ORG to 0 unless otherwise specified
    IntNum origin(0);
    if (m_org.get() != 0)
    {
        m_org->Simplify();
        if (!m_org->isIntNum())
        {
            diags.Report(m_org_source,
                         diags.getCustomDiagID(Diagnostic::Error,
                             N_("ORG expression is too complex")));
            return;
        }
        IntNum orgi = m_org->getIntNum();
        if (orgi.getSign() < 0)
        {
            diags.Report(m_org_source,
                         diags.getCustomDiagID(Diagnostic::Error,
                             N_("ORG expression is negative")));
            return;
        }
        origin = orgi;
    }

    // Check symbol table
    for (Object::const_symbol_iterator i=m_object.symbols_begin(),
         end=m_object.symbols_end(); i != end; ++i)
        CheckSymbol(*i, diags);

    BinLink link(m_object, diags);

    if (!link.DoLink(origin))
        return;

    // Output map file
    OutputMap(origin, link.getLMAGroups(), diags);

    // Ensure we don't have overlapping progbits LMAs.
    if (!link.CheckLMAOverlap())
        return;

    // Output sections
    BinOutput out(os, m_object, diags);
    for (Object::section_iterator i=m_object.sections_begin(),
         end=m_object.sections_end(); i != end; ++i)
    {
        out.OutputSection(*i, origin, diags);
    }
}

Section*
BinObject::AddDefaultSection()
{
    Section* section = AppendSection(".text", clang::SourceLocation());
    section->setDefault(true);
    return section;
}

Section*
BinObject::AppendSection(llvm::StringRef name, clang::SourceLocation source)
{
    bool bss = (name == ".bss");
    bool code = (name == ".text");
    Section* section = new Section(name, code, bss, source);
    m_object.AppendSection(std::auto_ptr<Section>(section));

    // Initialize section data and symbols.
    std::auto_ptr<BinSection> bsd(new BinSection());

    SymbolRef start = m_object.getSymbol(("section."+name+".start").str());
    if (start->okToDeclare(Symbol::EXTERN))
    {
        start->Declare(Symbol::EXTERN);
        start->setDeclSource(source);
    }
    start->AddAssocData(std::auto_ptr<BinSymbol>
        (new BinSymbol(*section, *bsd, BinSymbol::START)));

    SymbolRef vstart = m_object.getSymbol(("section."+name+".vstart").str());
    if (vstart->okToDeclare(Symbol::EXTERN))
    {
        vstart->Declare(Symbol::EXTERN);
        vstart->setDeclSource(source);
    }
    vstart->AddAssocData(std::auto_ptr<BinSymbol>
        (new BinSymbol(*section, *bsd, BinSymbol::VSTART)));

    SymbolRef length = m_object.getSymbol(("section."+name+".length").str());
    if (length->okToDeclare(Symbol::EXTERN))
    {
        length->Declare(Symbol::EXTERN);
        length->setDeclSource(source);
    }
    length->AddAssocData(std::auto_ptr<BinSymbol>
        (new BinSymbol(*section, *bsd, BinSymbol::LENGTH)));

    section->AddAssocData(std::auto_ptr<BinSection>(bsd.release()));

    return section;
}

void
BinObject::DirSection(DirectiveInfo& info, Diagnostic& diags)
{
    assert(info.isObject(m_object));
    NameValues& nvs = info.getNameValues();
    clang::SourceLocation source = info.getSource();

    NameValue& sectname_nv = nvs.front();
    if (!sectname_nv.isString())
    {
        diags.Report(sectname_nv.getValueRange().getBegin(),
                     diag::err_value_string_or_id);
        return;
    }
    llvm::StringRef sectname = sectname_nv.getString();

    Section* sect = m_object.FindSection(sectname);
    bool first = true;
    if (sect)
        first = sect->isDefault();
    else
        sect = AppendSection(sectname, source);

    m_object.setCurSection(sect);
    sect->setDefault(false);

    // No name/values, so nothing more to do
    if (nvs.size() <= 1)
        return;

    // Ignore flags if we've seen this section before
    if (!first)
    {
        diags.Report(info.getSource(), diag::warn_section_redef_flags);
        return;
    }

    // Parse section flags
    bool has_follows = false, has_vfollows = false;
    bool has_start = false, has_vstart = false;
    std::auto_ptr<Expr> start(0);
    std::auto_ptr<Expr> vstart(0);

    BinSection* bsd = sect->getAssocData<BinSection>();
    assert(bsd);
    unsigned long bss = sect->isBSS();
    unsigned long code = sect->isCode();

    DirHelpers helpers;
    helpers.Add("follows", true,
                BIND::bind(&DirString, _1, _2, &bsd->follows, &has_follows));
    helpers.Add("vfollows", true,
                BIND::bind(&DirString, _1, _2, &bsd->vfollows, &has_vfollows));
    helpers.Add("start", true,
                BIND::bind(&DirExpr, _1, _2, &m_object, &start, &has_start));
    helpers.Add("vstart", true,
                BIND::bind(&DirExpr, _1, _2, &m_object, &vstart, &has_vstart));
    helpers.Add("align", true,
                BIND::bind(&DirIntNumPower2, _1, _2, &m_object, &bsd->align,
                           &bsd->has_align));
    helpers.Add("valign", true,
                BIND::bind(&DirIntNumPower2, _1, _2, &m_object, &bsd->valign,
                           &bsd->has_valign));
    helpers.Add("nobits", false, BIND::bind(&DirSetFlag, _1, _2, &bss, 1));
    helpers.Add("progbits", false, BIND::bind(&DirClearFlag, _1, _2, &bss, 1));
    helpers.Add("code", false, BIND::bind(&DirSetFlag, _1, _2, &code, 1));
    helpers.Add("data", false, BIND::bind(&DirClearFlag, _1, _2, &code, 1));
    helpers.Add("execute", false, BIND::bind(&DirSetFlag, _1, _2, &code, 1));
    helpers.Add("noexecute", false,
                BIND::bind(&DirClearFlag, _1, _2, &code, 1));

    helpers(++nvs.begin(), nvs.end(), info.getSource(), diags,
            DirNameValueWarn);

    if (start.get() != 0)
    {
        bsd->start.reset(start.release());
        bsd->start_source = source;
    }
    if (vstart.get() != 0)
    {
        bsd->vstart.reset(vstart.release());
        bsd->vstart_source = source;
    }

    if (bsd->start.get() != 0 && !bsd->follows.empty())
    {
        diags.Report(info.getSource(), diags.getCustomDiagID(Diagnostic::Error,
            "cannot combine '%0' and '%1' section attributes"))
            << "START" << "FOLLOWS";
    }

    if (bsd->vstart.get() != 0 && !bsd->vfollows.empty())
    {
        diags.Report(info.getSource(), diags.getCustomDiagID(Diagnostic::Error,
            "cannot combine '%0' and '%1' section attributes"))
            << "VSTART" << "VFOLLOWS";
    }

    sect->setBSS(bss);
    sect->setCode(code);
}

void
BinObject::DirOrg(DirectiveInfo& info, Diagnostic& diags)
{
    // We only allow a single ORG in a program.
    if (m_org.get() != 0)
    {
        diags.Report(info.getSource(),
            diags.getCustomDiagID(Diagnostic::Error,
                                  "program origin redefined"));
        return;
    }

    // ORG takes just a simple expression as param
    const NameValue& nv = info.getNameValues().front();
    if (!nv.isExpr())
    {
        diags.Report(info.getSource(), diag::err_value_expression)
            << nv.getValueRange();
        return;
    }
    m_org.reset(new Expr(nv.getExpr(info.getObject())));
    m_org_source = info.getSource();
}

bool
BinObject::setMapFilename(const NameValue& nv,
                          clang::SourceLocation dir_source,
                          Diagnostic& diags)
{
    if (!m_map_filename.empty())
    {
        diags.Report(nv.getValueRange().getBegin(),
            diags.getCustomDiagID(Diagnostic::Error,
                                  "map file already specified"));
        return true;
    }

    if (!nv.isString())
    {
        diags.Report(nv.getValueRange().getBegin(),
                     diag::err_value_string_or_id);
        return false;
    }
    m_map_filename = nv.getString();
    return true;
}

void
BinObject::DirMap(DirectiveInfo& info, Diagnostic& diags)
{
    DirHelpers helpers;
    helpers.Add("all", false,
                BIND::bind(&DirSetFlag, _1, _2, &m_map_flags,
                           MAP_BRIEF|MAP_SECTIONS|MAP_SYMBOLS));
    helpers.Add("brief", false,
                BIND::bind(&DirSetFlag, _1, _2, &m_map_flags,
                           static_cast<unsigned long>(MAP_BRIEF)));
    helpers.Add("sections", false,
                BIND::bind(&DirSetFlag, _1, _2, &m_map_flags,
                           static_cast<unsigned long>(MAP_SECTIONS)));
    helpers.Add("segments", false,
                BIND::bind(&DirSetFlag, _1, _2, &m_map_flags,
                           static_cast<unsigned long>(MAP_SECTIONS)));
    helpers.Add("symbols", false,
                BIND::bind(&DirSetFlag, _1, _2, &m_map_flags,
                           static_cast<unsigned long>(MAP_SYMBOLS)));

    m_map_flags |= MAP_NONE;

    helpers(info.getNameValues().begin(), info.getNameValues().end(),
            info.getSource(), diags,
            BIND::bind(&BinObject::setMapFilename, this, _1, _2, _3));
}

std::vector<llvm::StringRef>
BinObject::getDebugFormatKeywords()
{
    static const char* keywords[] = {"null"};
    return std::vector<llvm::StringRef>(keywords, keywords+NELEMS(keywords));
}

void
BinObject::AddDirectives(Directives& dirs, llvm::StringRef parser)
{
    static const Directives::Init<BinObject> nasm_dirs[] =
    {
        {"section", &BinObject::DirSection, Directives::ARG_REQUIRED},
        {"segment", &BinObject::DirSection, Directives::ARG_REQUIRED},
        {"org",     &BinObject::DirOrg, Directives::ARG_REQUIRED},
        {"map",     &BinObject::DirMap, Directives::ANY},
    };
    static const Directives::Init<BinObject> gas_dirs[] =
    {
        {".section", &BinObject::DirSection, Directives::ARG_REQUIRED},
    };

    if (parser.equals_lower("nasm"))
        dirs.AddArray(this, nasm_dirs, NELEMS(nasm_dirs));
    else if (parser.equals_lower("gas") || parser.equals_lower("gnu"))
        dirs.AddArray(this, gas_dirs, NELEMS(gas_dirs));
}

void
yasm_objfmt_bin_DoRegister()
{
    RegisterModule<ObjectFormatModule,
                   ObjectFormatModuleImpl<BinObject> >("bin");
}
