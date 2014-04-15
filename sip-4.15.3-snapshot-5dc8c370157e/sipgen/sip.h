/*
 * The main header file for SIP.
 *
 * Copyright (c) 2013 Riverbank Computing Limited <info@riverbankcomputing.com>
 *
 * This file is part of SIP.
 *
 * This copy of SIP is licensed for use under the terms of the SIP License
 * Agreement.  See the file LICENSE for more details.
 *
 * This copy of SIP may also used under the terms of the GNU General Public
 * License v2 or v3 as published by the Free Software Foundation which can be
 * found in the files LICENSE-GPL2 and LICENSE-GPL3 included in this package.
 *
 * SIP is supplied WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */


#ifndef SIP_H
#define SIP_H

#include <stdio.h>
#include <sys/types.h>


/*
 * Define the SIP version number.
 */
#define SIP_VERSION         0x040f03
#define SIP_VERSION_STR     "4.15.3-snapshot-5dc8c370157e"


#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

#define TRUE    1
#define FALSE   0


#define DEFAULT_OFILE_EXT   ".o"    /* Default object file extension. */

#define MAX_NR_ARGS         20      /* Max. nr. args. to a function or template. */
#define MAX_NR_DEREFS       5       /* Max. nr. type derefences. */


/* For convenience. */

#define classBaseName(cd)   scopedNameTail((cd)->iff->fqcname)
#define classFQCName(cd)    ((cd)->iff->fqcname)


/* Handle module flags. */

#define MOD_HAS_DELAYED_DTORS   0x0001  /* It has a class with a delayed dtor. */
#define MOD_IS_CONSOLIDATED     0x0002  /* It is a consolidated module. */
#define MOD_IS_COMPOSITE        0x0004  /* It is a composite module. */
#define MOD_IS_TRANSFORMED      0x0008  /* It's types have been transformed. */
#define MOD_USE_ARG_NAMES       0x0010  /* Use real argument names. */
#define MOD_ALL_RAISE_PY_EXC    0x0020  /* All callable raise a Python exception. */
#define MOD_SUPER_INIT_NO       0x0040  /* Don't call super().__init__(). */
#define MOD_SUPER_INIT_YES      0x0080  /* Call super().__init__(). */
#define MOD_SUPER_INIT_UNDEF    0x0000  /* Calling super().__init__() is undefined. */
#define MOD_SUPER_INIT_MASK     0x00c0  /* The mask for the above flags. */

#define hasDelayedDtors(m)  ((m)->modflags & MOD_HAS_DELAYED_DTORS)
#define setHasDelayedDtors(m)   ((m)->modflags |= MOD_HAS_DELAYED_DTORS)
#define isConsolidated(m)   ((m)->modflags & MOD_IS_CONSOLIDATED)
#define setIsConsolidated(m)    ((m)->modflags |= MOD_IS_CONSOLIDATED)
#define isComposite(m)      ((m)->modflags & MOD_IS_COMPOSITE)
#define setIsComposite(m)   ((m)->modflags |= MOD_IS_COMPOSITE)
#define isContainer(m)      ((m)->modflags & (MOD_IS_CONSOLIDATED | MOD_IS_COMPOSITE))
#define setIsTransformed(m) ((m)->modflags |= MOD_IS_TRANSFORMED)
#define isTransformed(m)    ((m)->modflags & MOD_IS_TRANSFORMED)
#define setUseArgNames(m)   ((m)->modflags |= MOD_USE_ARG_NAMES)
#define useArgNames(m)      ((m)->modflags & MOD_USE_ARG_NAMES)
#define setAllRaisePyException(m)   ((m)->modflags |= MOD_ALL_RAISE_PY_EXC)
#define allRaisePyException(m)  ((m)->modflags & MOD_ALL_RAISE_PY_EXC)
#define setCallSuperInitNo(m)   ((m)->modflags = ((m)->modflags & MOD_SUPER_INIT_MASK) | MOD_SUPER_INIT_NO)
#define setCallSuperInitYes(m)  ((m)->modflags = ((m)->modflags & MOD_SUPER_INIT_MASK) | MOD_SUPER_INIT_YES)
#define isCallSuperInitYes(m)   (((m)->modflags & MOD_SUPER_INIT_MASK) == MOD_SUPER_INIT_YES)
#define isCallSuperInitUndefined(m) (((m)->modflags & MOD_SUPER_INIT_MASK) == MOD_SUPER_INIT_UNDEF)


/* Handle section flags. */

#define SECT_IS_PUBLIC      0x01        /* It is public. */
#define SECT_IS_PROT        0x02        /* It is protected. */
#define SECT_IS_PRIVATE     0x04        /* It is private. */
#define SECT_IS_SLOT        0x08        /* It is a slot. */
#define SECT_IS_SIGNAL      0x10        /* It is a signal. */
#define SECT_MASK           0x1f        /* The mask of all flags. */


/* Handle class flags.  These are combined with the section flags. */

#define CLASS_HAS_SIGSLOTS  0x00000200  /* It has signals or slots. */
#define CLASS_IS_ABSTRACT   0x00000400  /* It is an abstract class. */
#define CLASS_HAS_SHADOW    0x00000800  /* It is has a shadow class. */
#define CLASS_IS_OPAQUE     0x00001000  /* It is opaque. */
#define CLASS_HAS_VAR_HANDLERS  0x00002000  /* It has variable handlers. */
#define CLASS_DTOR_RELEASE_GIL  0x00004000  /* The dtor releases the GIL. */
#define CLASS_IS_PROTECTED  0x00008000  /* It is protected. */
#define CLASS_IS_PROTECTED_SAV  0x00010000  /* It is protected (saved). */
#define CLASS_IS_INCOMPLETE 0x00020000  /* The specification is incomplete. */
#define CLASS_CAN_CREATE    0x00040000  /* It has usable ctors. */
#define CLASS_IS_EXTERNAL   0x00080000  /* It is external. */
#define CLASS_IS_DELAYED_DTOR   0x00100000  /* The dtor is delayed. */
#define CLASS_NO_DEFAULT_CTORS  0x00200000  /* Don't create default ctors. */
#define CLASS_QOBJECT_SUB   0x00400000  /* It is derived from QObject. */
#define CLASS_DTOR_HOLD_GIL 0x00800000  /* The dtor holds the GIL. */
#define CLASS_ASSIGN_HELPER 0x01000000  /* Generate an assignment helper. */
#define CLASS_NO_QMETAOBJECT    0x02000000  /* It has no QMetaObject. */
#define CLASS_IS_TEMPLATE   0x04000000  /* It is a template class. */
#define CLASS_IS_DEPRECATED 0x08000000  /* It is deprecated. */
#define CLASS_CANNOT_COPY   0x10000000  /* It cannot be copied. */
#define CLASS_CANNOT_ASSIGN 0x20000000  /* It cannot be assigned. */
#define CLASS_ALLOW_NONE    0x40000000  /* The class will handle None. */
#define CLASS_HAS_NONLAZY   0x80000000  /* The class has non-lazy methods. */

