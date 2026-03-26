#include "../runtime.h"

using namespace slake;

SLAKE_API void Runtime::_destruct_destructible_objects(InstanceObject *destructible_list) {
	InternalExceptionPointer exception;
	/*
	for (InstanceObject *i = destructible_list, *next; i; i = next) {
		next = (InstanceObject *)i->next_same_kind_object;

		destructible_list = next;

		i->object_flags |= VF_DESTRUCTED;

		Value result_out;
		for (auto j : i->_class->cached_instantiated_method_table->destructors) {
			if ((exception = exec_fn(j, nullptr, i, nullptr, 0, result_out))) {
				if (!_uncaught_exception_handler) {
					std::terminate();
				}
				_uncaught_exception_handler(std::move(exception));
			}
		}
	}*/
}

SLAKE_API void Runtime::gc() {
	runtime_flags |= _RT_INGC;

	// TODO: This is a stupid way to make sure that all the destructible objects are destructed.
	// Can we create a separate GC thread in advance and let it to execute them?
	Object *young_objects_end;
	_gc_serial(young_object_list, young_objects_end, num_young_objects, ObjectGeneration::Persistent, &persistent_alloc);

	size_t young_size = young_alloc.sz_allocated.load();
	if (young_size) {
		persistent_alloc.sz_allocated += young_size;
		young_alloc.sz_allocated = 0;
	}

	if (young_objects_end) {
		young_objects_end->next_same_gen_object = persistent_object_list;
	}
	if (persistent_object_list) {
		persistent_object_list->prev_same_gen_object = young_objects_end;
	}
	if (young_object_list)
		persistent_object_list = young_object_list;
	num_persistent_objects += num_young_objects;

	young_object_list = nullptr;
	num_young_objects = 0;

	if (persistent_alloc.sz_allocated >= _sz_computed_persistent_gc_limit) {
		Object *persistent_objects_end;
		_gc_serial(persistent_object_list, persistent_objects_end, num_persistent_objects, ObjectGeneration::Persistent, nullptr);

		_sz_persistent_mem_used_after_last_gc = persistent_alloc.sz_allocated;
		_sz_computed_persistent_gc_limit = _sz_persistent_mem_used_after_last_gc + (_sz_persistent_mem_used_after_last_gc >> 1);
	}

	_sz_mem_used_after_last_gc = young_alloc.sz_allocated + persistent_alloc.sz_allocated;
	_sz_computed_gc_limit = _sz_mem_used_after_last_gc + (_sz_mem_used_after_last_gc >> 1);

#ifndef _NDEBUG
	if (young_alloc.ref_count) {
		puts("Detected unreplaced allocator references!");

		for (auto i : young_alloc.recorded_ref_points) {
			printf("Reference point #%zu\n", i);
		}

		puts("Dump completed");
		std::terminate();
	}
#endif

	runtime_flags &= ~_RT_INGC;
}
