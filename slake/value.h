#ifndef _SLAKE_OBJECT_H_
#define _SLAKE_OBJECT_H_

#include <cstdarg>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include "opcode.h"
#include "util/debug.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++17-extensions"

namespace Slake {
	class MismatchedValueTypeError : public std::runtime_error {
	public:
		inline MismatchedValueTypeError(std::string msg) : runtime_error(msg){};
		virtual inline ~MismatchedValueTypeError() {}
	};

	using AccessModifier = std::uint16_t;
	constexpr static AccessModifier
		ACCESS_PUB = 0x01,
		ACCESS_STATIC = 0x02,
		ACCESS_NATIVE = 0x04,
		ACCESS_OVERRIDE = 0x08,
		ACCESS_FINAL = 0x10,
		ACCESS_CONST = 0x20;

	class AccessModified {
	private:
		AccessModifier _modifier = 0;

	public:
		AccessModified() = delete;
		AccessModified(const AccessModified &) = delete;
		AccessModified(const AccessModified &&) = delete;

		inline AccessModified(AccessModifier modifier = 0) : _modifier(modifier) {}
		virtual inline ~AccessModified() {}
		inline AccessModifier getAccess() noexcept { return _modifier; }
		inline void setAccess(AccessModifier modifier) noexcept { _modifier = modifier; }

		inline bool isPublic() noexcept { return _modifier & ACCESS_PUB; }
		inline bool isStatic() noexcept { return _modifier & ACCESS_STATIC; }
		inline bool isNative() noexcept { return _modifier & ACCESS_NATIVE; }
		inline bool isOverriden() noexcept { return _modifier & ACCESS_OVERRIDE; }
		inline bool isFinal() noexcept { return _modifier & ACCESS_FINAL; }
		inline bool isConst() noexcept { return _modifier & ACCESS_CONST; }
	};

	enum class ValueType : std::uint8_t {
		NONE,

		// Literals
		U8,
		U16,
		U32,
		U64,
		I8,
		I16,
		I32,
		I64,
		FLOAT,
		DOUBLE,
		BOOL,
		STRING,

		// Built-in objects
		FN,
		MOD,
		VAR,
		ARRAY,
		MAP,

		// Custom types
		CLASS,
		STRUCT,
		OBJECT,

		// Following types are only used for variables.
		ANY,

		// Internal types
		REF,
		ROOT,

		INVALID = 0xff
	};

	class Runtime;
	class MemberValue;
	class Value;
	class RefValue;

	struct Type final {
		ValueType valueType;
		union {
			Value *customType;
			Type *array;
			RefValue *deferred;
			struct {
				Type *k;
				Type *v;
			} map;
		} exData;

		inline Type() : valueType(ValueType::NONE) {}
		inline Type(const Type &x) { *this = x; }
		inline Type(const Type &&x) { *this = x; }
		inline Type(ValueType valueType) : valueType(valueType) {}
		inline Type(ValueType valueType, Value *classObject) : valueType(valueType) { exData.customType = classObject; }
		inline Type(ValueType valueType, Type elementType) : valueType(valueType) { exData.array = new Type(elementType); }
		Type(RefValue *ref);

		inline Type(Type k, Type v) : valueType(ValueType::MAP) {
			exData.map.k = new Type(k), exData.map.v = new Type(v);
		}

		~Type();

		bool isDeferred() noexcept;

		inline operator bool() noexcept {
			return valueType != ValueType::NONE;
		}

		inline bool operator==(const Type &&x) noexcept {
			if (x.valueType != valueType)
				return false;
			switch (x.valueType) {
				case ValueType::CLASS:
				case ValueType::STRUCT:
				case ValueType::OBJECT:
					return exData.customType == x.exData.customType;
				case ValueType::ARRAY:
					return exData.array == x.exData.array;
				case ValueType::MAP:
					return (exData.map.k == x.exData.map.k) &&
						   (exData.map.v == x.exData.map.v);
			}
			return true;
		}

		inline bool operator==(const Type &x) noexcept {
			return *this == std::move(x);
		}

		inline bool operator!=(const Type &&x) noexcept { return !(*this == x); }
		inline bool operator!=(const Type &x) noexcept { return !(*this == x); }

