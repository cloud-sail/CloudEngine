#pragma once
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/IntVec3.hpp"
#include <functional>
#include <string>

// Boost
template <class T>
inline void hash_combine(std::size_t& seed, const T& v) {
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

namespace std {

	//-----------------------------------------------------------------------------------------------
	template <>
	struct hash<IntVec2> {
		size_t operator()(const IntVec2& p) const {
			size_t seed = 0;
			hash_combine(seed, p.x);
			hash_combine(seed, p.y);
			return seed;
		}
	};

	//-----------------------------------------------------------------------------------------------
	template <>
	struct hash<IntVec3> {
		size_t operator()(const IntVec3& v) const {
			size_t seed = 0;
			hash_combine(seed, v.x);
			hash_combine(seed, v.y);
			hash_combine(seed, v.z);
			return seed;
		}
	};


}






/**
struct Person {
	std::string name;
	int age;
	double salary;

	bool operator==(const Person& other) const {
		return name == other.name &&
			age == other.age &&
			salary == other.salary;
	}
};

// custom hash function
struct PersonHash {
	size_t operator()(const Person& p) const {
		size_t seed = 0;
		hash_combine(seed, p.name);
		hash_combine(seed, p.age);
		hash_combine(seed, p.salary);
		return seed;
	}
};

// Example
std::unordered_map<Person, int, PersonHash> personMap;
*/