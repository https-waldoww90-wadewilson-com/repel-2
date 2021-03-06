/*
 * uptest.cpp
 *
 *  Created on: Dec 12, 2011
 *      Author: joe
 */

#define BOOST_TEST_MODULE UPTest
#include "../src/config.h"
#define BOOST_TEST_MAIN
#ifdef USE_DYNAMIC_UNIT_TEST
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#else
#include <boost/test/included/unit_test.hpp>
#endif
#include <boost/shared_ptr.hpp>
#include <list>
#include <algorithm>
#include "logic/UnitProp.h"
#include "TestUtilities.h"
#include "SISet.h"
#include "SpanInterval.h"
#include "Interval.h"
#include "logic/ELSyntax.h"

BOOST_AUTO_TEST_CASE( qconstraints ) {
    boost::shared_ptr<Sentence> s = getAsSentence("<>{m} P(a)");
    boost::shared_ptr<DiamondOp> dia = boost::dynamic_pointer_cast<DiamondOp>(s);

    TQConstraints constraints(Interval(1,2));
    constraints.mustBeIn.add(SpanInterval(1,2,1,2));
    constraints.mustNotBeIn.add(SpanInterval(1,1,1,1));

    boost::shared_ptr<DiamondOp> diaCopy(new DiamondOp(dia->sentence(),dia->relations().begin(), dia->relations().end(), &constraints));
    BOOST_CHECK_EQUAL(diaCopy->toString(), "<>{m:&{[1:2]},\\{[1:1]}} P(a)");
}

BOOST_AUTO_TEST_CASE( simple_lit ) {
    std::string facts = "video(a) @ [1:20]";
    std::string formulas = "P(a) @ [1:3]\n"
            "!P(a) @ [8:11]\n"
            "Q(a) @ [1:10]\n"
            "P(a) v Q(a) v R(a) @ [1:20]";

    Domain d = loadDomainWithStreams(facts, formulas);

    std::vector<ELSentence> flist(d.formulas_begin(), d.formulas_end());
    QCNFClauseList clauseList = convertToQCNFClauseList(flist);

    QUnitsFormulasPair result = performUnitPropagation(clauseList);

    for(QCNFLiteralList::const_iterator it = result.first.begin(); it != result.first.end(); it++) {
        QCNFLiteral lit = *it;
        ELSentence s = convertFromQCNFClause(lit);
        std::cout << "unit clause: " << s.toString() << std::endl;
    }

    for(QCNFClauseList::const_iterator it = result.second.begin(); it != result.second.end(); it++) {
        QCNFClause c = *it;
        ELSentence s = convertFromQCNFClause(c);
        std::cout << "formula: " << s.toString() << std::endl;
    }
    BOOST_REQUIRE_EQUAL(result.first.size(), 3);
    // TODO: write test
}

BOOST_AUTO_TEST_CASE( dia_lit ) {
    std::string facts = "video(a) @ [1:20]";
    std::string formulas = "P(a) @ [1:3]\n"
            "<>{d} P(a) v Q(a) @ [1:4]\n";

    Domain d = loadDomainWithStreams(facts, formulas);

    std::vector<ELSentence> flist(d.formulas_begin(), d.formulas_end());
    QCNFClauseList clauseList = convertToQCNFClauseList(flist);

    QUnitsFormulasPair result = performUnitPropagation(clauseList);

    for(QCNFLiteralList::const_iterator it = result.first.begin(); it != result.first.end(); it++) {
        QCNFLiteral lit = *it;
        ELSentence s = convertFromQCNFClause(lit);
        std::cout << "unit clause: " << s.toString() << std::endl;
    }

    for(QCNFClauseList::const_iterator it = result.second.begin(); it != result.second.end(); it++) {
        QCNFClause c = *it;
        ELSentence s = convertFromQCNFClause(c);
        std::cout << "formula: " << s.toString() << std::endl;
    }
}

