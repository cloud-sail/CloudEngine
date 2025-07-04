#pragma once
#include <functional>
#include <string>

// Boost
template <class T>
inline void hash_combine(std::size_t& seed, const T& v) {
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
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