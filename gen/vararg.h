#ifndef LDC_GEN_VARARG_H
#define LDC_GEN_VARARG_H

#include <vector>
#include "arraytypes.h"
#include "llvm/Attributes.h"

struct Type;
struct TypeFunction;
namespace llvm
{
    class Value;
    class FunctionType;
}

/**
 * The way varargs are handled is highly platform-dependent. This class
 * encapsulates the needed types and rewrites for handling functions with
 * varargs.
 */
class VarargABI {
public:
    /**
     * Returns the vararg ABI for the target architecture, as defined by
     * global.params.cpu.
     */
    static VarargABI* target();

    /**
     * Returns the D frontend representation of the va_list type of the
     * target ABI.
     *
     * This is the type of _argptr in D variadics and is exposed to user code
     * via the va_list pragma.
     */
    virtual Type* vaListType() = 0;

    /**
     * Modifies the passed IrFuncTy to accomodate the present vararg style,
     * if any.
     */
    virtual void lowerVarargsToType(/*in*/ TypeFunction* type,
        IrFuncTy& irFuncTy) = 0;

    /**
     * Appends the given arguments to the parameter/attribute list in a fashion
     * depending on the variadic argument style of the passed function type,
     * assuming the implicit return value (sreg) and context (this/delegate)
     * arguments have already been processed.
     *
     * Returns true if varargs should be appended in the default way (like for a
     * C-variadic function), or false if they were already handled.
     */
    virtual bool lowerArgumentList(/*in*/ TypeFunction* calleeType,
        /*in*/ llvm::FunctionType* calleeLLType, /*in*/ Expressions* arguments,
        size_t startIndex, std::vector<llvm::Value*>& argumentValues,
        std::vector<llvm::AttributeWithIndex>& argumentAttrs) = 0;
};

#endif
