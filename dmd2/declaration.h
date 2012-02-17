
// Compiler implementation of the D programming language
// Copyright (c) 1999-2011 by Digital Mars
// All Rights Reserved
// written by Walter Bright
// http://www.digitalmars.com
// License for redistribution is by either the Artistic License
// in artistic.txt, or the GNU General Public License in gnu.txt.
// See the included readme.txt for details.

#ifndef DMD_DECLARATION_H
#define DMD_DECLARATION_H

#ifdef __DMC__
#pragma once
#endif /* __DMC__ */

#if IN_LLVM
#include <set>
#include <map>
#include <string>
#include <llvm/Analysis/DebugInfo.h>
#endif

#include "dsymbol.h"
#include "lexer.h"
#include "mtype.h"

struct Expression;
struct Statement;
struct LabelDsymbol;
#if IN_LLVM
struct LabelStatement;
#endif
struct Initializer;
struct Module;
struct InlineScanState;
struct ForeachStatement;
struct FuncDeclaration;
struct ExpInitializer;
struct StructDeclaration;
struct TupleType;
struct InterState;
struct IRState;
#if IN_LLVM
struct AnonDeclaration;
#endif

enum PROT;
enum LINK;
enum TOK;
enum MATCH;
enum PURE;

#define STCundefined    0LL
#define STCstatic       1LL
#define STCextern       2LL
#define STCconst        4LL
#define STCfinal        8LL
#define STCabstract     0x10LL
#define STCparameter    0x20LL
#define STCfield        0x40LL
#define STCoverride     0x80LL
#define STCauto         0x100LL
#define STCsynchronized 0x200LL
#define STCdeprecated   0x400LL
#define STCin           0x800LL         // in parameter
#define STCout          0x1000LL        // out parameter
#define STClazy         0x2000LL        // lazy parameter
#define STCforeach      0x4000LL        // variable for foreach loop
#define STCcomdat       0x8000LL        // should go into COMDAT record
#define STCvariadic     0x10000LL       // variadic function argument
#define STCctorinit     0x20000LL       // can only be set inside constructor
#define STCtemplateparameter  0x40000LL // template parameter
#define STCscope        0x80000LL       // template parameter
#define STCimmutable    0x100000LL
#define STCref          0x200000LL
#define STCinit         0x400000LL      // has explicit initializer
#define STCmanifest     0x800000LL      // manifest constant
#define STCnodtor       0x1000000LL     // don't run destructor
#define STCnothrow      0x2000000LL     // never throws exceptions
#define STCpure         0x4000000LL     // pure function
#define STCtls          0x8000000LL     // thread local
#define STCalias        0x10000000LL    // alias parameter
#define STCshared       0x20000000LL    // accessible from multiple threads
#define STCgshared      0x40000000LL    // accessible from multiple threads
                                        // but not typed as "shared"
#define STCwild         0x80000000LL    // for "wild" type constructor
#define STC_TYPECTOR    (STCconst | STCimmutable | STCshared | STCwild)
#define STC_FUNCATTR    (STCref | STCnothrow | STCpure | STCproperty | STCsafe | STCtrusted | STCsystem)

#define STCproperty     0x100000000LL
#define STCsafe         0x200000000LL
#define STCtrusted      0x400000000LL
#define STCsystem       0x800000000LL
#define STCctfe         0x1000000000LL  // can be used in CTFE, even if it is static
#define STCdisable      0x2000000000LL  // for functions that are not callable
#define STCresult       0x4000000000LL  // for result variables passed to out contracts
#define STCnodefaultctor 0x8000000000LL  // must be set inside constructor

struct Match
{
    int count;                  // number of matches found
    MATCH last;                 // match level of lastf
    FuncDeclaration *lastf;     // last matching function we found
    FuncDeclaration *nextf;     // current matching function
    FuncDeclaration *anyf;      // pick a func, any func, to use for error recovery
};

void overloadResolveX(Match *m, FuncDeclaration *f,
	Expression *ethis, Expressions *arguments, Module* from);
int overloadApply(FuncDeclaration *fstart,
        int (*fp)(void *, FuncDeclaration *),
        void *param);

enum Semantic
{
    SemanticStart,      // semantic has not been run
    SemanticIn,         // semantic() is in progress
    SemanticDone,       // semantic() has been run
    Semantic2Done,      // semantic2() has been run
};

/**************************************************************/

