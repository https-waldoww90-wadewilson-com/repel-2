#include "domain.h"
#include "fol.h"
#include "atom.h"

#include <boost/shared_ptr.hpp>
#include <stdexcept>
#include <sstream>
#include <vector>

std::string modelToString(const Model& m) {
	std::stringstream stream;

	for (Model::const_iterator it = m.begin(); it != m.end(); it++) {
		const Atom a = it->first;
		SISet set = it->second;
		stream << a.toString() << " @ " << set.toString() << std::endl;
	}
	return stream.str();
}

void Domain::setMaxInterval(const Interval& maxInterval) {
	maxInterval_ = Interval(maxInterval);
	Model resized;
	for (Model::iterator it = observations_.begin(); it != observations_.end(); it++) {
		const Atom atom = it->first;
		SISet set = it->second;
		set.setMaxInterval(maxInterval);
		if (set.size() != 0) {
			resized.insert(std::pair<const Atom, SISet>(atom,set));
		}
	}
	observations_.swap(resized);
}

bool Domain::isLiquid(const std::string& predicate) const {
	// TODO: implement this sometime!
	return true;
}

unsigned long Domain::score(const WSentence& s, const Model& m) const {
	SISet sat = satisfied(*(s.sentence()), m);
	return sat.size() * s.weight();
}

unsigned long Domain::score(const Model& m) const {
	unsigned long sum = 0;
	for (std::vector<WSentence>::const_iterator it = formulas_.begin(); it != formulas_.end(); it++) {
		sum += score(*it, m);
	}
	return sum;
}

SISet Domain::satisfied(const Sentence& s, const Model& m) const {
	// start with the base case
	if (dynamic_cast<const Atom *>(&s) != 0) {
		const Atom* a = dynamic_cast<const Atom *>(&s);
		return satisfiedAtom(*a, m);
	} else if (dynamic_cast<const Negation*>(&s) != 0) {
		const Negation* n = dynamic_cast<const Negation *>(&s);
		SISet set = satisfiedNegation(*n, m);
		//set.makeDisjoint();
		return set;
	} else if (dynamic_cast<const Disjunction*>(&s) != 0) {
		const Disjunction* d = dynamic_cast<const Disjunction *>(&s);
		SISet set = satisfiedDisjunction(*d, m);
		//set.makeDisjoint();
		return set;
	} else if (dynamic_cast<const LiquidOp*>(&s) != 0) {
		const LiquidOp* l = dynamic_cast<const LiquidOp *>(&s);
		SISet set = liqSatisfied(*(l->sentence()), m);
		// remove liquid restriction
		set.setForceLiquid(false);
		//set.makeDisjoint();
		return set;
	} else if (dynamic_cast<const DiamondOp*>(&s) != 0) {
		const DiamondOp* d = dynamic_cast<const DiamondOp *>(&s);
		SISet set = satisfiedDiamond(*d, m);
		//set.makeDisjoint();
		return set;
	} else if (dynamic_cast<const Conjunction*>(&s) != 0) {
		const Conjunction* c = dynamic_cast<const Conjunction *>(&s);
		SISet set = satisfiedConjunction(*c, m);
		return set;
	} else if (dynamic_cast<const BoolLit *>(&s) != 0) {
		const BoolLit* b = dynamic_cast<const BoolLit *>(&s);
		return satisfiedBoolLit(*b, m);
	}
	// made it this far, unimplemented!
	std::runtime_error e("Domain::satisfied not implemented yet!");
	throw e;
}

SISet Domain::satisfiedAtom(const Atom& a, const Model& m) const {
	if (a.isGrounded()) {
		// make sure its in model
		if (m.find(a) != m.end()){
			SISet set = m.at(a);
			// force non-liquidity
			set.setForceLiquid(false);
			return set;
		} else {
			return SISet(false, maxInterval_);
		}
	}
	// grounding out of atoms currently not implemented yet!
	std::runtime_error e("Domain::satisfiedAtom grounding out of atoms not implemented!");
	throw e;
}

SISet Domain::satisfiedNegation(const Negation& n, const Model& m) const {
	// return the compliment of the set inside the negation
	SISet sat = satisfied(*(n.sentence()), m);
	sat = sat.compliment();
	return sat;
}

SISet Domain::satisfiedDisjunction(const Disjunction& d, const Model& m) const {
	// union the left and the right
	SISet leftSat = satisfied(*(d.left()),m);
	SISet rightSat = satisfied(*(d.right()), m);
	leftSat.add(rightSat);
	return leftSat;
}