#define hasSigSlots(cd)     ((cd)->classflags & CLASS_HAS_SIGSLOTS)
#define setHasSigSlots(cd)  ((cd)->classflags |= CLASS_HAS_SIGSLOTS)
#define isAbstractClass(cd) ((cd)->classflags & CLASS_IS_ABSTRACT)
#define setIsAbstractClass(cd)  ((cd)->classflags |= CLASS_IS_ABSTRACT)
#define hasShadow(cd)       ((cd)->classflags & CLASS_HAS_SHADOW)
#define setHasShadow(cd)    ((cd)->classflags |= CLASS_HAS_SHADOW)
#define resetHasShadow(cd)  ((cd)->classflags &= ~CLASS_HAS_SHADOW)
#define isOpaque(cd)        ((cd)->classflags & CLASS_IS_OPAQUE)
#define setIsOpaque(cd)     ((cd)->classflags |= CLASS_IS_OPAQUE)
#define hasVarHandlers(cd)  ((cd)->classflags & CLASS_HAS_VAR_HANDLERS)
#define setHasVarHandlers(cd)   ((cd)->classflags |= CLASS_HAS_VAR_HANDLERS)
#define isProtectedClass(cd)    ((cd)->classflags & CLASS_IS_PROTECTED)
#define setIsProtectedClass(cd) ((cd)->classflags |= CLASS_IS_PROTECTED)
#define resetIsProtectedClass(cd)   ((cd)->classflags &= ~CLASS_IS_PROTECTED)
#define wasProtectedClass(cd)   ((cd)->classflags & CLASS_IS_PROTECTED_SAV)
#define setWasProtectedClass(cd)    ((cd)->classflags |= CLASS_IS_PROTECTED_SAV)
#define resetWasProtectedClass(cd)  ((cd)->classflags &= ~CLASS_IS_PROTECTED_SAV)
#define isReleaseGILDtor(cd)    ((cd)->classflags & CLASS_DTOR_RELEASE_GIL)
#define setIsReleaseGILDtor(cd) ((cd)->classflags |= CLASS_DTOR_RELEASE_GIL)
#define isIncomplete(cd)    ((cd)->classflags & CLASS_IS_INCOMPLETE)
#define setIsIncomplete(cd) ((cd)->classflags |= CLASS_IS_INCOMPLETE)
#define canCreate(cd)       ((cd)->classflags & CLASS_CAN_CREATE)
#define setCanCreate(cd)    ((cd)->classflags |= CLASS_CAN_CREATE)
#define resetCanCreate(cd)  ((cd)->classflags &= ~CLASS_CAN_CREATE)
#define isExternal(cd)      ((cd)->classflags & CLASS_IS_EXTERNAL)
#define setIsExternal(cd)   ((cd)->classflags |= CLASS_IS_EXTERNAL)
#define isDelayedDtor(cd)   ((cd)->classflags & CLASS_IS_DELAYED_DTOR)
#define setIsDelayedDtor(cd)    ((cd)->classflags |= CLASS_IS_DELAYED_DTOR)
#define noDefaultCtors(cd)  ((cd)->classflags & CLASS_NO_DEFAULT_CTORS)
#define setNoDefaultCtors(cd)   ((cd)->classflags |= CLASS_NO_DEFAULT_CTORS)
#define isQObjectSubClass(cd)   ((cd)->classflags & CLASS_QOBJECT_SUB)
#define setIsQObjectSubClass(cd)    ((cd)->classflags |= CLASS_QOBJECT_SUB)
#define isHoldGILDtor(cd)   ((cd)->classflags & CLASS_DTOR_HOLD_GIL)
#define setIsHoldGILDtor(cd) ((cd)->classflags |= CLASS_DTOR_HOLD_GIL)
#define assignmentHelper(cd) ((cd)->classflags & CLASS_ASSIGN_HELPER)
#define setAssignmentHelper(cd) ((cd)->classflags |= CLASS_ASSIGN_HELPER)
#define noPyQt4QMetaObject(cd)  ((cd)->classflags & CLASS_NO_QMETAOBJECT)
#define setPyQt4NoQMetaObject(cd)   ((cd)->classflags |= CLASS_NO_QMETAOBJECT)
#define isTemplateClass(cd) ((cd)->classflags & CLASS_IS_TEMPLATE)
#define setIsTemplateClass(cd)  ((cd)->classflags |= CLASS_IS_TEMPLATE)
#define resetIsTemplateClass(cd)    ((cd)->classflags &= ~CLASS_IS_TEMPLATE)
#define isDeprecatedClass(cd)   ((cd)->classflags & CLASS_IS_DEPRECATED)
#define setIsDeprecatedClass(cd)    ((cd)->classflags |= CLASS_IS_DEPRECATED)
#define cannotCopy(cd)      ((cd)->classflags & CLASS_CANNOT_COPY)
#define setCannotCopy(cd)   ((cd)->classflags |= CLASS_CANNOT_COPY)
#define cannotAssign(cd)    ((cd)->classflags & CLASS_CANNOT_ASSIGN)
#define setCannotAssign(cd) ((cd)->classflags |= CLASS_CANNOT_ASSIGN)
#define classHandlesNone(cd)    ((cd)->classflags & CLASS_ALLOW_NONE)
#define setClassHandlesNone(cd) ((cd)->classflags |= CLASS_ALLOW_NONE)
#define hasNonlazyMethod(cd)    ((cd)->classflags & CLASS_HAS_NONLAZY)
#define setHasNonlazyMethod(cd) ((cd)->classflags |= CLASS_HAS_NONLAZY)

#define isPublicDtor(cd)    ((cd)->classflags & SECT_IS_PUBLIC)
#define setIsPublicDtor(cd) ((cd)->classflags |= SECT_IS_PUBLIC)
#define isProtectedDtor(cd) ((cd)->classflags & SECT_IS_PROT)
#define isPrivateDtor(cd)   ((cd)->classflags & SECT_IS_PRIVATE)

#define isDtor(cd)          ((cd)->classflags & (SECT_IS_PUBLIC | SECT_IS_PROT | SECT_IS_PRIVATE))


/* Handle the second group of class flags. */
#define CLASS2_TMPL_ARG     0x01        /* The class is a template argument. */
#define CLASS2_MIXIN        0x02        /* The class is a mixin. */
#define CLASS2_EXPORT_DERIVED   0x04    /* Export the derived class declaration. */

#define isTemplateArg(cd)   ((cd)->classflags2 & CLASS2_TMPL_ARG)
#define setTemplateArg(cd)  ((cd)->classflags2 |= CLASS2_TMPL_ARG)
#define isMixin(cd)         ((cd)->classflags2 & CLASS2_MIXIN)
#define setMixin(cd)        ((cd)->classflags2 |= CLASS2_MIXIN)
#define isExportDerived(cd) ((cd)->classflags2 & CLASS2_EXPORT_DERIVED)
#define setExportDerived(cd)    ((cd)->classflags2 |= CLASS2_EXPORT_DERIVED)


/* Handle ctor flags.  These are combined with the section flags. */

#define CTOR_RELEASE_GIL    0x00000100  /* The ctor releases the GIL. */
#define CTOR_EXPLICIT       0x00000200  /* The ctor is explicit. */
#define CTOR_CAST           0x00000400  /* The ctor is a cast. */
#define CTOR_HOLD_GIL       0x00000800  /* The ctor holds the GIL. */
#define CTOR_XFERRED        0x00001000  /* Ownership is transferred. */
#define CTOR_IS_DEPRECATED  0x00002000  /* The ctor is deprecated. */
#define CTOR_RAISES_PY_EXC  0x00004000  /* It raises a Python exception. */

#define isPublicCtor(c)     ((c)->ctorflags & SECT_IS_PUBLIC)
#define setIsPublicCtor(c)  ((c)->ctorflags |= SECT_IS_PUBLIC)
#define isProtectedCtor(c)  ((c)->ctorflags & SECT_IS_PROT)
#define setIsProtectedCtor(c)   ((c)->ctorflags |= SECT_IS_PROT)
#define isPrivateCtor(c)    ((c)->ctorflags & SECT_IS_PRIVATE)
#define setIsPrivateCtor(c) ((c)->ctorflags |= SECT_IS_PRIVATE)

#define isReleaseGILCtor(c) ((c)->ctorflags & CTOR_RELEASE_GIL)
#define setIsReleaseGILCtor(c)  ((c)->ctorflags |= CTOR_RELEASE_GIL)
#define isExplicitCtor(c)   ((c)->ctorflags & CTOR_EXPLICIT)
#define setIsExplicitCtor(c)    ((c)->ctorflags |= CTOR_EXPLICIT)
#define isCastCtor(c)       ((c)->ctorflags & CTOR_CAST)
#define isHoldGILCtor(c)    ((c)->ctorflags & CTOR_HOLD_GIL)
#define setIsHoldGILCtor(c) ((c)->ctorflags |= CTOR_HOLD_GIL)
#define isResultTransferredCtor(c)  ((c)->ctorflags & CTOR_XFERRED)
#define setIsResultTransferredCtor(c)   ((c)->ctorflags |= CTOR_XFERRED)
#define isDeprecatedCtor(c) ((c)->ctorflags & CTOR_IS_DEPRECATED)
#define setIsDeprecatedCtor(c)  ((c)->ctorflags |= CTOR_IS_DEPRECATED)
#define raisesPyExceptionCtor(c)    ((c)->ctorflags & CTOR_RAISES_PY_EXC)
#define setRaisesPyExceptionCtor(c) ((c)->ctorflags |= CTOR_RAISES_PY_EXC)


/* Handle member flags. */

#define MEMBR_NUMERIC       0x0001      /* It is a numeric slot. */
#define MEMBR_SEQUENCE      0x0002      /* It is a sequnce slot. */
#define MEMBR_NO_ARG_PARSER 0x0004      /* Don't generate an argument parser. */
#define MEMBR_NOT_VERSIONED 0x0008      /* There is an unversioned overload. */
#define MEMBR_KEYWORD_ARGS  0x0010      /* It allows keyword arguments. */
#define MEMBR_HAS_PROTECTED 0x0011      /* It has a protected overload. */