struct Declaration : Dsymbol
{
    Type *type;
    Type *originalType;         // before semantic analysis
    StorageClass storage_class;
    enum PROT protection;
    enum LINK linkage;
    int inuse;                  // used to detect cycles

    enum Semantic sem;

    Declaration(Identifier *id);
    void semantic(Scope *sc);
    const char *kind();
    unsigned size(Loc loc);
    void checkModify(Loc loc, Scope *sc, Type *t);

    void emitComment(Scope *sc);
    void toJsonBuffer(OutBuffer *buf);
    void toDocBuffer(OutBuffer *buf);

    char *mangle();
    int isStatic() { return storage_class & STCstatic; }
    virtual int isDelete();
    virtual int isDataseg();
    virtual int isThreadlocal();
    virtual int isCodeseg();
    int isCtorinit()     { return storage_class & STCctorinit; }
    int isFinal()        { return storage_class & STCfinal; }
    int isAbstract()     { return storage_class & STCabstract; }
    int isConst()        { return storage_class & STCconst; }
    int isImmutable()    { return storage_class & STCimmutable; }
    int isWild()         { return storage_class & STCwild; }
    int isAuto()         { return storage_class & STCauto; }
    int isScope()        { return storage_class & STCscope; }
    int isSynchronized() { return storage_class & STCsynchronized; }
    int isParameter()    { return storage_class & STCparameter; }
    int isDeprecated()   { return storage_class & STCdeprecated; }
    int isOverride()     { return storage_class & STCoverride; }
    StorageClass isResult()       { return storage_class & STCresult; }

    int isIn()    { return storage_class & STCin; }
    int isOut()   { return storage_class & STCout; }
    int isRef()   { return storage_class & STCref; }

    enum PROT prot();

    Declaration *isDeclaration() { return this; }

#if IN_LLVM
    /// Codegen traversal
    virtual void codegen(Ir* ir);
#endif
};

/**************************************************************/

struct TupleDeclaration : Declaration
{
    Objects *objects;
    int isexp;                  // 1: expression tuple

    TypeTuple *tupletype;       // !=NULL if this is a type tuple

    TupleDeclaration(Loc loc, Identifier *ident, Objects *objects);
    Dsymbol *syntaxCopy(Dsymbol *);
    const char *kind();
    Type *getType();
    int needThis();

    TupleDeclaration *isTupleDeclaration() { return this; }

#if IN_LLVM
    void semantic3(Scope *sc);
    /// Codegen traversal
    void codegen(Ir* ir);
#endif
};

/**************************************************************/

struct TypedefDeclaration : Declaration
{
    Type *basetype;
    Initializer *init;

    TypedefDeclaration(Loc loc, Identifier *ident, Type *basetype, Initializer *init);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);
    void semantic2(Scope *sc);
    char *mangle();
    const char *kind();
    Type *getType();
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);
    Type *htype;
    Type *hbasetype;

    void toDocBuffer(OutBuffer *buf);

#if IN_DMD
    void toObjFile(int multiobj);                       // compile to .obj file
    void toDebug();
    int cvMember(unsigned char *p);
#endif

    TypedefDeclaration *isTypedefDeclaration() { return this; }

#if IN_DMD
    Symbol *sinit;
    Symbol *toInitializer();
#endif

#if IN_LLVM
    /// Codegen traversal
    void codegen(Ir* ir);
#endif
};

/**************************************************************/

struct AliasDeclaration : Declaration
{
    Dsymbol *aliassym;
    Dsymbol *overnext;          // next in overload list
    int inSemantic;
    PROT importprot;	// if generated by import, store its protection

    AliasDeclaration(Loc loc, Identifier *ident, Type *type);
    AliasDeclaration(Loc loc, Identifier *ident, Dsymbol *s);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);
    int overloadInsert(Dsymbol *s);
    const char *kind();
    Type *getType();
    Dsymbol *toAlias();
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);
    Type *htype;
    Dsymbol *haliassym;

    void toDocBuffer(OutBuffer *buf);

    AliasDeclaration *isAliasDeclaration() { return this; }
};

/**************************************************************/

