/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// The list of checker classes that are compatible with clang-tidy.

CHECK(ArithmeticArgChecker, "arithmetic-argument")
CHECK(AssertAssignmentChecker, "assignment-in-assert")
CHECK(CanRunScriptChecker, "can-run-script")
CHECK(DanglingOnTemporaryChecker, "dangling-on-temporary")
CHECK(ExplicitImplicitChecker, "implicit-constructor")
CHECK(ExplicitOperatorBoolChecker, "explicit-operator-bool")
CHECK(KungFuDeathGripChecker, "kungfu-death-grip")
#ifdef _WIN32
CHECK(LoadLibraryUsageChecker, "load-library-usage")
CHECK(FopenUsageChecker, "fopen-usage")
#endif
CHECK(MustOverrideChecker, "must-override")
CHECK(MustReturnFromCallerChecker, "must-return-from-caller")
CHECK(MustUseChecker, "must-use")
CHECK(NaNExprChecker, "nan-expr")
CHECK(NeedsNoVTableTypeChecker, "needs-no-vtable-type")
CHECK(NoAddRefReleaseOnReturnChecker, "no-addref-release-on-return")
CHECK(NoAutoTypeChecker, "no-auto-type")
CHECK(NoDuplicateRefCntMemberChecker, "no-duplicate-refcnt-member")
CHECK(NoExplicitMoveConstructorChecker, "no-explicit-move-constructor")
CHECK(NonMemMovableMemberChecker, "non-memmovable-member")
CHECK(NonMemMovableTemplateArgChecker, "non-memmovable-template-arg")
CHECK(NoUsingNamespaceMozillaJavaChecker, "no-using-namespace-mozilla-java")
CHECK(NonParamInsideFunctionDeclChecker, "non-memmovable-template-arg")
CHECK(NonTrivialTypeInFfiChecker, "non-trivial-type-in-ffi-boundary")
CHECK(OverrideBaseCallChecker, "override-base-call")
CHECK(OverrideBaseCallUsageChecker, "override-base-call-usage")
CHECK(ParamTraitsEnumChecker, "paramtraits-enum")
CHECK(RefCountedCopyConstructorChecker, "refcounted-copy-constructor")
CHECK(RefCountedInsideLambdaChecker, "refcounted-inside-lambda")
CHECK(ScopeChecker, "scope")
CHECK(SprintfLiteralChecker, "sprintf-literal")
CHECK(TempRefPtrChecker, "performance-temp-refptr")
CHECK(TrivialCtorDtorChecker, "trivial-constructor-destructor")
CHECK(TrivialDtorChecker, "trivial-destructor")
