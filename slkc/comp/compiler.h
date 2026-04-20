#ifndef _SLKC_COMP_COMPILER_H_
#define _SLKC_COMP_COMPILER_H_

#include "../ast/parser.h"
#include "../ast/class.h"
#include "../ast/module.h"
#include "../ast/import.h"

namespace slkc {
	SLKC_API peff::Option<slkc::CompilationError> type_name_cmp(AstNodePtr<TypeNameNode> lhs, AstNodePtr<TypeNameNode> rhs, int &out) noexcept;
	SLKC_API peff::Option<slkc::CompilationError> type_name_list_cmp(const peff::DynArray<AstNodePtr<TypeNameNode>> &lhs, const peff::DynArray<AstNodePtr<TypeNameNode>> &rhs, int &out) noexcept;

	enum class ExprEvalPurpose : uint8_t {
		EvalTypeActual,	 // Evaluate type only, as lvalue
		EvalType,		 // Evaluate type only, as rvalue
		Stmt,			 // As a statement
		LValue,			 // As a lvalue
		RValue,			 // As a rvalue
		Call,			 // As target of a calling expression
		Unpacking,		 // For unpacking, note that it is used for notifying the expression compiler to prepare the expression to be unpacked, not actually to unpack it.
	};

	struct LocalVarRegistry {
		AstNodePtr<VarNode> var_node;
		uint32_t idx_reg;
	};

	struct Label {
		peff::String name;
		uint32_t offset = 0;

		SLAKE_FORCEINLINE Label(peff::String &&name) : name(std::move(name)) {}
	};

	enum class NullOverrideType : uint8_t {
		Nullify = 0,
		Denullify,
		Uncertain
	};

	enum class PathPossibility : uint8_t {
		Never = 0,	// The path will never consist of a property.
		May,		// The path may consist of a property.
		Must,		// The path will always consist of a property.
	};

	SLKC_API PathPossibility combine_possibility(PathPossibility outer, PathPossibility inner) noexcept;

	struct PathEnv {
		const PathEnv *parent = nullptr;
		peff::Map<AstNodePtr<VarNode>, NullOverrideType> local_var_nullity_overrides;

		/// @brief Indicates if the path will be executed at least one times.
		PathPossibility exec_possibility = PathPossibility::Must;

		/// @brief Indicates if the path will return from the function.
		PathPossibility return_possibility = PathPossibility::Never;

		/// @brief Indicates if the path will never halt.
		PathPossibility no_return_possibility = PathPossibility::Never;

		/// @brief Indicates if the path will break current control flow (e.g. jump out from current loop).
		PathPossibility break_possibility = PathPossibility::Never;

		PathEnv(peff::Alloc *allocator) noexcept;
		PathEnv(PathEnv &&) noexcept = default;
		SLAKE_FORCEINLINE ~PathEnv() {}

		SLAKE_FORCEINLINE void set_parent(PathEnv *parent) noexcept {
			this->parent = parent;
		}
		SLAKE_API peff::Option<NullOverrideType> lookup_var_nullity_override(const AstNodePtr<VarNode> &var_node);
		SLAKE_API peff::Option<CompilationError> set_local_var_nullity_override(AstNodePtr<VarNode> var_node, NullOverrideType type);
		SLAKE_API void remove_var_nullity_override(const AstNodePtr<VarNode> &var_node);

		SLAKE_FORCEINLINE void reset() {
			local_var_nullity_overrides.clear();
			exec_possibility = PathPossibility::Must;
			return_possibility = PathPossibility::Never;
			no_return_possibility = PathPossibility::Never;
			break_possibility = PathPossibility::Never;
		}
	};

	class CompilationContext;

	SLKC_API peff::Option<CompilationError> combine_path_env(PathEnv &outer, const PathEnv &inner) noexcept;
	SLKC_API peff::Option<CompilationError> combine_parallel_path_env(peff::Alloc *allocator, CompileEnv *compile_env, CompilationContext *compilation_context, PathEnv &outer, PathEnv *const *inners, size_t num_inners) noexcept;

	class CompilationContext {
	protected:
		virtual uint32_t do_get_break_label() const = 0;
		virtual uint32_t do_get_continue_label() const = 0;

		virtual uint32_t do_get_break_label_block_level() const = 0;
		virtual uint32_t do_get_continue_label_block_level() const = 0;

	public:
		CompilationContext *parent = nullptr;

		SLKC_API CompilationContext(CompilationContext *parent);
		SLKC_API virtual ~CompilationContext();

		CompilationContext(const CompilationContext &) = delete;
		CompilationContext(CompilationContext &&) = delete;

		[[nodiscard]] virtual peff::Option<CompilationError> alloc_label(uint32_t &label_id_out) = 0;
		virtual void set_label_offset(uint32_t label_id, uint32_t offset) const = 0;
		[[nodiscard]] virtual peff::Option<CompilationError> set_label_name(uint32_t label_id, const std::string_view &name) = 0;
		virtual uint32_t get_label_offset(uint32_t label_id) const = 0;
		virtual peff::Option<uint32_t> get_label_index_by_name(const std::string_view &sv) const = 0;