struct VarDeclaration : Declaration
{
    Initializer *init;
    unsigned offset;
    int noscope;                 // no auto semantics
#if DMDV2
    FuncDeclarations nestedrefs; // referenced by these lexically nested functions
    bool isargptr;              // if parameter that _argptr points to
#else
    int nestedref;              // referenced by a lexically nested function
#endif
    int ctorinit;               // it has been initialized in a ctor
    int onstack;                // 1: it has been allocated on the stack
                                // 2: on stack, run destructor anyway
    int canassign;              // it can be assigned to
    Dsymbol *aliassym;          // if redone as alias to another symbol

    // When interpreting, these point to the value (NULL if value not determinable)
    // The index of this variable on the CTFE stack, -1 if not allocated
    size_t ctfeAdrOnStack;
    // The various functions are used only to detect compiler CTFE bugs
    Expression *getValue();
    bool hasValue();
    void setValueNull();
    void setValueWithoutChecking(Expression *newval);
    void setValue(Expression *newval);

#if DMDV2
    VarDeclaration *rundtor;    // if !NULL, rundtor is tested at runtime to see
                                // if the destructor should be run. Used to prevent
                                // dtor calls on postblitted vars
    Expression *edtor;          // if !=NULL, does the destruction of the variable
#endif

    VarDeclaration(Loc loc, Type *t, Identifier *id, Initializer *init);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);
    void semantic2(Scope *sc);
    const char *kind();
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);
    Type *htype;
    Initializer *hinit;
    AggregateDeclaration *isThis();
    int needThis();
    int isImportedSymbol();
    int isDataseg();
    int isThreadlocal();
    int isCTFE();
    int hasPointers();
#if DMDV2
    int canTakeAddressOf();
    int needsAutoDtor();
#endif
    Expression *callScopeDtor(Scope *sc);
    ExpInitializer *getExpInitializer();
    Expression *getConstInitializer();
    void checkCtorConstInit();
    void checkNestedReference(Scope *sc, Loc loc);
    Dsymbol *toAlias();

#if IN_DMD
    void toObjFile(int multiobj);                       // compile to .obj file
    Symbol *toSymbol();
    int cvMember(unsigned char *p);
#endif

    // Eliminate need for dynamic_cast
    VarDeclaration *isVarDeclaration() { return (VarDeclaration *)this; }

#if IN_LLVM
    /// Codegen traversal
    virtual void codegen(Ir* ir);

    /// Index into parent aggregate.
    /// Set during type generation.
    unsigned aggrIndex;

    /// Variables that wouldn't have gotten semantic3'ed if we weren't inlining set this flag.
    bool availableExternally;
    /// Override added to set above flag.
    void semantic3(Scope *sc);

    // FIXME: we're not using these anymore!
    AnonDeclaration* anonDecl;
    unsigned offset2;

    /// This var is used by a naked function.
    bool nakedUse;

    // debug description
    llvm::DIVariable debugVariable;
    llvm::DISubprogram debugFunc;
#endif
};

/**************************************************************/

// LDC uses this to denote static struct initializers

struct StaticStructInitDeclaration : Declaration
{
    StructDeclaration *dsym;

    StaticStructInitDeclaration(Loc loc, StructDeclaration *dsym);

#if IN_DMD
    Symbol *toSymbol();
#endif

    // Eliminate need for dynamic_cast
    StaticStructInitDeclaration *isStaticStructInitDeclaration() { return (StaticStructInitDeclaration *)this; }
};

struct ClassInfoDeclaration : VarDeclaration
{
    ClassDeclaration *cd;

    ClassInfoDeclaration(ClassDeclaration *cd);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);

    void emitComment(Scope *sc);
    void toJsonBuffer(OutBuffer *buf);

#if IN_DMD
    Symbol *toSymbol();
#endif

    ClassInfoDeclaration* isClassInfoDeclaration() { return this; }
};

struct ModuleInfoDeclaration : VarDeclaration
{
    Module *mod;

    ModuleInfoDeclaration(Module *mod);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);

    void emitComment(Scope *sc);
    void toJsonBuffer(OutBuffer *buf);

#if IN_DMD
    Symbol *toSymbol();
#endif
};

struct TypeInfoDeclaration : VarDeclaration
{
    Type *tinfo;

    TypeInfoDeclaration(Type *tinfo, int internal);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);

    void emitComment(Scope *sc);
    void toJsonBuffer(OutBuffer *buf);

#if IN_DMD
    void toObjFile(int multiobj);                       // compile to .obj file
    Symbol *toSymbol();
    virtual void toDt(dt_t **pdt);
#endif

    virtual TypeInfoDeclaration* isTypeInfoDeclaration() { return this; }

