#include "../compiler.h"
#include <climits>

using namespace slkc;

SLKC_API peff::Option<CompilationError> slkc::remove_ref_of_type(
	AstNodePtr<TypeNameNode> src,
	AstNodePtr<TypeNameNode> &type_name_out) {
	SLKC_RETURN_IF_COMP_ERROR(unwrap_facade_type_name(src, src));

	switch (src->type_name_kind) {
		case TypeNameKind::Ref:
			type_name_out = src.cast_to<RefTypeNameNode>()->referenced_type;
			break;
		default:
			type_name_out = src;
	}

	return {};
}

[[nodiscard]] SLKC_API peff::Option<CompilationError> slkc::remove_nullable_of_type(
	AstNodePtr<TypeNameNode> src,
	AstNodePtr<TypeNameNode> &type_name_out) {
	SLKC_RETURN_IF_COMP_ERROR(unwrap_facade_type_name(src, src));

	if (src->is_nullable) {
		auto new_node = src->duplicate<TypeNameNode>(src->self_allocator.get());
		if (!new_node)
			return gen_out_of_memory_comp_error();
		new_node->is_nullable = false;
		type_name_out = new_node;
	} else
		type_name_out = src;

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::is_lvalue_type(
	AstNodePtr<TypeNameNode> src,
	bool &whether_out) {
	SLKC_RETURN_IF_COMP_ERROR(unwrap_facade_type_name(src, src));

	if (!src) {
		whether_out = false;
		return {};
	}

	switch (src->type_name_kind) {
		case TypeNameKind::Ref:
			whether_out = true;
			break;
		default:
			whether_out = false;
			break;
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::is_same_type(
	AstNodePtr<TypeNameNode> lhs,
	AstNodePtr<TypeNameNode> rhs,
	bool &whether_out) {
	peff::SharedPtr<Document> document = lhs->document->shared_from_this();
	if (document != rhs->document->shared_from_this())
		std::terminate();

	SLKC_RETURN_IF_COMP_ERROR(unwrap_facade_type_name(lhs, lhs));
	SLKC_RETURN_IF_COMP_ERROR(unwrap_facade_type_name(rhs, rhs));

	if (lhs->type_name_kind != rhs->type_name_kind) {
		whether_out = false;
		return {};
	}

	if (lhs->is_final != rhs->is_final) {
		whether_out = false;
		return {};
	}

	if (lhs->is_nullable != rhs->is_nullable) {
		whether_out = false;
		return {};
	}

	switch (lhs->type_name_kind) {
		case TypeNameKind::Custom: {
			AstNodePtr<CustomTypeNameNode>
				converted_lhs = lhs.cast_to<CustomTypeNameNode>(),
				converted_rhs = rhs.cast_to<CustomTypeNameNode>();

			AstNodePtr<MemberNode> lhs_member, rhs_member;

			SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(nullptr, document, converted_lhs, lhs_member));
			SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(nullptr, document, converted_rhs, rhs_member));

			whether_out = lhs_member == rhs_member;
			break;
		}
		case TypeNameKind::Array: {
			AstNodePtr<ArrayTypeNameNode>
				converted_lhs = lhs.cast_to<ArrayTypeNameNode>(),
				converted_rhs = rhs.cast_to<ArrayTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(is_same_type(converted_lhs->element_type, converted_rhs->element_type, whether_out));
			break;
		}
		case TypeNameKind::Ref: {
			AstNodePtr<RefTypeNameNode>
				converted_lhs = lhs.cast_to<RefTypeNameNode>(),
				converted_rhs = rhs.cast_to<RefTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(is_same_type(converted_lhs->referenced_type, converted_rhs->referenced_type, whether_out));
			break;
		}
		default:
			whether_out = true;
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::get_type_promotion_level(
	AstNodePtr<TypeNameNode> type_name,
	int &level_out) {
	SLKC_RETURN_IF_COMP_ERROR(unwrap_facade_type_name(type_name, type_name));

	switch (type_name->type_name_kind) {
		case TypeNameKind::Bool:
			level_out = 1;
			break;
		case TypeNameKind::I8:
			level_out = 11;
			break;
		case TypeNameKind::I16:
			level_out = 12;
			break;
		case TypeNameKind::I32:
			level_out = 13;
			break;
		case TypeNameKind::I64:
			level_out = 14;
			break;
		case TypeNameKind::U8:
			level_out = 21;
			break;
		case TypeNameKind::U16:
			level_out = 22;
			break;
		case TypeNameKind::U32:
			level_out = 23;
			break;
		case TypeNameKind::U64:
			level_out = 24;
			break;
		case TypeNameKind::F32:
			level_out = 31;
			break;
		case TypeNameKind::F64:
			level_out = 32;
			break;
		case TypeNameKind::Any:
			level_out = INT_MAX - 1;
			break;
		default:
			level_out = INT_MAX;
			break;
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::determine_promotional_type(
	AstNodePtr<TypeNameNode> lhs,
	AstNodePtr<TypeNameNode> rhs,
	AstNodePtr<TypeNameNode> &type_name_out) {
	int lhs_weight, rhs_weight;

	if (!lhs) {
		type_name_out = rhs;
		return {};
	}

	if (!rhs) {
		type_name_out = lhs;
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(unwrap_facade_type_name(lhs, lhs));
	SLKC_RETURN_IF_COMP_ERROR(unwrap_facade_type_name(rhs, rhs));

	SLKC_RETURN_IF_COMP_ERROR(get_type_promotion_level(lhs, lhs_weight));
	SLKC_RETURN_IF_COMP_ERROR(get_type_promotion_level(rhs, rhs_weight));

	if (lhs_weight < rhs_weight) {
		type_name_out = rhs;
	} else if (lhs_weight > rhs_weight) {
		type_name_out = lhs;
	} else {
		switch (lhs->type_name_kind) {
			case TypeNameKind::Array: {
				switch (rhs->type_name_kind) {
					case TypeNameKind::Array: {
						AstNodePtr<ArrayTypeNameNode> lt = lhs.cast_to<ArrayTypeNameNode>(), rt = rhs.cast_to<ArrayTypeNameNode>();
						AstNodePtr<TypeNameNode> final_type;

						SLKC_RETURN_IF_COMP_ERROR(determine_promotional_type(lt->element_type, rt->element_type, final_type));

						type_name_out = final_type == rt->element_type ? rhs : lhs;
						break;
					}
					default:
						type_name_out = lhs;
						break;
				}
				break;
			}
			case TypeNameKind::Custom: {
				switch (rhs->type_name_kind) {
					case TypeNameKind::Custom: {
						AstNodePtr<CustomTypeNameNode> lt = lhs.cast_to<CustomTypeNameNode>(), rt = rhs.cast_to<CustomTypeNameNode>();

						bool b;

						SLKC_RETURN_IF_COMP_ERROR(is_convertible(rhs, lhs, true, b));
						if (b) {
							// In sealed context, derived types cannot be converted to base types,
							// therefore when rhs can be converted to lhs, rhs is the base type.
							type_name_out = rhs;
						} else {
							type_name_out = lhs;
						}
						break;
					}
					default:
						type_name_out = lhs;
						break;
				}
				break;
			}
			default:
				type_name_out = lhs;
		}
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::is_subtype_of(
	AstNodePtr<TypeNameNode> subtype,
	AstNodePtr<TypeNameNode> base_type,
	bool &result_out) {
	if (!subtype->is_local) {
		if (base_type->is_local) {
			result_out = false;
			return {};
		}
	}

recheck:
	switch (subtype->type_name_kind) {
		case TypeNameKind::Null:
			if (base_type->is_nullable)
				// Null is nullable.
				result_out = true;
			else
				switch (base_type->type_name_kind) {
					case TypeNameKind::Null:
						result_out = true;
						break;
					case TypeNameKind::Any:
						result_out = true;
						break;
					default:
						result_out = false;
						break;
				}
			break;
		case TypeNameKind::Void:
			result_out = false;
			break;
		case TypeNameKind::I8:
			switch (base_type->type_name_kind) {
				case TypeNameKind::I8:
					result_out = true;
					break;
				case TypeNameKind::Any:
					result_out = true;
					break;
				default:
					result_out = false;
					break;
			}
			break;
		case TypeNameKind::I16:
			switch (base_type->type_name_kind) {
				case TypeNameKind::I16:
					result_out = true;
					break;
				case TypeNameKind::Any:
					result_out = true;
					break;
				default:
					result_out = false;
					break;
			}
			break;
		case TypeNameKind::I32:
			switch (base_type->type_name_kind) {
				case TypeNameKind::I32:
					result_out = true;
					break;
				case TypeNameKind::Any:
					result_out = true;
					break;
				default:
					result_out = false;
					break;
			}
			break;
		case TypeNameKind::I64:
			switch (base_type->type_name_kind) {
				case TypeNameKind::I64:
					result_out = true;
					break;
				case TypeNameKind::Any:
					result_out = true;
					break;
				default:
					result_out = false;
					break;
			}
			break;
		case TypeNameKind::ISize:
			switch (base_type->type_name_kind) {
				case TypeNameKind::ISize:
					result_out = true;
					break;
				case TypeNameKind::Any:
					result_out = true;
					break;
				default:
					result_out = false;
					break;
			}
			break;
		case TypeNameKind::U8:
			switch (base_type->type_name_kind) {
				case TypeNameKind::U8:
					result_out = true;
					break;
				case TypeNameKind::Any:
					result_out = true;
					break;
				default:
					result_out = false;
					break;
			}
			break;
		case TypeNameKind::U16:
			switch (base_type->type_name_kind) {
				case TypeNameKind::U16:
					result_out = true;
					break;
				case TypeNameKind::Any:
					result_out = true;
					break;
				default:
					result_out = false;
					break;
			}
			break;
		case TypeNameKind::U32:
			switch (base_type->type_name_kind) {
				case TypeNameKind::U32:
					result_out = true;
					break;
				case TypeNameKind::Any:
					result_out = true;
					break;
				default:
					result_out = false;
					break;
			}
			break;
		case TypeNameKind::U64:
			switch (base_type->type_name_kind) {
				case TypeNameKind::U64:
					result_out = true;
					break;
				case TypeNameKind::Any:
					result_out = true;
					break;
				default:
					result_out = false;
					break;
			}
			break;
		case TypeNameKind::USize:
			switch (base_type->type_name_kind) {
				case TypeNameKind::USize:
					result_out = true;
					break;
				case TypeNameKind::Any:
					result_out = true;
					break;
				default:
					result_out = false;
					break;
			}
			break;
		case TypeNameKind::F32:
			switch (base_type->type_name_kind) {
				case TypeNameKind::F32:
					result_out = true;
					break;
				case TypeNameKind::Any:
					result_out = true;
					break;
				default:
					result_out = false;
					break;
			}
			break;
		case TypeNameKind::F64:
			switch (base_type->type_name_kind) {
				case TypeNameKind::F64:
					result_out = true;
					break;
				case TypeNameKind::Any:
					result_out = true;
					break;
				default:
					result_out = false;
					break;
			}
			break;
		case TypeNameKind::String:
			switch (base_type->type_name_kind) {
				case TypeNameKind::Object:
					result_out = true;
					break;
				case TypeNameKind::String:
					result_out = true;
					break;
				case TypeNameKind::Any:
					result_out = true;
					break;
				default:
					result_out = false;
					break;
			}
			break;
		case TypeNameKind::Bool:
			switch (base_type->type_name_kind) {
				case TypeNameKind::Bool:
					result_out = true;
					break;
				case TypeNameKind::Any:
					result_out = true;
					break;
				default:
					result_out = false;
					break;
			}
			break;
		case TypeNameKind::Object:
			switch (base_type->type_name_kind) {
				case TypeNameKind::Object:
					result_out = true;
					break;
				case TypeNameKind::Any:
					result_out = true;
					break;
				default:
					result_out = false;
					break;
			}
			break;
		case TypeNameKind::Any:
			switch (base_type->type_name_kind) {
				case TypeNameKind::Any:
					result_out = true;
					break;
				default:
					result_out = false;
					break;
			}
			break;
		case TypeNameKind::Ref: {
			switch (base_type->type_name_kind) {
				case TypeNameKind::Ref:
					SLKC_RETURN_IF_COMP_ERROR(is_same_type(subtype.cast_to<RefTypeNameNode>()->referenced_type, base_type.cast_to<RefTypeNameNode>()->referenced_type, result_out));
					// Nullability is invalid in this context.
					break;
				default:
					SLKC_RETURN_IF_COMP_ERROR(remove_ref_of_type(subtype, subtype));
					goto recheck;
			}
			break;
		}
		case TypeNameKind::Custom:
			switch (base_type->type_name_kind) {
				case TypeNameKind::Object: {
					AstNodePtr<MemberNode> stm;

					SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(nullptr, subtype->document->shared_from_this(), subtype.cast_to<CustomTypeNameNode>(), stm));

					if (stm->get_ast_node_type() == AstNodeType::Class)
						// class <: object
						result_out = true;
					else
						result_out = false;
					break;
				}
				case TypeNameKind::Custom: {
					AstNodePtr<MemberNode> subtype_member;  // Subtype member
					AstNodePtr<MemberNode> type_member;	   // Type member

					SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(nullptr, base_type->document->shared_from_this(), base_type.cast_to<CustomTypeNameNode>(), type_member));
					SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(nullptr, subtype->document->shared_from_this(), subtype.cast_to<CustomTypeNameNode>(), subtype_member));

					switch (type_member->get_ast_node_type()) {
						case AstNodeType::GenericParam:
							switch (subtype_member->get_ast_node_type()) {
								case AstNodeType::Class:
								case AstNodeType::Interface:
									// Nope - the generic parameter may not be exactly the base base_type of the subtype,
									// it may be more derived.
									result_out = false;
									break;
								case AstNodeType::GenericParam:
									// Generic parameters are always incompatible.
									result_out = false;
									break;
								default:
									result_out = false;
									break;
							}
							break;
						case AstNodeType::Class:
							switch (subtype_member->get_ast_node_type()) {
								case AstNodeType::Class:
									SLKC_RETURN_IF_COMP_ERROR(is_base_of(base_type->document->shared_from_this(), type_member.cast_to<ClassNode>(), subtype_member.cast_to<ClassNode>(), result_out));
									if (result_out) {
										// subtype <: base_type
										result_out = true;
									}
									break;
								case AstNodeType::GenericParam: {
									auto gp = subtype_member.cast_to<GenericParamNode>();
									AstNodePtr<MemberNode> gp_base_member;

									result_out = false;
									if (gp->generic_constraint) {
										if (gp->generic_constraint->base_type) {
											SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(nullptr, subtype->document->shared_from_this(), gp->generic_constraint->base_type.cast_to<CustomTypeNameNode>(), gp_base_member));

											if (gp_base_member->get_ast_node_type() == AstNodeType::Class) {
												// Only check if the base base_type name is not malformed.
												SLKC_RETURN_IF_COMP_ERROR(is_base_of(base_type->document->shared_from_this(), type_member.cast_to<ClassNode>(), gp_base_member.cast_to<ClassNode>(), result_out));
												if (result_out) {
													// subtype <: base_type
													result_out = true;
												}
											}
										}
									}
									break;
								}
								default:
									result_out = false;
									break;
							}
							break;
						case AstNodeType::Interface:
							switch (subtype_member->get_ast_node_type()) {
								case AstNodeType::Class:
									SLKC_RETURN_IF_COMP_ERROR(is_implemented_by_class(base_type->document->shared_from_this(), type_member.cast_to<InterfaceNode>(), subtype_member.cast_to<ClassNode>(), result_out));
									if (result_out) {
										// subtype <: base_type
										result_out = true;
									}
									break;
								case AstNodeType::Interface:
									SLKC_RETURN_IF_COMP_ERROR(is_implemented_by_interface(base_type->document->shared_from_this(), type_member.cast_to<InterfaceNode>(), subtype_member.cast_to<InterfaceNode>(), result_out));
									if (result_out) {
										// subtype <: base_type
										result_out = true;
									}
									break;
								case AstNodeType::Struct:
									// TODO: Process struct here...
									result_out = false;
									break;
								case AstNodeType::GenericParam: {
									auto gp = subtype_member.cast_to<GenericParamNode>();
									AstNodePtr<MemberNode> gp_base_member;

									result_out = false;
									if (gp->generic_constraint) {
										if (gp->generic_constraint->base_type) {
											SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(nullptr, subtype->document->shared_from_this(), gp->generic_constraint->base_type.cast_to<CustomTypeNameNode>(), gp_base_member));

											if (gp_base_member->get_ast_node_type() == AstNodeType::Class) {
												// Only check if the base_type name in the constraint is not malformed.
												SLKC_RETURN_IF_COMP_ERROR(is_implemented_by_class(base_type->document->shared_from_this(), type_member.cast_to<InterfaceNode>(), gp_base_member.cast_to<ClassNode>(), result_out));
												if (result_out) {
													// subtype <: base_type
													result_out = true;
												}
											}
										}
										if (!result_out)
											for (auto i : gp->generic_constraint->impl_types) {
												SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(nullptr, subtype->document->shared_from_this(), i.cast_to<CustomTypeNameNode>(), gp_base_member));

												if (gp_base_member->get_ast_node_type() == AstNodeType::Interface) {
													// Only check if the base_type name in the constraint is not malformed.
													SLKC_RETURN_IF_COMP_ERROR(is_implemented_by_interface(base_type->document->shared_from_this(), type_member.cast_to<InterfaceNode>(), gp_base_member.cast_to<InterfaceNode>(), result_out));
													if (result_out) {
														// subtype <: base_type
														result_out = true;
													}
												}

												if (result_out)
													break;
											}
									}
									break;
								}
							}
							break;
						default:
							result_out = false;
							break;
					}
					break;
				}
				case TypeNameKind::Any:
					result_out = true;
					break;
				default:
					result_out = false;
					break;
			}
			break;
		default:
			result_out = false;
			break;
	}

	if (result_out) {
		if (subtype->is_nullable) {
			if (base_type->is_nullable)
				// T? <: P?
				// true
				result_out = true;
			else
				// T? <: P
				// false
				result_out = false;
		} else {
			if (base_type->is_nullable)
				// T <: P?
				// true
				result_out = true;
			else
				// T <: P
				// true
				result_out = true;
		}
	}
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::is_unsigned(
	AstNodePtr<TypeNameNode> type,
	bool &result_out) {
	switch (type->type_name_kind) {
		case TypeNameKind::U8:
		case TypeNameKind::U16:
		case TypeNameKind::U32:
		case TypeNameKind::U64:
		case TypeNameKind::USize:
			result_out = true;
			break;
		default:
			result_out = false;
	}
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::is_class_type(
	AstNodePtr<TypeNameNode> type,
	bool &result_out) {
recurse:
	switch (type->type_name_kind) {
		case TypeNameKind::Object: {
			AstNodePtr<MemberNode> stm;

			SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(nullptr, type->document->shared_from_this(), type.cast_to<CustomTypeNameNode>(), stm));

			if (stm->get_ast_node_type() == AstNodeType::Class)
				result_out = true;
			else
				result_out = false;
			break;
		}
		case TypeNameKind::Custom: {
			AstNodePtr<MemberNode> tm;	// Type member

			SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(nullptr, type->document->shared_from_this(), type.cast_to<CustomTypeNameNode>(), tm));

			switch (tm->get_ast_node_type()) {
				case AstNodeType::GenericParam: {
					auto gp = tm.cast_to<GenericParamNode>();

					if (gp->generic_constraint->base_type) {
						// Tail recurse.
						type = gp->generic_constraint->base_type;
						goto recurse;
					}
					result_out = false;
					break;
				}
				case AstNodeType::Class:
					result_out = true;
					break;
				case AstNodeType::Interface:
					// TODO: Is Interface& a class type?
					result_out = true;
					break;
				default:
					result_out = false;
					break;
			}
			break;
		}
	}

	return {};
}

[[nodiscard]] SLKC_API peff::Option<CompilationError> slkc::to_signed(
	AstNodePtr<TypeNameNode> type,
	AstNodePtr<TypeNameNode> &type_name_out) {
	switch (type->type_name_kind) {
		case TypeNameKind::U8:
			if (!(type_name_out = make_ast_node<I8TypeNameNode>(
					  type->self_allocator.get(),
					  type->self_allocator.get(),
					  type->document->shared_from_this())
						.cast_to<TypeNameNode>()))
				return gen_out_of_memory_comp_error();
			break;
		case TypeNameKind::U16:
			if (!(type_name_out = make_ast_node<I16TypeNameNode>(
					  type->self_allocator.get(),
					  type->self_allocator.get(),
					  type->document->shared_from_this())
						.cast_to<TypeNameNode>()))
				return gen_out_of_memory_comp_error();
			break;
		case TypeNameKind::U32:
			if (!(type_name_out = make_ast_node<I32TypeNameNode>(
					  type->self_allocator.get(),
					  type->self_allocator.get(),
					  type->document->shared_from_this())
						.cast_to<TypeNameNode>()))
				return gen_out_of_memory_comp_error();
			break;
		case TypeNameKind::U64:
			if (!(type_name_out = make_ast_node<I64TypeNameNode>(
					  type->self_allocator.get(),
					  type->self_allocator.get(),
					  type->document->shared_from_this())
						.cast_to<TypeNameNode>()))
				return gen_out_of_memory_comp_error();
			break;
		case TypeNameKind::USize:
			if (!(type_name_out = make_ast_node<ISizeTypeNameNode>(
					  type->self_allocator.get(),
					  type->self_allocator.get(),
					  type->document->shared_from_this())
						.cast_to<TypeNameNode>()))
				return gen_out_of_memory_comp_error();
			break;
		default:
			type_name_out = {};
			break;
	}

	return {};
}

[[nodiscard]] SLKC_API peff::Option<CompilationError> slkc::to_unsigned(
	AstNodePtr<TypeNameNode> type,
	AstNodePtr<TypeNameNode> &type_name_out) {
	switch (type->type_name_kind) {
		case TypeNameKind::I8:
			if (!(type_name_out = make_ast_node<U8TypeNameNode>(
					  type->self_allocator.get(),
					  type->self_allocator.get(),
					  type->document->shared_from_this())
						.cast_to<TypeNameNode>()))
				return gen_out_of_memory_comp_error();
			break;
		case TypeNameKind::I16:
			if (!(type_name_out = make_ast_node<U16TypeNameNode>(
					  type->self_allocator.get(),
					  type->self_allocator.get(),
					  type->document->shared_from_this())
						.cast_to<TypeNameNode>()))
				return gen_out_of_memory_comp_error();
			break;
		case TypeNameKind::I32:
			if (!(type_name_out = make_ast_node<U32TypeNameNode>(
					  type->self_allocator.get(),
					  type->self_allocator.get(),
					  type->document->shared_from_this())
						.cast_to<TypeNameNode>()))
				return gen_out_of_memory_comp_error();
			break;
		case TypeNameKind::I64:
			if (!(type_name_out = make_ast_node<U64TypeNameNode>(
					  type->self_allocator.get(),
					  type->self_allocator.get(),
					  type->document->shared_from_this())
						.cast_to<TypeNameNode>()))
				return gen_out_of_memory_comp_error();
			break;
		case TypeNameKind::ISize:
			if (!(type_name_out = make_ast_node<USizeTypeNameNode>(
					  type->self_allocator.get(),
					  type->self_allocator.get(),
					  type->document->shared_from_this())
						.cast_to<TypeNameNode>()))
				return gen_out_of_memory_comp_error();
			break;
		default:
			type_name_out = {};
			break;
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::is_floating_point(
	AstNodePtr<TypeNameNode> type,
	bool &result_out) {
	switch (type->type_name_kind) {
		case TypeNameKind::F32:
		case TypeNameKind::F64:
			result_out = true;
			break;
		default:
			result_out = false;
	}
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::is_signed(
	AstNodePtr<TypeNameNode> type,
	bool &result_out) {
	switch (type->type_name_kind) {
		case TypeNameKind::I8:
		case TypeNameKind::I16:
		case TypeNameKind::I32:
		case TypeNameKind::I64:
		case TypeNameKind::ISize:
			result_out = true;
			break;
		default:
			result_out = false;
	}
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::is_integral(
	AstNodePtr<TypeNameNode> type,
	bool &result_out) {
	switch (type->type_name_kind) {
		case TypeNameKind::I8:
		case TypeNameKind::I16:
		case TypeNameKind::I32:
		case TypeNameKind::I64:
		case TypeNameKind::ISize:
		case TypeNameKind::U8:
		case TypeNameKind::U16:
		case TypeNameKind::U32:
		case TypeNameKind::U64:
		case TypeNameKind::USize:
			result_out = true;
			break;
		default:
			result_out = false;
	}
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::is_basic_type(
	AstNodePtr<TypeNameNode> type,
	bool &result_out) {
	switch (type->type_name_kind) {
		case TypeNameKind::Void:
		case TypeNameKind::I8:
		case TypeNameKind::I16:
		case TypeNameKind::I32:
		case TypeNameKind::I64:
		case TypeNameKind::ISize:
		case TypeNameKind::U8:
		case TypeNameKind::U16:
		case TypeNameKind::U32:
		case TypeNameKind::U64:
		case TypeNameKind::USize:
		case TypeNameKind::F32:
		case TypeNameKind::F64:
		case TypeNameKind::String:
		case TypeNameKind::Bool:
		case TypeNameKind::Object:
		case TypeNameKind::Any:
			result_out = true;
			break;
		default:
			result_out = false;
	}
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::is_scoped_enum_base_type(
	AstNodePtr<TypeNameNode> lhs,
	bool &result_out) {
	switch (lhs->type_name_kind) {
		case TypeNameKind::I8:
		case TypeNameKind::I16:
		case TypeNameKind::I32:
		case TypeNameKind::I64:
		case TypeNameKind::U8:
		case TypeNameKind::U16:
		case TypeNameKind::U32:
		case TypeNameKind::U64:
		case TypeNameKind::F32:
		case TypeNameKind::F64:
		case TypeNameKind::Bool:
			result_out = true;
			break;
		default:
			result_out = false;
	}
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::deduce_common_type(
	AstNodePtr<TypeNameNode> lhs,
	AstNodePtr<TypeNameNode> rhs,
	AstNodePtr<TypeNameNode> &type_name_out) {
reconvert: {
	bool is_same;
	SLKC_RETURN_IF_COMP_ERROR(is_same_type(lhs, rhs, is_same));
	if (is_same) {
		type_name_out = lhs;
		return {};
	}
}

	bool whether;
	SLKC_RETURN_IF_COMP_ERROR(is_subtype_of(lhs, rhs, whether));
	if (whether) {
		type_name_out = rhs;
		return {};
	}
	SLKC_RETURN_IF_COMP_ERROR(is_subtype_of(rhs, lhs, whether));
	if (whether) {
		type_name_out = lhs;
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(is_unsigned(lhs, whether));
	if (whether) {
		SLKC_RETURN_IF_COMP_ERROR(is_signed(rhs, whether));
		if (whether) {
			// l = unsigned , r = signed
			SLKC_RETURN_IF_COMP_ERROR(to_unsigned(rhs, rhs));
			goto reconvert;
		}
	} else {
		SLKC_RETURN_IF_COMP_ERROR(is_unsigned(rhs, whether));
		if (whether) {
			AstNodePtr<TypeNameNode> tmp;
			SLKC_RETURN_IF_COMP_ERROR(to_unsigned(lhs, lhs));
			if (tmp) {
				rhs = tmp;
				goto reconvert;
			}
		}
	}

	SLKC_RETURN_IF_COMP_ERROR(is_floating_point(lhs, whether));
	if (whether) {
		SLKC_RETURN_IF_COMP_ERROR(is_floating_point(rhs, whether));
		if (!whether) {
			// l = FP, r = non-FP
			switch (rhs->type_name_kind) {
				case TypeNameKind::I8:
				case TypeNameKind::I16:
				case TypeNameKind::I32:
				case TypeNameKind::U8:
				case TypeNameKind::U16:
				case TypeNameKind::U32:
					if (!(rhs = make_ast_node<F32TypeNameNode>(
							  rhs->self_allocator.get(),
							  rhs->self_allocator.get(),
							  rhs->document->shared_from_this())
								.cast_to<TypeNameNode>()))
						return gen_out_of_memory_comp_error();
					goto reconvert;
				case TypeNameKind::I64:
				case TypeNameKind::U64:
					if (!(rhs = make_ast_node<F64TypeNameNode>(
							  rhs->self_allocator.get(),
							  rhs->self_allocator.get(),
							  rhs->document->shared_from_this())
								.cast_to<TypeNameNode>()))
						return gen_out_of_memory_comp_error();
					goto reconvert;
			}
		}
		// l = FP, r = ??? where r is not unsigned nor signed.
	} else {
		SLKC_RETURN_IF_COMP_ERROR(is_floating_point(rhs, whether));
		if (whether) {
			// l = FP, r = non-FP
			switch (lhs->type_name_kind) {
				case TypeNameKind::I8:
				case TypeNameKind::I16:
				case TypeNameKind::I32:
				case TypeNameKind::U8:
				case TypeNameKind::U16:
				case TypeNameKind::U32:
					if (!(lhs = make_ast_node<F32TypeNameNode>(
							  lhs->self_allocator.get(),
							  lhs->self_allocator.get(),
							  lhs->document->shared_from_this())
								.cast_to<TypeNameNode>()))
						return gen_out_of_memory_comp_error();
					goto reconvert;
				case TypeNameKind::I64:
				case TypeNameKind::U64:
					if (!(lhs = make_ast_node<F64TypeNameNode>(
							  lhs->self_allocator.get(),
							  lhs->self_allocator.get(),
							  lhs->document->shared_from_this())
								.cast_to<TypeNameNode>()))
						return gen_out_of_memory_comp_error();
					goto reconvert;
			}
		}
		// l = ???, r = FP where r is not unsigned nor signed.
	}

	// Failed - I give up.
	type_name_out = {};
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::is_same_type_in_signature(
	AstNodePtr<TypeNameNode> lhs,
	AstNodePtr<TypeNameNode> rhs,
	bool &whether_out) {
	peff::SharedPtr<Document> document = lhs->document->shared_from_this();
	if (document != rhs->document->shared_from_this())
		std::terminate();

	SLKC_RETURN_IF_COMP_ERROR(unwrap_facade_type_name(lhs, lhs));
	SLKC_RETURN_IF_COMP_ERROR(unwrap_facade_type_name(rhs, rhs));

	if (lhs->type_name_kind != rhs->type_name_kind) {
		whether_out = false;
		return {};
	}

	switch (lhs->type_name_kind) {
		case TypeNameKind::Custom: {
			AstNodePtr<CustomTypeNameNode>
				converted_lhs = lhs.cast_to<CustomTypeNameNode>(),
				converted_rhs = rhs.cast_to<CustomTypeNameNode>();

			AstNodePtr<MemberNode> lhs_member, rhs_member;

			SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(nullptr, document, converted_lhs, lhs_member));
			SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(nullptr, document, converted_rhs, rhs_member));

			if ((!lhs_member) || (!rhs_member)) {
				if ((!lhs_member) != (!rhs_member)) {
					whether_out = false;
					break;
				} else {
					whether_out = true;
					break;
				}
			}

			if (lhs_member->get_ast_node_type() != rhs_member->get_ast_node_type()) {
				whether_out = false;
				break;
			}

			switch (lhs_member->get_ast_node_type()) {
				case AstNodeType::GenericParam: {
					// TODO: Lookup the generic parameters recursively for classes, interfaces
					// and functions.
					AstNodePtr<GenericParamNode> l, r;

					l = lhs_member.cast_to<GenericParamNode>();
					r = rhs_member.cast_to<GenericParamNode>();

					auto lp = l->parent,
						 rp = r->parent;

					if (lp->get_ast_node_type() != rp->get_ast_node_type()) {
						whether_out = false;
						break;
					}

					switch (lp->get_ast_node_type()) {
						case AstNodeType::Class: {
							if (lp != rp) {
								whether_out = false;
								break;
							}

							if (((ClassNode *)lp)->generic_param_indices.at(l->name) ==
								((ClassNode *)rp)->generic_param_indices.at(r->name)) {
								whether_out = true;
								break;
							} else {
								whether_out = false;
								break;
							}
							break;
						}
						case AstNodeType::Interface: {
							if (lp != rp) {
								whether_out = false;
								break;
							}

							if (((InterfaceNode *)lp)->generic_param_indices.at(l->name) ==
								((InterfaceNode *)rp)->generic_param_indices.at(r->name)) {
								whether_out = true;
								break;
							} else {
								whether_out = false;
								break;
							}
							break;
						}
						case AstNodeType::FnOverloading: {
							auto lit = ((FnOverloadingNode *)lp)->generic_param_indices.find(l->name),
								 rit = ((FnOverloadingNode *)rp)->generic_param_indices.find(r->name);

							assert((lit != ((FnOverloadingNode *)lp)->generic_param_indices.end()) &&
								   (rit != ((FnOverloadingNode *)rp)->generic_param_indices.end()));

							if (*lit == *rit) {
								whether_out = true;
								break;
							} else {
								whether_out = false;
								break;
							}
							break;
						}
					}
					break;
				}
				default:
					whether_out = lhs_member == rhs_member;
					break;
			}
			break;
		}
		default:
			SLKC_RETURN_IF_COMP_ERROR(is_same_type(lhs, rhs, whether_out));
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::is_convertible(
	AstNodePtr<TypeNameNode> src,
	AstNodePtr<TypeNameNode> dest,
	bool is_sealed,
	bool &result_out) {
	peff::SharedPtr<Document> document = src->document->shared_from_this();
	if (document != dest->document->shared_from_this())
		std::terminate();

	SLKC_RETURN_IF_COMP_ERROR(unwrap_facade_type_name(src, src));
	SLKC_RETURN_IF_COMP_ERROR(unwrap_facade_type_name(dest, dest));

recheck:
	if (!dest->is_nullable) {
		if (src->is_nullable) {
			// T? to T should be eliminated by null check expressions.
			result_out = false;
			return {};
		}
	}

	if (!dest->is_local) {
		if (src->is_local) {
			result_out = false;
			return {};
		}
	}
	if (dest->is_final)
		is_sealed = true;

	// We only allow T to T? or null to T? below.
	// T? to T is handled above.
	switch (dest->type_name_kind) {
		case TypeNameKind::Void:
			result_out = false;
			return {};
		case TypeNameKind::I8:
		case TypeNameKind::I16:
		case TypeNameKind::I32:
		case TypeNameKind::I64:
		case TypeNameKind::ISize:
		case TypeNameKind::U8:
		case TypeNameKind::U16:
		case TypeNameKind::U32:
		case TypeNameKind::U64:
		case TypeNameKind::USize:
			if (dest->is_nullable) {
				result_out = (src->type_name_kind == TypeNameKind::Null) || (src->type_name_kind == dest->type_name_kind);
			} else {
				assert(!src->is_nullable);
				switch (src->type_name_kind) {
					case TypeNameKind::I8:
					case TypeNameKind::I16:
					case TypeNameKind::I32:
					case TypeNameKind::I64:
					case TypeNameKind::ISize:
					case TypeNameKind::U8:
					case TypeNameKind::U16:
					case TypeNameKind::U32:
					case TypeNameKind::U64:
					case TypeNameKind::USize:
					case TypeNameKind::F32:
					case TypeNameKind::F64:
					case TypeNameKind::Bool:
					case TypeNameKind::Any:
						result_out = true;
						break;
					case TypeNameKind::Ref:
						SLKC_RETURN_IF_COMP_ERROR(remove_ref_of_type(src, src));
						goto recheck;
					default:
						result_out = false;
						break;
				}
			}
			return {};
		case TypeNameKind::F32:
		case TypeNameKind::F64:
			if (dest->is_nullable) {
				result_out = (src->type_name_kind == TypeNameKind::Null) || (src->type_name_kind == dest->type_name_kind);
			} else {
				switch (src->type_name_kind) {
					case TypeNameKind::I8:
					case TypeNameKind::I16:
					case TypeNameKind::I32:
					case TypeNameKind::I64:
					case TypeNameKind::ISize:
					case TypeNameKind::U8:
					case TypeNameKind::U16:
					case TypeNameKind::U32:
					case TypeNameKind::U64:
					case TypeNameKind::USize:
					case TypeNameKind::F32:
					case TypeNameKind::F64:
					case TypeNameKind::Any:
						result_out = true;
						break;
					case TypeNameKind::Ref:
						SLKC_RETURN_IF_COMP_ERROR(remove_ref_of_type(src, src));
						goto recheck;
					default:
						result_out = false;
						break;
				}
			}
			return {};
		case TypeNameKind::Bool:
			if (dest->is_nullable) {
				result_out = (src->type_name_kind == TypeNameKind::Null) || (src->type_name_kind == dest->type_name_kind);
			} else {
				switch (src->type_name_kind) {
					case TypeNameKind::I8:
					case TypeNameKind::I16:
					case TypeNameKind::I32:
					case TypeNameKind::I64:
					case TypeNameKind::ISize:
					case TypeNameKind::U8:
					case TypeNameKind::U16:
					case TypeNameKind::U32:
					case TypeNameKind::U64:
					case TypeNameKind::USize:
					case TypeNameKind::Bool:
					case TypeNameKind::Any:
						result_out = true;
						break;
					case TypeNameKind::Ref:
						SLKC_RETURN_IF_COMP_ERROR(remove_ref_of_type(src, src));
						goto recheck;
					default:
						result_out = false;
						break;
				}
			}
			return {};
		case TypeNameKind::Any:
			switch (src->type_name_kind) {
				case TypeNameKind::Ref:
					SLKC_RETURN_IF_COMP_ERROR(remove_ref_of_type(src, src));
					goto recheck;
				default:
					result_out = true;
					break;
			}
			return {};
		case TypeNameKind::Ref:
			switch (src->type_name_kind) {
				case TypeNameKind::Ref:
					SLKC_RETURN_IF_COMP_ERROR(is_same_type(dest.cast_to<RefTypeNameNode>()->referenced_type, src.cast_to<RefTypeNameNode>()->referenced_type, result_out));
					break;
				default:
					result_out = false;
					break;
			}
			return {};
	}

	if (result_out)
		return {};

	SLKC_RETURN_IF_COMP_ERROR(is_subtype_of(src, dest, result_out));

	if (result_out) {
		return {};
	}

	if (!is_sealed) {
		SLKC_RETURN_IF_COMP_ERROR(is_subtype_of(dest, src, result_out));
		return {};
	}

	result_out = false;
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::_is_type_name_param_list_type_name_tree(
	AstNodePtr<TypeNameNode> type,
	bool &whether_out) {
	if (!type) {
		whether_out = false;
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(unwrap_facade_type_name(type, type));

	switch (type->type_name_kind) {
		case TypeNameKind::Unpacking: {
			whether_out = true;
			break;
		}
		case TypeNameKind::Array: {
			auto t = type.cast_to<ArrayTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(_is_type_name_param_list_type_name_tree(t->element_type, whether_out));
			if (whether_out)
				return {};

			break;
		}
		case TypeNameKind::Ref: {
			auto t = type.cast_to<RefTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(_is_type_name_param_list_type_name_tree(t->referenced_type, whether_out));
			if (whether_out)
				return {};

			break;
		}
		case TypeNameKind::TempRef: {
			auto t = type.cast_to<TempRefTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(_is_type_name_param_list_type_name_tree(t->referenced_type, whether_out));
			if (whether_out)
				return {};

			break;
		}
		case TypeNameKind::Fn: {
			auto t = type.cast_to<FnTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(_is_type_name_param_list_type_name_tree(t->return_type, whether_out));
			if (whether_out)
				return {};

			for (auto &i : t->param_types) {
				SLKC_RETURN_IF_COMP_ERROR(_is_type_name_param_list_type_name_tree(i, whether_out));
				if (whether_out)
					return {};
			}

			SLKC_RETURN_IF_COMP_ERROR(_is_type_name_param_list_type_name_tree(t->this_type, whether_out));
			if (whether_out)
				return {};

			break;
		}
		default:
			break;
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::_do_expand_param_list_type_name_tree(
	AstNodePtr<TypeNameNode> &type) {
	if (!type) {
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(unwrap_facade_type_name(type, type));

	switch (type->type_name_kind) {
		case TypeNameKind::Unpacking: {
			SLKC_RETURN_IF_COMP_ERROR(get_unpacked_type_of(type, type));
			break;
		}
		case TypeNameKind::Array: {
			auto t = type.cast_to<ArrayTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(_do_expand_param_list_type_name_tree(t->element_type));

			break;
		}
		case TypeNameKind::Ref: {
			auto t = type.cast_to<RefTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(_do_expand_param_list_type_name_tree(t->referenced_type));

			break;
		}
		case TypeNameKind::TempRef: {
			auto t = type.cast_to<TempRefTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(_do_expand_param_list_type_name_tree(t->referenced_type));

			break;
		}
		case TypeNameKind::Fn: {
			auto t = type.cast_to<FnTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(_do_expand_param_list_type_name_tree(t->return_type));

			for (auto &i : t->param_types) {
				SLKC_RETURN_IF_COMP_ERROR(_do_expand_param_list_type_name_tree(i));
			}

			SLKC_RETURN_IF_COMP_ERROR(_do_expand_param_list_type_name_tree(t->this_type));

			break;
		}
		default:
			break;
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::unwrap_param_list_type_name_tree(
	AstNodePtr<TypeNameNode> type,
	peff::Alloc *allocator,
	AstNodePtr<TypeNameNode> &type_name_out) {
	bool b;

	SLKC_RETURN_IF_COMP_ERROR(_is_type_name_param_list_type_name_tree(type, b));

	if (!b) {
		type_name_out = type;
		return {};
	}

	AstNodePtr<TypeNameNode> duplicated_type = type->duplicate<TypeNameNode>(allocator);

	if (!duplicated_type) {
		return gen_out_of_memory_comp_error();
	}

	SLKC_RETURN_IF_COMP_ERROR(_do_expand_param_list_type_name_tree(duplicated_type));

	type_name_out = duplicated_type;

	return {};
}

[[nodiscard]] SLKC_API peff::Option<CompilationError> slkc::unwrap_facade_type_name(
	AstNodePtr<TypeNameNode> type,
	AstNodePtr<TypeNameNode> &type_name_out) {
	if (!type) {
		type_name_out = type;
		return {};
	}
	switch (type->type_name_kind) {
		case TypeNameKind::Custom: {
			AstNodePtr<TypeNameNode> t;

			SLKC_RETURN_IF_COMP_ERROR(resolve_base_overriden_custom_type_name(type->document->shared_from_this(), type.cast_to<CustomTypeNameNode>(), t));

			if (t) {
				type_name_out = t;
				return {};
			}
			break;
		}
		default:
			break;
	}

	type_name_out = type;
	if (type->is_nullable)
		type_name_out->set_nullable();
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::get_unpacked_type_of(
	AstNodePtr<TypeNameNode> type,
	AstNodePtr<TypeNameNode> &type_name_out) {
	peff::SharedPtr<Document> document = type->document->shared_from_this();

	SLKC_RETURN_IF_COMP_ERROR(unwrap_facade_type_name(type, type));
	switch (type->type_name_kind) {
		case TypeNameKind::Custom: {
			AstNodePtr<MemberNode> m;

			SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(nullptr, document, type.cast_to<CustomTypeNameNode>(), m));

			if (!m) {
				return CompilationError(type->token_range, CompilationErrorKind::IdNotFound);
			}

			switch (m->get_ast_node_type()) {
				case AstNodeType::GenericParam: {
					auto p = m.cast_to<GenericParamNode>();

					if (p->is_param_type_list) {
						AstNodePtr<UnpackedParamsTypeNameNode> unpacked_type;

						if (!(unpacked_type = make_ast_node<UnpackedParamsTypeNameNode>(document->allocator.get(), document->allocator.get(), document))) {
							return gen_out_of_memory_comp_error();
						}

						if (p->param_type_list_generic_constraint) {
							if (!unpacked_type->param_types.resize(p->param_type_list_generic_constraint->arg_types.size())) {
								return gen_out_of_memory_comp_error();
							}

							for (size_t i = 0; i < unpacked_type->param_types.size(); ++i) {
								unpacked_type->param_types.at(i) = p->param_type_list_generic_constraint->arg_types.at(i);
							}
						}

						if (p->param_type_list_generic_constraint) {
							unpacked_type->has_var_args = p->param_type_list_generic_constraint->has_var_arg;
						}

						type_name_out = unpacked_type.cast_to<TypeNameNode>();
					} else {
						type_name_out = {};
					}
					break;
				}
				default:
					type_name_out = {};
			}
			break;
		}
		case TypeNameKind::ParamTypeList: {
			auto t = type.cast_to<ParamTypeListTypeNameNode>();

			AstNodePtr<UnpackedParamsTypeNameNode> unpacked_type;

			if (!(unpacked_type = make_ast_node<UnpackedParamsTypeNameNode>(document->allocator.get(), document->allocator.get(), document))) {
				return gen_out_of_memory_comp_error();
			}

			if (!unpacked_type->param_types.resize(t->param_types.size())) {
				return gen_out_of_memory_comp_error();
			}

			for (size_t i = 0; i < unpacked_type->param_types.size(); ++i) {
				unpacked_type->param_types.at(i) = t->param_types.at(i);
			}

			unpacked_type->has_var_args = t->has_var_args;

			type_name_out = unpacked_type.cast_to<TypeNameNode>();
			break;
		}
		case TypeNameKind::UnpackedParams: {
			auto t = type.cast_to<UnpackedParamsTypeNameNode>();

			AstNodePtr<UnpackedArgsTypeNameNode> unpacked_type;

			if (!(unpacked_type = make_ast_node<UnpackedArgsTypeNameNode>(document->allocator.get(), document->allocator.get(), document))) {
				return gen_out_of_memory_comp_error();
			}

			if (!unpacked_type->param_types.resize(t->param_types.size())) {
				return gen_out_of_memory_comp_error();
			}

			for (size_t i = 0; i < unpacked_type->param_types.size(); ++i) {
				unpacked_type->param_types.at(i) = t->param_types.at(i);
			}

			unpacked_type->has_var_args = t->has_var_args;

			type_name_out = unpacked_type.cast_to<TypeNameNode>();

			break;
		}
		case TypeNameKind::Unpacking: {
			SLKC_RETURN_IF_COMP_ERROR(get_unpacked_type_of(type.cast_to<UnpackingTypeNameNode>()->inner_type_name, type_name_out));

			break;
		}
		default:
			type_name_out = {};
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::fn_to_type_name(
	CompileEnv *compile_env,
	AstNodePtr<FnOverloadingNode> fn,
	AstNodePtr<FnTypeNameNode> &evaluated_type_out) {
	AstNodePtr<FnTypeNameNode> tn;

	if (!(tn = make_ast_node<FnTypeNameNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->document))) {
		return gen_out_of_memory_comp_error();
	}

	if (!tn->param_types.resize(fn->params.size())) {
		return gen_out_of_memory_comp_error();
	}

	for (size_t i = 0; i < tn->param_types.size(); ++i) {
		tn->param_types.at(i) = fn->params.at(i)->type;
	}

	if (fn->fn_flags & FN_VARG) {
		tn->has_var_args = true;
	}

	tn->return_type = fn->return_type;

	if (!(fn->access_modifier & slake::ACCESS_STATIC)) {
		if (fn->parent && fn->parent->parent) {
			switch (fn->parent->parent->get_ast_node_type()) {
				case AstNodeType::Class:
				case AstNodeType::Interface: {
					IdRefPtr full_id_ref;

					SLKC_RETURN_IF_COMP_ERROR(get_full_id_ref(compile_env->allocator.get(), fn->parent->parent->shared_from_this().cast_to<MemberNode>(), full_id_ref));

					auto this_type = make_ast_node<CustomTypeNameNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->document);

					if (!this_type) {
						return gen_out_of_memory_comp_error();
					}
					this_type->context_node = to_weak_ptr(compile_env->document->root_module.cast_to<MemberNode>());

					this_type->id_ref_ptr = std::move(full_id_ref);

					tn->this_type = this_type.cast_to<TypeNameNode>();
					break;
				}
				default:
					break;
			}
		}
	}

	evaluated_type_out = tn;

	return {};
}

SLKC_API peff::Option<slkc::CompilationError> slkc::type_name_cmp(AstNodePtr<TypeNameNode> lhs, AstNodePtr<TypeNameNode> rhs, int &out) noexcept {
	peff::SharedPtr<Document> doc = lhs->document->shared_from_this();

	if (doc != rhs->document->shared_from_this())
		std::terminate();

	if (((uint8_t)lhs->type_name_kind) < ((uint8_t)rhs->type_name_kind)) {
		out = -1;
		return {};
	}
	if (((uint8_t)lhs->type_name_kind) > ((uint8_t)rhs->type_name_kind)) {
		out = 1;
		return {};
	}
	if (((uint8_t)lhs->is_final) < ((uint8_t)rhs->is_final)) {
		out = -1;
		return {};
	}
	if (((uint8_t)lhs->is_final) > ((uint8_t)rhs->is_final)) {
		out = 1;
		return {};
	}
	if (((uint8_t)lhs->is_local) < ((uint8_t)rhs->is_local)) {
		out = -1;
		return {};
	}
	if (((uint8_t)lhs->is_local) > ((uint8_t)rhs->is_local)) {
		out = 1;
		return {};
	}
	if (((uint8_t)lhs->is_nullable) < ((uint8_t)rhs->is_nullable)) {
		out = -1;
		return {};
	}
	if (((uint8_t)lhs->is_nullable) > ((uint8_t)rhs->is_nullable)) {
		out = 1;
		return {};
	}
	switch (lhs->type_name_kind) {
		case TypeNameKind::Custom: {
			AstNodePtr<CustomTypeNameNode>
				l = lhs.cast_to<CustomTypeNameNode>(),
				r = rhs.cast_to<CustomTypeNameNode>();

			AstNodePtr<MemberNode>
				lm,
				rm;

			SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(nullptr, doc, l, lm));
			SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(nullptr, doc, r, rm));

			if (!lm) {
				if (rm) {
					// [Bad type] > [Regular custom type]
					out = 1;
					return {};
				}
				// [Bad type] == [Bad type]
				out = 0;
				return {};
			}
			if (!rm) {
				out = -1;
				return {};
			}
			if (lm < rm) {
				out = -1;
			} else if (lm > rm) {
				out = 1;
			} else {
				out = 0;
			}
			return {};
		}
		case TypeNameKind::Array: {
			return type_name_cmp(
				lhs.cast_to<ArrayTypeNameNode>()->element_type,
				rhs.cast_to<ArrayTypeNameNode>()->element_type,
				out);
		}
		case TypeNameKind::Ref: {
			return type_name_cmp(
				lhs.cast_to<RefTypeNameNode>()->referenced_type,
				rhs.cast_to<RefTypeNameNode>()->referenced_type,
				out);
		}
		default:
			out = 0;
			return {};
	}

	std::terminate();
}

SLKC_API peff::Option<slkc::CompilationError> slkc::type_name_list_cmp(const peff::DynArray<AstNodePtr<TypeNameNode>> &lhs, const peff::DynArray<AstNodePtr<TypeNameNode>> &rhs, int &out) noexcept {
	if (lhs.size() < rhs.size()) {
		out = -1;
		return {};
	}
	if (lhs.size() > rhs.size()) {
		out = 1;
		return {};
	}
	for (size_t i = 0; i < lhs.size(); ++i) {
		SLKC_RETURN_IF_COMP_ERROR(type_name_cmp(lhs.at(i), rhs.at(i), out));

		if (out != 0) {
			return {};
		}
	}

	out = 0;
	return {};
}