		[[nodiscard]] virtual peff::Option<CompilationError> alloc_reg(uint32_t &reg_out) = 0;

		[[nodiscard]] virtual peff::Option<CompilationError> emit_ins(uint32_t idx_sld, slake::Opcode opcode, uint32_t output_reg_index, const std::initializer_list<slake::Value> &operands) = 0;
		[[nodiscard]] virtual peff::Option<CompilationError> emit_ins(uint32_t idx_sld, slake::Opcode opcode, uint32_t output_reg_index, slake::Value *operands, size_t num_operands) = 0;

		[[nodiscard]] virtual peff::Option<CompilationError> alloc_local_var(const TokenRange &token_range, const std::string_view &name, uint32_t reg, AstNodePtr<TypeNameNode> type, AstNodePtr<VarNode> &local_var_out) = 0;
		[[nodiscard]] virtual AstNodePtr<VarNode> get_local_var_in_cur_level(const std::string_view &name) const = 0;
		virtual AstNodePtr<VarNode> get_local_var(const std::string_view &name) const = 0;

		virtual void set_break_label(uint32_t label_id, uint32_t block_level) = 0;
		virtual void set_continue_label(uint32_t label_id, uint32_t block_level) = 0;

		uint32_t get_break_label() const;
		uint32_t get_continue_label() const;

		uint32_t get_break_label_block_level() const;
		virtual uint32_t get_continue_label_block_level() const;

		virtual uint32_t get_cur_ins_off() const = 0;

		[[nodiscard]] virtual peff::Option<CompilationError> enter_block() = 0;
		virtual void leave_block() = 0;

		virtual uint32_t get_block_level() = 0;

		virtual peff::Option<CompilationError> register_source_loc_desc(slake::slxfmt::SourceLocDesc sld, uint32_t &index_out) = 0;

		SLKC_API AstNodePtr<VarNode> lookup_local_var(const std::string_view &name) const;
	};

	struct CompileEnv;

	class NormalCompilationContext : public CompilationContext {
	protected:
		SLKC_API virtual uint32_t do_get_break_label() const override;
		SLKC_API virtual uint32_t do_get_continue_label() const override;

		SLKC_API virtual uint32_t do_get_break_label_block_level() const override;
		SLKC_API virtual uint32_t do_get_continue_label_block_level() const override;

	public:
		struct BlockLayer {
			peff::HashMap<std::string_view, AstNodePtr<VarNode>> local_vars;

			SLAKE_FORCEINLINE BlockLayer(peff::Alloc *allocator) : local_vars(allocator) {
			}
			SLAKE_FORCEINLINE BlockLayer(BlockLayer &&rhs) : local_vars(std::move(rhs.local_vars)) {
			}
			SLKC_API ~BlockLayer();

			SLAKE_FORCEINLINE BlockLayer &operator=(BlockLayer &&rhs) {
				local_vars = std::move(rhs.local_vars);
				return *this;
			}
		};

		peff::RcObjectPtr<peff::Alloc> allocator;

		CompilationContext *const parent = nullptr;

		peff::SharedPtr<Document> document;

		const uint32_t base_block_level;
		peff::List<BlockLayer> saved_block_layers;
		BlockLayer cur_block_layer;

		uint32_t num_total_regs = 0;

		peff::DynArray<slake::slxfmt::SourceLocDesc> source_loc_descs;
		peff::Map<slake::slxfmt::SourceLocDesc, size_t> source_loc_descs_map;

		peff::DynArray<peff::SharedPtr<Label>> labels;
		peff::HashMap<std::string_view, size_t> label_name_indices;

		uint32_t break_stmt_jump_dest_label = UINT32_MAX, continue_stmt_jump_dest_label = UINT32_MAX;
		uint32_t break_stmt_block_level = 0, continue_stmt_block_level = 0;

		const uint32_t base_ins_off;
		peff::DynArray<slake::Instruction> generated_instructions;

		SLKC_API NormalCompilationContext(CompileEnv *compile_env, CompilationContext *parent);
		SLKC_API virtual ~NormalCompilationContext();

		SLKC_API virtual peff::Option<CompilationError> alloc_label(uint32_t &label_id_out) override;
		SLKC_API virtual void set_label_offset(uint32_t label_id, uint32_t offset) const override;
		SLKC_API virtual peff::Option<CompilationError> set_label_name(uint32_t label_id, const std::string_view &name) override;
		SLKC_API virtual uint32_t get_label_offset(uint32_t label_id) const override;
		SLKC_API virtual peff::Option<uint32_t> get_label_index_by_name(const std::string_view &sv) const override;

		SLKC_API virtual peff::Option<CompilationError> alloc_reg(uint32_t &reg_out) override;

		SLKC_API virtual peff::Option<CompilationError> emit_ins(uint32_t idx_sld, slake::Opcode opcode, uint32_t output_reg_index, const std::initializer_list<slake::Value> &operands) override;
		SLKC_API virtual peff::Option<CompilationError> emit_ins(uint32_t idx_sld, slake::Opcode opcode, uint32_t output_reg_index, slake::Value *operands, size_t num_operands) override;

