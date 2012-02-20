#include "domain.h"
#include "el_syntax.h"
#include "model.h"
#include "../log.h"

#include <boost/shared_ptr.hpp>
#include <stdexcept>
#include <sstream>
#include <vector>

void Domain::addObservedPredicate(const Atom& a) {
    if (observations_.hasAtom(a)) return;
    SISet newSet(true, maxInterval_);
    obsPreds_.insert(std::pair<std::string, SISet>(a.name(), newSet));
    observations_.setAtom(a, newSet);
}

SISet Domain::getModifiableSISet(const std::string& name) const {
    SISet everywhere(isLiquid(name), maxInterval_);
    everywhere = everywhere.compliment();
    return getModifiableSISet(name, everywhere);
}

SISet Domain::getModifiableSISet(const std::string& name, const SISet& where) const {
    // check to see if its an obs predicate
    if (obsPreds_.find(name) == obsPreds_.end() || !dontModifyObsPreds()) {
        return where;
    }

    SISet modifiable(where.forceLiquid(), where.maxInterval());
    if (assumeClosedWorld()) return modifiable; // if its a closed world, can't change any obs predicates
    modifiable = where;

    modifiable.subtract(obsPreds_.find(name)->second);
    return modifiable;
}

void Domain::unsetAtomAt(const std::string& name, const SISet& where) {
    SISet newSet = obsPreds_.at(name);
    newSet.subtract(where);
    obsPreds_.erase(name);
    obsPreds_.insert(std::pair<std::string, SISet>(name, newSet));

    observations_.unsetAtom(name, where);


}


Model Domain::randomModel() const {
    // first check to see if we are even allowed to modify obs preds.  if not, just return the default model
    if (assumeClosedWorld()) {
        return defaultModel();
    }
    Model newModel;
    std::set<Atom, atomcmp> atoms = observations_.atoms();
    BOOST_FOREACH(Atom atom, atoms) {
        SISet random = SISet::randomSISet(isLiquid(atom.name()), maxInterval_);
        // intersect it with the places that are currently unset
        SISet unsetAt = getModifiableSISet(atom.name());
        random = intersection(random, unsetAt);
        // add in the set parts

        SISet setAt = unsetAt.compliment();
        SISet trueVals = intersection(setAt, observations_.getAtom(atom));
        random.add(trueVals);

        //newModel.clearAtom(obsPair->first);
        newModel.setAtom(atom, random);
    }
    return newModel;
}


void Domain::setMaxInterval(const Interval& maxInterval) {
    maxInterval_ = Interval(maxInterval);
    Model resized;

    observations_.setMaxInterval(maxInterval);
}

bool Domain::isLiquid(const std::string& predicate) const {
    // TODO: implement this sometime!
    return true;
}

unsigned long Domain::score(const ELSentence& w, const Model& m) const {
    SISet quantification = SISet(maxSpanInterval(), false, maxInterval());
    if (w.isQuantified()) quantification = w.quantification();

    SISet sat = w.sentence()->dSatisfied(m, *this, quantification);
    if (!sat.isDisjoint()) sat.makeDisjoint();
    return sat.size() * w.weight();
}

unsigned long Domain::score(const Model& m) const {
    unsigned long sum = 0;
    for (FormulaList::const_iterator it = formulas_.begin(); it != formulas_.end(); it++) {
        sum += score(*it, m);
    }
    return sum;
}