BOOST_AUTO_TEST_CASE( simple_lit_directly ) {

    std::string facts = "video(a) @ [1:20]";
    std::string formulas = "P(a) @ [1:10]\n"
            "Q(a) @ [1:10]\n"
            "P(a) v Q(a) @ [1:20]";

    Domain d = loadDomainWithStreams(facts, formulas);

    std::vector<ELSentence> flist(d.formulas_begin(), d.formulas_end());
    ELSentence s1 = flist[0];
    ELSentence s2 = flist[1];
    ELSentence s3 = flist[2];

    QCNFClause qs1 = convertToQCNFClause(s1);
    QCNFClause qs2 = convertToQCNFClause(s2);
    QCNFClause qs3 = convertToQCNFClause(s3);

    QCNFLiteral l1(qs1.first.front(), qs1.second);
    /*
    l1.first = qs1.first.front();
    l1.second = qs1.second;
    */
    QCNFClauseList list = propagateLiteral(l1, qs3);
    BOOST_REQUIRE_EQUAL(list.size(), 1);
    BOOST_CHECK_EQUAL(list.front().second.toString(), "{[(1, 10), (11, 20)], [11:20]}");

    // try wrapping l1 in a negation and propagating
    boost::shared_ptr<Sentence> negl1(new Negation(l1.first));
    l1.first = negl1;

    list = propagateLiteral(l1, qs3);
    BOOST_REQUIRE_EQUAL(list.size(), 2);
    QCNFClauseList::const_iterator it = list.begin();
    ELSentence first = convertFromQCNFClause(*it);
    it++;
    ELSentence second = convertFromQCNFClause(*it);
    BOOST_CHECK_EQUAL(first.sentence()->toString(), "Q(a)");
    BOOST_CHECK_EQUAL(first.quantification().toString(), "{[1:10]}");

    BOOST_CHECK_EQUAL(second.sentence()->toString(), "P(a) v Q(a)");
    BOOST_CHECK_EQUAL(second.quantification().toString(), "{[(1, 10), (11, 20)], [11:20]}");
}

BOOST_AUTO_TEST_CASE( simpleLitToLiq) {
    std::string facts = "P(a) @ [1:20]\n";
    std::string formulas = "[ P(a) ] v Q(a) @ [1:30]\n";

    Domain d = loadDomainWithStreams(facts, formulas);
    BOOST_CHECK_EQUAL(d.formulas_size(), 1);
    Domain newD = performUnitPropagation(d);
    BOOST_REQUIRE_EQUAL(newD.formulas_size(), 1);

    std::ostringstream str;
    std::copy(newD.formulas_begin(), newD.formulas_end(), std::ostream_iterator<ELSentence>(str, ", "));
    BOOST_CHECK_EQUAL(str.str(), "inf: [ P(a) ] v Q(a) @ {[(1, 20), (21, 30)], [21:30]}, ");
    //str.str("");
    //std::copy(newD.facts_begin(), newD.facts_end(), std::ostream_iterator<ELSentence>(str, ", "));
    /*
    std::vector<std::pair<Proposition, SISet> > newFacts(newD.facts_begin(), newD.facts_end());
    BOOST_REQUIRE_EQUAL(newFacts.size(), 1);
    BOOST_CHECK_EQUAL(newf, "P(a) @ {[1:20]}, !P(a) @ {[21:30]}, Q(a) @ {[(1, 20), (21, 30)], [21:30]}, ");
    */

}

BOOST_AUTO_TEST_CASE( cnfConvertBasic ) {

    boost::shared_ptr<Sentence> a = getAsSentence("P(a) v Q(a) v !R(a) v S(a)");
    CNFClause c = convertToCNFClause(a);

    BOOST_REQUIRE_EQUAL(c.size(), 4);
    boost::shared_ptr<Sentence> pa = getAsSentence("P(a)");
    boost::shared_ptr<Sentence> qa = getAsSentence("Q(a)");
    boost::shared_ptr<Sentence> nra = getAsSentence("!R(a)");
    boost::shared_ptr<Sentence> sa = getAsSentence("S(a)");
    /* TODO: why doesn't the below work?
    BOOST_CHECK(std::find(c.begin(), c.end(), pa) != c.end());
    BOOST_CHECK(std::find(c.begin(), c.end(), qa) != c.end());
    BOOST_CHECK(std::find(c.begin(), c.end(), nra) != c.end());
    BOOST_CHECK(std::find(c.begin(), c.end(), sa) != c.end());
     */
}

BOOST_AUTO_TEST_CASE( contradictionTest ) {
    std::string facts = "P(a) @ [1:20]\n"
                        "Q(a) @ [20:30]\n";
    std::string formulas = "inf: [!P(a) v !Q(a)] @ [1:30]\n";
    Domain d = loadDomainWithStreams(facts, formulas);
    BOOST_CHECK_THROW(performUnitPropagation(d), contradiction);


}