#define isNumeric(m)        ((m)->memberflags & MEMBR_NUMERIC)
#define setIsNumeric(m)     ((m)->memberflags |= MEMBR_NUMERIC)
#define isSequence(m)       ((m)->memberflags & MEMBR_SEQUENCE)
#define setIsSequence(m)    ((m)->memberflags |= MEMBR_SEQUENCE)
#define noArgParser(m)      ((m)->memberflags & MEMBR_NO_ARG_PARSER)
#define setNoArgParser(m)   ((m)->memberflags |= MEMBR_NO_ARG_PARSER)
#define notVersioned(m)     ((m)->memberflags & MEMBR_NOT_VERSIONED)
#define setNotVersioned(m)  ((m)->memberflags |= MEMBR_NOT_VERSIONED)
#define useKeywordArgs(m)   ((m)->memberflags & MEMBR_KEYWORD_ARGS)
#define setUseKeywordArgs(m)    ((m)->memberflags |= MEMBR_KEYWORD_ARGS)
#define hasProtected(m)     ((m)->memberflags & MEMBR_HAS_PROTECTED)
#define setHasProtected(m)  ((m)->memberflags |= MEMBR_HAS_PROTECTED)


/* Handle enum flags.  These are combined with the section flags. */

#define ENUM_WAS_PROT       0x00000100  /* It was defined as protected. */
#define ENUM_NO_SCOPE       0x00000200  /* Omit the member scopes. */

#define isProtectedEnum(e)  ((e)->enumflags & SECT_IS_PROT)
#define setIsProtectedEnum(e)   ((e)->enumflags |= SECT_IS_PROT)
#define resetIsProtectedEnum(e) ((e)->enumflags &= ~SECT_IS_PROT)

#define wasProtectedEnum(e) ((e)->enumflags & ENUM_WAS_PROT)
#define setWasProtectedEnum(e)  ((e)->enumflags |= ENUM_WAS_PROT)
#define resetWasProtectedEnum(e)    ((e)->enumflags &= ~ENUM_WAS_PROT)
#define isNoScope(e)        ((e)->enumflags & ENUM_NO_SCOPE)
#define setIsNoScope(e)     ((e)->enumflags |= ENUM_NO_SCOPE)


/* Handle hierarchy flags. */

#define HIER_IS_DUPLICATE   0x0001      /* It is a super class duplicate. */
#define HIER_HAS_DUPLICATE  0x0002      /* It has a super class duplicate. */
#define HIER_BEING_SET      0x0004      /* The MRO is being set. */

#define isDuplicateSuper(m) ((m)->mroflags & HIER_IS_DUPLICATE)
#define setIsDuplicateSuper(m)  ((m)->mroflags |= HIER_IS_DUPLICATE)
#define hasDuplicateSuper(m)    ((m)->mroflags & HIER_HAS_DUPLICATE)
#define setHasDuplicateSuper(m) ((m)->mroflags |= HIER_HAS_DUPLICATE)
#define hierBeingSet(m)     ((m)->mroflags & HIER_BEING_SET)
#define setHierBeingSet(m)  ((m)->mroflags |= HIER_BEING_SET)
#define resetHierBeingSet(m)    ((m)->mroflags &= ~HIER_BEING_SET)


/* Handle overload flags.  These are combined with the section flags. */

#define OVER_IS_VIRTUAL     0x00000100  /* It is virtual. */
#define OVER_IS_ABSTRACT    0x00000200  /* It is abstract. */
#define OVER_IS_CONST       0x00000400  /* It is a const function. */
#define OVER_IS_STATIC      0x00000800  /* It is a static function. */
#define OVER_IS_AUTOGEN     0x00001000  /* It is auto-generated. */
#define OVER_IS_NEW_THREAD  0x00002000  /* It is in a new thread. */
#define OVER_IS_FACTORY     0x00004000  /* It is a factory method. */
#define OVER_XFERRED_BACK   0x00008000  /* Ownership is transferred back. */
#define OVER_XFERRED        0x00010000  /* Ownership is transferred. */
#define OVER_IS_VIRTUAL_REIMP   0x00020000  /* It is a re-implementation of a virtual. */
#define OVER_DONT_DEREF_SELF    0x00040000  /* For comparison operators, don't dereference self. */
#define OVER_HOLD_GIL       0x00080000  /* The function holds the GIL. */
#define OVER_RELEASE_GIL    0x00100000  /* The function releases the GIL. */
#define OVER_THIS_XFERRED   0x00200000  /* Ownership of this is transferred. */
#define OVER_IS_GLOBAL      0x00400000  /* It is a global operator. */
#define OVER_IS_COMPLEMENTARY   0x00800000  /* It is a complementary operator. */
#define OVER_IS_DEPRECATED  0x01000000  /* It is deprecated. */
#define OVER_REALLY_PROT    0x02000000  /* It really is protected. */
#define OVER_IS_DELATTR     0x04000000  /* It is __delattr__. */
#define OVER_RAISES_PY_EXC  0x08000000  /* It raises a Python exception. */
#define OVER_NO_ERROR_HANDLER   0x10000000  /* It doesn't use a virtual error handler. */

#define isPublic(o)         ((o)->overflags & SECT_IS_PUBLIC)
#define setIsPublic(o)      ((o)->overflags |= SECT_IS_PUBLIC)
#define isProtected(o)      ((o)->overflags & SECT_IS_PROT)
#define setIsProtected(o)   ((o)->overflags |= SECT_IS_PROT)
#define isPrivate(o)        ((o)->overflags & SECT_IS_PRIVATE)
#define setIsPrivate(o)     ((o)->overflags |= SECT_IS_PRIVATE)
#define isSlot(o)           ((o)->overflags & SECT_IS_SLOT)
#define setIsSlot(o)        ((o)->overflags |= SECT_IS_SLOT)
#define resetIsSlot(o)      ((o)->overflags &= ~SECT_IS_SLOT)
#define isSignal(o)         ((o)->overflags & SECT_IS_SIGNAL)
#define setIsSignal(o)      ((o)->overflags |= SECT_IS_SIGNAL)
#define resetIsSignal(o)    ((o)->overflags &= ~SECT_IS_SIGNAL)

#define isVirtual(o)        ((o)->overflags & OVER_IS_VIRTUAL)
#define setIsVirtual(o)     ((o)->overflags |= OVER_IS_VIRTUAL)
#define resetIsVirtual(o)   ((o)->overflags &= ~OVER_IS_VIRTUAL)
#define isAbstract(o)       ((o)->overflags & OVER_IS_ABSTRACT)
#define setIsAbstract(o)    ((o)->overflags |= OVER_IS_ABSTRACT)
#define isConst(o)          ((o)->overflags & OVER_IS_CONST)
#define setIsConst(o)       ((o)->overflags |= OVER_IS_CONST)
#define isStatic(o)         ((o)->overflags & OVER_IS_STATIC)
#define setIsStatic(o)      ((o)->overflags |= OVER_IS_STATIC)
#define isAutoGen(o)        ((o)->overflags & OVER_IS_AUTOGEN)
#define setIsAutoGen(o)     ((o)->overflags |= OVER_IS_AUTOGEN)
#define resetIsAutoGen(o)   ((o)->overflags &= ~OVER_IS_AUTOGEN)
#define isNewThread(o)      ((o)->overflags & OVER_IS_NEW_THREAD)
#define setIsNewThread(o)   ((o)->overflags |= OVER_IS_NEW_THREAD)
#define isFactory(o)        ((o)->overflags & OVER_IS_FACTORY)
#define setIsFactory(o)     ((o)->overflags |= OVER_IS_FACTORY)
#define isResultTransferredBack(o)  ((o)->overflags & OVER_XFERRED_BACK)
#define setIsResultTransferredBack(o)   ((o)->overflags |= OVER_XFERRED_BACK)
#define isResultTransferred(o)  ((o)->overflags & OVER_XFERRED)
#define setIsResultTransferred(o)   ((o)->overflags |= OVER_XFERRED)
#define isVirtualReimp(o)   ((o)->overflags & OVER_IS_VIRTUAL_REIMP)
#define setIsVirtualReimp(o)    ((o)->overflags |= OVER_IS_VIRTUAL_REIMP)
#define dontDerefSelf(o)    ((o)->overflags & OVER_DONT_DEREF_SELF)
#define setDontDerefSelf(o) ((o)->overflags |= OVER_DONT_DEREF_SELF)
#define isHoldGIL(o)        ((o)->overflags & OVER_HOLD_GIL)
#define setIsHoldGIL(o)     ((o)->overflags |= OVER_HOLD_GIL)
#define isReleaseGIL(o)     ((o)->overflags & OVER_RELEASE_GIL)
#define setIsReleaseGIL(o)  ((o)->overflags |= OVER_RELEASE_GIL)
#define isThisTransferredMeth(o)    ((o)->overflags & OVER_THIS_XFERRED)
#define setIsThisTransferredMeth(o) ((o)->overflags |= OVER_THIS_XFERRED)
#define isGlobal(o)         ((o)->overflags & OVER_IS_GLOBAL)
#define setIsGlobal(o)      ((o)->overflags |= OVER_IS_GLOBAL)
#define isComplementary(o)  ((o)->overflags & OVER_IS_COMPLEMENTARY)
#define setIsComplementary(o)   ((o)->overflags |= OVER_IS_COMPLEMENTARY)
#define isDeprecated(o)     ((o)->overflags & OVER_IS_DEPRECATED)
#define setIsDeprecated(o)  ((o)->overflags |= OVER_IS_DEPRECATED)
#define isReallyProtected(o)    ((o)->overflags & OVER_REALLY_PROT)
#define setIsReallyProtected(o) ((o)->overflags |= OVER_REALLY_PROT)
#define isDelattr(o)        ((o)->overflags & OVER_IS_DELATTR)
#define setIsDelattr(o)     ((o)->overflags |= OVER_IS_DELATTR)
#define raisesPyException(o)    ((o)->overflags & OVER_RAISES_PY_EXC)
#define setRaisesPyException(o) ((o)->overflags |= OVER_RAISES_PY_EXC)
#define noErrorHandler(o)   ((o)->overflags & OVER_NO_ERROR_HANDLER)
#define setNoErrorHandler(o)    ((o)->overflags |= OVER_NO_ERROR_HANDLER)


