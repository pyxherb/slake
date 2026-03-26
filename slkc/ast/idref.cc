#include "idref.h"

using namespace slkc;

SLKC_API IdRef::IdRef(
	peff::Alloc *self_allocator)
	: self_allocator(self_allocator),
	  entries(self_allocator) {
}

SLKC_API IdRef::~IdRef() {
}

SLKC_API void IdRef::dealloc() noexcept {
	peff::destroy_and_release<IdRef>(self_allocator.get(), this, ASTNODE_ALIGNMENT);
}

SLKC_API peff::Option<IdRefEntry> slkc::duplicate_id_ref_entry(peff::Alloc *self_allocator, const IdRefEntry &rhs) {
	IdRefEntry new_id_ref_entry(self_allocator);

	if (!new_id_ref_entry.generic_args.build(rhs.generic_args)) {
		return {};
	}

	if (!new_id_ref_entry.name.build(rhs.name)) {
		return {};
	}

	new_id_ref_entry.name_token_index = rhs.name_token_index;
	new_id_ref_entry.generic_scope_token_index = rhs.generic_scope_token_index;
	new_id_ref_entry.left_angle_bracket_token_index = rhs.left_angle_bracket_token_index;
	new_id_ref_entry.right_angle_bracket_token_index = rhs.right_angle_bracket_token_index;
	new_id_ref_entry.access_op_token_index = rhs.access_op_token_index;

	if (!new_id_ref_entry.comma_token_indices.build(rhs.comma_token_indices)) {
		return {};
	}

	return std::move(new_id_ref_entry);
}

SLKC_API IdRefPtr slkc::duplicate_id_ref(peff::Alloc *self_allocator, IdRef *rhs) {
	if (!rhs) {
		return {};
	}

	IdRefPtr new_id_ref_ptr = IdRefPtr(
		peff::alloc_and_construct<IdRef>(
			self_allocator,
			ASTNODE_ALIGNMENT,
			self_allocator));

	if (!new_id_ref_ptr->entries.resize_uninit(rhs->entries.size())) {
		return {};
	}

	for (size_t i = 0; i < rhs->entries.size(); ++i) {
		peff::Option<IdRefEntry> duplicated_entry = duplicate_id_ref_entry(self_allocator, rhs->entries.at(i));

		if (!duplicated_entry.has_value())
			return {};

		peff::construct_at<IdRefEntry>(&new_id_ref_ptr->entries.at(i), std::move(*duplicated_entry));
		duplicated_entry.reset();
	}

	new_id_ref_ptr->token_range = rhs->token_range;

	return new_id_ref_ptr;
}
