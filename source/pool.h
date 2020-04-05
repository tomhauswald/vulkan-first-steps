#pragma once

#include <bitset>

#include "common.h"

template<typename Elem, size_t Capacity>
class Pool {
	std::array<Elem, Capacity> m_data;
	std::bitset<Capacity> m_occupied;
	size_t m_nextFreeSlot;
	size_t m_numElems;

public:
	Pool() : 
		m_data{},
		m_occupied{}, 
		m_nextFreeSlot{ 0 }{
	}
	
	size_t store(Elem e) {
		
		crashIf(m_numElems == Capacity);

		auto index = m_nextFreeSlot;
		m_data[index] = std::move(e);
		m_occupied[index] = true;
		m_numElems++;
		
		// Find next free slot.
		while (m_occupied[m_nextFreeSlot]) {
			m_nextFreeSlot = (m_nextFreeSlot + 1) % Capacity;
		}
		
		return index;
	}

	void erase(size_t index) {
		crashIf(index >= Capacity || !m_occupied[index]);
		m_data[index] = {}; // Call dtor of element.
		m_occupied[index] = false;
		m_numElems--;
		m_nextFreeSlot = std::min(index, m_nextFreeSlot);
	}

	inline size_t size() const { 
		return m_numElems; 
	}

	constexpr size_t capacity() const { 
		return Capacity; 
	}

	inline Elem& operator[](size_t index) {
		crashIf(index >= Capacity || !m_occupied[index]);
		return m_data[index];
	}

	inline Elem const& operator[](size_t index) const { 
		return (*this)[index];
	}
};