/* Handle variable flags. */

#define VAR_IS_STATIC       0x01    /* It is a static variable. */
#define VAR_NEEDS_HANDLER   0x02    /* It the variable needs a handler. */

#define isStaticVar(v)      ((v)->varflags & VAR_IS_STATIC)
#define setIsStaticVar(v)   ((v)->varflags |= VAR_IS_STATIC)
#define needsHandler(v)     ((v)->varflags & VAR_NEEDS_HANDLER)
#define setNeedsHandler(v)  ((v)->varflags |= VAR_NEEDS_HANDLER)


/* Handle argument flags. */

#define ARG_IS_REF          0x0001  /* It is a reference. */
#define ARG_IS_CONST        0x0002  /* It is a const. */
#define ARG_XFERRED         0x0004  /* Ownership is transferred. */
#define ARG_THIS_XFERRED    0x0008  /* Ownership of this is transferred. */
#define ARG_XFERRED_BACK    0x0010  /* Ownership is transferred back. */
#define ARG_ARRAY           0x0020  /* Used as an array. */
#define ARG_ARRAY_SIZE      0x0040  /* Used as an array size. */
#define ARG_ALLOW_NONE      0x0080  /* Allow None as a value. */
#define ARG_GET_WRAPPER     0x0100  /* Get the wrapper object. */
#define ARG_IN              0x0200  /* It passes an argument. */
#define ARG_OUT             0x0400  /* It returns a result. */
#define ARG_CONSTRAINED     0x0800  /* Suppress type conversion. */
#define ARG_SINGLE_SHOT     0x1000  /* The slot is only ever fired once. */
#define ARG_RESULT_SIZE     0x2000  /* It defines the result size. */
#define ARG_KEEP_REF        0x4000  /* Keep a reference. */
#define ARG_NO_COPY         0x8000  /* Disable copying of const references. */

#define isReference(a)      ((a)->argflags & ARG_IS_REF)
#define setIsReference(a)   ((a)->argflags |= ARG_IS_REF)
#define resetIsReference(a) ((a)->argflags &= ~ARG_IS_REF)
#define isConstArg(a)       ((a)->argflags & ARG_IS_CONST)
#define setIsConstArg(a)    ((a)->argflags |= ARG_IS_CONST)
#define resetIsConstArg(a)  ((a)->argflags &= ~ARG_IS_CONST)
#define isTransferred(a)    ((a)->argflags & ARG_XFERRED)
#define setIsTransferred(a) ((a)->argflags |= ARG_XFERRED)
#define isThisTransferred(a)    ((a)->argflags & ARG_THIS_XFERRED)
#define setIsThisTransferred(a) ((a)->argflags |= ARG_THIS_XFERRED)
#define isTransferredBack(a)    ((a)->argflags & ARG_XFERRED_BACK)
#define setIsTransferredBack(a) ((a)->argflags |= ARG_XFERRED_BACK)
#define isArray(a)          ((a)->argflags & ARG_ARRAY)
#define setArray(a)         ((a)->argflags |= ARG_ARRAY)
#define isArraySize(a)      ((a)->argflags & ARG_ARRAY_SIZE)
#define setArraySize(a)     ((a)->argflags |= ARG_ARRAY_SIZE)
#define isAllowNone(a)      ((a)->argflags & ARG_ALLOW_NONE)
#define setAllowNone(a)     ((a)->argflags |= ARG_ALLOW_NONE)
#define isGetWrapper(a)     ((a)->argflags & ARG_GET_WRAPPER)
#define setGetWrapper(a)    ((a)->argflags |= ARG_GET_WRAPPER)
#define isInArg(a)          ((a)->argflags & ARG_IN)
#define setIsInArg(a)       ((a)->argflags |= ARG_IN)
#define isOutArg(a)         ((a)->argflags & ARG_OUT)
#define setIsOutArg(a)      ((a)->argflags |= ARG_OUT)
#define isConstrained(a)    ((a)->argflags & ARG_CONSTRAINED)
#define setIsConstrained(a) ((a)->argflags |= ARG_CONSTRAINED)
#define resetIsConstrained(a)   ((a)->argflags &= ~ARG_CONSTRAINED)
#define isSingleShot(a)     ((a)->argflags & ARG_SINGLE_SHOT)
#define isResultSize(a)     ((a)->argflags & ARG_RESULT_SIZE)
#define setResultSize(a)    ((a)->argflags |= ARG_RESULT_SIZE)
#define keepReference(a)    ((a)->argflags & ARG_KEEP_REF)
#define setKeepReference(a) ((a)->argflags |= ARG_KEEP_REF)
#define noCopy(a)           ((a)->argflags & ARG_NO_COPY)
#define setNoCopy(a)        ((a)->argflags |= ARG_NO_COPY)


/* Handle name flags. */

#define NAME_IS_USED        0x01    /* It is used in the main module. */
#define NAME_IS_SUBSTR      0x02    /* It is a substring of another. */

#define isUsedName(n)       ((n)->nameflags & NAME_IS_USED)
#define setIsUsedName(n)    ((n)->nameflags |= NAME_IS_USED)
#define resetIsUsedName(n)  ((n)->nameflags &= ~NAME_IS_USED)
#define isSubstring(n)      ((n)->nameflags & NAME_IS_SUBSTR)
#define setIsSubstring(n)   ((n)->nameflags |= NAME_IS_SUBSTR)


/* Handle virtual handler flags. */

#define VH_IS_DUPLICATE     0x01    /* It is a duplicate. */
#define VH_TRANSFERS        0x02    /* It transfers ownership of the result. */

#define isDuplicateVH(vh)   ((vh)->vhflags & VH_IS_DUPLICATE)
#define setIsDuplicateVH(vh)    ((vh)->vhflags |= VH_IS_DUPLICATE)
#define resetIsDuplicateVH(vh)  ((vh)->vhflags &= ~VH_IS_DUPLICATE)
#define isTransferVH(vh)    ((vh)->vhflags & VH_TRANSFERS)
#define setIsTransferVH(vh) ((vh)->vhflags |= VH_TRANSFERS)


/* Handle mapped type flags. */

#define MT_NO_RELEASE       0x01    /* Do not generate a release function. */
#define MT_ALLOW_NONE       0x02    /* The mapped type will handle None. */

#define noRelease(mt)       ((mt)->mtflags & MT_NO_RELEASE)
#define setNoRelease(mt)    ((mt)->mtflags |= MT_NO_RELEASE)
#define handlesNone(mt)     ((mt)->mtflags & MT_ALLOW_NONE)
#define setHandlesNone(mt)  ((mt)->mtflags |= MT_ALLOW_NONE)