#if IN_LLVM
    /// Codegen traversal
    void codegen(Ir* ir);
    virtual void llvmDefine();
#endif
};

struct TypeInfoStructDeclaration : TypeInfoDeclaration
{
    TypeInfoStructDeclaration(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif
};

struct TypeInfoClassDeclaration : TypeInfoDeclaration
{
    TypeInfoClassDeclaration(Type *tinfo);

#if IN_DMD
    Symbol *toSymbol();
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void codegen(Ir*);
    void llvmDefine();
#endif
};

struct TypeInfoInterfaceDeclaration : TypeInfoDeclaration
{
    TypeInfoInterfaceDeclaration(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif
};

struct TypeInfoTypedefDeclaration : TypeInfoDeclaration
{
    TypeInfoTypedefDeclaration(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif
};

struct TypeInfoPointerDeclaration : TypeInfoDeclaration
{
    TypeInfoPointerDeclaration(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif
};

struct TypeInfoArrayDeclaration : TypeInfoDeclaration
{
    TypeInfoArrayDeclaration(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif
};

struct TypeInfoStaticArrayDeclaration : TypeInfoDeclaration
{
    TypeInfoStaticArrayDeclaration(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif
};

struct TypeInfoAssociativeArrayDeclaration : TypeInfoDeclaration
{
    TypeInfoAssociativeArrayDeclaration(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif
};

struct TypeInfoEnumDeclaration : TypeInfoDeclaration
{
    TypeInfoEnumDeclaration(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif
};

struct TypeInfoFunctionDeclaration : TypeInfoDeclaration
{
    TypeInfoFunctionDeclaration(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif
};

struct TypeInfoDelegateDeclaration : TypeInfoDeclaration
{
    TypeInfoDelegateDeclaration(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif
};

struct TypeInfoTupleDeclaration : TypeInfoDeclaration
{
    TypeInfoTupleDeclaration(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif
};

#if DMDV2
struct TypeInfoConstDeclaration : TypeInfoDeclaration
{
    TypeInfoConstDeclaration(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif
};

struct TypeInfoInvariantDeclaration : TypeInfoDeclaration
{
    TypeInfoInvariantDeclaration(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif
};

struct TypeInfoSharedDeclaration : TypeInfoDeclaration
{
    TypeInfoSharedDeclaration(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif
};

struct TypeInfoWildDeclaration : TypeInfoDeclaration
{
    TypeInfoWildDeclaration(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif
};

struct TypeInfoVectorDeclaration : TypeInfoDeclaration
{
    TypeInfoVectorDeclaration(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif
};
#endif

/**************************************************************/

struct ThisDeclaration : VarDeclaration
{
    ThisDeclaration(Loc loc, Type *t);
    Dsymbol *syntaxCopy(Dsymbol *);
    ThisDeclaration *isThisDeclaration() { return this; }
};

enum ILS
{
    ILSuninitialized,   // not computed yet
    ILSno,              // cannot inline
    ILSyes,             // can inline
};

/**************************************************************/
#if DMDV2

enum BUILTIN
{
    BUILTINunknown = -1,        // not known if this is a builtin
    BUILTINnot,                 // this is not a builtin
    BUILTINsin,                 // std.math.sin
    BUILTINcos,                 // std.math.cos
    BUILTINtan,                 // std.math.tan
    BUILTINsqrt,                // std.math.sqrt
    BUILTINfabs,                // std.math.fabs
    BUILTINatan2,               // std.math.atan2
    BUILTINrndtol,              // std.math.rndtol
    BUILTINexpm1,               // std.math.expm1
    BUILTINexp2,                // std.math.exp2
    BUILTINyl2x,                // std.math.yl2x
    BUILTINyl2xp1,              // std.math.yl2xp1
    BUILTINbsr,                 // core.bitop.bsr
    BUILTINbsf,                 // core.bitop.bsf
    BUILTINbswap,               // core.bitop.bswap
};

Expression *eval_builtin(Loc loc, enum BUILTIN builtin, Expressions *arguments);

#else
enum BUILTIN { };
#endif

struct FuncDeclaration : Declaration
{
    Types *fthrows;                     // Array of Type's of exceptions (not used)
    Statement *frequire;
    Statement *fensure;
    Statement *fbody;

    FuncDeclarations foverrides;        // functions this function overrides
    FuncDeclaration *fdrequire;         // function that does the in contract
    FuncDeclaration *fdensure;          // function that does the out contract

