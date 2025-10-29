#ifndef _SLKC_AST_CLASS_H_
#define _SLKC_AST_CLASS_H_

#include "var.h"
#include "fn.h"

namespace slkc {
	class ClassNode : public ModuleNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		/// @brief Indicates if the cyclic inheritance is already checked.
		bool isCyclicInheritanceChecked = false;
		/// @brief Indicates if the class has cyclic inheritance.
		bool isCyclicInheritedFlag = false;

		AstNodePtr<TypeNameNode> baseType;
		peff::DynArray<AstNodePtr<TypeNameNode>> implTypes;
		peff::DynArray<AstNodePtr<GenericParamNode>> genericParams;
		peff::HashMap<std::string_view, size_t> genericParamIndices;
		peff::DynArray<size_t> idxGenericParamCommaTokens;
		size_t idxLAngleBracketToken = SIZE_MAX, idxRAngleBracketToken = SIZE_MAX;

		bool isGenericParamsIndexed = false;

		SLKC_API ClassNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API ClassNode(const ClassNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~ClassNode();

		SLKC_API peff::Option<CompilationError> isCyclicInherited(bool &whetherOut);
		SLKC_API peff::Option<CompilationError> updateCyclicInheritedStatus();
		SLAKE_FORCEINLINE void resetCyclicInheritanceFlag() {
			isCyclicInheritanceChecked = false;
			isCyclicInheritedFlag = false;
		}
	};

	class InterfaceNode : public ModuleNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		/// @brief Indicates if the cyclic inheritance is already checked.
		bool isCyclicInheritanceChecked = false;
		/// @brief Indicates if the interface has cyclic inheritance.
		bool isCyclicInheritedFlag = false;
		/// @brief Error indicates which one type name caused the inheritance error.
		peff::Option<CompilationError> cyclicInheritanceError;

		peff::DynArray<AstNodePtr<TypeNameNode>> implTypes;
		peff::DynArray<AstNodePtr<GenericParamNode>> genericParams;
		peff::HashMap<std::string_view, size_t> genericParamIndices;
		peff::DynArray<size_t> idxGenericParamCommaTokens;
		size_t idxLAngleBracketToken = SIZE_MAX, idxRAngleBracketToken = SIZE_MAX;

		bool isGenericParamsIndexed = false;

		SLKC_API InterfaceNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API InterfaceNode(const InterfaceNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~InterfaceNode();

		SLKC_API peff::Option<CompilationError> isCyclicInherited(bool &whetherOut);
		SLKC_API peff::Option<CompilationError> updateCyclicInheritedStatus();
		SLAKE_FORCEINLINE void resetCyclicInheritanceFlag() {
			isCyclicInheritanceChecked = false;
			isCyclicInheritedFlag = false;
			cyclicInheritanceError.reset();
		}
	};

	class StructNode : public ModuleNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		/// @brief Indicates if the cyclic inheritance is already checked.
		bool isRecursedTypeChecked = false;
		/// @brief Indicates if the class has cyclic inheritance.
		bool isRecursedTypeFlag = false;

		peff::DynArray<AstNodePtr<TypeNameNode>> implTypes;
		peff::DynArray<AstNodePtr<GenericParamNode>> genericParams;
		peff::HashMap<std::string_view, size_t> genericParamIndices;
		peff::DynArray<size_t> idxGenericParamCommaTokens;
		size_t idxLAngleBracketToken = SIZE_MAX, idxRAngleBracketToken = SIZE_MAX;

		bool isGenericParamsIndexed = false;

		SLKC_API StructNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API StructNode(const StructNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~StructNode();

		SLKC_API peff::Option<CompilationError> isRecursedType(bool &whetherOut);
		SLKC_API peff::Option<CompilationError> updateRecursedTypeStatus();
		SLAKE_FORCEINLINE void resetRecursedTypeFlag() {
			isRecursedTypeChecked = false;
			isRecursedTypeFlag = false;
		}
	};

	class ThisNode : public MemberNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<MemberNode> thisType;

		SLKC_API ThisNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API ThisNode(const ThisNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~ThisNode();
	};
}

#endif
