/*
 * MaxWalkSat.cpp
 *
 *  Created on: Mar 19, 2012
 *      Author: selman.joe@gmail.com
 */

#include "MaxWalkSat.h"
#include "../logic/Domain.h"
#include "../logic/Moves.h"
#include <boost/random/uniform_int.hpp>
#include <boost/random/mersenne_twister.hpp>

const unsigned int MWSSolver::defNumIterations = 1000;
const double MWSSolver::defProbOfRandomMove = 0.2;

Model MWSSolver::run(boost::mt19937& rng) {
    if (domain_ == NULL) {
        std::logic_error e("unable to run MWSSolver with Domain set to null ptr");
        throw e;
    }
    return run(rng, domain_->defaultModel());
}

Model MWSSolver::run(boost::mt19937& rng, const Model& initialModel) {
    std::runtime_error e("MWSSolver::run() not implemented.");
    throw e;
}

/*
Model maxWalkSat(Domain& d,
        int numIterations,
        double probOfRandomMove,
        boost::mt19937& rng,
        const Model* initialModel,
        std::ostream* dataout) {
    row_out datalog(dataout);

    Model currentModel(d.maxInterval());
    if (initialModel==0) currentModel = d.defaultModel();
    else currentModel = *initialModel;

    // filter out sentences we can't currently generate moves for
    std::vector<int> validForms;
    //std::vector<int> validNorm;
    std::vector<ELSentence> formulas(d.formulas_begin(), d.formulas_end());
    //std::vector<ELSentence> formulas = formSet.formulas();
    for (std::vector<ELSentence>::size_type i = 0; i < formulas.size(); i++) {
        ELSentence form = formulas[i];
        if (form.hasInfWeight()) {
            throw std::invalid_argument("maxWalkSat(): can't solve a problem with infinite weights - rewrite first");
        }
        if (canFindMovesFor(*(form.sentence()), d)) {
            validForms.push_back(i);
        } else {
            // TODO: use a logging warning instead of stderr
            //std::cerr << "WARNING: currently cannot generate moves for sentence: \"" << d.formulas().at(i).sentence()->toString() << "\"." << std::endl;
            LOG(LOG_WARN) << "currently cannot generate moves for sentence: \"" <<form.sentence()->toString() << "\".";
        }
    }
    if (validForms.size() ==0) {
        // TODO: log an error
        std::cerr << "ERROR: no valid sentences to generate moves for!" << std::endl;
        return currentModel;
    }

    AtomOccurences occurs = findAtomOccurences(formulas);
    std::vector<double> formScores;
    double currentScore = 0.0;
    for (unsigned int i = 0; i < formulas.size(); i++) {
        ELSentence formula = formulas[i];
        double localScore = d.score(formula, currentModel);
        formScores.push_back(localScore);
        currentScore += localScore;
    }

    // initialize best score to the current score
    double bestScore = currentScore;
    Model bestModel = currentModel;


    unsigned int showPeriodMod = (numIterations < 20 ? 1 : numIterations/20);

    for (int iteration=1; iteration <= numIterations; iteration++) {
        if (iteration % showPeriodMod == 0) {
            std::cout << ".";
            std::cout.flush();
        }
        LOG(LOG_DEBUG) << "currentModel: " << currentModel;
        LOG(LOG_DEBUG) << "current score: " << currentScore;

        datalog << currentScore;

        // make a list of the current unsatisfied formulas we can calc moves for
        std::vector<int> notFullySatisfied = validForms;
        std::vector<ELSentence> curFormulas = formulas;

        for (std::vector<int>::iterator it = notFullySatisfied.begin(); it != notFullySatisfied.end(); ) {
            int i = *it;

            ELSentence wsent = curFormulas.at(i);
            //const WSentence *wsentence = *it;
            if (wsent.fullySatisfied(currentModel, d)) {
                it = notFullySatisfied.erase(it);
            } else {
                it++;
            }
        }

        if (notFullySatisfied.size()==0) {
            // can't really improve on this
            LOG(LOG_INFO) << "no more sentences to satisfy!  exiting early after "<< iteration-1 << " iterations";
            return currentModel;
        }

        // pick one at random
        boost::uniform_int<std::size_t> curFormUniformPick(0, notFullySatisfied.size()-1);
        ELSentence toImprove = curFormulas.at(notFullySatisfied.at(curFormUniformPick(rng)));
        LOG(LOG_DEBUG) << "choosing formula: " << toImprove << " to improve.";
        // find the set of moves that improve it
        std::vector<Move> moves = findMovesFor(d, currentModel, toImprove, rng);
        if (moves.size() == 0) {
            LOG(LOG_WARN) << "WARNING: unable to find moves for sentence " << toImprove.sentence()->toString()
                    << " but couldn't find any (even though its violated)!  continuing...";
            continue; // TODO: this shouldn't happen, right?
        }
        if (FileLog::globalLogLevel() <= LOG_DEBUG) {
            std::ostringstream vecStream;
            for (std::vector<Move>::const_iterator it = moves.begin(); it != moves.end(); it++) {
                if (it != moves.begin()) vecStream << ", ";
                vecStream << "(" << it->toString() << ")";
            }
            LOG(LOG_DEBUG) << "moves to consider: " << vecStream.str();
        }
        boost::bernoulli_distribution<> randMovePick(probOfRandomMove);
        if (randMovePick(rng)) {
            // take a random move
            boost::uniform_int<std::size_t> movesPick(0, moves.size()-1);
            Move aMove = moves[movesPick(rng)];
            LOG(LOG_DEBUG) << "taking random move: " << aMove.toString();
            currentModel = executeMove(d, aMove, currentModel);
            score_pair scorePair = computeScoresForMove(d, currentModel, aMove, currentScore, formScores, occurs);
            currentScore = scorePair.totalScore;
            formScores = scorePair.formScores;
        } else {
            // find the models resulting from each move, and choose the highest scoring model as our next model
            double bestLocalScore = 0.0;
            std::vector<Model> bestLocalModels;
            std::vector<Move> bestLocalMoves;
            std::vector<score_pair> bestLocalScorePairs;

            //bestLocalModels.push_back(currentModel);
            for (std::vector<Move>::const_iterator it=moves.begin(); it != moves.end(); it++) {
                Model nextModel = executeMove(d, *it, currentModel);
                score_pair scorePair = computeScoresForMove(d, nextModel, *it, currentScore, formScores, occurs);
                double nextScore = scorePair.totalScore;
                if (nextScore > bestLocalScore) {
                    bestLocalModels.clear();
                    bestLocalMoves.clear();
                    bestLocalScorePairs.clear();

                    bestLocalScore = nextScore;
                    bestLocalModels.push_back(nextModel);
                    bestLocalMoves.push_back(*it);
                    bestLocalScorePairs.push_back(scorePair);
                } else if (nextScore == bestLocalScore) {
                    bestLocalModels.push_back(nextModel);
                    bestLocalMoves.push_back(*it);
                    bestLocalScorePairs.push_back(scorePair);
                }
            }
            boost::uniform_int<std::size_t> modelPick(0, bestLocalModels.size()-1);
            int idx = modelPick(rng);  // choose one at random
            currentModel = bestLocalModels[idx];
            score_pair scorePair = bestLocalScorePairs[idx];
            currentScore = scorePair.totalScore;
            formScores = scorePair.formScores;
            LOG(LOG_DEBUG) << "choosing best local move: " << bestLocalMoves[idx].toString();
        }
        // evaluate and see if our model is better than any found so far
        if (currentScore > bestScore) {
            LOG(LOG_DEBUG) << "remembering this model as best scoring so far";
            bestModel = currentModel;
            bestScore = currentScore;
        }
    }

    return bestModel;
}


namespace {
    AtomOccurences findAtomOccurences(const std::vector<ELSentence>& sentences) {
        // set up a mapping from atom to formula index.  this represents formulas where the atom occurs
        AtomCollector collector;
        AtomOccurences occurs;
        for (std::vector<ELSentence>::size_type i = 0; i < sentences.size(); i++) {
            ELSentence formula = sentences.at(i);

            collector.atoms.clear();
            formula.sentence()->visit(collector);
            // add this index to all occurrences of our atom
            BOOST_FOREACH(Atom a, collector.atoms) {
                FormSet set = occurs[a];
                if (set.count(i) == 0) {
                    set.insert(i);
                    occurs[a] = set;
                }
            }
        }

        return occurs;
    }

    score_pair computeScoresForMove(const Domain& d,
            const Model& m,
            const Move& move,
            double currentScore,
            const std::vector<double>& curFormScores,
            const AtomOccurences& occurs) {
        // first, find the formulas we need to recompute
        std::set<int> formsToRescore;
        std::vector<Move::change> allchanges(move.toAdd);
        std::copy(move.toDel.begin(), move.toDel.end(), std::back_inserter(allchanges));

        BOOST_FOREACH(Move::change change, allchanges) {
            Atom a = change.get<0>();
            if (occurs.count(a) > 0) {
                AtomOccurences::const_iterator it = occurs.find(a);
                FormSet changedForms = it->second;
                std::copy(changedForms.begin(), changedForms.end(), std::inserter(formsToRescore, formsToRescore.end()));
            }
        }

        score_pair pair;
        pair.formScores = curFormScores;
        pair.totalScore = currentScore;

        // start recomputing, adjusting the total score as necessary
        std::vector<ELSentence> formulas(d.formulas_begin(), d.formulas_end());
        for (std::set<int>::const_iterator it = formsToRescore.begin(); it != formsToRescore.end(); it++) {
            int formNum = *it;
            ELSentence sentence = formulas[formNum];

            double score = d.score(sentence, m);
            if (score != curFormScores.at(formNum)) {   // TODO: require some sort of epsilon check?
                double difference = score - curFormScores.at(formNum);
                pair.formScores[formNum] = score;
                pair.totalScore += difference;
            }
        }

        return pair;
    }
}
*/