/* Handle typedef flags. */

#define TD_NO_TYPE_NAME     0x01    /* Do not use the typedef name. */

#define noTypeName(td)      ((td)->tdflags & TD_NO_TYPE_NAME)
#define setNoTypeName(td)   ((td)->tdflags |= TD_NO_TYPE_NAME)


/* Warning categories. */
typedef enum {
    ParserWarning,
    DeprecationWarning
} Warning;


/* Docstring formatting. */
typedef enum {
    raw,
    deindented
} Format;


/* Levels of keyword argument support. */
typedef enum {
    NoKwArgs = 0,
    AllKwArgs,
    OptionalKwArgs
} KwArgs;


/* Slot types. */
typedef enum {
    str_slot,
    unicode_slot,
    int_slot,
    long_slot,
    float_slot,
    len_slot,
    contains_slot,
    add_slot,
    concat_slot,
    sub_slot,
    mul_slot,
    repeat_slot,
    div_slot,
    mod_slot,
    floordiv_slot,
    truediv_slot,
    and_slot,
    or_slot,
    xor_slot,
    lshift_slot,
    rshift_slot,
    iadd_slot,
    iconcat_slot,
    isub_slot,
    imul_slot,
    irepeat_slot,
    idiv_slot,
    imod_slot,
    ifloordiv_slot,
    itruediv_slot,
    iand_slot,
    ior_slot,
    ixor_slot,
    ilshift_slot,
    irshift_slot,
    invert_slot,
    call_slot,
    getitem_slot,
    setitem_slot,
    delitem_slot,
    lt_slot,
    le_slot,
    eq_slot,
    ne_slot,
    gt_slot,
    ge_slot,
    cmp_slot,
    bool_slot,
    neg_slot,
    pos_slot,
    abs_slot,
    repr_slot,
    hash_slot,
    index_slot,
    iter_slot,
    next_slot,
    setattr_slot,
    delattr_slot,       /* This is local to the parser. */
    no_slot
} slotType;


/*
 * Argument types.  Always add new ones at the end because the numeric values
 * can appear in generated code.
 */
typedef enum {
    no_type,
    defined_type,
    class_type,
    struct_type,
    void_type,
    enum_type,
    template_type,
    signal_type,
    slot_type,
    rxcon_type,
    rxdis_type,
    slotcon_type,
    slotdis_type,
    ustring_type,
    string_type,
    short_type,
    ushort_type,
    cint_type,
    int_type,
    uint_type,
    long_type,
    ulong_type,
    float_type,
    cfloat_type,
    double_type,
    cdouble_type,
    bool_type,
    mapped_type,
    pyobject_type,
    pytuple_type,
    pylist_type,
    pydict_type,
    pycallable_type,
    pyslice_type,
    qobject_type,
    function_type,
    pytype_type,
    ellipsis_type,
    longlong_type,
    ulonglong_type,
    anyslot_type,
    cbool_type,
    sstring_type,
    wstring_type,
    fake_void_type,
    ssize_type,
    ascii_string_type,
    latin1_string_type,
    utf8_string_type,
    byte_type,
    sbyte_type,
    ubyte_type,
    capsule_type,
    pybuffer_type
} argType;


/* Value types. */

typedef enum {
    qchar_value,
    string_value,
    numeric_value,
    real_value,
    scoped_value,
    fcall_value
} valueType;


/* Version types. */

typedef enum {
    time_qualifier,
    platform_qualifier,
    feature_qualifier
} qualType;


/* Interface file types. */

typedef enum {
    exception_iface,
    mappedtype_iface,
    namespace_iface,
    class_iface
} ifaceFileType;


/* A location in a .sip source file. */
typedef struct {
    int linenr;                         /* The line number. */
    const char *name;                   /* The filename. */
} sourceLocation;


/* A software license. */

typedef struct {
    const char *type;                   /* The license type. */
    const char *licensee;               /* The licensee. */
    const char *timestamp;              /* The timestamp. */
    const char *sig;                    /* The signature. */
} licenseDef;


/* A version qualifier. */

typedef struct _qualDef {
    const char *name;                   /* The qualifier name. */
    qualType qtype;                     /* The qualifier type. */
    struct _moduleDef *module;          /* The defining module. */
    int line;                           /* Timeline if it is a time. */
    int order;                          /* Order if it is a time. */
    struct _qualDef *next;              /* Next in the list. */
} qualDef;


/* A scoped name. */

typedef struct _scopedNameDef {
    char *name;                         /* The name. */
    struct _scopedNameDef *next;        /* Next in the scope list. */
} scopedNameDef;


/* A name. */

typedef struct _nameDef {
    int nameflags;                      /* The name flags. */
    const char *text;                   /* The text of the name. */
    size_t len;                         /* The length of the name. */
    size_t offset;                      /* The offset in the string pool. */
    struct _nameDef *next;              /* Next in the list. */
} nameDef;


/* A literal code block. */

typedef struct _codeBlock {
    char *frag;                         /* The code itself. */
    const char *filename;               /* The original file. */
    int linenr;                         /* The line in the file. */
} codeBlock;


/* A list of literal code blocks. */

typedef struct _codeBlockList {
    codeBlock *block;                   /* The code block. */
    struct _codeBlockList *next;        /* The next in the list. */
} codeBlockList;


/* The arguments to a throw specifier. */

typedef struct _throwArgs {
    int nrArgs;                         /* The number of arguments. */
    struct _exceptionDef *args[MAX_NR_ARGS];    /* The arguments. */
} throwArgs;


/* An exception. */

typedef struct _exceptionDef {
    int exceptionnr;                    /* The exception number. */
    struct _ifaceFileDef *iff;          /* The interface file. */
    const char *pyname;                 /* The exception Python name. */
    struct _classDef *cd;               /* The exception class. */
    char *bibase;                       /* The builtin base exception. */
    struct _exceptionDef *base;         /* The defined base exception. */
    codeBlockList *raisecode;           /* Raise exception code. */
    struct _exceptionDef *next;         /* The next in the list. */
} exceptionDef;


/* A value. */

typedef struct _valueDef {
    valueType vtype;                    /* The type. */
    char vunop;                         /* Any unary operator. */
    char vbinop;                        /* Any binary operator. */
    scopedNameDef *cast;                /* Any cast. */
    union {
        char vqchar;                    /* Quoted character value. */
        long vnum;                      /* Numeric value. */
        double vreal;                   /* Real value. */
        char *vstr;                     /* String value. */
        scopedNameDef *vscp;            /* Scoped value. */
        struct _fcallDef *fcd;          /* Function call. */
    } u;
    struct _valueDef *next;             /* Next in the expression. */
} valueDef;


/* A member function argument (or result). */

typedef struct {
    argType atype;                      /* The type. */
    nameDef *name;                      /* The name. */
    const char *doctype;                /* The documented type. */
    int argflags;                       /* The argument flags. */
    int nrderefs;                       /* Nr. of dereferences. */
    int derefs[MAX_NR_DEREFS];          /* The const for each dereference. */
    valueDef *defval;                   /* The default value. */
    const char *docval;                 /* The documented value. */
    int key;                            /* The optional /KeepReference/ key. */
    struct _typedefDef *original_type;  /* The original type if typedef'd. */
    union {
        struct _signatureDef *sa;       /* If it is a function. */
        struct _templateDef *td;        /* If it is a template. */
        struct _scopedNameDef *snd;     /* If it is a defined type. */
        struct _classDef *cd;           /* If it is a class. */
        struct _enumDef *ed;            /* If it is an enum. */
        struct _scopedNameDef *sname;   /* If it is a struct. */
        struct _mappedTypeDef *mtd;     /* If it is a mapped type. */
        struct _scopedNameDef *cap;     /* If it is a capsule. */
    } u;
} argDef;


/* An entry in a linked argument list. */
typedef struct _argList {
    argDef arg;                         /* The argument itself. */
    struct _argList *next;              /* Next in the list. */
} argList;


/* A function call. */
typedef struct _fcallDef {
    argDef type;                        /* The type. */
    int nrArgs;                         /* The number of arguments. */
    struct _valueDef *args[MAX_NR_ARGS];    /* The arguments. */
} fcallDef;


/* An API version range definition. */
typedef struct _apiVersionRangeDef {
    nameDef *api_name;                  /* The API name. */
    int from;                           /* The lower bound. */
    int to;                             /* The upper bound. */
    int index;                          /* The range index. */
    struct _apiVersionRangeDef *next;   /* The next in the list. */
} apiVersionRangeDef;