    Expressions *fdrequireParams;
    Expressions *fdensureParams;

    Identifier *outId;                  // identifier for out statement
    VarDeclaration *vresult;            // variable corresponding to outId
    LabelDsymbol *returnLabel;          // where the return goes

    DsymbolTable *localsymtab;          // used to prevent symbols in different
                                        // scopes from having the same name
    VarDeclaration *vthis;              // 'this' parameter (member and nested)
    VarDeclaration *v_arguments;        // '_arguments' parameter
#if IN_GCC
    VarDeclaration *v_argptr;           // '_argptr' variable
#endif
    VarDeclaration *v_argsave;          // save area for args passed in registers for variadic functions
    VarDeclarations *parameters;        // Array of VarDeclaration's for parameters
    DsymbolTable *labtab;               // statement label symbol table
    Declaration *overnext;              // next in overload list
    Loc endloc;                         // location of closing curly bracket
    int vtblIndex;                      // for member functions, index into vtbl[]
    int naked;                          // !=0 if naked
    ILS inlineStatusStmt;
    ILS inlineStatusExp;
    int inlineNest;                     // !=0 if nested inline
    int isArrayOp;                      // !=0 if array operation
    enum PASS semanticRun;
    int semantic3Errors;                // !=0 if errors in semantic3
                                        // this function's frame ptr
    ForeachStatement *fes;              // if foreach body, this is the foreach
    int introducing;                    // !=0 if 'introducing' function
    Type *tintro;                       // if !=NULL, then this is the type
                                        // of the 'introducing' function
                                        // this one is overriding
    int inferRetType;                   // !=0 if return type is to be inferred
    StorageClass storage_class2;        // storage class for template onemember's

    // Things that should really go into Scope
    int hasReturnExp;                   // 1 if there's a return exp; statement
                                        // 2 if there's a throw statement
                                        // 4 if there's an assert(0)
                                        // 8 if there's inline asm

    // Support for NRVO (named return value optimization)
    int nrvo_can;                       // !=0 means we can do it
    VarDeclaration *nrvo_var;           // variable to replace with shidden
#if IN_DMD
    Symbol *shidden;                    // hidden pointer passed to function
#endif

#if DMDV2
    enum BUILTIN builtin;               // set if this is a known, builtin
                                        // function we can evaluate at compile
                                        // time

    int tookAddressOf;                  // set if someone took the address of
                                        // this function
    VarDeclarations closureVars;        // local variables in this function
                                        // which are referenced by nested
                                        // functions

    unsigned flags;
    #define FUNCFLAGpurityInprocess 1   // working on determining purity
    #define FUNCFLAGsafetyInprocess 2   // working on determining safety
    #define FUNCFLAGnothrowInprocess 4  // working on determining nothrow
#else
    int nestedFrameRef;                 // !=0 if nested variables referenced
#endif