		inline bool operator==(ValueType x) noexcept {
			return this->valueType == x;
		}
		inline bool operator!=(ValueType x) noexcept {
			return this->valueType != x;
		}

		inline Type &operator=(const Type &&x) noexcept {
			valueType = x.valueType;
			std::memcpy(&exData, &(x.exData), sizeof(exData));
			return *this;
		}

		inline Type &operator=(const Type &x) noexcept {
			return *this = std::move(x);
		}

		inline Value *resolveCustomType() {
			if (valueType == ValueType::CLASS || valueType == ValueType::STRUCT)
				return exData.customType;
			return nullptr;
		}
	};

	template <typename T = Value, bool isHostRef = true>
	class ValueRef final {
	public:
		T *_value;

		inline void release() {
			if (_value) {
				if constexpr (isHostRef) {
					_value->decHostRefCount();
				} else {
					_value->decRefCount();
				}
			}
			_value = nullptr;
		}

		inline void discard() {
			_value = nullptr;
		}

		inline ValueRef(const ValueRef<T, isHostRef> &x) : _value(x._value) {
			if (x._value) {
				if constexpr (isHostRef) {
					_value->incHostRefCount();
				} else {
					_value->incRefCount();
				}
			}
		}
		inline ValueRef(const ValueRef<T, isHostRef> &&x) : _value(x._value) {
			if (x._value) {
				if constexpr (isHostRef) {
					_value->incHostRefCount();
				} else {
					_value->incRefCount();
				}
			}
		}
		inline ValueRef(T *value = nullptr) : _value(value) {
			if (_value) {
				if constexpr (isHostRef) {
					_value->incHostRefCount();
				} else {
					_value->incRefCount();
				}
			}
		}
		inline ~ValueRef() {
			release();
		}
		inline T *operator*() { return _value; }
		inline const T *operator*() const { return _value; }
		inline const T *operator->() const { return _value; }
		inline T *operator->() { return _value; }

		template <typename T1 = T>
		T1 *get() { return (T1 *)_value; }
		template <typename T1 = T>
		const T1 *get() const { return (T1 *)_value; }

		inline ValueRef &operator=(const ValueRef<T, isHostRef> &x) {
			return *this = std::move(x);
		}
		inline ValueRef &operator=(const ValueRef<T, isHostRef> &&x) {
			if (_value) {
				if constexpr (isHostRef) {
					_value->decHostRefCount();
				} else {
					_value->decRefCount();
				}
			}
			if ((_value = x._value)) {
				if constexpr (isHostRef) {
					_value->incHostRefCount();
				} else {
					_value->incRefCount();
				}
			}
			return *this;
		}
		template <typename T1, bool isHostRef1>
		inline ValueRef &operator=(const ValueRef<T1, isHostRef1> &&x) {
			if (_value) {
				if constexpr (isHostRef) {
					_value->decHostRefCount();
				} else {
					_value->decRefCount();
				}
			}
			if ((_value = x._value)) {
				if constexpr (isHostRef) {
					_value->incHostRefCount();
				} else {
					_value->incRefCount();
				}
			}
			return *this;
		}

		template <typename T1, bool isHostRef1>
		inline operator ValueRef<T1, isHostRef1>() {
			return ValueRef<T1, isHostRef1>((T1 *)_value);
		}

		inline operator bool() const {
			return _value;
		}
	};

	using ValueFlags = std::uint8_t;
	constexpr static ValueFlags VF_GCSTAT = 0x01;

	class ValueIterator {
	public:
		inline ValueIterator() {}
		inline ValueIterator(const ValueIterator &x) { *this = x; }
		inline ValueIterator(const ValueIterator &&x) { *this = x; }

		virtual inline ValueIterator &operator++() { return *this; }
		virtual inline ValueIterator &&operator++(int) { return std::move(*this); }
		virtual inline ValueIterator &operator--() { return *this; }
		virtual inline ValueIterator &&operator--(int) { return std::move(*this); }
		virtual inline Value *operator*() { return nullptr; };