/* A virtual error handler. */
typedef struct _virtErrorHandler {
    const char *name;                   /* The name of the handler. */
    codeBlockList *code;                /* The handler code. */
    struct _moduleDef *mod;             /* The defining module. */
    int index;                          /* The index within the module. */
    struct _virtErrorHandler *next;     /* The next in the list. */
} virtErrorHandler;


/* A module definition. */
typedef struct _moduleDef {
    nameDef *fullname;                  /* The full module name. */
    const char *name;                   /* The module base name. */
    int version;                        /* The module version. */
    apiVersionRangeDef *api_versions;   /* The defined APIs. */
    apiVersionRangeDef *api_ranges;     /* The list of API version ranges. */
    int modflags;                       /* The module flags. */
    KwArgs kwargs;                      /* The styleof keyword argument support. */
    int qobjclass;                      /* QObject class, -1 if none. */
    struct _memberDef *othfuncs;        /* List of other functions. */
    struct _overDef *overs;             /* Global overloads. */
    Format defdocstring;                /* The default docstring format. */
    argType encoding;                   /* The default string encoding. */
    nameDef *defmetatype;               /* The optional default meta-type. */
    nameDef *defsupertype;              /* The optional default super-type. */
    struct _exceptionDef *defexception; /* The default exception. */
    codeBlockList *hdrcode;             /* Header code. */
    codeBlockList *cppcode;             /* Global C++ code. */
    codeBlockList *copying;             /* Software license. */
    codeBlockList *preinitcode;         /* Pre-initialisation code. */
    codeBlockList *initcode;            /* Initialisation code. */
    codeBlockList *postinitcode;        /* Post-initialisation code. */
    codeBlockList *unitcode;            /* Compilation unit code. */
    codeBlockList *unitpostinccode;     /* Compilation unit post-include code. */
    codeBlockList *docstring;           /* The docstring. */
    const char *virt_error_handler;     /* The virtual error handler. */
    int parts;                          /* The number of parts generated. */
    const char *file;                   /* The filename. */
    qualDef *qualifiers;                /* The list of qualifiers. */
    argDef *types;                      /* The array of numbered types. */
    int nrtypes;                        /* The number of numbered types. */
    int nrtimelines;                    /* The nr. of timelines. */
    int nrexceptions;                   /* The nr. of exceptions. */
    int nrtypedefs;                     /* The nr. of typedefs. */
    int nrvirthandlers;                 /* The nr. of virtual handlers. */
    int nrvirterrorhandlers;            /* The nr. of virtual error handlers. */
    int next_key;                       /* The next key to allocate. */
    struct _virtHandlerDef *virthandlers;   /* The virtual handlers. */
    licenseDef *license;                /* The software license. */
    struct _classDef *proxies;          /* The list of proxy classes. */
    struct _moduleDef *container;       /* The container module, if any. */
    struct _ifaceFileList *used;        /* Interface files used. */
    struct _moduleListDef *allimports;  /* The list of all imports. */
    struct _moduleListDef *imports;     /* The list of direct imports. */
    struct _autoPyNameDef *autopyname;  /* The Python naming rules. */
    struct _moduleDef *next;            /* Next in the list. */
} moduleDef;


/* An entry in a linked module list. */
typedef struct _moduleListDef {
    moduleDef *module;                  /* The module itself. */
    struct _moduleListDef *next;        /* The next in the list. */
} moduleListDef;


/* An interface file definition. */

typedef struct _ifaceFileDef {
    nameDef *name;                      /* The name. */
    apiVersionRangeDef *api_range;      /* The optional API version range. */
    struct _ifaceFileDef *first_alt;    /* The first alternate API. */
    struct _ifaceFileDef *next_alt;     /* The next alternate API. */
    ifaceFileType type;                 /* Interface file type. */
    int ifacenr;                        /* The index into the types table. */
    scopedNameDef *fqcname;             /* The fully qualified C++ name. */
    moduleDef *module;                  /* The owning module. */
    codeBlockList *hdrcode;             /* Header code. */
    struct _ifaceFileList *used;        /* Interface files used. */
    struct _ifaceFileDef *next;         /* Next in the list. */
} ifaceFileDef;


/* An entry in a linked interface file list. */

typedef struct _ifaceFileList {
    ifaceFileDef *iff;                  /* The interface file itself. */
    struct _ifaceFileList *next;        /* Next in the list. */
} ifaceFileList;


/* A mapped type. */

typedef struct _mappedTypeDef {
    int mtflags;                        /* The mapped type flags. */
    argDef type;                        /* The type being mapped. */
    nameDef *pyname;                    /* The Python name. */
    nameDef *cname;                     /* The C/C++ name. */
    const char *doctype;                /* The documented type. */
    ifaceFileDef *iff;                  /* The interface file. */
    struct _memberDef *members;         /* The static member functions. */
    struct _overDef *overs;             /* The static overloads. */
    codeBlockList *instancecode;        /* Create instance code. */
    codeBlockList *typecode;            /* Type code. */
    codeBlockList *convfromcode;        /* Convert from C++ code. */
    codeBlockList *convtocode;          /* Convert to C++ code. */
    struct _mappedTypeDef *next;        /* Next in the list. */
} mappedTypeDef;


/* A function signature. */

typedef struct _signatureDef {
    argDef result;                      /* The result. */
    int nrArgs;                         /* The number of arguments. */
    argDef args[MAX_NR_ARGS];           /* The arguments. */
} signatureDef;


/* A list of function signatures. */

typedef struct _signatureList {
    struct _signatureDef *sd;           /* The signature. */
    struct _signatureList *next;        /* Next in the list. */
} signatureList;


/* A template type. */

typedef struct _templateDef {
    scopedNameDef *fqname;              /* The name. */
    signatureDef types;                 /* The types. */
} templateDef;


/* A list of virtual handlers. */

typedef struct _virtHandlerDef {
    int virthandlernr;                  /* The nr. of the virtual handler. */
    int vhflags;                        /* The virtual handler flags. */
    signatureDef *pysig;                /* The Python signature. */
    signatureDef *cppsig;               /* The C++ signature. */
    struct _moduleDef *module;          /* The defining module. */
    codeBlockList *virtcode;            /* Virtual handler code. */
    struct _virtHandlerDef *next;       /* Next in the list. */
} virtHandlerDef;


/* A typedef definition. */

typedef struct _typedefDef {
    int tdflags;                        /* The typedef flags. */
    scopedNameDef *fqname;              /* The fully qualified name. */
    struct _classDef *ecd;              /* The enclosing class. */
    moduleDef *module;                  /* The owning module. */
    argDef type;                        /* The actual type. */
    struct _typedefDef *next;           /* Next in the list. */
} typedefDef;


/* A variable definition. */

typedef struct _varDef {
    nameDef *pyname;                    /* The variable Python name. */
    scopedNameDef *fqcname;             /* The fully qualified C/C++ name. */
    struct _classDef *ecd;              /* The enclosing class. */
    moduleDef *module;                  /* The owning module. */
    int varflags;                       /* The variable flags. */
    argDef type;                        /* The actual type. */
    codeBlockList *accessfunc;          /* The access function. */
    codeBlockList *getcode;             /* The get code. */
    codeBlockList *setcode;             /* The set code. */
    struct _varDef *next;               /* Next in the list. */
} varDef;


/* A property definition. */

typedef struct _propertyDef {
    nameDef *name;                      /* The property name. */
    const char *get;                    /* The name of the getter method. */
    const char *set;                    /* The name of the setter method. */
    codeBlockList *docstring;           /* The docstring. */
    struct _propertyDef *next;          /* Next in the list. */
} propertyDef;


/* An overloaded member function definition. */

typedef struct _overDef {
    sourceLocation sloc;                /* The source location. */
    char *cppname;                      /* The C++ name. */
    int overflags;                      /* The overload flags. */
    KwArgs kwargs;                      /* The keyword argument support. */
    struct _memberDef *common;          /* Common parts. */
    apiVersionRangeDef *api_range;      /* The optional API version range. */
    signatureDef pysig;                 /* The Python signature. */
    signatureDef *cppsig;               /* The C++ signature. */
    throwArgs *exceptions;              /* The exceptions. */
    codeBlockList *methodcode;          /* Method code. */
    virtHandlerDef *virthandler;        /* The virtual handler. */
    char *prehook;                      /* The pre-hook name. */
    char *posthook;                     /* The post-hook name. */
    const char *virt_error_handler;     /* The virtual error handler. */
    struct _overDef *next;              /* Next in the list. */
} overDef;