SISet Domain::satisfiedDiamond(const DiamondOp& d, const Model& m) const {
	SISet sat = satisfied(*(d.sentence()), m);
	SISet newsat(false, sat.maxInterval());
	BOOST_FOREACH(SpanInterval sp, sat.set()) {
		BOOST_FOREACH(Interval::INTERVAL_RELATION rel, d.relations()) {
			boost::optional<SpanInterval> spR = sp.satisfiesRelation(rel);
			if (spR) newsat.add(spR.get());
		}
	}
	return newsat;
}

SISet Domain::satisfiedConjunction(const Conjunction& c, const Model& m) const {
	SISet leftSat = satisfied(*(c.left()), m);
	SISet rightSat = satisfied(*(c.right()), m);

	SISet result(false, leftSat.maxInterval());

	BOOST_FOREACH(SpanInterval i, leftSat.set()) {
		BOOST_FOREACH(SpanInterval j, rightSat.set()) {
			BOOST_FOREACH(Interval::INTERVAL_RELATION rel, c.relations()) {
				result.add(composedOf(i, j, rel));
			}
		}
	}

	return result;
}

SISet Domain::satisfiedBoolLit(const BoolLit& b, const Model& m) const {
	SISet toReturn(false, maxInterval_);
	if (b.value()) {
		// max interval
		SpanInterval max(maxInterval_.start(), maxInterval_.finish(), maxInterval_.start(), maxInterval_.finish(), maxInterval_);
		toReturn.add(max);
	}
	return toReturn;
}


SISet Domain::liqSatisfied(const Sentence& s, const Model& m) const {
	if (dynamic_cast<const Atom *>(&s) != 0) {
		const Atom* a = dynamic_cast<const Atom *>(&s);
		return liqSatisfiedAtom(*a, m);
	} else if (dynamic_cast<const Negation *>(&s) != 0) {
		const Negation* n = dynamic_cast<const Negation *>(&s);
		return liqSatisfiedNegation(*n, m);
	} else if (dynamic_cast<const Disjunction *>(&s) != 0) {
		const Disjunction* d = dynamic_cast<const Disjunction *>(&s);
		return liqSatisfiedDisjunction(*d, m);
	} else if (dynamic_cast<const Conjunction *>(&s) != 0) {
		const Conjunction* c = dynamic_cast<const Conjunction *>(&s);
		return liqSatisfiedConjunction(*c, m);
	} else if (dynamic_cast<const BoolLit *>(&s) != 0) {
		const BoolLit* b = dynamic_cast<const BoolLit *>(&s);
		return liqSatisfiedBoolLit(*b, m);
	}
	// made it this far, unimplemented!
	std::runtime_error e("Domain::liqSatisfied not implemented yet!");
	throw e;
}

SISet Domain::liqSatisfiedAtom(const Atom& a, const Model& m) const {
	if (!a.isGrounded()) {
		std::runtime_error e("Domain::liqSatisfiedAtom currently doesn't support variables!");
		throw e;
	}
	// make sure its in model
	if (m.find(a) != m.end()){
		SISet set = m.at(a);
		set.setForceLiquid(true);
		return set;
	} else {
		return SISet(true, maxInterval_);
	}
}

SISet Domain::liqSatisfiedNegation(const Negation& n, const Model& m) const {
	SISet sat = liqSatisfied(*(n.sentence()), m);
	sat = sat.compliment();
	return sat;
}

SISet Domain::liqSatisfiedDisjunction(const Disjunction& d, const Model& m) const {
	SISet leftSat = liqSatisfied(*(d.left()),m);
	SISet rightSat = liqSatisfied(*(d.right()), m);
	leftSat.add(rightSat);
	return leftSat;
}

SISet Domain::liqSatisfiedConjunction(const Conjunction& c, const Model& m) const {
	SISet leftSat = liqSatisfied(*(c.left()), m);
	SISet rightSat = liqSatisfied(*(c.right()), m);
	// intersection now!
	return intersection(leftSat, rightSat);
}

SISet Domain::liqSatisfiedBoolLit(const BoolLit& b, const Model& m) const {
	SISet toReturn(true, maxInterval_);
	if (b.value()) {
		// max interval
		SpanInterval max(maxInterval_.start(), maxInterval_.finish(), maxInterval_.start(), maxInterval_.finish(), maxInterval_);
		toReturn.add(max);
	}
	return toReturn;
}