		SLKC_API virtual peff::Option<CompilationError> alloc_local_var(const TokenRange &token_range, const std::string_view &name, uint32_t reg, AstNodePtr<TypeNameNode> type, AstNodePtr<VarNode> &local_var_out) override;
		SLKC_API virtual AstNodePtr<VarNode> get_local_var_in_cur_level(const std::string_view &name) const override;
		SLKC_API virtual AstNodePtr<VarNode> get_local_var(const std::string_view &name) const override;

		SLKC_API virtual void set_break_label(uint32_t label_id, uint32_t block_level) override;
		SLKC_API virtual void set_continue_label(uint32_t label_id, uint32_t block_level) override;

		SLKC_API virtual uint32_t get_cur_ins_off() const override;

		SLKC_API virtual peff::Option<CompilationError> enter_block() override;
		SLKC_API virtual void leave_block() override;

		SLKC_API virtual uint32_t get_block_level() override;

		SLKC_API virtual peff::Option<CompilationError> register_source_loc_desc(slake::slxfmt::SourceLocDesc sld, uint32_t &index_out) override;
	};

	struct CompileEnv final {
		slake::Runtime *runtime;
		slake::HostRefHolder host_ref_holder;
		/// @brief The shortcut pointer to the document's allocator.
		peff::RcObjectPtr<peff::Alloc> allocator;
		// FIXME: reference to document causes the weak reference cannot become zero and thus seems to be buggy.
		peff::WeakPtr<Document> document;
		peff::DynArray<CompilationError> errors;
		peff::DynArray<CompilationWarning> warnings;
		AstNodePtr<FnOverloadingNode> cur_overloading;
		AstNodePtr<ModuleNode> cur_parent_access_node;
		AstNodePtr<ModuleNode> cur_module;
		AstNodePtr<ThisNode> this_node;
		uint32_t flags;

		SLAKE_FORCEINLINE CompileEnv(
			slake::Runtime *runtime,
			peff::SharedPtr<Document> document,
			peff::Alloc *self_allocator,
			peff::Alloc *allocator)
			: runtime(runtime),
			  document(document),
			  host_ref_holder(allocator),
			  allocator(allocator),
			  errors(allocator),
			  warnings(allocator),
			  flags(0) {}

		SLKC_API virtual ~CompileEnv();

		SLAKE_FORCEINLINE peff::Option<CompilationError> push_error(CompilationError &&error) {
			if (!errors.push_back(std::move(error)))
				return gen_oom_comp_error();

			return {};
		}

		SLAKE_FORCEINLINE peff::Option<CompilationError> push_warning(CompilationWarning &&warning) {
			if (!warnings.push_back(std::move(warning)))
				return gen_oom_comp_error();

			return {};
		}

		SLAKE_FORCEINLINE void reset() {
			cur_overloading = {};
			this_node = {};
		}

		SLAKE_FORCEINLINE peff::SharedPtr<Document> get_document() const noexcept {
			return document.lock();
		}
	};

	struct PrevBreakPointHolder {
		CompilationContext *compile_env;
		uint32_t last_break_stmt_jump_dest_label,
			last_break_stmt_block_level;

		SLAKE_FORCEINLINE PrevBreakPointHolder(CompilationContext *compile_env)
			: compile_env(compile_env),
			  last_break_stmt_jump_dest_label(compile_env->get_break_label()),
			  last_break_stmt_block_level(compile_env->get_break_label_block_level()) {}
		SLAKE_FORCEINLINE ~PrevBreakPointHolder() {
			compile_env->set_break_label(last_break_stmt_jump_dest_label, last_break_stmt_block_level);
		}
	};

	struct PrevContinuePointHolder {
		CompilationContext *compile_env;
		uint32_t last_continue_stmt_jump_dest_label,
			last_continue_stmt_block_level;

		SLAKE_FORCEINLINE PrevContinuePointHolder(CompilationContext *compile_env)
			: compile_env(compile_env),
			  last_continue_stmt_jump_dest_label(compile_env->get_continue_label()),
			  last_continue_stmt_block_level(compile_env->get_continue_label_block_level()) {}
		SLAKE_FORCEINLINE ~PrevContinuePointHolder() {
			compile_env->set_continue_label(last_continue_stmt_jump_dest_label, last_continue_stmt_block_level);
		}
	};

	struct CompileExprResult {
		AstNodePtr<TypeNameNode> evaluated_type;
		AstNodePtr<MemberNode> evaluated_final_member;

		PathEnv var_nullity_override_path_env;

		// For parameter name query, etc, if exists.
		AstNodePtr<FnNode> call_target_fn_slot;
		peff::DynArray<AstNodePtr<FnOverloadingNode>> call_target_matched_overloadings;
		uint32_t idx_this_reg_out = UINT32_MAX, idx_result_reg_out = UINT32_MAX;

		SLAKE_FORCEINLINE CompileExprResult(peff::Alloc *allocator) : call_target_matched_overloadings(allocator), var_nullity_override_path_env(allocator) {}

		SLAKE_FORCEINLINE void reset() {
			evaluated_type = {};
			evaluated_final_member = {};
			var_nullity_override_path_env.reset();
			call_target_fn_slot = {};
			call_target_matched_overloadings.clear_and_shrink();
			idx_this_reg_out = UINT32_MAX;
			idx_result_reg_out = UINT32_MAX;
		}
	};

