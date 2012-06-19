#include "aggregate.h"
#include "module.h"
#include "mtype.h"
#include "gen/functions.h"
#include "gen/irstate.h"
#include "gen/llvmhelpers.h"
#include "gen/logger.h"
#include "gen/tollvm.h"
#include "gen/vararg.h"

namespace
{
    class X86VarargABI : public VarargABI
    {
    public:
        virtual Type* vaListType()
        {
            return Type::tvoidptr;
        }

        virtual void lowerVarargsToType(TypeFunction* f, IrFuncTy& fty)
        {
            if (f->varargs)
            {
                if (f->linkage == LINKd)
                {
                    // d style with hidden args
                    // 2 (array) is handled by the frontend
                    if (f->varargs == 1)
                    {
                        fty.arg_arguments = new IrFuncTyArg(
                            Type::typeinfo->type->arrayOf(), false);
                        fty.arg_argptr = new IrFuncTyArg(
                            Type::tvoid->pointerTo(), false,
                            llvm::Attribute::NoAlias | llvm::Attribute::NoCapture);
                    }
                }
                else if (f->linkage == LINKc)
                {
                    fty.c_vararg = true;
                }
                else
                {
                    f->error(0, "invalid linkage for variadic function");
                    fatal();
                }
            }
        }

        virtual bool lowerArgumentList(/*in*/ TypeFunction* tf,
            /*in*/ llvm::FunctionType* callableTy, /*in*/ Expressions* arguments,
            size_t argidx, std::vector<llvm::Value*>& args,
            std::vector<llvm::AttributeWithIndex>& attrs)
        {
            // Only D-style variadics are interesting for us.
            if (!(tf->linkage == LINKd && tf->varargs == 1)) return true;

            // On x86, we create a "struct" (only in the LLVM sense) containing
            // all the arguments on the stack, then passing off a pointer to it.
            // We also need to build the type info array (_arguments).
            Logger::println("doing d-style variadic arguments");
            LOG_SCOPE

            std::vector<LLType*> vtypes;

            // number of non variadic args
            int begin = Parameter::dim(tf->parameters);
            Logger::println("num non vararg params = %d", begin);

            // get n args in arguments list
            size_t n_arguments = arguments ? arguments->dim : 0;

            // build struct with argument types (non variadic args)
            for (int i=begin; i<n_arguments; i++)
            {
                Expression* argexp = (Expression*)arguments->data[i];
                assert(argexp->type->ty != Ttuple);
                vtypes.push_back(DtoType(argexp->type));
                size_t sz = getTypePaddedSize(vtypes.back());
                size_t asz = (sz + PTRSIZE - 1) & ~(PTRSIZE -1);
                if (sz != asz)
                {
                    if (sz < PTRSIZE)
                    {
                        vtypes.back() = DtoSize_t();
                    }
                    else
                    {
                        // ok then... so we build some type that is big enough
                        // and aligned to PTRSIZE
                        std::vector<LLType*> gah;
                        gah.reserve(asz/PTRSIZE);
                        size_t gah_sz = 0;
                        while (gah_sz < asz)
                        {
                            gah.push_back(DtoSize_t());
                            gah_sz += PTRSIZE;
                        }
                        vtypes.back() = LLStructType::get(gIR->context(), gah, true);
                    }
                }
            }
            LLStructType* vtype = LLStructType::get(gIR->context(), vtypes);

            if (Logger::enabled())
                Logger::cout() << "d-variadic argument struct type:\n" << *vtype << '\n';

            LLValue* mem = DtoRawAlloca(vtype, 0, "_argptr_storage");

            // store arguments in the struct
            for (int i=begin,k=0; i<n_arguments; i++,k++)
            {
                Expression* argexp = (Expression*)arguments->data[i];
                if (global.params.llvmAnnotate)
                    DtoAnnotation(argexp->toChars());
                LLValue* argdst = DtoGEPi(mem,0,k);
                argdst = DtoBitCast(argdst, getPtrToType(DtoType(argexp->type)));
                DtoVariadicArgument(argexp, argdst);
            }

            // Build type info array.
            // TODO: Should we transition this to a type tuple TypeInfo like
            // done for D2? DMD does it that way for D1 since ages as well.
            LLType* typeinfotype = DtoType(Type::typeinfo->type);
            LLArrayType* typeinfoarraytype = LLArrayType::get(typeinfotype,vtype->getNumElements());

            llvm::GlobalVariable* typeinfomem =
                new llvm::GlobalVariable(*gIR->module, typeinfoarraytype, true, llvm::GlobalValue::InternalLinkage, NULL, "._arguments.storage");
            if (Logger::enabled())
                Logger::cout() << "_arguments storage: " << *typeinfomem << '\n';

            std::vector<LLConstant*> vtypeinfos;
            for (int i=begin,k=0; i<n_arguments; i++,k++)
            {
                Expression* argexp = (Expression*)arguments->data[i];
                vtypeinfos.push_back(DtoTypeInfoOf(argexp->type));
            }

            // apply initializer
            LLConstant* tiinits = LLConstantArray::get(typeinfoarraytype, vtypeinfos);
            typeinfomem->setInitializer(tiinits);

            // put data in d-array
            std::vector<LLConstant*> pinits;
            pinits.push_back(DtoConstSize_t(vtype->getNumElements()));
            pinits.push_back(llvm::ConstantExpr::getBitCast(typeinfomem, getPtrToType(typeinfotype)));
            LLType* tiarrty = DtoType(Type::typeinfo->type->arrayOf());
            tiinits = LLConstantStruct::get(isaStruct(tiarrty), pinits);
            LLValue* typeinfoarrayparam = new llvm::GlobalVariable(*gIR->module, tiarrty,
                true, llvm::GlobalValue::InternalLinkage, tiinits, "._arguments.array");

            llvm::AttributeWithIndex Attr;
            // specify arguments
            args.push_back(DtoLoad(typeinfoarrayparam));
            if (llvm::Attributes atts = tf->fty.arg_arguments->attrs) {
                Attr.Index = argidx;
                Attr.Attrs = atts;
                attrs.push_back(Attr);
            }
            ++argidx;

            args.push_back(gIR->ir->CreateBitCast(mem, getPtrToType(LLType::getInt8Ty(gIR->context())), "tmp"));
            if (llvm::Attributes atts = tf->fty.arg_argptr->attrs) {
                Attr.Index = argidx;
                Attr.Attrs = atts;
                attrs.push_back(Attr);
            }

            // pass non variadic args
            return false;
        }
    };


