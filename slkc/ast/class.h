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
		/// @brief Error indicates which type name caused the inheritance error.
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

	class EnumItemNode : public MemberNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<ExprNode> enumValue;

		SLKC_API EnumItemNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API EnumItemNode(const EnumItemNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~EnumItemNode();
	};

	class ConstEnumNode : public ModuleNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<TypeNameNode> baseType;

		SLKC_API ConstEnumNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API ConstEnumNode(const ConstEnumNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~ConstEnumNode();
	};

	class ScopedEnumNode : public ModuleNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<TypeNameNode> baseType;

		SLKC_API ScopedEnumNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API ScopedEnumNode(const ScopedEnumNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~ScopedEnumNode();
	};

	class UnionEnumItemNode : public MemberNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		peff::DynArray<AstNodePtr<TypeNameNode>> elementTypes;

		SLKC_API UnionEnumItemNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API UnionEnumItemNode(const UnionEnumItemNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~UnionEnumItemNode();
	};

	class RecordUnionEnumItemNode : public ModuleNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		SLKC_API RecordUnionEnumItemNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API RecordUnionEnumItemNode(const RecordUnionEnumItemNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~RecordUnionEnumItemNode();
	};

	class ClassEnumNode : public ModuleNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<TypeNameNode> baseType;
		peff::DynArray<AstNodePtr<GenericParamNode>> genericParams;
		peff::HashMap<std::string_view, size_t> genericParamIndices;
		peff::DynArray<size_t> idxGenericParamCommaTokens;
		size_t idxLAngleBracketToken = SIZE_MAX, idxRAngleBracketToken = SIZE_MAX;
		bool isGenericParamsIndexed = false;

		SLKC_API ClassEnumNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API ClassEnumNode(const ClassEnumNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~ClassEnumNode();
	};

	class StructEnumNode : public ModuleNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		/// @brief Indicates if the cyclic inheritance is already checked.
		bool isRecursedTypeChecked = false;
		/// @brief Indicates if the class has cyclic inheritance.
		bool isRecursedTypeFlag = false;

		AstNodePtr<TypeNameNode> baseType;
		peff::DynArray<AstNodePtr<GenericParamNode>> genericParams;
		peff::HashMap<std::string_view, size_t> genericParamIndices;
		peff::DynArray<size_t> idxGenericParamCommaTokens;
		size_t idxLAngleBracketToken = SIZE_MAX, idxRAngleBracketToken = SIZE_MAX;
		bool isGenericParamsIndexed = false;

		SLKC_API StructEnumNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API StructEnumNode(const StructEnumNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~StructEnumNode();

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