    FuncDeclaration(Loc loc, Loc endloc, Identifier *id, StorageClass storage_class, Type *type);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);
    void semantic2(Scope *sc);
    void semantic3(Scope *sc);
    // called from semantic3
    void varArgs(Scope *sc, TypeFunction*, VarDeclaration *&, VarDeclaration *&);
    VarDeclaration *declareThis(Scope *sc, AggregateDeclaration *ad);
    int equals(Object *o);

    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);
    void bodyToCBuffer(OutBuffer *buf, HdrGenState *hgs);
    int overrides(FuncDeclaration *fd);
    int findVtblIndex(Dsymbols *vtbl, int dim);
    int overloadInsert(Dsymbol *s);
    FuncDeclaration *overloadExactMatch(Type *t, Module* from);
    FuncDeclaration *overloadResolve(Loc loc, Expression *ethis, Expressions *arguments, int flags = 0, Module* from=NULL);
    MATCH leastAsSpecialized(FuncDeclaration *g);
    LabelDsymbol *searchLabel(Identifier *ident);
    AggregateDeclaration *isThis();
    AggregateDeclaration *isMember2();
    int getLevel(Loc loc, Scope *sc, FuncDeclaration *fd); // lexical nesting level difference
    void appendExp(Expression *e);
    void appendState(Statement *s);
    char *mangle();
    const char *toPrettyChars();
    int isMain();
    int isWinMain();
    int isDllMain();
    enum BUILTIN isBuiltin();
    int isExport();
    int isImportedSymbol();
    int isAbstract();
    int isCodeseg();
    int isOverloadable();
    enum PURE isPure();
    enum PURE isPureBypassingInference();
    bool setImpure();
    int isSafe();
    int isTrusted();
    bool setUnsafe();
    virtual int isNested();
    int needThis();
    int isVirtualMethod();
    virtual int isVirtual();
    virtual int isFinal();
    virtual int addPreInvariant();
    virtual int addPostInvariant();
    Expression *interpret(InterState *istate, Expressions *arguments, Expression *thisexp = NULL);
    void inlineScan();
    int canInline(int hasthis, int hdrscan = false, int statementsToo = true);
    Expression *expandInline(InlineScanState *iss, Expression *ethis, Expressions *arguments, Statement **ps);
    const char *kind();
    void toDocBuffer(OutBuffer *buf);
    FuncDeclaration *isUnique();
    void checkNestedReference(Scope *sc, Loc loc);
    int needsClosure();
    int hasNestedFrameRefs();
    Statement *mergeFrequire(Statement *, Expressions *params = 0);
    Statement *mergeFensure(Statement *, Expressions *params = 0);
    Parameters *getParameters(int *pvarargs);

// LDC: give argument types to runtime functions
    static FuncDeclaration *genCfunc(Parameters *args, Type *treturn, const char *name);
    static FuncDeclaration *genCfunc(Parameters *args, Type *treturn, Identifier *id);

#if IN_DMD
    Symbol *toSymbol();
    Symbol *toThunkSymbol(int offset);  // thunk version
    void toObjFile(int multiobj);                       // compile to .obj file
    int cvMember(unsigned char *p);
    void buildClosure(IRState *irs); // Should this be inside or outside the #if IN_DMD?
#endif
    FuncDeclaration *isFuncDeclaration() { return this; }

#if IN_LLVM
    // LDC stuff

    /// Codegen traversal
    void codegen(Ir* ir);

    // vars declared in this function that nested funcs reference
    // is this is not empty, nestedFrameRef is set and these VarDecls
    // probably have nestedref set too, see VarDeclaration::checkNestedReference
    std::set<VarDeclaration*> nestedVars;

    std::string intrinsicName;

    bool isIntrinsic();
    bool isVaIntrinsic();

    // we keep our own table of label statements as LabelDsymbolS
    // don't always carry their corresponding statement along ...
    typedef std::map<const char*, LabelStatement*> LabelMap;
    LabelMap labmap;

    // Functions that wouldn't have gotten semantic3'ed if we weren't inlining set this flag.
    bool availableExternally;

    // true if overridden with the pragma(allow_inline); stmt
    bool allowInlining;
    
    // true if has inline assembler
    bool inlineAsm;
#endif
};

#if DMDV2
FuncDeclaration *resolveFuncCall(Scope *sc, Loc loc, Dsymbol *s,
        Objects *tiargs,
        Expression *ethis,
        Expressions *arguments,
        int flags);
#endif

struct FuncAliasDeclaration : FuncDeclaration
{
    FuncDeclaration *funcalias;
    PROT importprot;	// if generated by import, store its protection
    
    FuncAliasDeclaration(FuncDeclaration *funcalias);

    FuncAliasDeclaration *isFuncAliasDeclaration() { return this; }
    const char *kind();
#if IN_DMD
    Symbol *toSymbol();
#endif
};

struct FuncLiteralDeclaration : FuncDeclaration
{
    enum TOK tok;                       // TOKfunction or TOKdelegate

    FuncLiteralDeclaration(Loc loc, Loc endloc, Type *type, enum TOK tok,
        ForeachStatement *fes);
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);
    Dsymbol *syntaxCopy(Dsymbol *);
    int isNested();
    int isVirtual();

    FuncLiteralDeclaration *isFuncLiteralDeclaration() { return this; }
    const char *kind();
};

struct CtorDeclaration : FuncDeclaration
{
    CtorDeclaration(Loc loc, Loc endloc, StorageClass stc, Type *type);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);
    const char *kind();
    char *toChars();
    int isVirtual();
    int addPreInvariant();
    int addPostInvariant();