    class X86_64VarargABI : public VarargABI
    {
    public:
        X86_64VarargABI() : vaListCached(NULL) {}

        virtual Type* vaListType()
        {
            if (!vaListCached)
            {
                vaListCached = new TypeIdentifier(NULL, Lexer::idPool("__va_list"));
                // Kludge: Need to get a scope to resolve.
                assert(Module::rootModule && "Root module not yet built, need "
                    "to rethink __va_list lookup.");
                Scope* sc = Module::rootModule->scope;
                assert(sc && "Root module has no scope setup yet, need to rethink "
                    "__va_list lookup.");
                vaListCached = vaListCached->semantic(NULL, sc);
                if (vaListCached == Type::terror)
                {
                    error("Could not resolve __va_list - druntime (object.di) out of sync?");
                    fatal();
                }
            }
            return vaListCached;
        }

        virtual void lowerVarargsToType(/*in*/ TypeFunction* type, IrFuncTy& irFuncTy)
        {
            if (type->varargs)
            {
                if (type->linkage == LINKd)
                {
                    // d style with hidden args
                    // 2 (array) is handled by the frontend
                    if (type->varargs == 1)
                    {
                        irFuncTy.arg_arguments =
                            new IrFuncTyArg(Type::typeinfotypelist->type, false);
                        irFuncTy.c_vararg = true;
                    }
                }
                else if (type->linkage == LINKc)
                {
                    irFuncTy.c_vararg = true;
                }
                else
                {
                    type->error(0, "invalid linkage for variadic function");
                    fatal();
                }
            }
        }

        virtual bool lowerArgumentList(/*in*/ TypeFunction* calleeType,
            /*in*/ llvm::FunctionType* calleeLLType, /*in*/ Expressions* arguments,
            size_t startIndex, std::vector<llvm::Value*>& argumentValues,
            std::vector<llvm::AttributeWithIndex>& argumentAttrs)
        {
            // Only D-style variadics are interesting for us.
            if (!(calleeType->linkage == LINKd && calleeType->varargs == 1))
            {
                return true;
            }

            // Build the type tuple TypeInfo argument, then continue with the
            // rest of the arguments as usual.
            size_t numFixed = calleeType->parameters->dim;
            assert(arguments->dim >= numFixed && "Expected at least as many "
                "arguments as there are fixed parameters");
            size_t numVariable = arguments->dim - numFixed;

            Parameters *args = new Parameters;
            args->setDim(numVariable);
            for (size_t i = 0; i < numVariable; i++)
            {
                (*args)[i] = new Parameter(STCin, (*arguments)[numFixed + i]->type, NULL, NULL);
            }

            LLValue* typeInfo = DtoTypeInfoOf(new TypeTuple(args), false);
            argumentValues.push_back(DtoBitCast(typeInfo, DtoType(Type::typeinfotypelist->type)));

            return true;
        }

    private:
        Type* vaListCached;
    };
}

VarargABI* VarargABI::target()
{
    switch(global.params.cpu)
    {
    default:
        Logger::cout() << "WARNING: Unknown ABI for varargs, defaulting to x86.\n";
        // Fall through.
    case ARCHx86:
        static X86VarargABI* x86ABI = NULL;
        if (!x86ABI) x86ABI = new X86VarargABI;
        return x86ABI;
    case ARCHx86_64:
        static X86_64VarargABI* x86_64ABI = NULL;
        if (!x86_64ABI) x86_64ABI = new X86_64VarargABI;
        return x86_64ABI;
    }
}