/* An overloaded constructor definition. */

typedef struct _ctorDef {
    int ctorflags;                      /* The ctor flags. */
    KwArgs kwargs;                      /* The keyword argument support. */
    apiVersionRangeDef *api_range;      /* The optional API version range. */
    signatureDef pysig;                 /* The Python signature. */
    signatureDef *cppsig;               /* The C++ signature, NULL if /NoDerived/. */
    throwArgs *exceptions;              /* The exceptions. */
    codeBlockList *methodcode;          /* Method code. */
    char *prehook;                      /* The pre-hook name. */
    char *posthook;                     /* The post-hook name. */
    struct _ctorDef *next;              /* Next in the list. */
} ctorDef;


/* An enumerated type member definition. */

typedef struct _enumMemberDef {
    nameDef *pyname;                    /* The Python name. */
    char *cname;                        /* The C/C++ name. */
    struct _enumDef *ed;                /* The enclosing enum. */
    struct _enumMemberDef *next;        /* Next in the list. */
} enumMemberDef;


/* An enumerated type definition. */

typedef struct _enumDef {
    int enumflags;                      /* The enum flags. */
    nameDef *pyname;                    /* The Python name (may be NULL). */
    scopedNameDef *fqcname;             /* The C/C++ name (may be NULL). */
    nameDef *cname;                     /* The C/C++ name (may be NULL). */
    struct _enumDef *first_alt;         /* The first alternate API. */
    struct _enumDef *next_alt;          /* The next alternate API. */
    int enumnr;                         /* The enum number. */
    int enum_idx;                       /* The enum index within the module. */
    struct _classDef *ecd;              /* The enclosing class, if any. */
    struct _mappedTypeDef *emtd;        /* The enclosing mapped type, if any. */
    moduleDef *module;                  /* The owning module. */
    enumMemberDef *members;             /* The list of members. */
    struct _memberDef *slots;           /* The list of slots. */
    struct _overDef *overs;             /* The list of slot overloads. */
    struct _enumDef *next;              /* Next in the list. */
} enumDef;


/* An member function definition. */

typedef struct _memberDef {
    nameDef *pyname;                    /* The Python name. */
    int memberflags;                    /* The member flags. */
    int membernr;                       /* The index in the method table. */
    slotType slot;                      /* The slot type. */
    moduleDef *module;                  /* The owning module. */
    codeBlockList *docstring;           /* The overloads docstrings. */
    struct _memberDef *next;            /* Next in the list. */
} memberDef;


/* A list of visible member functions. */

typedef struct _visibleList {
    memberDef *m;                       /* The member definition. */
    struct _classDef *cd;               /* The class. */
    struct _visibleList *next;          /* Next in the list. */
} visibleList;


/* An entry in a linked class list. */

typedef struct _classList {
    struct _classDef *cd;               /* The class itself. */
    struct _classList *next;            /* Next in the list. */
} classList;


/* A virtual overload definition. */

typedef struct _virtOverDef {
    overDef o;                          /* The overload. */
    struct _virtOverDef *next;          /* Next in the list. */
} virtOverDef;


/* A class that appears in a class's hierarchy. */

typedef struct _mroDef {
    struct _classDef *cd;               /* The class. */
    int mroflags;                       /* The hierarchy flags. */
    struct _mroDef *next;               /* The next in the list. */
} mroDef;


/* A class definition. */

typedef struct _classDef {
    unsigned classflags;                /* The class flags. */
    unsigned classflags2;               /* The class flags, part 2. */
    int pyqt4_flags;                    /* The PyQt4 specific flags. */
    const char *pyqt_interface;         /* The Qt interface name. */
    nameDef *pyname;                    /* The Python name. */
    ifaceFileDef *iff;                  /* The interface file. */
    struct _classDef *ecd;              /* The enclosing scope. */
    struct _classDef *real;             /* The real class if this is a proxy or extender. */
    classList *supers;                  /* The parent classes. */
    mroDef *mro;                        /* The super-class hierarchy. */
    nameDef *metatype;                  /* The meta-type. */
    nameDef *supertype;                 /* The super-type. */
    templateDef *td;                    /* The instantiated template. */
    ctorDef *ctors;                     /* The constructors. */
    ctorDef *defctor;                   /* The default ctor. */
    codeBlockList *dealloccode;         /* Handwritten dealloc code. */
    codeBlockList *dtorcode;            /* Handwritten dtor code. */
    throwArgs *dtorexceptions;          /* The dtor exceptions. */
    memberDef *members;                 /* The member functions. */
    overDef *overs;                     /* The overloads. */
    argList *casts;                     /* The operator casts. */
    virtOverDef *vmembers;              /* The virtual members. */
    visibleList *visible;               /* The visible members. */
    codeBlockList *cppcode;             /* Class C++ code. */
    codeBlockList *convtosubcode;       /* Convert to sub C++ code. */
    struct _classDef *subbase;          /* Sub-class base class. */
    codeBlockList *docstring;           /* Class and ctor docstrings. */
    codeBlockList *instancecode;        /* Create instance code. */
    codeBlockList *convtocode;          /* Convert to C++ code. */
    codeBlockList *convfromcode;        /* Convert from C++ code. */
    codeBlockList *travcode;            /* Traverse code. */
    codeBlockList *clearcode;           /* Clear code. */
    codeBlockList *getbufcode;          /* Get buffer code (Python v3). */
    codeBlockList *releasebufcode;      /* Release buffer code (Python v3). */
    codeBlockList *readbufcode;         /* Read buffer code (Python v2). */
    codeBlockList *writebufcode;        /* Write buffer code (Python v2). */
    codeBlockList *segcountcode;        /* Segment count code (Python v2). */
    codeBlockList *charbufcode;         /* Character buffer code (Python v2). */
    codeBlockList *picklecode;          /* Pickle code. */
    codeBlockList *finalcode;           /* Finalisation code. */
    propertyDef *properties;            /* The properties. */
    const char *virt_error_handler;     /* The virtual error handler. */
    struct _classDef *next;             /* Next in the list. */
} classDef;


/* A class template definition. */

typedef struct _classTmplDef {
    signatureDef sig;                   /* The template arguments. */
    classDef *cd;                       /* The class itself. */
    struct _classTmplDef *next;         /* The next in the list. */
} classTmplDef;


/* A mapped type template definition. */

typedef struct _mappedTypeTmplDef {
    signatureDef sig;                   /* The template arguments. */
    mappedTypeDef *mt;                  /* The mapped type itself. */
    struct _mappedTypeTmplDef *next;    /* The next in the list. */
} mappedTypeTmplDef;


/* The extracts for an identifier. */

typedef struct _extractDef {
    const char *id;                     /* The identifier. */
    struct _extractPartDef *parts;      /* The ordered list of parts. */
    struct _extractDef *next;           /* The next in the list. */
} extractDef;


/* Part of an extract for an identifier. */

typedef struct _extractPartDef {
    int order;                          /* The order of the part. */
    codeBlock *part;                    /* The part itself. */
    struct _extractPartDef *next;       /* The next in the list. */
} extractPartDef;


/* A rule for automatic Python naming. */

typedef struct _autoPyNameDef {
    const char *remove_leading;         /* Leading string to remove. */
    struct _autoPyNameDef *next;        /* The next in the list. */
} autoPyNameDef;


/* The parse tree corresponding to the specification file. */

typedef struct {
    moduleDef *module;                  /* The module being generated. */
    moduleDef *modules;                 /* The list of modules. */
    nameDef *namecache;                 /* The name cache. */
    ifaceFileDef *ifacefiles;           /* The list of interface files. */
    classDef *classes;                  /* The list of classes. */
    classTmplDef *classtemplates;       /* The list of class templates. */
    exceptionDef *exceptions;           /* The list of exceptions. */
    mappedTypeDef *mappedtypes;         /* The mapped types. */
    mappedTypeTmplDef *mappedtypetemplates; /* The list of mapped type templates. */
    enumDef *enums;                     /* List of enums. */
    varDef *vars;                       /* List of variables. */
    typedefDef *typedefs;               /* List of typedefs. */
    virtErrorHandler *errorhandlers;    /* The list of virtual error handlers. */
    codeBlockList *exphdrcode;          /* Exported header code. */
    codeBlockList *docs;                /* Documentation. */
    int sigslots;                       /* Set if signals or slots are used. */
    int genc;                           /* Set if we are generating C code. */
    struct _stringList *plugins;        /* The list of plugins. */
    struct _extractDef *extracts;       /* The list of extracts. */
} sipSpec;