	struct ResolvedIdRefPart {
		bool is_static;
		size_t num_entries;
		AstNodePtr<MemberNode> member;
	};

	using ResolvedIdRefPartList = peff::DynArray<ResolvedIdRefPart>;

	[[nodiscard]] SLKC_API peff::Option<CompilationError> _compile_or_cast_operand(
		CompileEnv *compile_env,
		CompilationContext *compilation_context,
		PathEnv *path_env,
		ExprEvalPurpose eval_purpose,
		AstNodePtr<TypeNameNode> desired_type,
		AstNodePtr<ExprNode> operand,
		AstNodePtr<TypeNameNode> operand_type,
		CompileExprResult &result_out);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> resolve_static_member(
		CompileEnv *compile_env,
		peff::SharedPtr<Document> document,
		const AstNodePtr<MemberNode> &member_node,
		const IdRefEntry &name,
		AstNodePtr<MemberNode> &member_out,
		bool instantiate_generic_member = true);
	[[nodiscard]] SLKC_API
		peff::Option<CompilationError>
		resolve_instance_member(
			CompileEnv *compile_env,
			peff::SharedPtr<Document> document,
			AstNodePtr<MemberNode> member_node,
			const IdRefEntry &name,
			AstNodePtr<MemberNode> &member_out,
			bool instantiate_generic_member = true);
	[[nodiscard]] SLKC_API
		peff::Option<CompilationError>
		is_member_accessible(
			CompileEnv *compile_env,
			AstNodePtr<MemberNode> parent,
			AstNodePtr<MemberNode> member,
			bool &result_out);
	/// @brief Resolve an identifier reference.
	/// @param compile_env The compile context. Leave this parameter empty bypasses the access check.
	/// @param document Document for resolution.
	/// @param resolve_root Root object for resolution.
	/// @param id_ref Identifier entries for resolution.
	/// @param num_entries Number of the identifier entries.
	/// @param member_out Will be used to store the output member, `nullptr` if not found.
	/// @param resolved_part_list_out Will be used to store the resolved part information.
	/// @param is_static Whether the initial resolution is static.
	/// @return The fatal error encountered during the resolution.
	[[nodiscard]] SLKC_API
		peff::Option<CompilationError>
		resolve_id_ref(
			CompileEnv *compile_env,
			peff::SharedPtr<Document> document,
			const AstNodePtr<MemberNode> &resolve_root,
			IdRefEntry *id_ref,
			size_t num_entries,
			AstNodePtr<MemberNode> &member_out,
			ResolvedIdRefPartList *resolved_part_list_out,
			bool is_static = true,
			bool instantiate_generic_member = true);
	/// @brief Resolve an identifier reference with a scope object and its parents.
	/// @param compile_env The compile context. Leave this parameter empty bypasses the access check.
	/// @param document Document for resolution.
	/// @param walked_nodes Reference to the container to store the walked nodes, should be empty on the top level.
	/// @param resolve_scope Scope object for resolution.
	/// @param id_ref Identifier entry array for resolution.
	/// @param num_entries Number of identifier entries.
	/// @param member_out Will be used for output member storage, `nullptr` if not found.
	/// @param is_static Controls if the initial resolution is static or instance.
	/// @param is_sealed Controls if not go into the parent of the current scope object.
	/// @return The fatal error encountered during the resolution.
	[[nodiscard]] SLKC_API
		peff::Option<CompilationError>
		resolve_id_ref_with_scope_node(
			CompileEnv *compile_env,
			peff::SharedPtr<Document> document,
			peff::Set<AstNodePtr<MemberNode>> &walked_nodes,
			AstNodePtr<MemberNode> resolve_scope,
			IdRefEntry *id_ref,
			size_t num_entries,
			AstNodePtr<MemberNode> &member_out,
			ResolvedIdRefPartList *resolved_part_list_out,
			bool is_static = true,
			bool is_sealed = false,
			bool instantiate_generic_member = true);
	/// @brief Resolve a custom type name.
	/// @param compile_env The compile context. Leave this parameter empty bypasses the access check.
	/// @param type_name Type name to be resolved.
	/// @param member_node_out Where the resolved member node will be stored.
	/// @param walked_nodes Reserved for internal use, stores the nodes walked to void cyclic walking.
	/// @return Critical error encountered that forced the resolution to interrupt.
	[[nodiscard]] SLKC_API
		peff::Option<CompilationError>
		resolve_custom_type_name(
			CompileEnv *compile_env,
			peff::SharedPtr<Document> document,
			const AstNodePtr<CustomTypeNameNode> &type_name,
			AstNodePtr<MemberNode> &member_node_out,
			bool instantiate_generic_member = true,
			peff::Set<AstNodePtr<MemberNode>> *walked_nodes = nullptr);
	[[nodiscard]] SLKC_API
		peff::Option<CompilationError>
		resolve_base_overriden_custom_type_name(
			peff::SharedPtr<Document> document,
			const AstNodePtr<CustomTypeNameNode> &type_name,
			AstNodePtr<TypeNameNode> &type_name_out);

