#ifndef _SWAMPEAK_RT_H_
#define _SWAMPEAK_RT_H_

#include <functional>
#include <unordered_map>
#include <swampeak/base/uuid.hh>

#include "context.h"
#include "value.h"
#include "opcode.h"

namespace Swampeak {
	class Function final {
	};

	using NativeFnHandler = std::function<void()>;

	struct NativeConstRegistry {
		std::function<void(std::weak_ptr<IValue> value)> read;
	};
	struct NativeVarRegistry final : public NativeConstRegistry {
		std::function<void(std::weak_ptr<IValue> value)> write;
	};

	struct Instruction {

	};

	class IProgram {
	public:
	};

	class IRuntime {
	public:
		virtual std::shared_ptr<IProgram> loadProgram(const void* src, std::size_t size) = 0;

		virtual inline ~IRuntime() {}
	};
}

#endif
