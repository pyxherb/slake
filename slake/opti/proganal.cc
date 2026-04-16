#include "proganal.h"

using namespace slake;
using namespace slake::opti;

bool opti::is_ins_has_side_effect(Opcode opcode) {
	switch (opcode) {
		case Opcode::STORE:
		case Opcode::LVAR:
		case Opcode::ALLOCA:
		case Opcode::ENTER:
		case Opcode::LEAVE:
		case Opcode::JMP:
		case Opcode::BR:
		case Opcode::PUSHARG:
		case Opcode::PUSHAP:
		case Opcode::CALL:
		case Opcode::MCALL:
		case Opcode::CTORCALL:
		case Opcode::RET:
		case Opcode::COCALL:
		case Opcode::COMCALL:
		case Opcode::YIELD:
		case Opcode::RESUME:
		case Opcode::NEW:
		case Opcode::ARRNEW:
		case Opcode::THROW:
		case Opcode::PUSHEH:
			return true;
		default:
			break;
	}
	return false;
}

bool opti::is_ins_simplifiable(Opcode opcode) {
	if (!is_ins_has_side_effect(opcode))
		return true;

	switch (opcode) {
		case Opcode::COCALL:
		case Opcode::COMCALL:
			return true;
		default:
			break;
	}
	return false;
}

void slake::opti::mark_reg_as_for_output(ProgramAnalyzeContext &analyze_context, uint32_t i) {
	switch (auto &reg_info = analyze_context.analyzed_info_out.analyzed_reg_info.at(i); reg_info.storage_type) {
		case opti::RegStorageType::None:
			break;
		case opti::RegStorageType::FieldVar:
			reg_info.storage_info.as_field_var.is_used_for_output = true;
			break;
		case opti::RegStorageType::LocalVar:
			reg_info.storage_info.as_local_var.is_used_for_output = true;
			break;
		case opti::RegStorageType::ArgRef:
			reg_info.storage_info.as_arg_ref.is_used_for_output = true;
			break;
	}
}

InternalExceptionPointer slake::opti::wrap_into_heap_type(
	Runtime *runtime,
	TypeRef type,
	HostRefHolder &host_ref_holder,
	HeapTypeObject *&heap_type_out) {
	HostObjectRef<HeapTypeObject> heap_type = HeapTypeObject::alloc(runtime);
	if (!heap_type)
		return OutOfMemoryError::alloc();
	heap_type->type_ref = type;

	if (!host_ref_holder.add_object(heap_type.get()))
		return OutOfMemoryError::alloc();
	heap_type_out = heap_type.get();

	return {};
}

InternalExceptionPointer slake::opti::wrap_into_ref_type(
	Runtime *runtime,
	TypeRef type,
	HostRefHolder &host_ref_holder,
	TypeRef &type_out) {
	HostObjectRef<RefTypeDefObject> type_def = RefTypeDefObject::alloc(runtime);
	if (!type_def)
		return OutOfMemoryError::alloc();
	HostObjectRef<HeapTypeObject> heap_type = HeapTypeObject::alloc(runtime);
	if (!heap_type)
		return OutOfMemoryError::alloc();

	heap_type->type_ref = type;
	type_def->referenced_type = heap_type.get();

	if (auto td = runtime->get_equal_type_def(type_def.get()); td) {
		type_out = TypeRef(TypeId::Ref, td);
	} else {
		if (!host_ref_holder.add_object(type_def.get()))
			return OutOfMemoryError::alloc();
		type_out = TypeRef(TypeId::Ref, type_def.get());
	}

	return {};
}

InternalExceptionPointer slake::opti::wrap_into_array_type(
	Runtime *runtime,
	TypeRef type,
	HostRefHolder &host_ref_holder,
	TypeRef &type_out) {
	HostObjectRef<ArrayTypeDefObject> type_def = ArrayTypeDefObject::alloc(runtime);
	if (!type_def)
		return OutOfMemoryError::alloc();
	HostObjectRef<HeapTypeObject> heap_type = HeapTypeObject::alloc(runtime);
	if (!heap_type)
		return OutOfMemoryError::alloc();

	heap_type->type_ref = type;
	type_def->element_type = heap_type.get();

	if (auto td = runtime->get_equal_type_def(type_def.get()); td) {
		type_out = TypeRef(TypeId::Array, td);
	} else {
		if (!host_ref_holder.add_object(type_def.get()))
			return OutOfMemoryError::alloc();
		type_out = TypeRef(TypeId::Array, type_def.get());
	}

	return {};
}