    CtorDeclaration *isCtorDeclaration() { return this; }
};

#if DMDV2
struct PostBlitDeclaration : FuncDeclaration
{
    PostBlitDeclaration(Loc loc, Loc endloc, StorageClass stc = STCundefined);
    PostBlitDeclaration(Loc loc, Loc endloc, Identifier *id);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);
    int isVirtual();
    int addPreInvariant();
    int addPostInvariant();
    int overloadInsert(Dsymbol *s);
    void emitComment(Scope *sc);
    void toJsonBuffer(OutBuffer *buf);

    PostBlitDeclaration *isPostBlitDeclaration() { return this; }
};
#endif

struct DtorDeclaration : FuncDeclaration
{
    DtorDeclaration(Loc loc, Loc endloc);
    DtorDeclaration(Loc loc, Loc endloc, Identifier *id);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);
    const char *kind();
    char *toChars();
    int isVirtual();
    int addPreInvariant();
    int addPostInvariant();
    int overloadInsert(Dsymbol *s);
    void emitComment(Scope *sc);
    void toJsonBuffer(OutBuffer *buf);

    DtorDeclaration *isDtorDeclaration() { return this; }
};

struct StaticCtorDeclaration : FuncDeclaration
{
    StaticCtorDeclaration(Loc loc, Loc endloc);
    StaticCtorDeclaration(Loc loc, Loc endloc, const char *name);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);
    AggregateDeclaration *isThis();
    int isVirtual();
    int addPreInvariant();
    int addPostInvariant();
    bool hasStaticCtorOrDtor();
    void emitComment(Scope *sc);
    void toJsonBuffer(OutBuffer *buf);
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);

    StaticCtorDeclaration *isStaticCtorDeclaration() { return this; }
};

#if DMDV2
struct SharedStaticCtorDeclaration : StaticCtorDeclaration
{
    SharedStaticCtorDeclaration(Loc loc, Loc endloc);
    Dsymbol *syntaxCopy(Dsymbol *);
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);

    SharedStaticCtorDeclaration *isSharedStaticCtorDeclaration() { return this; }
};
#endif

struct StaticDtorDeclaration : FuncDeclaration
{   VarDeclaration *vgate;      // 'gate' variable

    StaticDtorDeclaration(Loc loc, Loc endloc);
    StaticDtorDeclaration(Loc loc, Loc endloc, const char *name);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);
    AggregateDeclaration *isThis();
    int isVirtual();
    bool hasStaticCtorOrDtor();
    int addPreInvariant();
    int addPostInvariant();
    void emitComment(Scope *sc);
    void toJsonBuffer(OutBuffer *buf);
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);

    StaticDtorDeclaration *isStaticDtorDeclaration() { return this; }
};

#if DMDV2
struct SharedStaticDtorDeclaration : StaticDtorDeclaration
{
    SharedStaticDtorDeclaration(Loc loc, Loc endloc);
    Dsymbol *syntaxCopy(Dsymbol *);
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);

    SharedStaticDtorDeclaration *isSharedStaticDtorDeclaration() { return this; }
};
#endif

struct InvariantDeclaration : FuncDeclaration
{
    InvariantDeclaration(Loc loc, Loc endloc);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);
    int isVirtual();
    int addPreInvariant();
    int addPostInvariant();
    void emitComment(Scope *sc);
    void toJsonBuffer(OutBuffer *buf);
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);

    InvariantDeclaration *isInvariantDeclaration() { return this; }
};

struct UnitTestDeclaration : FuncDeclaration
{
    UnitTestDeclaration(Loc loc, Loc endloc);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);
    AggregateDeclaration *isThis();
    int isVirtual();
    int addPreInvariant();
    int addPostInvariant();
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);
    void toJsonBuffer(OutBuffer *buf);

    UnitTestDeclaration *isUnitTestDeclaration() { return this; }
};

struct NewDeclaration : FuncDeclaration
{   Parameters *arguments;
    int varargs;

    NewDeclaration(Loc loc, Loc endloc, Parameters *arguments, int varargs);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);
    const char *kind();
    int isVirtual();
    int addPreInvariant();
    int addPostInvariant();

    NewDeclaration *isNewDeclaration() { return this; }
};


struct DeleteDeclaration : FuncDeclaration
{   Parameters *arguments;

    DeleteDeclaration(Loc loc, Loc endloc, Parameters *arguments);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);
    const char *kind();
    int isDelete();
    int isVirtual();
    int addPreInvariant();
    int addPostInvariant();
    DeleteDeclaration *isDeleteDeclaration() { return this; }
};

#endif /* DMD_DECLARATION_H */