	/// @brief Collect interfaces involved in the whole inheritance chain.
	/// @note Note that this function does not clear current set.
	/// @param document Document to be operated.
	/// @param derived Leaf interface node.
	/// @param walked_interfaces Where the involved interfaces are stored.
	/// @param insert_self Controls whether to insert the leaf interface itself into the involved interface set.
	/// @return std::nullopt No error.
	/// @return CompilationErrorKind::CyclicInheritedInterface Cyclic inherited interface was detected.
	[[nodiscard]] SLKC_API peff::Option<CompilationError> collect_involved_interfaces(
		peff::SharedPtr<Document> document,
		const AstNodePtr<InterfaceNode> &bottom,
		peff::Set<AstNodePtr<InterfaceNode>> &walked_interfaces,
		bool insert_self);
	///
	/// @brief Walk and collect involved interfaces like in a BFS phase.
	///
	/// @param interfaces_in Current interface set.
	/// @param new_interfaces_out New interface set.
	/// @return Any fatal error occurred during the collecting operation.
	///
	[[nodiscard]] SLKC_API peff::Option<CompilationError> collect_involved_interfaces_phased_bfs(
		const peff::Set<AstNodePtr<InterfaceNode>> &interfaces_in,
		peff::Set<AstNodePtr<InterfaceNode>> &new_interfaces_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> collect_inherited_members(
		peff::SharedPtr<Document> document,
		const AstNodePtr<ClassNode> &bottom,
		peff::Set<AstNodePtr<MemberNode>> &walked_members,
		bool insert_self);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> is_implemented_by_interface(
		peff::SharedPtr<Document> document,
		const AstNodePtr<InterfaceNode> &base,
		const AstNodePtr<InterfaceNode> &derived,
		bool &whether_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> is_implemented_by_class(
		peff::SharedPtr<Document> document,
		const AstNodePtr<InterfaceNode> &base,
		const AstNodePtr<ClassNode> &derived,
		bool &whether_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> is_struct_recursed(
		peff::SharedPtr<Document> document,
		const AstNodePtr<StructNode> &derived,
		bool &whether_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> is_union_enum_recursed(
		peff::SharedPtr<Document> document,
		const AstNodePtr<UnionEnumNode> &derived,
		bool &whether_out);
	SLKC_API peff::Option<CompilationError> is_higher_ranked_cyclic_inherited(
		AstNodePtr<MemberNode> cls,
		peff::Alloc *allocator,
		bool &result_out,
		bool forced_update = false) noexcept;
	[[nodiscard]] SLKC_API peff::Option<CompilationError> is_base_of(
		peff::SharedPtr<Document> document,
		const AstNodePtr<ClassNode> &base,
		const AstNodePtr<ClassNode> &derived,
		bool &whether_out);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> remove_ref_of_type(
		AstNodePtr<TypeNameNode> src,
		AstNodePtr<TypeNameNode> &type_name_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> remove_nullable_of_type(
		AstNodePtr<TypeNameNode> src,
		AstNodePtr<TypeNameNode> &type_name_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> is_lvalue_type(
		AstNodePtr<TypeNameNode> src,
		bool &whether_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> is_same_type(
		AstNodePtr<TypeNameNode> lhs,
		AstNodePtr<TypeNameNode> rhs,
		bool &whether_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> get_type_promotion_level(
		AstNodePtr<TypeNameNode> type_name,
		int &level_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> determine_promotional_type(
		AstNodePtr<TypeNameNode> lhs,
		AstNodePtr<TypeNameNode> rhs,
		AstNodePtr<TypeNameNode> &type_name_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> is_floating_point(
		AstNodePtr<TypeNameNode> type,
		bool &result_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> is_signed(
		AstNodePtr<TypeNameNode> type,
		bool &result_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> is_unsigned(
		AstNodePtr<TypeNameNode> type,
		bool &result_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> is_class_type(
		AstNodePtr<TypeNameNode> type,
		bool &result_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> to_signed(
		AstNodePtr<TypeNameNode> type,
		AstNodePtr<TypeNameNode> &type_name_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> to_unsigned(
		AstNodePtr<TypeNameNode> type,
		AstNodePtr<TypeNameNode> &type_name_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> is_integral(
		AstNodePtr<TypeNameNode> type,
		bool &result_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> is_subtype_of(
		AstNodePtr<TypeNameNode> subtype,
		AstNodePtr<TypeNameNode> base_type,
		bool &result_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> is_basic_type(
		AstNodePtr<TypeNameNode> lhs,
		bool &result_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> is_object_type(
		AstNodePtr<TypeNameNode> lhs,
		bool &result_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> is_scoped_enum_base_type(
		AstNodePtr<TypeNameNode> lhs,
		bool &result_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> infer_common_type(
		AstNodePtr<TypeNameNode> lhs,
		AstNodePtr<TypeNameNode> rhs,
		AstNodePtr<TypeNameNode> &type_name_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> is_same_type_in_signature(
		AstNodePtr<TypeNameNode> lhs,
		AstNodePtr<TypeNameNode> rhs,
		bool &whether_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> is_convertible(
		AstNodePtr<TypeNameNode> src,
		AstNodePtr<TypeNameNode> dest,
		bool is_sealed,
		bool &whether_out);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> _is_type_name_param_list_type_name_tree(
		AstNodePtr<TypeNameNode> type,
		bool &whether_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> _do_expand_param_list_type_name_tree(
		AstNodePtr<TypeNameNode> &type);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> unwrap_param_list_type_name_tree(
		AstNodePtr<TypeNameNode> type,
		peff::Alloc *allocator,
		AstNodePtr<TypeNameNode> &type_name_out);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> unwrap_facade_type_name(
		AstNodePtr<TypeNameNode> type,
		AstNodePtr<TypeNameNode> &type_name_out);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> get_unpacked_type_of(
		AstNodePtr<TypeNameNode> type,
		AstNodePtr<TypeNameNode> &type_name_out);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> compile_unary_expr(
		CompileEnv *compile_env,
		CompilationContext *compilation_context,
		PathEnv *path_env,
		AstNodePtr<UnaryExprNode> expr,
		ExprEvalPurpose eval_purpose,
		CompileExprResult &result_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> compile_binary_expr(
		CompileEnv *compile_env,
		CompilationContext *compilation_context,
		PathEnv *path_env,
		AstNodePtr<BinaryExprNode> expr,
		ExprEvalPurpose eval_purpose,
		CompileExprResult &result_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> compile_expr(
		CompileEnv *compile_env,
		CompilationContext *compilation_context,
		PathEnv *path_env,
		const AstNodePtr<ExprNode> &expr,
		ExprEvalPurpose eval_purpose,
		AstNodePtr<TypeNameNode> desired_type,
		CompileExprResult &result_out);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> compile_expr_stmt(
		CompileEnv *compile_env,
		CompilationContext *compilation_context,
		PathEnv *path_env,
		AstNodePtr<ExprStmtNode> s,
		uint32_t sld_index);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> compile_var_def_stmt(
		CompileEnv *compile_env,
		CompilationContext *compilation_context,
		PathEnv *path_env,
		AstNodePtr<VarDefStmtNode> s,
		uint32_t sld_index);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> compile_break_stmt(
		CompileEnv *compile_env,
		CompilationContext *compilation_context,
		PathEnv *path_env,
		AstNodePtr<BreakStmtNode> s,
		uint32_t sld_index);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> compile_continue_stmt(
		CompileEnv *compile_env,
		CompilationContext *compilation_context,
		PathEnv *path_env,
		AstNodePtr<ContinueStmtNode> s,
		uint32_t sld_index);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> compile_for_stmt(
		CompileEnv *compile_env,
		CompilationContext *compilation_context,
		PathEnv *path_env,
		AstNodePtr<ForStmtNode> s,
		uint32_t sld_index);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> compile_while_stmt(
		CompileEnv *compile_env,
		CompilationContext *compilation_context,
		PathEnv *path_env,
		AstNodePtr<WhileStmtNode> s,
		uint32_t sld_index);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> compile_do_while_stmt(
		CompileEnv *compile_env,
		CompilationContext *compilation_context,
		PathEnv *path_env,
		AstNodePtr<DoWhileStmtNode> s,
		uint32_t sld_index);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> compile_if_stmt(
		CompileEnv *compile_env,
		CompilationContext *compilation_context,
		PathEnv *path_env,
		AstNodePtr<IfStmtNode> s,
		uint32_t sld_index);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> compile_with_stmt(
		CompileEnv *compile_env,
		CompilationContext *compilation_context,
		PathEnv *path_env,
		AstNodePtr<WithStmtNode> s,
		uint32_t sld_index);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> compile_switch_stmt(
		CompileEnv *compile_env,
		CompilationContext *compilation_context,
		PathEnv *path_env,
		AstNodePtr<SwitchStmtNode> s,
		uint32_t sld_index);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> compile_return_stmt(
		CompileEnv *compile_env,
		CompilationContext *compilation_context,
		PathEnv *path_env,
		AstNodePtr<ReturnStmtNode> s,
		uint32_t sld_index);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> compile_yield_stmt(
		CompileEnv *compile_env,
		CompilationContext *compilation_context,
		PathEnv *path_env,
		AstNodePtr<YieldStmtNode> s,
		uint32_t sld_index);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> compile_stmt(
		CompileEnv *compile_env,
		CompilationContext *compilation_context,
		PathEnv *path_env,
		const AstNodePtr<StmtNode> &stmt);
	///
	/// @brief Try compiling a statement. Won't affect the compilation context but path environment.
	///
	/// @param compile_env Compile environment for the compilation.
	/// @param parnet_compilation_context Parent of the temporary compilation context.
	/// @param path_env Path environment for compilation.
	/// @param stmt Statement to be compiled.
	/// @return Any fatal error occurred during compilation.
	///
	[[nodiscard]] SLKC_API peff::Option<CompilationError> try_compile_stmt(
		CompileEnv *compile_env,
		CompilationContext *parent_compilation_context,
		PathEnv *path_env,
		const AstNodePtr<StmtNode> &stmt);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> eval_expr_type(
		CompileEnv *compile_env,
		CompilationContext *compilation_context,
		PathEnv *path_env,
		const AstNodePtr<ExprNode> &expr,
		AstNodePtr<TypeNameNode> &type_out,
		AstNodePtr<TypeNameNode> desired_type = {});
	[[nodiscard]] SLKC_API peff::Option<CompilationError> eval_decayed_expr_type(
		CompileEnv *compile_env,
		CompilationContext *compilation_context,
		PathEnv *path_env,
		const AstNodePtr<ExprNode> &expr,
		AstNodePtr<TypeNameNode> &type_out,
		AstNodePtr<TypeNameNode> desired_type = {});
	[[nodiscard]] SLKC_API peff::Option<CompilationError> eval_actual_expr_type(
		CompileEnv *compile_env,
		CompilationContext *compilation_context,
		PathEnv *path_env,
		const AstNodePtr<ExprNode> &expr,
		AstNodePtr<TypeNameNode> &type_out,
		AstNodePtr<TypeNameNode> desired_type = {});

	[[nodiscard]] SLAKE_API peff::Option<CompilationError> _do_eval_const_expr(
		CompileEnv *compile_env,
		CompilationContext *compilation_context,
		PathEnv *path_env,
		AstNodePtr<ExprNode> expr,
		AstNodePtr<ExprNode> &expr_out);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> eval_const_expr(
		CompileEnv *compile_env,
		CompilationContext *compilation_context,
		PathEnv *path_env,
		AstNodePtr<ExprNode> expr,
		AstNodePtr<ExprNode> &expr_out);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> get_full_id_ref(peff::Alloc *allocator, AstNodePtr<MemberNode> m, IdRefPtr &id_ref_out);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> get_succeeding_enum_value(
		CompileEnv *compile_env,
		CompilationContext *compilation_context,
		AstNodePtr<TypeNameNode> base_type,
		AstNodePtr<ExprNode> last_value,
		AstNodePtr<ExprNode> &value_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> fill_scoped_enum(
		CompileEnv *compile_env,
		CompilationContext *compilation_context,
		AstNodePtr<ScopedEnumNode> enum_node);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> compile_type_name(
		CompileEnv *compile_env,
		CompilationContext *compilation_context,
		AstNodePtr<TypeNameNode> type_name,
		slake::TypeRef &type_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> compile_id_ref(
		CompileEnv *compile_env,
		CompilationContext *compilation_context,
		const IdRefEntry *entries,
		size_t num_entries,
		AstNodePtr<TypeNameNode> *param_types,
		size_t num_params,
		bool has_var_args,
		AstNodePtr<TypeNameNode> overriden_type,
		slake::HostObjectRef<slake::IdRefObject> &id_ref_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> compile_value_expr(
		CompileEnv *compile_env,
		CompilationContext *compilation_context,
		AstNodePtr<ExprNode> expr,
		slake::Value &value_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> compile_generic_params(
		CompileEnv *compile_env,
		CompilationContext *compilation_context,
		AstNodePtr<ModuleNode> mod,
		const AstNodePtr<GenericParamNode> *generic_params,
		size_t num_generic_params,
		slake::GenericParamList &generic_param_list_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> compile_module_like_node(
		CompileEnv *compile_env,
		AstNodePtr<ModuleNode> mod,
		slake::BasicModuleObject *mod_out);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> reindex_fn_params(
		CompileEnv *compile_env,
		AstNodePtr<FnOverloadingNode> fn);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> index_fn_params(
		CompileEnv *compile_env,
		AstNodePtr<FnOverloadingNode> fn);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> determine_fn_overloading(
		CompileEnv *compile_env,
		AstNodePtr<FnNode> fn_slot,
		const AstNodePtr<TypeNameNode> *arg_types,
		size_t num_arg_types,
		bool is_static,
		peff::DynArray<AstNodePtr<FnOverloadingNode>> &matched_overloadings);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> fn_to_type_name(
		CompileEnv *compile_env,
		AstNodePtr<FnOverloadingNode> fn,
		AstNodePtr<FnTypeNameNode> &evaluated_type_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> cleanup_unused_module_tree(
		CompileEnv *compile_env,
		AstNodePtr<ModuleNode> leaf);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> complete_parent_modules(
		CompileEnv *compile_env,
		IdRef *module_path,
		AstNodePtr<ModuleNode> leaf);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> gen_binary_op_expr(CompileEnv *compile_env, BinaryOp binary_op, AstNodePtr<ExprNode> lhs, AstNodePtr<ExprNode> rhs, TokenRange token_range, AstNodePtr<BinaryExprNode> &result_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> eval_const_binary_op_expr(CompileEnv *compile_env, CompilationContext *compilation_context, PathEnv *path_env, BinaryOp binary_op, AstNodePtr<ExprNode> lhs, AstNodePtr<ExprNode> rhs, AstNodePtr<ExprNode> &result_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> gen_implicit_cast_expr(CompileEnv *compile_env, AstNodePtr<ExprNode> source, AstNodePtr<TypeNameNode> dest_type, AstNodePtr<CastExprNode> &result_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> implicit_convert_const_expr(CompileEnv *compile_env, CompilationContext *compilation_context, PathEnv *path_env, AstNodePtr<ExprNode> source, AstNodePtr<TypeNameNode> dest_type, AstNodePtr<ExprNode> &result_out);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> is_fn_signature_same(AstNodePtr<VarNode> *l_params, AstNodePtr<VarNode> *r_params, size_t num_params, AstNodePtr<TypeNameNode> l_overriden_type, AstNodePtr<TypeNameNode> r_overriden_type, bool &whether_out);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> is_fn_signature_duplicated(AstNodePtr<FnOverloadingNode> lhs, AstNodePtr<FnOverloadingNode> rhs, bool &whether_out);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> visit_base_class(AstNodePtr<TypeNameNode> base_type_name, AstNodePtr<ClassNode> &class_out, peff::Set<AstNodePtr<MemberNode>> *walked_nodes);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> visit_base_interface(AstNodePtr<TypeNameNode> base_type_name, AstNodePtr<InterfaceNode> &class_out, peff::Set<AstNodePtr<MemberNode>> *walked_nodes);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> check_stack_bounds(size_t reserved_size);

	SLAKE_FORCEINLINE slake::slxfmt::SourceLocDesc token_range_to_sld(const TokenRange &token_range) {
		slake::slxfmt::SourceLocDesc sld;

		SourceLocation src_loc = token_range.module_node->parser->token_list.at(token_range.begin_index)->source_location;

		sld.line = src_loc.begin_position.line;
		sld.column = src_loc.begin_position.column;

		return sld;
	}

	class Writer {
	public:
		SLKC_API virtual ~Writer();

		virtual peff::Option<CompilationError> write(const char *src, size_t size) = 0;

		SLAKE_FORCEINLINE peff::Option<CompilationError> write_i8(int8_t data) noexcept {
			return write((char *)&data, sizeof(int8_t));
		}

		SLAKE_FORCEINLINE peff::Option<CompilationError> write_i16(int16_t data) noexcept {
			return write((char *)&data, sizeof(int16_t));
		}

		SLAKE_FORCEINLINE peff::Option<CompilationError> write_i32(int32_t data) noexcept {
			return write((char *)&data, sizeof(int32_t));
		}

		SLAKE_FORCEINLINE peff::Option<CompilationError> write_i64(int64_t data) noexcept {
			return write((char *)&data, sizeof(int64_t));
		}

		SLAKE_FORCEINLINE peff::Option<CompilationError> write_u8(uint8_t data) noexcept {
			return write((char *)&data, sizeof(uint8_t));
		}

		SLAKE_FORCEINLINE peff::Option<CompilationError> write_u16(uint16_t data) noexcept {
			return write((char *)&data, sizeof(uint16_t));
		}

		SLAKE_FORCEINLINE peff::Option<CompilationError> write_u32(uint32_t data) noexcept {
			return write((char *)&data, sizeof(uint32_t));
		}

		SLAKE_FORCEINLINE peff::Option<CompilationError> write_u64(uint64_t data) noexcept {
			return write((char *)&data, sizeof(uint64_t));
		}

		SLAKE_FORCEINLINE peff::Option<CompilationError> write_f32(float data) noexcept {
			return write((char *)&data, sizeof(float));
		}

		SLAKE_FORCEINLINE peff::Option<CompilationError> write_f64(double data) noexcept {
			return write((char *)&data, sizeof(double));
		}

		SLAKE_FORCEINLINE peff::Option<CompilationError> write_bool(bool data) noexcept {
			return write((char *)&data, sizeof(bool));
		}
	};

	[[nodiscard]] SLKC_API peff::Option<CompilationError> dump_generic_param(
		peff::Alloc *allocator,
		Writer *writer,
		const slake::GenericParam &generic_params);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> dump_id_ref_entries(
		peff::Alloc *allocator,
		Writer *writer,
		const peff::DynArray<slake::IdRefEntry> &entries);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> dump_id_ref(
		peff::Alloc *allocator,
		Writer *writer,
		slake::IdRefObject *ref);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> dump_value(
		peff::Alloc *allocator,
		Writer *writer,
		const slake::Value &value);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> dump_type_name(
		peff::Alloc *allocator,
		Writer *writer,
		const slake::TypeRef &type);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> dump_module_members(
		peff::Alloc *allocator,
		Writer *writer,
		slake::BasicModuleObject *mod);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> dump_module(
		peff::Alloc *allocator,
		Writer *writer,
		slake::ModuleObject *mod);

	class ExternalModuleProvider {
	public:
		const char *provider_name;

		SLKC_API ExternalModuleProvider(const char *provider_name);
		SLKC_API virtual ~ExternalModuleProvider();

		virtual peff::Option<CompilationError> load_module(CompileEnv *compile_env, IdRef *module_name) = 0;
	};

	class FileSystemExternalModuleProvider : public ExternalModuleProvider {
	public:
		peff::DynArray<peff::String> import_paths;

		SLKC_API FileSystemExternalModuleProvider(peff::Alloc *allocator);
		SLKC_API virtual ~FileSystemExternalModuleProvider();

		SLKC_API virtual peff::Option<CompilationError> load_module(CompileEnv *compile_env, IdRef *module_name) override;
		SLKC_API bool register_import_path(peff::String &&path);
	};

	extern size_t sz_default_parse_thread_stack;
	extern size_t sz_default_compile_thread_stack;
}

#endif