		virtual inline bool operator==(const ValueIterator &&) const { return true; };
		inline bool operator==(const ValueIterator &x) const {
			return *this == std::move(x);
		}
		virtual inline bool operator!=(const ValueIterator &&) const { return false; };
		inline bool operator!=(const ValueIterator &x) const {
			return *this != std::move(x);
		}

		virtual inline ValueIterator &operator=(const ValueIterator &&) { return *this; }
		virtual inline ValueIterator &operator=(const ValueIterator &x) {
			return *this = std::move(x);
		}
	};

	class Value {
	private:
		std::uint32_t _refCount = 0;
		// The garbage collector will never release it if its host reference count is not 0.
		std::uint32_t _hostRefCount = 0;
		Runtime *_rt;
		ValueFlags flags = 0;

		friend class Runtime;

	protected:
		void reportSizeToRuntime(long size);

	public:
		Value(Runtime *rt);
		virtual ~Value();
		virtual Type getType() const = 0;

		virtual Value *getMember(std::string name) { return nullptr; };
		virtual const Value *getMember(std::string name) const { return nullptr; }

		virtual ValueRef<> call(std::uint8_t nArgs, ValueRef<> *args) { return nullptr; }

		inline void incRefCount() { _refCount++; }
		inline void decRefCount() {
			if (!(--_refCount || _hostRefCount))
				delete this;
		}
		inline void incHostRefCount() { _hostRefCount++; }
		inline void decHostRefCount() {
			if (!(--_hostRefCount || _refCount))
				delete this;
		}
		inline std::uint32_t getRefCount() { return _refCount; }
		inline std::uint32_t getHostRefCount() { return _hostRefCount; }
		inline Runtime *getRuntime() noexcept { return _rt; }

		virtual inline ValueIterator &begin() { return ValueIterator(); }
		virtual inline ValueIterator &end() { return ValueIterator(); }

		Value &operator=(const Value &) = delete;
		Value &operator=(const Value &&) = delete;
	};

	class MemberValue : public Value, public AccessModified {
	protected:
		Value* _parent;

	public:
		inline MemberValue(Runtime *rt, AccessModifier access, Value *parent) : Value(rt), AccessModified(access), _parent(parent) {}
		virtual inline ~MemberValue() {}
	};

	template <typename T, ValueType VT>
	class LiteralValue final : public Value {
	protected:
		T _value;

	public:
		inline LiteralValue(Runtime *rt, T value) : Value(rt), _value(value) {
			reportSizeToRuntime(sizeof(*this));
			if constexpr (std::is_same<T, std::string>::value) {
				reportSizeToRuntime(value.size());
			}
		}
		virtual inline ~LiteralValue() {}

		virtual inline Type getType() const override { return VT; }

		virtual inline const T &getValue() const { return _value; }
		virtual inline void setValue(T &value) {
			if constexpr (std::is_same<T, std::string>::value) {
				reportSizeToRuntime(((long)value.size()) - (long)_value.size());
			}
			_value = value;
		}

		LiteralValue &operator=(const LiteralValue &) = delete;
		LiteralValue &operator=(const LiteralValue &&) = delete;
	};

	template <typename T>
	constexpr inline ValueType getValueType() {
		if constexpr (std::is_same<T, std::int8_t>::value)
			return ValueType::I8;
		else if constexpr (std::is_same<T, std::int16_t>::value)
			return ValueType::I16;
		else if constexpr (std::is_same<T, std::int32_t>::value)
			return ValueType::I32;
		else if constexpr (std::is_same<T, std::int64_t>::value)
			return ValueType::I64;
		else if constexpr (std::is_same<T, std::uint8_t>::value)
			return ValueType::U8;
		else if constexpr (std::is_same<T, std::uint16_t>::value)
			return ValueType::U16;
		else if constexpr (std::is_same<T, std::uint32_t>::value)
			return ValueType::U32;
		else if constexpr (std::is_same<T, std::uint64_t>::value)
			return ValueType::U64;
		else if constexpr (std::is_same<T, float>::value)
			return ValueType::FLOAT;
		else if constexpr (std::is_same<T, double>::value)
			return ValueType::DOUBLE;
		else if constexpr (std::is_same<T, bool>::value)
			return ValueType::BOOL;
		else if constexpr (std::is_same<T, std::string>::value)
			return ValueType::STRING;
		else
			// We don't `false' as the condition due to the compiler will
			// evaluate it prematurely.
			static_assert(!std::is_same<T, T>::value);
	}