InternalExceptionPointer slake::opti::eval_object_type(
	ProgramAnalyzeContext &analyze_context,
	const Reference &entity_ref,
	TypeRef &type_out) {
	switch (entity_ref.kind) {
		case ReferenceKind::StaticFieldRef:
		case ReferenceKind::ArrayElementRef:
		case ReferenceKind::LocalVarRef:
		case ReferenceKind::ArgRef:
		case ReferenceKind::ObjectFieldRef: {
			TypeRef var_type = Runtime::typeof_var(entity_ref);

			SLAKE_RETURN_IF_EXCEPT(
				wrap_into_ref_type(
					analyze_context.runtime,
					var_type,
					analyze_context.host_ref_holder,
					type_out));
			break;
		}
		case ReferenceKind::ObjectRef: {
			Object *object = entity_ref.as_object;
			switch (object->get_object_kind()) {
				case ObjectKind::String: {
					type_out = TypeRef(TypeId::String);
					break;
				}
				case ObjectKind::Array: {
					SLAKE_RETURN_IF_EXCEPT(wrap_into_array_type(
						analyze_context.runtime,
						((ArrayObject *)object)->element_type,
						analyze_context.host_ref_holder,
						type_out));
					break;
				}
				case ObjectKind::FnOverloading: {
					FnOverloadingObject *fn_overloading_object = (FnOverloadingObject *)object;

					auto type_def = FnTypeDefObject::alloc(
						analyze_context.runtime);

					SLAKE_RETURN_IF_EXCEPT(wrap_into_heap_type(analyze_context.runtime, fn_overloading_object->return_type, analyze_context.host_ref_holder, type_def->return_type));

					if (!type_def->param_types.resize(fn_overloading_object->param_types.size())) {
						return OutOfMemoryError::alloc();
					}

					for (size_t i = 0; i < fn_overloading_object->param_types.size(); ++i) {
						SLAKE_RETURN_IF_EXCEPT(wrap_into_heap_type(analyze_context.runtime, TypeId::Void, analyze_context.host_ref_holder, type_def->return_type));
					}

					for (size_t i = 0; i < fn_overloading_object->param_types.size(); ++i) {
						SLAKE_RETURN_IF_EXCEPT(wrap_into_heap_type(analyze_context.runtime, fn_overloading_object->param_types.at(i), analyze_context.host_ref_holder, type_def->param_types.at(i)));
					}

					if (!analyze_context.host_ref_holder.add_object(type_def.get()))
						return OutOfMemoryError::alloc();
					type_out = TypeRef(TypeId::Fn, type_def.get());
					break;
				}
				default:
					return alloc_oom_error_if_alloc_failed(
						ErrorEvaluatingObjectTypeError::alloc(
							object->associated_runtime->get_fixed_alloc(),
							object));
			}
			break;
		}
	}

	return {};
}

InternalExceptionPointer slake::opti::eval_value_type(
	ProgramAnalyzeContext &analyze_context,
	const Value &value,
	TypeRef &type_out) {
	switch (value.value_type) {
		case ValueType::I8:
			type_out = TypeId::I8;
			break;
		case ValueType::I16:
			type_out = TypeId::I16;
			break;
		case ValueType::I32:
			type_out = TypeId::I32;
			break;
		case ValueType::I64:
			type_out = TypeId::I64;
			break;
		case ValueType::U8:
			type_out = TypeId::U8;
			break;
		case ValueType::U16:
			type_out = TypeId::U16;
			break;
		case ValueType::U32:
			type_out = TypeId::U32;
			break;
		case ValueType::U64:
			type_out = TypeId::U64;
			break;
		case ValueType::F32:
			type_out = TypeId::F32;
			break;
		case ValueType::F64:
			type_out = TypeId::F64;
			break;
		case ValueType::Bool:
			type_out = TypeId::Bool;
			break;
		case ValueType::Reference: {
			const Reference &entity_ref = value.get_reference();

			SLAKE_RETURN_IF_EXCEPT(eval_object_type(analyze_context, entity_ref, type_out));
			break;
		}
		case ValueType::RegIndex: {
			uint32_t reg_index = value.get_reg_index();

			if (!analyze_context.analyzed_info_out.analyzed_reg_info.contains(reg_index)) {
				return alloc_oom_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			type_out = analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).type;
			break;
		}
		default: {
			return alloc_oom_error_if_alloc_failed(
				MalformedProgramError::alloc(
					analyze_context.runtime->get_fixed_alloc(),
					analyze_context.fn_object,
					analyze_context.idx_cur_ins));
		}
	}
	return {};
}

