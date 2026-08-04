#include "bc_builder.h"

using namespace slake;

SLAKE_API BCBuilder::BCBuilder(Runtime *rt, peff::Alloc *resource_allocator)
	: _rt(rt),
	  label_to_offset_map(resource_allocator),
	  operand_to_label_replacement_map(resource_allocator) {}
SLAKE_API BCBuilder::~BCBuilder() {}