	using I8Value = LiteralValue<std::int8_t, ValueType::I8>;
	using I16Value = LiteralValue<std::int16_t, ValueType::I16>;
	using I32Value = LiteralValue<std::int32_t, ValueType::I32>;
	using I64Value = LiteralValue<std::int64_t, ValueType::I64>;
	using U8Value = LiteralValue<std::uint8_t, ValueType::U8>;
	using U16Value = LiteralValue<std::uint16_t, ValueType::U16>;
	using U32Value = LiteralValue<std::uint32_t, ValueType::U32>;
	using U64Value = LiteralValue<std::uint64_t, ValueType::U64>;
	using FloatValue = LiteralValue<float, ValueType::FLOAT>;
	using DoubleValue = LiteralValue<double, ValueType::DOUBLE>;
	using BoolValue = LiteralValue<bool, ValueType::BOOL>;
	using StringValue = LiteralValue<std::string, ValueType::STRING>;

	class ObjectValue final : public Value {
	protected:
		std::unordered_map<std::string, MemberValue *> _members;
		Value *const _type;

		inline void addMember(std::string name, MemberValue *value) {
			value->incRefCount();
			_members[name] = value;
		}

		class MyValueIterator : public ValueIterator {
		protected:
			decltype(_members)::iterator it;

		public:
			inline MyValueIterator(decltype(it) &&it) : it(it) {}
			inline MyValueIterator(decltype(it) &it) : it(it) {}
			inline MyValueIterator(MyValueIterator &&x) : it(x.it) {}
			inline MyValueIterator(MyValueIterator &x) : it(x.it) {}
			virtual inline ValueIterator &operator++() override {
				++it;
				return *this;
			}
			virtual inline ValueIterator &&operator++(int) override {
				auto o = *this;
				++it;
				return std::move(o);
			}
			virtual inline ValueIterator &operator--() override {
				--it;
				return *this;
			}
			virtual inline ValueIterator &&operator--(int) override {
				auto o = *this;
				--it;
				return std::move(o);
			}
			virtual inline Value *operator*() override {
				return it->second;
			}

			virtual inline bool operator==(const ValueIterator &&) const override { return true; }
			virtual inline bool operator!=(const ValueIterator &&) const override { return false; }

			virtual inline ValueIterator &operator=(const ValueIterator &&x) override {
				return *this = (const MyValueIterator &&)x;
			}

			inline MyValueIterator &operator=(const MyValueIterator &&x) {
				it = x.it;
				return *this;
			}
			inline MyValueIterator &operator=(const MyValueIterator &x) {
				return *this = std::move(x);
			}
		};

		friend class Runtime;

	public:
		inline ObjectValue(Runtime *rt, Value *type) : Value(rt), _type(type) {
			reportSizeToRuntime(sizeof(*this));
		}
		virtual inline ~ObjectValue() {
			if (!getRefCount())
				for (auto i : _members)
					i.second->decRefCount();
		}

		virtual inline Type getType() const override { return Type(ValueType::OBJECT, _type); }

		virtual inline Value *getMember(std::string name) override { return _members.count(name) ? _members.at(name) : nullptr; }
		virtual inline const Value *getMember(std::string name) const override { return _members.at(name); }

		virtual inline ValueIterator &begin() override { return MyValueIterator(_members.begin()); }
		virtual inline ValueIterator &end() override { return MyValueIterator(_members.end()); }

		ObjectValue(ObjectValue &) = delete;
		ObjectValue(ObjectValue &&) = delete;
		ObjectValue &operator=(const ObjectValue &) = delete;
		ObjectValue &operator=(const ObjectValue &&) = delete;
	};

	class StructValue : public MemberValue {
	protected:
		std::unordered_map<std::string, ValueRef<MemberValue, false>> _members;

