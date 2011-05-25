/*
 * siset.h
 *
 *  Created on: May 24, 2011
 *      Author: joe
 */

#ifndef SISET_H_
#define SISET_H_
#include <set>
#include "spaninterval.h"
class SISet {
public:
	SISet(bool forceLiquid=false) : forceLiquid_(forceLiquid) {}
	template <class InputIterator>
	SISet(InputIterator begin, InputIterator end, bool forceLiquid=false) : set_(begin, end), forceLiquid_(forceLiquid) {}

	const std::set<SpanInterval>& set() const {return set_;};
	bool forceLiquid() const {return forceLiquid_;};

	bool isDisjoint() const;
	SISet compliment() const;

	// modifiers
	void add(const SpanInterval &s);
	void makeDisjoint();
	void clear() {set_.clear();};

	std::string toString() const;

private:
	bool forceLiquid_;
	std::set<SpanInterval> set_;
};

#endif /* SISET_H_ */
