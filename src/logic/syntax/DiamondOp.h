#ifndef DIAMONDOP_H
#define DIAMONDOP_H

#include <boost/shared_ptr.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/export.hpp>
#include <set>
#include "Sentence.h"
#include "SentenceVisitor.h"
#include "../../Interval.h"

class DiamondOp : public Sentence {
public:
    static const std::size_t TypeCode = 3;

    static const std::set<Interval::INTERVAL_RELATION>& defaultRelations();

    DiamondOp();
    DiamondOp(boost::shared_ptr<Sentence> sentence, const TQConstraints* tqconstraints=0);
    DiamondOp(boost::shared_ptr<Sentence> sentence,
            Interval::INTERVAL_RELATION relation,
            const TQConstraints* tqconstraints=0);
    template <class InputIterator>
    DiamondOp(boost::shared_ptr<Sentence> sentence,
            InputIterator begin,
            InputIterator end,
            const TQConstraints* tqconstraints=0);
    DiamondOp(const DiamondOp& dia); // shallow copy
    virtual ~DiamondOp();

    friend void swap(DiamondOp& left, DiamondOp& right);
    DiamondOp& operator=(DiamondOp other);

    boost::shared_ptr<Sentence> sentence();
    boost::shared_ptr<const Sentence> sentence() const;
    const std::set<Interval::INTERVAL_RELATION>& relations() const;
    const TQConstraints& tqconstraints() const;

    void setSentence(boost::shared_ptr<Sentence> s);
    template<typename T>
    void setRelations(T begin, T end);
    void setTQConstraints(const TQConstraints& tq);

    friend std::size_t hash_value(const DiamondOp& d);
    virtual std::size_t getTypeCode() const;
    virtual SISet satisfied(const Model& m, const Domain& d, bool forceLiquid) const;
private:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version);

    std::set<Interval::INTERVAL_RELATION> rels_;
    boost::shared_ptr<Sentence> s_;
    TQConstraints tqconstraints_;   // TODO: make these optional?

    virtual Sentence* doClone() const;
    virtual bool doEquals(const Sentence& s) const;

    virtual void doToString(std::stringstream& str) const;

    virtual int doPrecedence() const;
    virtual void visit(SentenceVisitor& v) const;
    virtual bool doContains(const Sentence& s) const;
    virtual std::size_t doHashValue() const;
};


// implementation below
// constructors
inline DiamondOp::DiamondOp()
    : rels_(), s_(), tqconstraints_() {}
inline DiamondOp::DiamondOp(boost::shared_ptr<Sentence> sentence, const TQConstraints* tqconstraints)
    : rels_(), s_(sentence), tqconstraints_() {
        rels_ = std::set<Interval::INTERVAL_RELATION>(DiamondOp::defaultRelations().begin(), DiamondOp::defaultRelations().end());
        if (tqconstraints) tqconstraints_ = *tqconstraints;
}
inline DiamondOp::DiamondOp(boost::shared_ptr<Sentence> sentence,
        Interval::INTERVAL_RELATION relation,
        const TQConstraints* tqconstraints)
    : rels_(), s_(sentence), tqconstraints_() {
    rels_.insert(relation);
    if (tqconstraints) tqconstraints_ = *tqconstraints;
}
template <class InputIterator>
inline DiamondOp::DiamondOp(boost::shared_ptr<Sentence> sentence,
        InputIterator begin,
        InputIterator end,
        const TQConstraints* tqconstraints)
        : rels_(begin, end), s_(sentence), tqconstraints_() {
    if (tqconstraints) tqconstraints_ = *tqconstraints;
}
// copy constructor
inline DiamondOp::DiamondOp(const DiamondOp& dia)
    : rels_(dia.rels_), s_(dia.s_), tqconstraints_(dia.tqconstraints_) {}; // shallow copy
inline DiamondOp::~DiamondOp() {}

// public methods
inline void swap(DiamondOp& left, DiamondOp& right) {
    using std::swap;
    swap(left.s_, right.s_);
    swap(left.rels_, right.rels_);
}

inline DiamondOp& DiamondOp::operator=(DiamondOp other) {
    swap(*this, other);
    return *this;
}

inline boost::shared_ptr<Sentence> DiamondOp::sentence() {return s_;}
inline boost::shared_ptr<const Sentence> DiamondOp::sentence() const {return s_;}
inline const std::set<Interval::INTERVAL_RELATION>& DiamondOp::relations() const {return rels_;}
inline const TQConstraints& DiamondOp::tqconstraints() const {return tqconstraints_;}

inline void DiamondOp::setSentence(boost::shared_ptr<Sentence> s) {s_ = s;}
template<typename T>
inline void DiamondOp::setRelations(T begin, T end) {rels_.clear(); std::copy(begin, end, std::inserter(rels_, rels_.end()));}
inline void DiamondOp::setTQConstraints(const TQConstraints& tq) {tqconstraints_ = tq;}

inline std::size_t hash_value(const DiamondOp& d) {
    std::size_t seed = DiamondOp::TypeCode;
    boost::hash_range(seed, d.rels_.begin(), d.rels_.end());
    boost::hash_combine(seed, *d.s_);
    // TODO: we can't currently hash SISets, so we just ignore TQConstraints
    // This is ok, but causes conflicts and reduces performance.
    return seed;
}

inline std::size_t DiamondOp::getTypeCode() const {
    return DiamondOp::TypeCode;
}
// private methods
inline Sentence* DiamondOp::doClone() const { return new DiamondOp(*this); }
inline bool DiamondOp::doEquals(const Sentence& s) const {
    const DiamondOp *dia = dynamic_cast<const DiamondOp*>(&s);
    if (dia == NULL) {
        return false;
    }
    return *s_ == *(dia->s_);
}
inline int DiamondOp::doPrecedence() const { return 2; };
inline bool DiamondOp::doContains(const Sentence& s) const {
    if (*this == s) return true;
    return (s_->contains(s));
}

inline void DiamondOp::visit(SentenceVisitor& v) const {
    s_->visit(v);

    v.accept(*this);
}

inline std::size_t DiamondOp::doHashValue() const { return hash_value(*this);}

template <class Archive>
void DiamondOp::serialize(Archive& ar, const unsigned int version) {
    // register that there is no need to call the base class serialize
    boost::serialization::void_cast_register<DiamondOp, Sentence>(
            static_cast<DiamondOp*>(NULL),
            static_cast<Sentence*>(NULL)
    );

    ar & rels_;
    ar & s_;
    ar & tqconstraints_;
}

#endif