	public:
		inline StructValue(Runtime *rt, AccessModifier access, Value *parent) : MemberValue(rt, access, parent) {
			reportSizeToRuntime(sizeof(*this));
		}
		virtual inline ~StructValue() {}

		virtual inline Type getType() const override { return ValueType::STRUCT; }

		virtual inline Value *getMember(std::string name) override {
			return *(_members.at(name));
		}
		virtual inline const Value *getMember(std::string name) const override { return *(_members.at(name)); }

		ObjectValue &operator=(const ObjectValue &) = delete;
		ObjectValue &operator=(const ObjectValue &&) = delete;
	};

	class RefValue final : public Value {
	public:
		std::vector<std::string> scopes;

		inline RefValue(Runtime *rt) : Value(rt) {
			reportSizeToRuntime(sizeof(*this));
		}
		virtual inline ~RefValue() {}
		virtual inline Type getType() const override { return ValueType::REF; }

		RefValue &operator=(const RefValue &) = delete;
		RefValue &operator=(const RefValue &&) = delete;
	};

	class VarValue final : public MemberValue {
	protected:
		ValueRef<Slake::Value, false> value;
		const Type type = Type(ValueType::ANY);

	public:
		inline VarValue(Runtime *rt, AccessModifier access, Type type, Value *parent)
			: MemberValue(rt, access, parent), type(type) {
			reportSizeToRuntime(sizeof(*this));
		}

		virtual inline ~VarValue() {}
		virtual inline Type getType() const override { return ValueType::VAR; }
		inline Type getVarType() const { return type; }

		virtual inline Value *getMember(std::string name) override {
			return value ? value->getMember(name) : nullptr;
		}
		virtual inline const Value *getMember(std::string name) const override {
			return value ? value->getMember(name) : nullptr;
		}

		ValueRef<> getValue() { return value; }
		void setValue(Value *value) {
			// if (value->getType() != type)
			//	throw std::runtime_error("Mismatched types");
			this->value = value;
		}

		VarValue &operator=(const VarValue &) = delete;
		VarValue &operator=(const VarValue &&) = delete;
	};

	class ArrayValue final : public Value {
	protected:
		std::deque<ValueRef<Value, false>> values;
		const Type type;

		class MyValueIterator : public ValueIterator {
		protected:
			decltype(values)::iterator it;

		public:
			inline MyValueIterator(decltype(it) &&it) : it(it) {}
			inline MyValueIterator(decltype(it) &it) : it(it) {}
			inline MyValueIterator(MyValueIterator &&x) : it(x.it) {}
			inline MyValueIterator(MyValueIterator &x) : it(x.it) {}
			virtual inline ValueIterator &operator++() override {
				++it;
				return *this;
			}
			virtual inline ValueIterator &&operator++(int) override {
				auto o = *this;
				++it;
				return std::move(o);
			}
			virtual inline ValueIterator &operator--() override {
				--it;
				return *this;
			}
			virtual inline ValueIterator &&operator--(int) override {
				auto o = *this;
				--it;
				return std::move(o);
			}
			virtual inline Value *operator*() override {
				return **it;
			}

			virtual inline bool operator==(const ValueIterator &&) const override { return true; }
			virtual inline bool operator!=(const ValueIterator &&) const override { return false; }

			virtual inline ValueIterator &operator=(const ValueIterator &&x) override {
				return *this = (const MyValueIterator &&)x;
			}

			inline MyValueIterator &operator=(const MyValueIterator &&x) {
				it = x.it;
				return *this;
			}
			inline MyValueIterator &operator=(const MyValueIterator &x) {
				return *this = std::move(x);
			}
		};

		friend class Runtime;

	public:
		inline ArrayValue(Runtime *rt, Type type)
			: Value(rt), type(type) {
			reportSizeToRuntime(sizeof(*this));
		}

		virtual inline ~ArrayValue() {
			for (auto i : values)
				delete *i;
		}
		virtual inline Type getType() const override { return ValueType::ARRAY; }
		inline Type getVarType() const { return type; }

		Value *operator[](std::uint32_t i) {
			if (i >= values.size())
				throw std::out_of_range("Out of array range");
			return *(values[i]);
		}

