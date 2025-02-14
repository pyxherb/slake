#include "string.h"

using namespace slake;
using namespace slake::util;

ANSIString::ANSIString(std::pmr::memory_resource *memoryResource) noexcept
	: memoryResource(memoryResource) {
}

ANSIString::ANSIString(std::pmr::memory_resource *memoryResource, size_t len, char c)
	: memoryResource(memoryResource) {
	data = (char *)memoryResource->allocate(len + 1);
	if (!data)
		throw std::bad_alloc();

	memset(data, c, len);
	data[len] = '\0';

	length = len;
}

ANSIString::ANSIString(std::pmr::memory_resource *memoryResource, const char *s)
	: memoryResource(memoryResource) {
	set(s);
}

ANSIString::ANSIString(const ANSIString &other) {
	data = (char *)memoryResource->allocate(other.length + 1);
	if (!data)
		throw std::bad_alloc();

	memcpy(data, other.data, other.length);
	data[other.length] = '\0';
	length = other.length;
}

ANSIString::ANSIString(ANSIString &&other) noexcept {
	data = other.data;
	length = other.length;

	other.data = nullptr;
	other.length = 0;
}

void ANSIString::clear() noexcept {
	if (data) {
		memoryResource->deallocate(data, length);
	}
	length = 0;
}

void ANSIString::discardAndResize(size_t newSize) {
	char *newData = (char *)memoryResource->allocate(newSize + 1);
	if (!newData)
		throw std::bad_alloc();

	clear();
	length = newSize;
}

void ANSIString::resizeUninitialized(size_t newSize) {
	char *newData = (char *)memoryResource->allocate(newSize + 1);
	if (!newData)
		throw std::bad_alloc();

	if (length < newSize) {
		memcpy(newData, data, length);
	} else {
		memcpy(newData, data, newSize);
		newData[newSize] = '\0';
	}

	clear();
	length = newSize;
}

void ANSIString::resize(size_t newSize) {
	char *newData = (char *)memoryResource->allocate(newSize + 1);
	if (!newData)
		throw std::bad_alloc();

	if (length < newSize) {
		memcpy(newData, data, length);
		memset(newData + length, 0, newSize - length + 1);
	} else {
		memcpy(newData, data, newSize);
		newData[newSize] = '\0';
	}

	clear();
	length = newSize;
}

char &ANSIString::at(size_t index) {
	return data[index];
}

const char &ANSIString::at(size_t index) const {
	return data[index];
}

ANSIString ANSIString::substr(size_t index, size_t length) const {
	if (index + length > this->length)
		throw std::out_of_range("Cannot slice substrings out of the string");

	size_t newStrLen = length - index;

	ANSIString newStr(memoryResource);
	newStr.resizeUninitialized(newStrLen);
	memcpy(data + index, newStr.data, length);
	newStr.data[length] = '\0';

	return newStr;
}

void ANSIString::set(const char *s) {
	size_t len = strlen(s);

	data = (char *)memoryResource->allocate(len + 1);
	if (!data)
		throw std::bad_alloc();

	memcpy(data, s, len);
	data[len] = '\0';
	length = len;
}
