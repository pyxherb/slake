#include <slake/runtime.h>

using namespace slake;

SLAKE_API IdRefEntry::IdRefEntry(peff::Alloc *self_allocator)
	: name(self_allocator), generic_args(self_allocator) {
	// For resize() methods.
}

SLAKE_API IdRefEntry::IdRefEntry(peff::String &&name,
	peff::DynArray<Value> &&generic_args)
	: name(std::move(name)),
	  generic_args(std::move(generic_args)) {}

SLAKE_API void IdRefEntry::replace_allocator(peff::Alloc *allocator) noexcept {
	name.replace_allocator(allocator);

	generic_args.replace_allocator(allocator);
}

SLAKE_API slake::IdRefObject::IdRefObject(Runtime *rt, peff::Alloc *self_allocator)
	: Object(rt, self_allocator, ObjectKind::IdRef),
	  entries(self_allocator),
	  param_types(),
	  has_var_args(false) {
}

SLAKE_API IdRefObject::IdRefObject(const IdRefObject &x, peff::Alloc *allocator, bool &succeeded_out)
	: Object(x, allocator),
	  entries(allocator) {
	if (!entries.resize_uninit(x.entries.size())) {
		succeeded_out = false;
		return;
	}
	for (size_t i = 0; i < x.entries.size(); ++i) {
		if (!x.entries.at(i).copy(entries.at(i))) {
			for (size_t j = i; j; --j) {
				peff::destroy_at<IdRefEntry>(&entries.at(j - 1));
			}
			succeeded_out = false;
			return;
		}
	}

	if (x.param_types.has_value()) {
		peff::DynArray<TypeRef> copied_param_types(allocator);

		if (!copied_param_types.resize(x.param_types->size())) {
			succeeded_out = false;
			return;
		}

		for (size_t i = 0; i < x.param_types->size(); ++i) {
			copied_param_types.at(i) = x.param_types->at(i);
		}

		param_types = std::move(copied_param_types);
	}

	has_var_args = x.has_var_args;
}

SLAKE_API IdRefObject::~IdRefObject() {
}

SLAKE_API Object *IdRefObject::duplicate(Duplicator *duplicator) const {
	SLAKE_REFERENCED_PARAM(duplicator);

	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<IdRefObject> slake::IdRefObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = rt->get_cur_gen_alloc();

	std::unique_ptr<IdRefObject, peff::DeallocableDeleter<IdRefObject>> ptr(
		peff::alloc_and_construct<IdRefObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			rt,
			cur_generation_allocator.get()));
	if (!ptr)
		return nullptr;

	if (!rt->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<IdRefObject> slake::IdRefObject::alloc(const IdRefObject *other) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = other->associated_runtime->get_cur_gen_alloc();

	bool succeeded = true;

	std::unique_ptr<IdRefObject, peff::DeallocableDeleter<IdRefObject>> ptr(
		peff::alloc_and_construct<IdRefObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			*other, cur_generation_allocator.get(), succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associated_runtime->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::IdRefObject::dealloc() {
	peff::destroy_and_release<IdRefObject>(get_allocator(), this, alignof(IdRefObject));
}

SLAKE_API void IdRefObject::replace_allocator(peff::Alloc *allocator) noexcept {
	this->Object::replace_allocator(allocator);

	entries.replace_allocator(allocator);

	for (auto& i : entries) {
		i.replace_allocator(allocator);
	}

	if (param_types.has_value())
		param_types->replace_allocator(allocator);
}

SLAKE_API int IdRefComparator::operator()(const IdRefObject *lhs, const IdRefObject *rhs) const noexcept {
	if (lhs->entries.size() < rhs->entries.size())
		return -1;
	if (lhs->entries.size() > rhs->entries.size())
		return 1;

	for (size_t i = 0; i < lhs->entries.size(); ++i) {
		auto &le = lhs->entries.at(i), &re = rhs->entries.at(i);

		if (le.name < re.name)
			return -1;
		if (le.name > re.name)
			return 1;

		if (le.generic_args.size() < re.generic_args.size())
			return -1;
		if (le.generic_args.size() > re.generic_args.size())
			return 1;

		for (size_t j = 0; j < le.generic_args.size(); ++j) {
			auto &lga = le.generic_args.at(j), &rga = re.generic_args.at(j);

			if (lga < rga)
				return -1;
			if (lga > rga)
				return 1;
		}
	}

	if ((uint8_t)lhs->param_types.has_value() < (uint8_t)rhs->param_types.has_value())
		return -1;
	if ((uint8_t)lhs->param_types.has_value() > (uint8_t)rhs->param_types.has_value())
		return 1;
	if (lhs->param_types.has_value()) {
		if (lhs->param_types->size() < rhs->param_types->size())
			return -1;
		if (lhs->param_types->size() > rhs->param_types->size())
			return 1;

		for (size_t i = 0; i < lhs->param_types->size(); ++i) {
			const TypeRef &lt = lhs->param_types->at(i), &rt = rhs->param_types->at(i);

			if (lt < rt)
				return -1;
			if (lt > rt)
				return 1;
		}
	}

	if ((uint8_t)lhs->has_var_args < (uint8_t)rhs->has_var_args)
		return -1;
	if ((uint8_t)lhs->has_var_args > (uint8_t)rhs->has_var_args)
		return 1;

	return 0;
}

SLAKE_API std::string std::to_string(const slake::IdRefObject *ref) {
	string s;
	for (size_t i = 0; i < ref->entries.size(); ++i) {
		auto &scope = ref->entries.at(i);

		if (i)
			s += ".";
		s += scope.name.data();

		if (auto num_generic_params = scope.generic_args.size(); num_generic_params) {
			s += "<";
			/* for (size_t j = 0; j < num_generic_params; ++j) {
				if (j)
					s += ",";
				s += to_string(scope.generic_args.at(j), ref->get_runtime());
			}*/
			s += ">";
		}
	}
	return s;
}