		virtual inline ValueIterator &begin() override { return MyValueIterator(values.begin()); }
		virtual inline ValueIterator &end() override { return MyValueIterator(values.end()); }

		VarValue &operator=(const VarValue &) = delete;
		VarValue &operator=(const VarValue &&) = delete;
	};

	class RootValue final : public Value {
	protected:
		std::unordered_map<std::string, ValueRef<Value, false>> _members;

		class MyValueIterator : public ValueIterator {
		protected:
			decltype(_members)::iterator it;

		public:
			inline MyValueIterator(decltype(it) &&it) : it(it) {}
			inline MyValueIterator(decltype(it) &it) : it(it) {}
			inline MyValueIterator(MyValueIterator &&x) : it(x.it) {}
			inline MyValueIterator(MyValueIterator &x) : it(x.it) {}
			virtual inline ValueIterator &operator++() override {
				++it;
				return *this;
			}
			virtual inline ValueIterator &&operator++(int) override {
				auto o = *this;
				++it;
				return std::move(o);
			}
			virtual inline ValueIterator &operator--() override {
				--it;
				return *this;
			}
			virtual inline ValueIterator &&operator--(int) override {
				auto o = *this;
				--it;
				return std::move(o);
			}
			virtual inline Value *operator*() override {
				return *(it->second);
			}

			virtual inline bool operator==(const ValueIterator &&) const override { return true; }
			virtual inline bool operator!=(const ValueIterator &&) const override { return false; }

			virtual inline ValueIterator &operator=(const ValueIterator &&x) override {
				return *this = (const MyValueIterator &&)x;
			}

			virtual inline MyValueIterator &operator=(const MyValueIterator &&x) {
				it = x.it;
				return *this;
			}
			inline MyValueIterator &operator=(const MyValueIterator &x) {
				return *this = std::move(x);
			}
		};

		friend class Runtime;

	public:
		inline RootValue(Runtime *rt)
			: Value(rt) {
			reportSizeToRuntime(sizeof(*this));
		}

		virtual inline ~RootValue() {
		}
		virtual inline Type getType() const override { return ValueType::ROOT; }

		virtual inline Value *getMember(std::string name) override {
			return _members.count(name) ? *(_members.at(name)) : nullptr;
		}
		virtual inline const Value *getMember(std::string name) const override {
			return _members.count(name) ? *(_members.at(name)) : nullptr;
		}

		virtual inline void addMember(std::string name, Value *value) {
			_members[name] = value;
		}

		virtual inline ValueIterator &begin() override { return MyValueIterator(_members.begin()); }
		virtual inline ValueIterator &end() override { return MyValueIterator(_members.end()); }

		RootValue &operator=(const RootValue &) = delete;
		RootValue &operator=(const RootValue &&) = delete;
	};

	struct Instruction final {
		Opcode opcode;
		ValueRef<> operands[3];
		std::uint8_t nOperands;

		inline std::uint8_t getOperandCount() {
			return nOperands;
		}
		inline ~Instruction() {
		}
	};

	class FnValue final : public MemberValue {
	protected:
		Instruction *const _body;
		const std::uint32_t _nIns;

		friend class Runtime;

	public:
		inline FnValue(Runtime *rt, std::uint32_t nIns, AccessModifier access, Value *parent)
			: _nIns(nIns),
			  _body(new Instruction[nIns]),
			  MemberValue(rt, access, parent) {
			if (!nIns)
				throw std::invalid_argument("Invalid instruction count");
			reportSizeToRuntime(sizeof(*this) + sizeof(Instruction) * nIns);
		}
		virtual inline ~FnValue() { delete[] _body; }

		inline std::uint32_t getInsCount() const noexcept { return _nIns; }
		inline const Instruction *getBody() const noexcept { return _body; }
		inline Instruction *getBody() noexcept { return _body; }
		virtual inline Type getType() const override { return ValueType::FN; }

		virtual ValueRef<> call(std::uint8_t nArgs, ValueRef<> *args) override;

		FnValue &operator=(const FnValue &) = delete;
		FnValue &operator=(const FnValue &&) = delete;
	};

	using NativeFnCallback = std::function<ValueRef<>(Runtime *rt, std::uint8_t nArgs, ValueRef<> *args)>;
	class NativeFnValue final : public MemberValue {
	protected:
		NativeFnCallback _body;