/* A list of strings. */

typedef struct _stringList {
    const char *s;                      /* The string. */
    struct _stringList *next;           /* The next in the list. */
} stringList;


/* File specific context information for the parser. */

typedef struct _parserContext {
    const char *filename;               /* The %Import or %Include filename. */
    int ifdepth;                        /* The depth of nested if's. */
    moduleDef *prevmod;                 /* The previous module. */
} parserContext;


extern char *sipVersion;                /* The version of SIP. */
extern stringList *includeDirList;      /* The include directory list for SIP files. */


void parse(sipSpec *, FILE *, char *, stringList *, stringList *, KwArgs, int);
void parserEOF(const char *,parserContext *);
void transform(sipSpec *);
void generateCode(sipSpec *, char *, char *, char *, const char *, int, int,
        int, int, stringList *needed_qualifiers, stringList *, const char *,
        int, int);
void generateExtracts(sipSpec *pt, const stringList *extracts);
void addExtractPart(sipSpec *pt, const char *id, int order, codeBlock *part);
void generateAPI(sipSpec *pt, moduleDef *mod, const char *apiFile);
void generateXML(sipSpec *pt, moduleDef *mod, const char *xmlFile);
void generateExpression(valueDef *vd, int in_str, FILE *fp);
void warning(Warning w, const char *fmt, ...);
void deprecated(const char *msg);
void fatal(char *,...);
void fatalScopedName(scopedNameDef *);
void getSourceLocation(sourceLocation *slp);
int setInputFile(FILE *open_fp, parserContext *pc, int optional);
void resetLexerState();
void *sipMalloc(size_t n);
void *sipCalloc(size_t nr, size_t n);
char *sipStrdup(const char *);
char *concat(const char *, ...);
void append(char **, const char *);
void addToUsedList(ifaceFileList **, ifaceFileDef *);
int selectedQualifier(stringList *needed_qualifiers, qualDef *qd);
int excludedFeature(stringList *,qualDef *);
int sameSignature(signatureDef *,signatureDef *,int);
int sameTemplateSignature(signatureDef *tmpl_sd, signatureDef *args_sd,
        int deep);
int compareScopedNames(scopedNameDef *snd1, scopedNameDef *snd2);
int sameBaseType(argDef *,argDef *);
char *scopedNameTail(scopedNameDef *);
scopedNameDef *copyScopedName(scopedNameDef *);
void appendScopedName(scopedNameDef **,scopedNameDef *);
void freeScopedName(scopedNameDef *);
void appendToClassList(classList **,classDef *);
void appendCodeBlockList(codeBlockList **headp, codeBlockList *cbl);
void prcode(FILE *fp, const char *fmt, ...);
void prOverloadName(FILE *fp, overDef *od);
void prOverloadDecl(FILE *fp, ifaceFileDef *scope, overDef *od, int defval);
void prScopedPythonName(FILE *fp, classDef *scope, const char *pyname);
int prPythonSignature(sipSpec *pt, FILE *fp, signatureDef *sd, int sec,
        int names, int defaults, int in_str, int is_signal);
void searchTypedefs(sipSpec *pt, scopedNameDef *snd, argDef *ad);
int isZeroArgSlot(memberDef *md);
int isIntReturnSlot(memberDef *md);
int isSSizeReturnSlot(memberDef *md);
int isLongReturnSlot(memberDef *md);
int isVoidReturnSlot(memberDef *md);
int isNumberSlot(memberDef *md);
int isInplaceNumberSlot(memberDef *md);
int isRichCompareSlot(memberDef *md);
mappedTypeDef *allocMappedType(sipSpec *pt, argDef *type);
void appendString(stringList **headp, const char *s);
void appendTypeStrings(scopedNameDef *ename, signatureDef *patt, signatureDef *src, signatureDef *known, scopedNameDef **names, scopedNameDef **values);
codeBlockList *templateCode(sipSpec *pt, ifaceFileList **used,
        codeBlockList *ocbl, scopedNameDef *names, scopedNameDef *values);
ifaceFileDef *findIfaceFile(sipSpec *pt, moduleDef *mod,
        scopedNameDef *fqname, ifaceFileType iftype,
        apiVersionRangeDef *api_range, argDef *ad);
int pluginPyQt3(sipSpec *pt);
int pluginPyQt4(sipSpec *pt);
int pluginPyQt5(sipSpec *pt);
void yywarning(char *);
nameDef *cacheName(sipSpec *pt, const char *name);
scopedNameDef *encodedTemplateName(templateDef *td);
apiVersionRangeDef *findAPI(sipSpec *pt, const char *name);
memberDef *findMethod(classDef *cd, const char *name);


/* These are only here because bison publically references them. */

/* Represent a set of option flags. */

#define MAX_NR_FLAGS    5

typedef enum {
    bool_flag,
    string_flag,
    name_flag,
    opt_name_flag,
    dotted_name_flag,
    integer_flag,
    opt_integer_flag,
    api_range_flag
} flagType;

typedef struct {
    const char *fname;                  /* The flag name. */
    flagType ftype;                     /* The flag type. */
    union {                             /* The flag value. */
        char *sval;                     /* A string value. */
        long ival;                      /* An integer value. */
        apiVersionRangeDef *aval;       /* An API range value. */
    } fvalue;
} optFlag;

typedef struct {
    int nrFlags;                        /* The number of flags. */
    optFlag flags[MAX_NR_FLAGS];        /* Each flag. */
} optFlags;


/* These represent the configuration of different directives. */

/* %API */
typedef struct _apiCfg {
    int token;
    const char *name;
    int version;
} apiCfg;

/* %AutoPyName */
typedef struct _autoPyNameCfg {
    int token;
    const char *remove_leading;
} autoPyNameCfg;

/* %CompositeModule */
typedef struct _compModuleCfg {
    int token;
    const char *name;
    codeBlock *docstring;
} compModuleCfg;

/* %ConsolidatedModule */
typedef struct _consModuleCfg {
    int token;
    const char *name;
    codeBlock *docstring;
} consModuleCfg;

/* %DefaultDocstringFormat */
typedef struct _defDocstringCfg {
    int token;
    const char *name;
} defDocstringCfg;

/* %DefaultEncoding */
typedef struct _defEncodingCfg {
    int token;
    const char *name;
} defEncodingCfg;

/* %DefaultMetatype */
typedef struct _defMetatypeCfg {
    int token;
    const char *name;
} defMetatypeCfg;

/* %DefaultSupertype */
typedef struct _defSupertypeCfg {
    int token;
    const char *name;
} defSupertypeCfg;

/* %Docstring */
typedef struct _docstringCfg {
    int token;
    Format format;
} docstringCfg;

/* %Exception */
typedef struct _exceptionCfg {
    int token;
    codeBlock *type_header_code;
    codeBlock *raise_code;
} exceptionCfg;

/* %Extract */
typedef struct _extractCfg {
    int token;
    const char *id;
    int order;
} extractCfg;

/* %Feature */
typedef struct _featureCfg {
    int token;
    const char *name;
} featureCfg;

/* %Import */
typedef struct _importCfg {
    int token;
    const char *name;
} importCfg;

/* %Include */
typedef struct _includeCfg {
    int token;
    const char *name;
    int optional;
} includeCfg;

/* %License */
typedef struct _licenseCfg {
    int token;
    const char *type;
    const char *licensee;
    const char *signature;
    const char *timestamp;
} licenseCfg;

/* %Module and its sub-directives. */
typedef struct _moduleCfg {
    int token;
    int c_module;
    KwArgs kwargs;
    const char *name;
    int use_arg_names;
    int all_raise_py_exc;
    int call_super_init;
    const char *def_error_handler;
    int version;
    codeBlock *docstring;
} moduleCfg;

/* %Plugin */
typedef struct _pluginCfg {
    int token;
    const char *name;
} pluginCfg;

/* %Property */
typedef struct _propertyCfg {
    int token;
    const char *get;
    const char *name;
    const char *set;
    codeBlock *docstring;
} propertyCfg;

/* Variable sub-directives. */
typedef struct _variableCfg {
    int token;
    codeBlock *access_code;
    codeBlock *get_code;
    codeBlock *set_code;
} variableCfg;

/* %VirtualErrorHandler */
typedef struct _vehCfg {
    int token;
    const char *name;
} vehCfg;

#endif