InternalExceptionPointer slake::opti::eval_const_value(
	ProgramAnalyzeContext &analyze_context,
	const Value &value,
	Value &const_value_out) {
	switch (value.value_type) {
		case ValueType::I8:
		case ValueType::I16:
		case ValueType::I32:
		case ValueType::I64:
		case ValueType::U8:
		case ValueType::U16:
		case ValueType::U32:
		case ValueType::U64:
		case ValueType::F32:
		case ValueType::F64:
		case ValueType::Bool:
			const_value_out = value;
			break;
		case ValueType::RegIndex: {
			uint32_t idx_reg = value.get_reg_index();

			if (!analyze_context.analyzed_info_out.analyzed_reg_info.contains(idx_reg)) {
				return alloc_oom_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			const_value_out = analyze_context.analyzed_info_out.analyzed_reg_info.at(idx_reg).expected_value;
			break;
		} /*
		case ValueType::VarRef: {
			VarRef var_ref = value.get_var_ref();

			SLAKE_RETURN_IF_EXCEPT(var_ref.var_ptr->get_data(var_ref.context, const_value_out));
			break;
		}*/
		default:
			const_value_out = Value(ValueType::Undefined);
	}
	return {};
}

InternalExceptionPointer slake::opti::analyze_program_info_pass(
	Runtime *runtime,
	peff::Alloc *resource_allocator,
	RegularFnOverloadingObject *fn_object,
	ProgramAnalyzedInfo &analyzed_info_out,
	HostRefHolder &host_ref_holder) {
	if (fn_object->instructions.size() > UINT32_MAX) {
		// TODO: Deal with this case elegently.
		std::terminate();
	}
	uint32_t num_ins = (uint32_t)fn_object->instructions.size();
	analyzed_info_out.context_object = ContextObject::alloc(runtime, SLAKE_STACK_SIZE_MAX);
	{
		SLAKE_RETURN_IF_EXCEPT(runtime->_create_new_major_frame(analyzed_info_out.context_object.get(), nullptr, nullptr, nullptr, SIZE_MAX, 0, UINT32_MAX, nullptr));
	}

	ProgramAnalyzeContext analyze_context = {
		runtime,
		resource_allocator,
		fn_object,
		analyzed_info_out,
		host_ref_holder
	};

	//if (!pseudo_major_frame->resumable_context_data.arg_stack.resize_uninit(fn_object->param_types.size()))
	//	return OutOfMemoryError::alloc();
	//for (size_t i = 0; i < fn_object->param_types.size(); ++i)
	//	pseudo_major_frame->resumable_context_data.arg_stack.at(i) = Value();

	// Analyze lifetime of virtual registers.
	bool new_expectable_reg_found;
	auto set_expected_value = [&analyzed_info_out, &new_expectable_reg_found](uint32_t reg_index, const Value &value) {
		analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = value;
		new_expectable_reg_found = true;
	};
	do {
		new_expectable_reg_found = false;
		for (uint32_t &i = analyze_context.idx_cur_ins; i < num_ins; ++i) {
			const Instruction &cur_ins = fn_object->instructions.at(i);

			uint32_t reg_index = UINT32_MAX;

			if (cur_ins.output != UINT32_MAX) {
				reg_index = cur_ins.output;

				if (analyzed_info_out.analyzed_reg_info.contains(reg_index)) {
					// Malformed program, return.
					return alloc_oom_error_if_alloc_failed(
						MalformedProgramError::alloc(
							runtime->get_fixed_alloc(),
							fn_object,
							i));
				}

				if (!analyzed_info_out.analyzed_reg_info.insert(+reg_index, { resource_allocator }))
					return OutOfMemoryError::alloc();
				if (!analyzed_info_out.analyzed_reg_info.at(reg_index).use_points.insert(+i))
					return OutOfMemoryError::alloc();
				analyzed_info_out.analyzed_reg_info.at(reg_index).lifetime = { i, i };
			}

			for (size_t j = 0; j < cur_ins.num_operands; ++j) {
				if (cur_ins.operands[j].value_type == ValueType::RegIndex) {
					uint32_t index = cur_ins.operands[j].get_reg_index();

					if (!analyzed_info_out.analyzed_reg_info.contains(index)) {
						// Malformed program, return.
						return alloc_oom_error_if_alloc_failed(
							MalformedProgramError::alloc(
								runtime->get_fixed_alloc(),
								fn_object,
								i));
					}

					analyzed_info_out.analyzed_reg_info.at(index).lifetime.off_end_ins = i;
				}
			}

			switch (cur_ins.opcode) {
				case Opcode::LOAD: {
					if (reg_index != UINT32_MAX) {
						if (cur_ins.num_operands != 1) {
							return alloc_oom_error_if_alloc_failed(
								MalformedProgramError::alloc(
									runtime->get_fixed_alloc(),
									fn_object,
									i));
						}

						if (cur_ins.operands[0].value_type != ValueType::Reference) {
							return alloc_oom_error_if_alloc_failed(
								MalformedProgramError::alloc(
									runtime->get_fixed_alloc(),
									fn_object,
									i));
						}

						IdRefObject *id_ref;
						{
							Reference object = cur_ins.operands[0].get_reference();

							if ((object.kind != ReferenceKind::ObjectRef) ||
								(object.as_object->get_object_kind() != ObjectKind::IdRef)) {
								return alloc_oom_error_if_alloc_failed(
									MalformedProgramError::alloc(
										runtime->get_fixed_alloc(),
										fn_object,
										i));
							}
							id_ref = (IdRefObject *)object.as_object;
						}

						Reference entity_ref;
						SLAKE_RETURN_IF_EXCEPT(
							runtime->resolve_id_ref(
								id_ref,
								entity_ref));

						InternalExceptionPointer e = eval_object_type(
							analyze_context,
							entity_ref,
							analyzed_info_out.analyzed_reg_info.at(reg_index).type);
						if (e) {
							if (e->kind != ErrorKind::OptimizerError) {
								return e;
							} else {
								e.reset();
								return alloc_oom_error_if_alloc_failed(
									MalformedProgramError::alloc(
										runtime->get_fixed_alloc(),
										fn_object,
										i));
							}
						}

						switch (entity_ref.kind) {
							case ReferenceKind::StaticFieldRef:
								analyzed_info_out.analyzed_reg_info.at(reg_index).storage_type = RegStorageType::FieldVar;
								break;
							case ReferenceKind::ObjectFieldRef:
								analyzed_info_out.analyzed_reg_info.at(reg_index).storage_type = RegStorageType::InstanceFieldVar;
								break;
							case ReferenceKind::ObjectRef: {
								break;
							}
							default:
								std::terminate();
						}

						set_expected_value(reg_index, entity_ref);
						SLAKE_RETURN_IF_EXCEPT(eval_object_type(analyze_context, entity_ref, analyzed_info_out.analyzed_reg_info.at(reg_index).type));
					}

					break;
				}
				case Opcode::RLOAD: {
					if (reg_index != UINT32_MAX) {
						if (cur_ins.num_operands != 2) {
							return alloc_oom_error_if_alloc_failed(
								MalformedProgramError::alloc(
									runtime->get_fixed_alloc(),
									fn_object,
									i));
						}

						if (cur_ins.operands[0].value_type != ValueType::RegIndex) {
							return alloc_oom_error_if_alloc_failed(
								MalformedProgramError::alloc(
									runtime->get_fixed_alloc(),
									fn_object,
									i));
						}

						if (cur_ins.operands[1].value_type != ValueType::Reference) {
							return alloc_oom_error_if_alloc_failed(
								MalformedProgramError::alloc(
									runtime->get_fixed_alloc(),
									fn_object,
									i));
						}

						IdRefObject *id_ref;
						{
							const Reference &object = cur_ins.operands[1].get_reference();

							if ((object.kind != ReferenceKind::ObjectRef) ||
								(object.as_object->get_object_kind() != ObjectKind::IdRef)) {
								return alloc_oom_error_if_alloc_failed(
									MalformedProgramError::alloc(
										runtime->get_fixed_alloc(),
										fn_object,
										i));
							}
							id_ref = (IdRefObject *)object.as_object;
						}

						uint32_t call_target_reg_index = cur_ins.operands[0].get_reg_index();
						TypeRef type = analyze_context.analyzed_info_out.analyzed_reg_info.at(call_target_reg_index).type;

						switch (type.type_id) {
							case TypeId::Instance: {
								Reference entity_ref;

								SLAKE_RETURN_IF_EXCEPT(
									runtime->resolve_id_ref(
										id_ref,
										entity_ref,
										(MemberObject *)type.get_custom_type_def()));

								switch (entity_ref.kind) {
									case ReferenceKind::ObjectRef: {
										Object *object = entity_ref.as_object;
										switch (object->get_object_kind()) {
											case ObjectKind::FnOverloading:
												if (((FnOverloadingObject *)object)->access & ACCESS_STATIC) {
													return alloc_oom_error_if_alloc_failed(
														MalformedProgramError::alloc(
															runtime->get_fixed_alloc(),
															fn_object,
															i));
												}
												break;
											default: {
												return alloc_oom_error_if_alloc_failed(
													MalformedProgramError::alloc(
														runtime->get_fixed_alloc(),
														fn_object,
														i));
											}
										}
										break;
									}
									case ReferenceKind::ObjectFieldRef:
										analyzed_info_out.analyzed_reg_info.at(reg_index).storage_type = RegStorageType::InstanceFieldVar;
										break;
									case ReferenceKind::StaticFieldRef:
										analyzed_info_out.analyzed_reg_info.at(reg_index).storage_type = RegStorageType::FieldVar;
										break;
									default: {
										return alloc_oom_error_if_alloc_failed(
											MalformedProgramError::alloc(
												runtime->get_fixed_alloc(),
												fn_object,
												i));
									}
								}

								set_expected_value(reg_index, entity_ref);
								SLAKE_RETURN_IF_EXCEPT(eval_object_type(analyze_context, entity_ref, analyzed_info_out.analyzed_reg_info.at(reg_index).type));
								break;
							}
							default: {
								return alloc_oom_error_if_alloc_failed(
									MalformedProgramError::alloc(
										runtime->get_fixed_alloc(),
										fn_object,
										i));
							}
						}
					}

					break;
				}
				case Opcode::STORE: {
					if (reg_index != UINT32_MAX) {
						return alloc_oom_error_if_alloc_failed(
							MalformedProgramError::alloc(
								runtime->get_fixed_alloc(),
								fn_object,
								i));
					}

					break;
				}
				case Opcode::COPY: {
					if (reg_index != UINT32_MAX) {
						switch (cur_ins.operands[0].value_type) {
							case ValueType::I8:
								set_expected_value(reg_index, cur_ins.operands[0]);
								analyzed_info_out.analyzed_reg_info.at(reg_index).type = TypeId::I8;
								break;
							case ValueType::I16:
								set_expected_value(reg_index, cur_ins.operands[0]);
								analyzed_info_out.analyzed_reg_info.at(reg_index).type = TypeId::I16;
								break;
							case ValueType::I32:
								set_expected_value(reg_index, cur_ins.operands[0]);
								analyzed_info_out.analyzed_reg_info.at(reg_index).type = TypeId::I32;
								break;
							case ValueType::I64:
								set_expected_value(reg_index, cur_ins.operands[0]);
								analyzed_info_out.analyzed_reg_info.at(reg_index).type = TypeId::I64;
								break;
							case ValueType::U8:
								set_expected_value(reg_index, cur_ins.operands[0]);
								analyzed_info_out.analyzed_reg_info.at(reg_index).type = TypeId::U8;
								break;
							case ValueType::U16:
								set_expected_value(reg_index, cur_ins.operands[0]);
								analyzed_info_out.analyzed_reg_info.at(reg_index).type = TypeId::U16;
								break;
							case ValueType::U32:
								set_expected_value(reg_index, cur_ins.operands[0]);
								analyzed_info_out.analyzed_reg_info.at(reg_index).type = TypeId::U32;
								break;
							case ValueType::U64:
								set_expected_value(reg_index, cur_ins.operands[0]);
								analyzed_info_out.analyzed_reg_info.at(reg_index).type = TypeId::U64;
								break;
							case ValueType::F32:
								set_expected_value(reg_index, cur_ins.operands[0]);
								analyzed_info_out.analyzed_reg_info.at(reg_index).type = TypeId::F32;
								break;
							case ValueType::F64:
								set_expected_value(reg_index, cur_ins.operands[0]);
								analyzed_info_out.analyzed_reg_info.at(reg_index).type = TypeId::F64;
								break;
							case ValueType::Bool:
								set_expected_value(reg_index, cur_ins.operands[0]);
								analyzed_info_out.analyzed_reg_info.at(reg_index).type = TypeId::Bool;
								break;
							case ValueType::Reference: {
								set_expected_value(reg_index, cur_ins.operands[0]);
								SLAKE_RETURN_IF_EXCEPT(eval_object_type(
									analyze_context,
									cur_ins.operands[0].get_reference(),
									analyzed_info_out.analyzed_reg_info.at(reg_index).type));
								break;
							}
							case ValueType::RegIndex:
								set_expected_value(reg_index, Value(ValueType::RegIndex, cur_ins.operands[0].get_reg_index()));
								analyzed_info_out.analyzed_reg_info.at(reg_index).storage_info = analyzed_info_out.analyzed_reg_info.at(cur_ins.operands[0].get_reg_index()).storage_info;
								analyzed_info_out.analyzed_reg_info.at(reg_index).storage_type = analyzed_info_out.analyzed_reg_info.at(cur_ins.operands[0].get_reg_index()).storage_type;
								analyzed_info_out.analyzed_reg_info.at(reg_index).type = analyzed_info_out.analyzed_reg_info.at(cur_ins.operands[0].get_reg_index()).type;
								break;
							default: {
								return alloc_oom_error_if_alloc_failed(
									MalformedProgramError::alloc(
										runtime->get_fixed_alloc(),
										fn_object,
										i));
							}
						}
					}

					break;
				}
				case Opcode::LARG: {
					if (reg_index != UINT32_MAX) {
						if (cur_ins.num_operands != 1) {
							return alloc_oom_error_if_alloc_failed(
								MalformedProgramError::alloc(
									runtime->get_fixed_alloc(),
									fn_object,
									i));
						}

						if (cur_ins.operands[0].value_type != ValueType::U32) {
							return alloc_oom_error_if_alloc_failed(
								MalformedProgramError::alloc(
									runtime->get_fixed_alloc(),
									fn_object,
									i));
						}

						uint32_t index = cur_ins.operands[0].get_u32();
						TypeRef type;

						if (fn_object->overloading_flags & OL_VARG) {
							if (index > fn_object->param_types.size()) {
								return alloc_oom_error_if_alloc_failed(
									MalformedProgramError::alloc(
										runtime->get_fixed_alloc(),
										fn_object,
										i));
							} else if (index == fn_object->param_types.size()) {
								TypeRef var_arg_type_ref;

								wrap_into_array_type(analyze_context.runtime, TypeId::Any, analyze_context.host_ref_holder, var_arg_type_ref);
								SLAKE_RETURN_IF_EXCEPT(
									wrap_into_ref_type(
										runtime,
										var_arg_type_ref,
										host_ref_holder,
										type));
							} else {
								SLAKE_RETURN_IF_EXCEPT(
									wrap_into_ref_type(
										runtime,
										fn_object->param_types.at(index),
										host_ref_holder,
										type));
							}
						} else {
							if (index >= fn_object->param_types.size()) {
								return alloc_oom_error_if_alloc_failed(
									MalformedProgramError::alloc(
										runtime->get_fixed_alloc(),
										fn_object,
										i));
							} else {
								SLAKE_RETURN_IF_EXCEPT(
									wrap_into_ref_type(
										runtime,
										fn_object->param_types.at(index),
										host_ref_holder,
										type));
							}
						}

						analyzed_info_out.analyzed_reg_info.at(reg_index).type = type;

						analyzed_info_out.analyzed_reg_info.at(reg_index).storage_type = RegStorageType::ArgRef;
						analyzed_info_out.analyzed_reg_info.at(reg_index).storage_info.as_arg_ref = {};
						analyzed_info_out.analyzed_reg_info.at(reg_index).storage_info.as_arg_ref.idx_arg = index;
						//set_expected_value(reg_index, Value(ArgRef(pseudo_major_frame.get(), /*stub*/nullptr, 0, index)));
					}
					break;
				}
				case Opcode::LVAR: {
					if (reg_index == UINT32_MAX) {
						return alloc_oom_error_if_alloc_failed(
							MalformedProgramError::alloc(
								runtime->get_fixed_alloc(),
								fn_object,
								i));
					}

					if (cur_ins.num_operands != 1) {
						return alloc_oom_error_if_alloc_failed(
							MalformedProgramError::alloc(
								runtime->get_fixed_alloc(),
								fn_object,
								i));
					}

					if (cur_ins.operands[0].value_type != ValueType::TypeName) {
						return alloc_oom_error_if_alloc_failed(
							MalformedProgramError::alloc(
								runtime->get_fixed_alloc(),
								fn_object,
								i));
					}

					TypeRef type_name = cur_ins.operands[0].get_type_name();

					if (cur_ins.output != UINT32_MAX) {
						Reference entity_ref;
						//SLAKE_RETURN_IF_EXCEPT(runtime->_add_local_var(&analyzed_info_out.context_object->_context, pseudo_major_frame.get(), type_name, cur_ins.output, entity_ref));

						SLAKE_RETURN_IF_EXCEPT(
							wrap_into_ref_type(
								runtime,
								type_name,
								host_ref_holder,
								analyzed_info_out.analyzed_reg_info.at(reg_index).type));

						analyzed_info_out.analyzed_reg_info.at(reg_index).storage_type = RegStorageType::LocalVar;
						analyzed_info_out.analyzed_reg_info.at(reg_index).storage_info.as_local_var.definition_reg = cur_ins.output;
						set_expected_value(reg_index, Value(entity_ref));
					}
					break;
				}
				case Opcode::LVALUE: {
					if (reg_index != UINT32_MAX) {
						if (cur_ins.num_operands != 1) {
							return alloc_oom_error_if_alloc_failed(
								MalformedProgramError::alloc(
									runtime->get_fixed_alloc(),
									fn_object,
									i));
						}

						if (cur_ins.operands[0].value_type != ValueType::RegIndex) {
							return alloc_oom_error_if_alloc_failed(
								MalformedProgramError::alloc(
									runtime->get_fixed_alloc(),
									fn_object,
									i));
						}

						uint32_t index = cur_ins.operands[0].get_reg_index();
						TypeRef type = analyzed_info_out.analyzed_reg_info.at(index).type;

						if (type.type_id != TypeId::Ref) {
							return alloc_oom_error_if_alloc_failed(
								MalformedProgramError::alloc(
									runtime->get_fixed_alloc(),
									fn_object,
									i));
						}

						TypeRef unwrapped_type = type.get_ref_type_def()->referenced_type->type_ref;
						analyzed_info_out.analyzed_reg_info.at(reg_index).type = unwrapped_type;
					}
					break;
				}
				case Opcode::ENTER: {
					break;
				}
				case Opcode::LEAVE: {
					break;
				}
				case Opcode::ADD:
				case Opcode::SUB:
				case Opcode::MUL:
				case Opcode::DIV:
				case Opcode::AND:
				case Opcode::OR:
				case Opcode::XOR:
				case Opcode::LAND:
				case Opcode::LOR:
				case Opcode::EQ:
				case Opcode::NEQ:
				case Opcode::LT:
				case Opcode::GT:
				case Opcode::LTEQ:
				case Opcode::GTEQ:
				case Opcode::LSH:
				case Opcode::RSH:
				case Opcode::CMP:
				case Opcode::NOT:
				case Opcode::LNOT:
				case Opcode::NEG:
					SLAKE_RETURN_IF_EXCEPT(analyze_arithmetic_ins(analyze_context, reg_index));
					break;
				case Opcode::AT: {
					if (reg_index != UINT32_MAX) {
						if (cur_ins.num_operands != 2) {
							return alloc_oom_error_if_alloc_failed(
								MalformedProgramError::alloc(
									runtime->get_fixed_alloc(),
									fn_object,
									i));
						}

						if (cur_ins.operands[0].value_type != ValueType::RegIndex) {
							return alloc_oom_error_if_alloc_failed(
								MalformedProgramError::alloc(
									runtime->get_fixed_alloc(),
									fn_object,
									i));
						}

						uint32_t index = cur_ins.operands[0].get_reg_index();
						TypeRef type = analyzed_info_out.analyzed_reg_info.at(index).type;

						if (type.type_id != TypeId::Array) {
							return alloc_oom_error_if_alloc_failed(
								MalformedProgramError::alloc(
									runtime->get_fixed_alloc(),
									fn_object,
									i));
						}

						TypeRef unwrapped_type = type.get_array_type_def()->element_type->type_ref;
						SLAKE_RETURN_IF_EXCEPT(wrap_into_ref_type(
							analyze_context.runtime,
							unwrapped_type,
							analyze_context.host_ref_holder,
							analyzed_info_out.analyzed_reg_info.at(reg_index).type));

						analyzed_info_out.analyzed_reg_info.at(reg_index).storage_type = RegStorageType::ArrayElement;
					}
					break;
				}
				case Opcode::JMP:
					if (reg_index != UINT32_MAX) {
						return alloc_oom_error_if_alloc_failed(
							MalformedProgramError::alloc(
								runtime->get_fixed_alloc(),
								fn_object,
								i));
					}

					if (!analyze_context.analyzed_info_out.code_block_boundaries.insert(i + 1))
						return OutOfMemoryError::alloc();
					if (!analyze_context.analyzed_info_out.code_block_boundaries.insert(cur_ins.operands[0].get_u32()))
						return OutOfMemoryError::alloc();
					break;
				case Opcode::PUSHARG: {
					if (reg_index != UINT32_MAX) {
						return alloc_oom_error_if_alloc_failed(
							MalformedProgramError::alloc(
								runtime->get_fixed_alloc(),
								fn_object,
								i));
					}
					if (cur_ins.num_operands != 1) {
						return alloc_oom_error_if_alloc_failed(
							MalformedProgramError::alloc(
								runtime->get_fixed_alloc(),
								fn_object,
								i));
					}
					switch (cur_ins.operands[0].value_type) {
						case ValueType::I8:
						case ValueType::I16:
						case ValueType::I32:
						case ValueType::I64:
						case ValueType::U8:
						case ValueType::U16:
						case ValueType::U32:
						case ValueType::U64:
						case ValueType::Bool:
						case ValueType::F32:
						case ValueType::F64:
						case ValueType::Reference:
							break;
						case ValueType::RegIndex:
							mark_reg_as_for_output(analyze_context, cur_ins.operands[0].get_reg_index());
							break;
						default:
							return alloc_oom_error_if_alloc_failed(
								MalformedProgramError::alloc(
									runtime->get_fixed_alloc(),
									fn_object,
									i));
					}
					if (!analyze_context.arg_push_ins_offs.push_back(+i))
						return OutOfMemoryError::alloc();
					break;
				}
				case Opcode::CALL: {
					if (cur_ins.num_operands != 1) {
						return alloc_oom_error_if_alloc_failed(
							MalformedProgramError::alloc(
								runtime->get_fixed_alloc(),
								fn_object,
								i));
					}

					Value call_target = cur_ins.operands[0];
					if (call_target.value_type != ValueType::RegIndex) {
						return alloc_oom_error_if_alloc_failed(
							MalformedProgramError::alloc(
								runtime->get_fixed_alloc(),
								fn_object,
								i));
					}

					uint32_t call_target_reg_index = call_target.get_reg_index();
					if (!analyzed_info_out.analyzed_reg_info.contains(call_target_reg_index)) {
						return alloc_oom_error_if_alloc_failed(
							MalformedProgramError::alloc(
								runtime->get_fixed_alloc(),
								fn_object,
								i));
					}

					TypeRef call_target_type = analyzed_info_out.analyzed_reg_info.at(call_target_reg_index).type;

					switch (call_target_type.type_id) {
						case TypeId::Fn:
							if (reg_index != UINT32_MAX) {
								FnTypeDefObject *type_def = (FnTypeDefObject *)call_target_type.get_custom_type_def();
								analyzed_info_out.analyzed_reg_info.at(reg_index).type = type_def->return_type->type_ref;
							}
							break;
						default: {
							return alloc_oom_error_if_alloc_failed(
								MalformedProgramError::alloc(
									runtime->get_fixed_alloc(),
									fn_object,
									i));
						}
					}

					analyze_context.analyzed_info_out.analyzed_fn_call_info.insert(+i, FnCallAnalyzedInfo(analyze_context.resource_allocator.get()));
					analyze_context.analyzed_info_out.analyzed_fn_call_info.at(i).arg_push_ins_offs = std::move(analyze_context.arg_push_ins_offs);
					analyze_context.arg_push_ins_offs = peff::DynArray<uint32_t>(analyze_context.resource_allocator.get());

					Value expected_fn_value = analyzed_info_out.analyzed_reg_info.at(call_target_reg_index).expected_value;

					if (expected_fn_value.value_type != ValueType::Undefined) {
						FnOverloadingObject *expected_fn_object = (FnOverloadingObject *)expected_fn_value.get_reference().as_object;
						if (!analyze_context.analyzed_info_out.fn_call_map.contains(expected_fn_object)) {
							analyze_context.analyzed_info_out.fn_call_map.insert(+expected_fn_object, peff::DynArray<uint32_t>(analyze_context.runtime->get_fixed_alloc()));
						}
						if (!analyze_context.analyzed_info_out.fn_call_map.at(expected_fn_object).push_back(+i))
							return OutOfMemoryError::alloc();
					}

					break;
				}
				case Opcode::MCALL:
				case Opcode::CTORCALL: {
					if (cur_ins.num_operands != 2) {
						return alloc_oom_error_if_alloc_failed(
							MalformedProgramError::alloc(
								runtime->get_fixed_alloc(),
								fn_object,
								i));
					}

					Value call_target = cur_ins.operands[0];
					if (call_target.value_type != ValueType::RegIndex) {
						return alloc_oom_error_if_alloc_failed(
							MalformedProgramError::alloc(
								runtime->get_fixed_alloc(),
								fn_object,
								i));
					}

					uint32_t call_target_reg_index = call_target.get_reg_index();
					if (!analyzed_info_out.analyzed_reg_info.contains(call_target_reg_index)) {
						return alloc_oom_error_if_alloc_failed(
							MalformedProgramError::alloc(
								runtime->get_fixed_alloc(),
								fn_object,
								i));
					}

					TypeRef call_target_type = analyzed_info_out.analyzed_reg_info.at(call_target_reg_index).type;

					switch (call_target_type.type_id) {
						case TypeId::Fn:
							if (reg_index != UINT32_MAX) {
								FnTypeDefObject *type_def = (FnTypeDefObject *)call_target_type.get_custom_type_def();
								analyzed_info_out.analyzed_reg_info.at(reg_index).type = type_def->return_type->type_ref;
							}
							break;
						default: {
							return alloc_oom_error_if_alloc_failed(
								MalformedProgramError::alloc(
									runtime->get_fixed_alloc(),
									fn_object,
									i));
						}
					}
					analyze_context.analyzed_info_out.analyzed_fn_call_info.insert(+i, FnCallAnalyzedInfo(analyze_context.resource_allocator.get()));
					analyze_context.analyzed_info_out.analyzed_fn_call_info.at(i).arg_push_ins_offs = std::move(analyze_context.arg_push_ins_offs);
					analyze_context.arg_push_ins_offs = peff::DynArray<uint32_t>(analyze_context.resource_allocator.get());

					Value expected_fn_value = analyzed_info_out.analyzed_reg_info.at(call_target_reg_index).expected_value;

					if (expected_fn_value.value_type != ValueType::Undefined) {
						FnOverloadingObject *expected_fn_object = (FnOverloadingObject *)expected_fn_value.get_reference().as_object;
						if (!analyze_context.analyzed_info_out.fn_call_map.contains(expected_fn_object)) {
							analyze_context.analyzed_info_out.fn_call_map.insert(+expected_fn_object, peff::DynArray<uint32_t>(analyze_context.resource_allocator.get()));
						}
						if (!analyze_context.analyzed_info_out.fn_call_map.at(expected_fn_object).push_back(+i))
							return OutOfMemoryError::alloc();
					}

					break;
				}
				case Opcode::RET:
					if (reg_index != UINT32_MAX) {
						return alloc_oom_error_if_alloc_failed(
							MalformedProgramError::alloc(
								runtime->get_fixed_alloc(),
								fn_object,
								i));
					}
					if (cur_ins.num_operands != 1) {
						return alloc_oom_error_if_alloc_failed(
							MalformedProgramError::alloc(
								runtime->get_fixed_alloc(),
								fn_object,
								i));
					}
					switch (cur_ins.operands[0].value_type) {
						case ValueType::I8:
						case ValueType::I16:
						case ValueType::I32:
						case ValueType::I64:
						case ValueType::U8:
						case ValueType::U16:
						case ValueType::U32:
						case ValueType::U64:
						case ValueType::Bool:
						case ValueType::F32:
						case ValueType::F64:
						case ValueType::Reference:
							break;
						case ValueType::RegIndex:
							mark_reg_as_for_output(analyze_context, cur_ins.operands[0].get_reg_index());
							break;
						default:
							return alloc_oom_error_if_alloc_failed(
								MalformedProgramError::alloc(
									runtime->get_fixed_alloc(),
									fn_object,
									i));
					}
					if (!analyze_context.analyzed_info_out.code_block_boundaries.insert(i + 1))
						return OutOfMemoryError::alloc();
					break;
				case Opcode::YIELD:
					if (reg_index != UINT32_MAX) {
						return alloc_oom_error_if_alloc_failed(
							MalformedProgramError::alloc(
								runtime->get_fixed_alloc(),
								fn_object,
								i));
					}
					if (!analyze_context.analyzed_info_out.code_block_boundaries.insert(i + 1))
						return OutOfMemoryError::alloc();
					break;
				case Opcode::LTHIS:
					if (reg_index == UINT32_MAX) {
						return alloc_oom_error_if_alloc_failed(
							MalformedProgramError::alloc(
								runtime->get_fixed_alloc(),
								fn_object,
								i));
					}

					if (analyze_context.fn_object->this_type.type_id == TypeId::Void) {
						return alloc_oom_error_if_alloc_failed(
							MalformedProgramError::alloc(
								runtime->get_fixed_alloc(),
								fn_object,
								i));
					}
					analyzed_info_out.analyzed_reg_info.at(reg_index).type = analyze_context.fn_object->this_type;
					break;
				case Opcode::NEW:
					if (reg_index == UINT32_MAX) {
						return alloc_oom_error_if_alloc_failed(
							MalformedProgramError::alloc(
								runtime->get_fixed_alloc(),
								fn_object,
								i));
					}

					if (cur_ins.num_operands != 1) {
						return alloc_oom_error_if_alloc_failed(
							MalformedProgramError::alloc(
								runtime->get_fixed_alloc(),
								fn_object,
								i));
					}

					if (cur_ins.operands[0].value_type != ValueType::TypeName) {
						return alloc_oom_error_if_alloc_failed(
							MalformedProgramError::alloc(
								runtime->get_fixed_alloc(),
								fn_object,
								i));
					}

					analyzed_info_out.analyzed_reg_info.at(reg_index).type = cur_ins.operands[0].get_type_name();
					break;
				case Opcode::ARRNEW: {
					if (reg_index == UINT32_MAX) {
						return alloc_oom_error_if_alloc_failed(
							MalformedProgramError::alloc(
								runtime->get_fixed_alloc(),
								fn_object,
								i));
					}

					if (cur_ins.num_operands != 2) {
						return alloc_oom_error_if_alloc_failed(
							MalformedProgramError::alloc(
								runtime->get_fixed_alloc(),
								fn_object,
								i));
					}

					if (cur_ins.operands[0].value_type != ValueType::TypeName) {
						return alloc_oom_error_if_alloc_failed(
							MalformedProgramError::alloc(
								runtime->get_fixed_alloc(),
								fn_object,
								i));
					}

					TypeRef length_type;
					SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[1], length_type));
					if (!is_value_type_compatible_type_id(length_type.type_id)) {
						return alloc_oom_error_if_alloc_failed(
							MalformedProgramError::alloc(
								runtime->get_fixed_alloc(),
								fn_object,
								i));
					}

					if (length_type.compares_to(TypeId::U32)) {
						return alloc_oom_error_if_alloc_failed(
							MalformedProgramError::alloc(
								runtime->get_fixed_alloc(),
								fn_object,
								i));
					}

					SLAKE_RETURN_IF_EXCEPT(wrap_into_array_type(
						runtime,
						cur_ins.operands[0].get_type_name(),
						host_ref_holder,
						analyzed_info_out.analyzed_reg_info.at(reg_index).type));
					break;
				}
				case Opcode::THROW:
				case Opcode::PUSHEH:
					if (reg_index != UINT32_MAX) {
						return alloc_oom_error_if_alloc_failed(
							MalformedProgramError::alloc(
								runtime->get_fixed_alloc(),
								fn_object,
								i));
					}
					break;
				case Opcode::LEXCEPT:
					// stub
					break;
				case Opcode::CAST: {
					SLAKE_RETURN_IF_EXCEPT(analyze_cast_ins(analyze_context, reg_index));
					break;
				}
				default: {
					// Malformed program, return.
					return alloc_oom_error_if_alloc_failed(
						MalformedProgramError::alloc(
							runtime->get_fixed_alloc(),
							fn_object,
							i));
				}
			}
		}
	} while (new_expectable_reg_found);

	// A well-formed program should not have unused argument pushing instructions.
	if (analyze_context.arg_push_ins_offs.size()) {
		return alloc_oom_error_if_alloc_failed(
			MalformedProgramError::alloc(
				runtime->get_fixed_alloc(),
				fn_object,
				num_ins - 1));
	}

	return {};
}