	public:
		inline NativeFnValue(Runtime *rt, NativeFnCallback body, AccessModifier access = 0, Value *parent = nullptr)
			: MemberValue(rt, access | ACCESS_NATIVE, parent), _body(body) {
			reportSizeToRuntime(sizeof(*this));
		}
		virtual inline ~NativeFnValue() {}

		inline const NativeFnCallback getBody() const noexcept { return _body; }
		virtual inline Type getType() const override { return ValueType::FN; }

		virtual ValueRef<> call(std::uint8_t nArgs, ValueRef<> *args) override { return _body(getRuntime(), nArgs, args); }

		NativeFnValue &operator=(const NativeFnValue &) = delete;
		NativeFnValue &operator=(const NativeFnValue &&) = delete;
	};

	class ModuleValue : public MemberValue {
	protected:
		std::unordered_map<std::string, ValueRef<Value, false>> _members;

		class MyValueIterator : public ValueIterator {
		protected:
			decltype(_members)::iterator it;

		public:
			inline MyValueIterator(decltype(it) &&it) : it(it) {}
			inline MyValueIterator(decltype(it) &it) : it(it) {}
			inline MyValueIterator(MyValueIterator &&x) : it(x.it) {}
			inline MyValueIterator(MyValueIterator &x) : it(x.it) {}
			virtual inline ValueIterator &operator++() override {
				++it;
				return *this;
			}
			virtual inline ValueIterator &&operator++(int) override {
				auto o = *this;
				++it;
				return std::move(o);
			}
			virtual inline ValueIterator &operator--() override {
				--it;
				return *this;
			}
			virtual inline ValueIterator &&operator--(int) override {
				auto o = *this;
				--it;
				return std::move(o);
			}
			virtual inline Value *operator*() override {
				return *(it->second);
			}

			virtual inline bool operator==(const ValueIterator &&) const override { return true; }
			virtual inline bool operator!=(const ValueIterator &&) const override { return false; }

			virtual inline ValueIterator &operator=(const ValueIterator &&x) override {
				return *this = (const MyValueIterator &&)x;
			}

			inline MyValueIterator &operator=(const MyValueIterator &&x) {
				it = x.it;
				return *this;
			}
			inline MyValueIterator &operator=(const MyValueIterator &x) {
				return *this = std::move(x);
			}
		};

		friend class Runtime;

	public:
		inline ModuleValue(Runtime *rt, AccessModifier access, Value *parent) : MemberValue(rt, access, parent) {
			reportSizeToRuntime(sizeof(*this));
		}
		virtual inline ~ModuleValue() {}

		virtual inline Value *getMember(std::string name) override { return _members.count(name) ? *(_members.at(name)) : nullptr; }
		virtual inline const Value *getMember(std::string name) const override { return _members.count(name) ? *(_members.at(name)) : nullptr; }
		virtual inline Type getType() const override { return ValueType::MOD; }

		virtual inline void addMember(std::string name, MemberValue *value) {
			_members[name] = value;
		}

		virtual inline ValueIterator &begin() override { return MyValueIterator(_members.begin()); }
		virtual inline ValueIterator &end() override { return MyValueIterator(_members.end()); }

		ModuleValue &operator=(const ModuleValue &) = delete;
		ModuleValue &operator=(const ModuleValue &&) = delete;
	};

	class ClassValue : public ModuleValue {
	protected:
		Type _parentClass;
		std::vector<Type> _interfaces;

		friend class Runtime;

	public:
		inline ClassValue(Runtime *rt, AccessModifier access, Value *parent, Type parentClass = Type())
			: ModuleValue(rt, access, parent), _parentClass(parentClass) {
			reportSizeToRuntime(sizeof(*this) - sizeof(ModuleValue));
		}
		virtual inline ~ClassValue() {}

		virtual inline Type getType() const override { return ValueType::CLASS; }

		ClassValue &operator=(const ClassValue &) = delete;
		ClassValue &operator=(const ClassValue &&) = delete;
	};
}

#pragma clang diagnostic pop

#endif
