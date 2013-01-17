/*
 *  SMT-RAT - Satisfiability-Modulo-Theories Real Algebra Toolbox
 * Copyright (C) 2012 Florian Corzilius, Ulrich Loup, Erika Abraham, Sebastian Junges
 *
 * This file is part of SMT-RAT.
 *
 * SMT-RAT is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SMT-RAT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SMT-RAT.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/*
 * File:   LRAModule.cpp
 * @author name surname <emailadress>
 *
 * @version 2012-04-05
 * Created on April 5th, 2012, 3:22 PM
 */


#include "TLRAModule.h"
#include <iostream>

//#define DEBUG_TLRA_MODULE
#define TLRA_SIMPLE_THEORY_PROPAGATION
#define TLRA_ONE_REASON

using namespace std;
using namespace tlra;
using namespace GiNaC;
#ifdef TLRA_USE_GINACRA
using namespace GiNaCRA;
#endif

namespace smtrat
{
    /**
     * Constructor
     */
    TLRAModule::TLRAModule( const Formula* const _formula, Manager* const _tsManager ):
        Module( _formula, _tsManager ),
        mInitialized( false ),
        mAssignmentFullfilsNonlinearConstraints( false ),
        mNumberOfReceivedLinearNeqConstraints( 0 ),
        mTableau( mpPassedFormula->end() ),
        mLinearConstraints(),
        mNonlinearConstraints(),
        mOriginalVars(),
        mSlackVars(),
        mConstraintToBound(),
        mBoundCandidatesToPass()
    {
        mModuleType = MT_TLRAModule;
    }

    /**
     * Destructor:
     */
    TLRAModule::~TLRAModule()
    {
        while( !mConstraintToBound.empty() )
        {
            vector< const Bound<Numeric>* >* toDelete = mConstraintToBound.back();
            mConstraintToBound.pop_back();
            if( toDelete != NULL ) delete toDelete;
        }
        while( !mOriginalVars.empty() )
        {
            const ex* exToDelete = mOriginalVars.begin()->first;
            mOriginalVars.erase( mOriginalVars.begin() );
            delete exToDelete;
        }
        while( !mSlackVars.empty() )
        {
            const ex* exToDelete = mSlackVars.begin()->first;
            mSlackVars.erase( mSlackVars.begin() );
            delete exToDelete;
        }
    }

    /**
     * Methods:
     */

    /**
     * Informs this module about the existence of the given constraint, which means
     * that it could be added in future.
     * @param _constraint The constraint to inform about.
     * @return False, if the it can be determined that the constraint itself is conflicting;
     *         True,  otherwise.
     */
    bool TLRAModule::inform( const Constraint* const _constraint )
    {
        #ifdef DEBUG_TLRA_MODULE
        cout << "inform about " << *_constraint << endl;
        #endif
        Module::inform( _constraint );
        if( !_constraint->variables().empty() && _constraint->isLinear() && _constraint->relation() != CR_NEQ )
        {
            mLinearConstraints.insert( _constraint );
            if( mInitialized )
            {
                initialize( _constraint );
            }
        }
        return _constraint->isConsistent() != 0;
    }

    /**
     * Adds a constraint to the so far received constraints.
     *
     * @param _subformula The position of the constraint within the received constraints.
     * @return False, if a conflict is detected;
     *         True,  otherwise.
     */
    bool TLRAModule::assertSubformula( Formula::const_iterator _subformula )
    {
        Module::assertSubformula( _subformula );
        if( (*_subformula)->getType() == REALCONSTRAINT )
        {
            #ifdef DEBUG_TLRA_MODULE
            cout << "assert " << (*_subformula)->constraint() << endl;
            #endif
            if( !mInitialized ) initialize();

            const Constraint* constraint  = (*_subformula)->pConstraint();
            int               consistency = constraint->isConsistent();
            if( consistency == 2 )
            {
                mAssignmentFullfilsNonlinearConstraints = false;
                if( constraint->isLinear() )
                {
                    if( (*_subformula)->constraint().relation() != CR_NEQ )
                    {
                        vector< const Bound<Numeric>* >* bounds = mConstraintToBound[constraint->id()];
                        assert( bounds != NULL );
                        set<const Formula*> originSet = set<const Formula*>();
                        originSet.insert( *_subformula );
                        activateBound( *bounds->begin(), originSet );

                        assert( mInfeasibleSubsets.empty() || !mInfeasibleSubsets.begin()->empty() );
                        return mInfeasibleSubsets.empty() || !mNonlinearConstraints.empty();
                    }
                    else
                    {
                        ++mNumberOfReceivedLinearNeqConstraints;
                    }
                }
                else
                {
                    addSubformulaToPassedFormula( new Formula( constraint ), *_subformula );
                    mNonlinearConstraints.insert( constraint );
                    return true;
                }
            }
            else if( consistency == 0 )
            {
                set< const Formula* > infSubSet = set< const Formula* >();
                infSubSet.insert( *_subformula );
                mInfeasibleSubsets.push_back( infSubSet );
                mSolverState = False;
                return false;
            }
            else
            {
                return true;
            }
        }
        return true;
    }

    /**
     * Removes a constraint of the so far received constraints.
     *
     * @param _subformula The position of the constraint within the received constraints.
     */
    void TLRAModule::removeSubformula( Formula::const_iterator _subformula )
    {
        if( (*_subformula)->getType() == REALCONSTRAINT )
        {
            #ifdef DEBUG_TLRA_MODULE
            cout << "remove " << (*_subformula)->constraint() << endl;
            #endif
            // Remove the mapping of the constraint to the sub-formula in the received formula
            const Constraint* constraint = (*_subformula)->pConstraint();
            if( constraint->isConsistent() == 2 )
            {
                if( constraint->isLinear() )
                {
                    // Deactivate the bounds regarding the given constraint
                    vector< const Bound<Numeric>* >* bounds = mConstraintToBound[constraint->id()];
                    assert( bounds != NULL );
                    auto bound = bounds->begin();
                    while( bound != bounds->end() )
                    {
                        if( !(*bound)->origins().empty() )
                        {
                            auto originSet = (*bound)->pOrigins()->begin();
                            while( originSet != (*bound)->origins().end() )
                            {
                                if( originSet->find( *_subformula ) != originSet->end() ) originSet = (*bound)->pOrigins()->erase( originSet );
                                else ++originSet;
                            }
                            if( (*bound)->origins().empty() )
                            {
                                (*bound)->pVariable()->deactivateBound( *bound, mpPassedFormula->end() );
                                if( !(*bound)->pVariable()->pSupremum()->isInfinite() )
                                {
                                    mBoundCandidatesToPass.push_back( (*bound)->pVariable()->pSupremum() );
                                }
                                if( !(*bound)->pVariable()->pInfimum()->isInfinite() )
                                {
                                    mBoundCandidatesToPass.push_back( (*bound)->pVariable()->pInfimum() );
                                }
                                if( ((*bound)->isUpperBound() && (*bound)->variable().pSupremum()->isInfinite())
                                    || ((*bound)->isLowerBound() && (*bound)->variable().pInfimum()->isInfinite()) )
                                {
                                    if( (*bound)->variable().isBasic() )
                                    {
                                        mTableau.decrementBasicActivity( (*bound)->variable() );
                                    }
                                    else
                                    {
                                        mTableau.decrementNonbasicActivity( (*bound)->variable() );
                                    }
                                }
                            }
                        }
                        if( (*bound)->origins().empty() && (*bound)->deduced() )
                        {
                            bound = bounds->erase( bound );
                        }
                        else
                        {
                            ++bound;
                        }
                    }
                }
                else
                {
                    ConstraintSet::iterator nonLinearConstraint = mNonlinearConstraints.find( constraint );
                    assert( nonLinearConstraint != mNonlinearConstraints.end() );
                    mNonlinearConstraints.erase( nonLinearConstraint );
                }
            }
        }
        Module::removeSubformula( _subformula );
    }

    /**
     * Checks the consistency of the so far received constraints.
     *
     * @return True,    if the so far received constraints are consistent;
     *         False,   if the so far received constraints are inconsistent;
     *         Unknown, if this module cannot determine whether the so far received constraints are consistent or not.
     */
    Answer TLRAModule::isConsistent()
    {
        #ifdef DEBUG_TLRA_MODULE
        cout << "check for consistency" << endl;
        #endif
        if( mpReceivedFormula->isConstraintConjunction() && mNumberOfReceivedLinearNeqConstraints == 0 )
        {
            if( !mInfeasibleSubsets.empty() )
            {
                return False;
            }
            unsigned posNewLearnedBound = 0;
            for( ; ; )
            {
                #ifdef DEBUG_TLRA_MODULE
                cout << endl;
                mTableau.printVariables( cout, "    " );
                cout << endl;
                mTableau.print( cout, 15, "    " );
                cout << endl;
                #endif

                pair<EntryID,bool> pivotingElement = mTableau.nextPivotingElement();

                #ifdef DEBUG_TLRA_MODULE
                cout << "    Next pivoting element: ";
                mTableau.printEntry( cout, pivotingElement.first );
                cout << (pivotingElement.second ? "(True)" : "(False)");
                cout << " [" << pivotingElement.first << "]" << endl;
                #endif

                if( pivotingElement.second )
                {
                    if( pivotingElement.first == 0 )
                    {
                        #ifdef DEBUG_TLRA_MODULE
                        cout << "True" << endl;
                        #endif
                        if( checkAssignmentForNonlinearConstraint() )
                        {
                            #ifdef TLRA_REFINEMENT
                            learnRefinements();
                            #endif
                            mSolverState = True;
                            return True;
                        }
                        else
                        {
                            adaptPassedFormula();
                            Answer a = runBackends();
                            if( a == False )
                            {
                                getInfeasibleSubsets();
                            }
                            #ifdef TLRA_REFINEMENT
                            learnRefinements();
                            #endif
                            mSolverState = a;
                            return a;
                        }
                    }
                    else
                    {
                        mTableau.pivot( pivotingElement.first );
                        while( posNewLearnedBound < mTableau.rLearnedBounds().size() )
                        {
                            set< const Formula*> originSet = set< const Formula*>();
                            vector<const Bound<Numeric>*>& bounds = *mTableau.rLearnedBounds()[posNewLearnedBound].premise;
                            for( auto bound = bounds.begin(); bound != bounds.end(); ++bound )
                            {
                                assert( !(*bound)->origins().empty() );
                                originSet.insert( (*bound)->origins().begin()->begin(), (*bound)->origins().begin()->end() );
                                for( auto origin = (*bound)->origins().begin()->begin(); origin != (*bound)->origins().begin()->end(); ++origin )
                                {
                                    const Constraint* constraint = (*origin)->pConstraint();
                                    if( constraint != NULL )
                                    {
                                        mConstraintToBound[constraint->id()]->push_back( mTableau.rLearnedBounds()[posNewLearnedBound].nextWeakerBound );
                                        mConstraintToBound[constraint->id()]->push_back( mTableau.rLearnedBounds()[posNewLearnedBound].newBound );
                                    }
                                }
                            }
                            activateBound( mTableau.rLearnedBounds()[posNewLearnedBound].nextWeakerBound, originSet );
                            activateBound( mTableau.rLearnedBounds()[posNewLearnedBound].newBound, originSet );
                            ++posNewLearnedBound;
                        }
                        if( !mInfeasibleSubsets.empty() )
                        {
                            #ifdef TLRA_REFINEMENT
                            learnRefinements();
                            #endif
                            mSolverState = False;
                            return False;
                        }
                    }
                }
                else
                {
                    mInfeasibleSubsets.clear();
                    #ifdef TLRA_ONE_REASON
                    vector< const Bound<Numeric>* > conflict = mTableau.getConflict( pivotingElement.first );
                    set< const Formula* > infSubSet = set< const Formula* >();
                    for( auto bound = conflict.begin(); bound != conflict.end(); ++bound )
                    {
                        assert( (*bound)->isActive() );
                        infSubSet.insert( (*bound)->pOrigins()->begin()->begin(), (*bound)->pOrigins()->begin()->end() );
                    }
                    mInfeasibleSubsets.push_back( infSubSet );
                    #else
                    vector< set< const Bound<Numeric>* > > conflictingBounds = mTableau.getConflictsFrom( pivotingElement.first );
                    for( auto conflict = conflictingBounds.begin(); conflict != conflictingBounds.end(); ++conflict )
                    {
                        set< const Formula* > infSubSet = set< const Formula* >();
                        for( auto bound = conflict->begin(); bound != conflict->end(); ++bound )
                        {
                            assert( (*bound)->isActive() );
                            infSubSet.insert( *(*bound)->pOrigins()->begin() );
                        }
                        mInfeasibleSubsets.push_back( infSubSet );
                    }
                    #endif
                    #ifdef TLRA_REFINEMENT
                    learnRefinements();
                    #endif
                    #ifdef DEBUG_TLRA_MODULE
                    cout << "False" << endl;
                    #endif
                    mSolverState = False;
                    return False;
                }
            }
            assert( false );
            #ifdef TLRA_REFINEMENT
            learnRefinements();
            #endif
            mSolverState = True;
            return True;
        }
        else
        {
            return Unknown;
        }
    }

    /**
     *
     */
    void TLRAModule::updateModel()
    {
        mModel.clear();
        if( mSolverState == True )
        {
            if( mAssignmentFullfilsNonlinearConstraints )
            {
                for( ExVariableMap::const_iterator originalVar = mOriginalVars.begin(); originalVar != mOriginalVars.end(); ++originalVar )
                {
                    stringstream outA;
                    outA << *originalVar->first;
                    stringstream outB;
                    outB << originalVar->second->assignment().mainPart();
                    if( originalVar->second->assignment().deltaPart() != 0 )
                    {
                        outB << "+delta_" << mId << "*" << originalVar->second->assignment().deltaPart();
                    }
                    mModel.insert( pair< const string, string >( outA.str(), outB.str() ) );
                }
            }
            else
            {
                Module::getBackendsModel();
            }
        }
    }

    /**
     * Gives a rational model if the received formula is satisfiable. Note, that it
     * is calculated from scratch every time you call this method.
     *
     * @return The rational model.
     */
    exmap TLRAModule::getRationalModel() const
    {
        exmap result = exmap();
        if( mInfeasibleSubsets.empty() )
        {
            /*
            * Check whether the found satisfying assignment is by coincidence a
            * satisfying assignment of the non linear constraints
            */
            Numeric minDelta = -1;
            Numeric curDelta = 0;
            Variable<Numeric>* variable = NULL;
            /*
            * For all slack variables find the minimum of all (c2-c1)/(k1-k2), where ..
            */
            for( auto originalVar = mOriginalVars.begin(); originalVar != mOriginalVars.end(); ++originalVar )
            {
                variable = originalVar->second;
                const Value<Numeric>& assValue = variable->assignment();
                const Bound<Numeric>& inf = variable->infimum();
                if( !inf.isInfinite() )
                {
                    /*
                    * .. the supremum is c2+k2*delta, the variable assignment is c1+k1*delta, c1<c2 and k1>k2.
                    */
                    if( inf.limit().mainPart() < assValue.mainPart() && inf.limit().deltaPart() > assValue.deltaPart() )
                    {
                        curDelta = ( assValue.mainPart() - inf.limit().mainPart() ) / ( inf.limit().deltaPart() - assValue.deltaPart() );
                        if( minDelta < 0 || curDelta < minDelta )
                        {
                            minDelta = curDelta;
                        }
                    }
                }
                const Bound<Numeric>& sup = variable->supremum();
                if( !sup.isInfinite() )
                {
                    /*
                    * .. the infimum is c1+k1*delta, the variable assignment is c2+k2*delta, c1<c2 and k1>k2.
                    */
                    if( sup.limit().mainPart() > assValue.mainPart() && sup.limit().deltaPart() < assValue.deltaPart() )
                    {
                        curDelta = ( sup.limit().mainPart() - assValue.mainPart() ) / ( assValue.deltaPart() - sup.limit().deltaPart() );
                        if( minDelta < 0 || curDelta < minDelta )
                        {
                            minDelta = curDelta;
                        }
                    }
                }
            }
            /*
            * For all slack variables find the minimum of all (c2-c1)/(k1-k2), where ..
            */
            for( auto slackVar = mSlackVars.begin(); slackVar != mSlackVars.end(); ++slackVar )
            {
                variable = slackVar->second;
                const Value<Numeric>& assValue = variable->assignment();
                const Bound<Numeric>& inf = variable->infimum();
                if( !inf.isInfinite() )
                {
                    /*
                    * .. the infimum is c1+k1*delta, the variable assignment is c2+k2*delta, c1<c2 and k1>k2.
                    */
                    if( inf.limit().mainPart() < assValue.mainPart() && inf.limit().deltaPart() > assValue.deltaPart() )
                    {
                        curDelta = ( assValue.mainPart() - inf.limit().mainPart() ) / ( inf.limit().deltaPart() - assValue.deltaPart() );
                        if( minDelta < 0 || curDelta < minDelta )
                        {
                            minDelta = curDelta;
                        }
                    }
                }
                const Bound<Numeric>& sup = variable->supremum();
                if( !sup.isInfinite() )
                {
                    /*
                    * .. the supremum is c2+k2*delta, the variable assignment is c1+k1*delta, c1<c2 and k1>k2.
                    */
                    if( sup.limit().mainPart() > assValue.mainPart() && sup.limit().deltaPart() < assValue.deltaPart() )
                    {
                        curDelta = ( sup.limit().mainPart() - assValue.mainPart() ) / ( assValue.deltaPart() - sup.limit().deltaPart() );
                        if( minDelta < 0 || curDelta < minDelta )
                        {
                            minDelta = curDelta;
                        }
                    }
                }
            }

            curDelta = minDelta < 0 ? 1 : minDelta;
            /*
            * Calculate the rational assignment of all original variables.
            */
            for( auto var = mOriginalVars.begin(); var != mOriginalVars.end(); ++var )
            {
                const Value<Numeric>& value = var->second->assignment();
                result.insert( pair< ex, ex >( *var->first, ex( (value.mainPart() + value.deltaPart() * curDelta).ginacNumeric() ) ) );
            }
        }
        return result;
    }

    #ifdef TLRA_USE_GINACRA
    /**
     * Returns the bounds of the variables as intervals.
     *
     * @return The bounds of the variables as intervals.
     */
    evalintervalmap TLRAModule::getVariableBounds() const
    {
        evalintervalmap result = evalintervalmap();
        for( auto iter = mOriginalVars.begin(); iter != mOriginalVars.end(); ++iter )
        {
            const Variable<Numeric>& var = *iter->second;
            Interval::BoundType lowerBoundType;
            numeric lowerBoundValue;
            Interval::BoundType upperBoundType;
            numeric upperBoundValue;
            if( var.infimum().isInfinite() )
            {
                lowerBoundType = Interval::INFINITY_BOUND;
                lowerBoundValue = 0;
            }
            else
            {
                lowerBoundType = var.infimum().isWeak() ? Interval::WEAK_BOUND : Interval::STRICT_BOUND;
                lowerBoundValue = var.infimum().limit().mainPart().ginacNumeric();
            }
            if( var.supremum().isInfinite() )
            {
                upperBoundType = Interval::INFINITY_BOUND;
                upperBoundValue = 0;
            }
            else
            {
                upperBoundType = var.supremum().isWeak() ? Interval::WEAK_BOUND : Interval::STRICT_BOUND;
                upperBoundValue = var.supremum().limit().mainPart().ginacNumeric();
            }
            Interval interval = Interval( lowerBoundValue, lowerBoundType, upperBoundValue, upperBoundType );
            result.insert( pair< symbol, Interval >( ex_to< symbol >( *iter->first ), interval ) );
        }
        return result;
    }
    #endif

    #ifdef TLRA_REFINEMENT
    /**
     * Adds the refinements learned during pivoting to the deductions.
     */
    void TLRAModule::learnRefinements()
    {
        vector<class Tableau<Numeric>::LearnedBound>& lBs = mTableau.rLearnedBounds();
        while( !lBs.empty() )
        {
            auto originsIterA = lBs.back().nextWeakerBound->origins().begin();
            while( originsIterA != lBs.back().nextWeakerBound->origins().end() )
            {
                // TODO: Learn also those deductions with a conclusion containing more than one constraint.
                //       This must be hand over via a non clause formula and could introduce new
                //       Boolean variables.
                if( originsIterA->size() == 1 )
                {
                    if( originsIterA != lBs.back().nextWeakerBound->origins().end() )
                    {
                        auto originIterA = originsIterA->begin();
                        while( originIterA != originsIterA->end() )
                        {
                            Formula* deduction = new Formula( OR );
                            for( auto bound = lBs.back().premise->begin(); bound != lBs.back().premise->end(); ++bound )
                            {
                                auto originIterB = (*bound)->origins().begin()->begin();
                                while( originIterB != (*bound)->origins().begin()->end() )
                                {
                                    deduction->addSubformula( new Formula( NOT ) );
                                    deduction->back()->addSubformula( (*originIterB)->pConstraint() );
                                    ++originIterB;
                                }
                            }
                            deduction->addSubformula( (*originIterA)->pConstraint() );
                            addDeduction( deduction );
                            ++originIterA;
                        }
                    }
                }
                ++originsIterA;
            }
            vector<const Bound<Numeric>* >* toDelete = lBs.back().premise;
            lBs.pop_back();
            delete toDelete;
        }
    }
    #endif

    /**
     * Adapt the passed formula, such that it consists of the finite infimums and supremums
     * of all variables and the non linear constraints.
     */
    void TLRAModule::adaptPassedFormula()
    {
        while( !mBoundCandidatesToPass.empty() )
        {
            const Bound<Numeric>& bound = *mBoundCandidatesToPass.back();
            if( bound.pInfo()->updated > 0 )
            {
                addSubformulaToPassedFormula( new Formula( bound.pAsConstraint() ), bound.origins() );
                bound.pInfo()->position = mpPassedFormula->last();
                bound.pInfo()->updated = 0;
            }
            else if( bound.pInfo()->updated < 0 )
            {
                removeSubformulaFromPassedFormula( bound.pInfo()->position );
                bound.pInfo()->position = mpPassedFormula->end();
                bound.pInfo()->updated = 0;
            }
            mBoundCandidatesToPass.pop_back();
        }
    }

    /**
     * Checks whether the current assignment of the linear constraints fulfills the non linear constraints.
     *
     * @return True, if the current assignment of the linear constraints fulfills the non linear constraints;
     *         False, otherwise.
     */
    bool TLRAModule::checkAssignmentForNonlinearConstraint()
    {
        if( mNonlinearConstraints.empty() )
        {
            mAssignmentFullfilsNonlinearConstraints = true;
            return true;
        }
        else
        {
            exmap assignments = getRationalModel();
            /*
             * Check whether the assignment satisfies the non linear constraints.
             */
            for( auto constraint = mNonlinearConstraints.begin(); constraint != mNonlinearConstraints.end(); ++constraint )
            {
                if( (*constraint)->satisfiedBy( assignments ) != 1 )
                {
                    return false;
                }
            }
            mAssignmentFullfilsNonlinearConstraints = true;
            return true;
        }
    }

    /**
     * Activate the given bound and update the supremum, the infimum and the assignment of
     * variable to which the bound belongs.
     *
     * @param _bound The bound to activate.
     * @param _formulas The constraints which form this bound.
     * @return False, if a conflict occurs;
     *         True, otherwise.
     */
    bool TLRAModule::activateBound( const Bound<Numeric>* _bound, set<const Formula*>& _formulas )
    {
        bool result = true;
        _bound->pOrigins()->push_back( _formulas );
        const Variable<Numeric>& var = _bound->variable();
        if( (_bound->isUpperBound() && var.pSupremum()->isInfinite()) )
        {
            if( var.isBasic() )
            {
                mTableau.incrementBasicActivity( var );
            }
            else
            {
                mTableau.incrementNonbasicActivity( var );
            }
        }
        if( (_bound->isLowerBound() && var.pInfimum()->isInfinite()) )
        {
            if( var.isBasic() )
            {
                mTableau.incrementBasicActivity( var );
            }
            else
            {
                mTableau.incrementNonbasicActivity( var );
            }
        }
        if( _bound->isUpperBound() )
        {
            if( *var.pInfimum() > _bound->limit() )
            {
                set<const Formula*> infsubset = set<const Formula*>();
                infsubset.insert( _bound->pOrigins()->begin()->begin(), _bound->pOrigins()->begin()->end() );
                infsubset.insert( var.pInfimum()->pOrigins()->back().begin(), var.pInfimum()->pOrigins()->back().end() );
                mInfeasibleSubsets.push_back( infsubset );
                result = false;
            }
            if( *var.pSupremum() > *_bound )
            {
                if( !var.pSupremum()->isInfinite() )
                {
                    mBoundCandidatesToPass.push_back( var.pSupremum() );
                }
                mBoundCandidatesToPass.push_back( _bound );
                _bound->pVariable()->setSupremum( _bound );

                if( result && !var.isBasic() && (*var.pSupremum() < var.assignment()) )
                {
                    mTableau.updateBasicAssignments( var.position(), Value<Numeric>( (*var.pSupremum()).limit() - var.assignment() ) );
                    _bound->pVariable()->rAssignment() = (*var.pSupremum()).limit();
                }
            }
        }
        if( _bound->isLowerBound() )
        {
            if( *var.pSupremum() < _bound->limit() )
            {
                set<const Formula*> infsubset = set<const Formula*>();
                infsubset.insert( _bound->pOrigins()->begin()->begin(), _bound->pOrigins()->begin()->end() );
                infsubset.insert( var.pSupremum()->pOrigins()->back().begin(), var.pSupremum()->pOrigins()->back().end() );
                mInfeasibleSubsets.push_back( infsubset );
                result = false;
            }
            if( *var.pInfimum() < *_bound )
            {
                if( !var.pInfimum()->isInfinite() )
                {
                    mBoundCandidatesToPass.push_back( var.pInfimum() );
                }
                mBoundCandidatesToPass.push_back( _bound );
                _bound->pVariable()->setInfimum( _bound );

                if( result && !var.isBasic() && (*var.pInfimum() > var.assignment()) )
                {
                    mTableau.updateBasicAssignments( var.position(), Value<Numeric>( (*var.pInfimum()).limit() - var.assignment() ) );
                    _bound->pVariable()->rAssignment() = (*var.pInfimum()).limit();
                }
            }
        }
        return result;
    }

    /**
     * Creates a bound corresponding to the given constraint.
     *
     * @param _var The variable to which the bound must be added.
     * @param _constraintInverted A flag, which is true if the inverted form of the given constraint forms the bound.
     * @param _boundValue The limit of the bound.
     * @param _constraint The constraint corresponding to the bound to create.
     */
    void TLRAModule::setBound( Variable<Numeric>& _var, bool _constraintInverted, const Numeric& _boundValue, const Constraint* _constraint )
    {
        if( _constraint->relation() == CR_EQ )
        {
            // TODO: Take value from an allocator to assure the values are located close to each other in the memory.
            Value<Numeric>* value  = new Value<Numeric>( _boundValue );
            pair<const Bound<Numeric>*, pair<const Bound<Numeric>*, const Bound<Numeric>*> > result = _var.addEqualBound( value, mpPassedFormula->end(), _constraint );
            #ifdef TLRA_SIMPLE_CONFLICT_SEARCH
            findSimpleConflicts( *result.first );
            #endif
            vector< const Bound<Numeric>* >* boundVector = new vector< const Bound<Numeric>* >();
            boundVector->push_back( result.first );
            mConstraintToBound[_constraint->id()] = boundVector;
            #ifdef TLRA_SIMPLE_THEORY_PROPAGATION
            if( result.second.first != NULL && !result.second.first->isInfinite() )
            {
                Formula* deduction = new Formula( OR );
                deduction->addSubformula( new Formula( NOT ) );
                deduction->back()->addSubformula( _constraint );
                deduction->addSubformula( result.second.first->pAsConstraint() );
                addDeduction( deduction );
            }
            if( result.second.first != NULL && !result.second.first->isInfinite() )
            {
                Formula* deduction = new Formula( OR );
                deduction->addSubformula( new Formula( NOT ) );
                deduction->back()->addSubformula( _constraint );
                deduction->addSubformula( result.second.first->pAsConstraint() );
                addDeduction( deduction );
            }
            if( result.second.second != NULL && !result.second.second->isInfinite() )
            {
                Formula* deduction = new Formula( OR );
                deduction->addSubformula( new Formula( NOT ) );
                deduction->back()->addSubformula( _constraint );
                deduction->addSubformula( result.second.second->pAsConstraint() );
                addDeduction( deduction );
            }
            if( result.second.second != NULL && !result.second.second->isInfinite() )
            {
                Formula* deduction = new Formula( OR );
                deduction->addSubformula( new Formula( NOT ) );
                deduction->back()->addSubformula( _constraint );
                deduction->addSubformula( result.second.second->pAsConstraint() );
                addDeduction( deduction );
            }
            #endif
        }
        else if( _constraint->relation() == CR_LEQ )
        {
            Value<Numeric>* value = new Value<Numeric>( _boundValue );
            pair<const Bound<Numeric>*,pair<const Bound<Numeric>*, const Bound<Numeric>*> > result = _constraintInverted ? _var.addLowerBound( value, mpPassedFormula->end(), _constraint ) : _var.addUpperBound( value, mpPassedFormula->end(), _constraint );
            #ifdef TLRA_SIMPLE_CONFLICT_SEARCH
            findSimpleConflicts( *result.first );
            #endif
            vector< const Bound<Numeric>* >* boundVector = new vector< const Bound<Numeric>* >();
            boundVector->push_back( result.first );
            mConstraintToBound[_constraint->id()] = boundVector;
            #ifdef TLRA_SIMPLE_THEORY_PROPAGATION
            if( result.second.first != NULL && !result.second.first->isInfinite() )
            {
                Formula* deduction = new Formula( OR );
                deduction->addSubformula( new Formula( NOT ) );
                deduction->back()->addSubformula( result.second.first->pAsConstraint() );
                deduction->addSubformula( _constraint );
                addDeduction( deduction );
            }
            if( result.second.second != NULL && !result.second.second->isInfinite() )
            {
                Formula* deduction = new Formula( OR );
                deduction->addSubformula( new Formula( NOT ) );
                deduction->back()->addSubformula( _constraint );
                deduction->addSubformula( result.second.second->pAsConstraint() );
                addDeduction( deduction );
            }
            #endif
        }
        else if( _constraint->relation() == CR_GEQ )
        {
            Value<Numeric>* value = new Value<Numeric>( _boundValue );
            pair<const Bound<Numeric>*,pair<const Bound<Numeric>*, const Bound<Numeric>*> > result = _constraintInverted ? _var.addUpperBound( value, mpPassedFormula->end(), _constraint ) : _var.addLowerBound( value, mpPassedFormula->end(), _constraint );
            #ifdef TLRA_SIMPLE_CONFLICT_SEARCH
            findSimpleConflicts( *result.first );
            #endif
            vector< const Bound<Numeric>* >* boundVector = new vector< const Bound<Numeric>* >();
            boundVector->push_back( result.first );
            mConstraintToBound[_constraint->id()] = boundVector;
            #ifdef TLRA_SIMPLE_THEORY_PROPAGATION
            if( result.second.first != NULL && !result.second.first->isInfinite() )
            {
                Formula* deduction = new Formula( OR );
                deduction->addSubformula( new Formula( NOT ) );
                deduction->back()->addSubformula( result.second.first->pAsConstraint() );
                deduction->addSubformula( _constraint );
                addDeduction( deduction );
            }
            if( result.second.second != NULL && !result.second.second->isInfinite() )
            {
                Formula* deduction = new Formula( OR );
                deduction->addSubformula( new Formula( NOT ) );
                deduction->back()->addSubformula( _constraint );
                deduction->addSubformula( result.second.second->pAsConstraint() );
                addDeduction( deduction );
            }
            #endif
        }
        else if( _constraint->relation() == CR_LESS )
        {
            Value<Numeric>* value = new Value<Numeric>( _boundValue, (_constraintInverted ? 1 : -1) );
            pair<const Bound<Numeric>*,pair<const Bound<Numeric>*, const Bound<Numeric>*> > result = _constraintInverted ? _var.addLowerBound( value, mpPassedFormula->end(), _constraint ) : _var.addUpperBound( value, mpPassedFormula->end(), _constraint );
            #ifdef TLRA_SIMPLE_CONFLICT_SEARCH
            findSimpleConflicts( *result.first );
            #endif
            vector< const Bound<Numeric>* >* boundVector = new vector< const Bound<Numeric>* >();
            boundVector->push_back( result.first );
            mConstraintToBound[_constraint->id()] = boundVector;
            #ifdef TLRA_SIMPLE_THEORY_PROPAGATION
            if( result.second.first != NULL && !result.second.first->isInfinite() )
            {
                Formula* deduction = new Formula( OR );
                deduction->addSubformula( new Formula( NOT ) );
                deduction->back()->addSubformula( result.second.first->pAsConstraint() );
                deduction->addSubformula( _constraint );
                addDeduction( deduction );
            }
            if( result.second.second != NULL && !result.second.second->isInfinite() )
            {
                Formula* deduction = new Formula( OR );
                deduction->addSubformula( new Formula( NOT ) );
                deduction->back()->addSubformula( _constraint );
                deduction->addSubformula( result.second.second->pAsConstraint() );
                addDeduction( deduction );
            }
            #endif
        }
        else if( _constraint->relation() == CR_GREATER )
        {
            Value<Numeric>* value = new Value<Numeric>( _boundValue, (_constraintInverted ? -1 : 1) );
            pair<const Bound<Numeric>*,pair<const Bound<Numeric>*, const Bound<Numeric>*> > result = _constraintInverted ? _var.addUpperBound( value, mpPassedFormula->end(), _constraint ) : _var.addLowerBound( value, mpPassedFormula->end(), _constraint );
            #ifdef TLRA_SIMPLE_CONFLICT_SEARCH
            findSimpleConflicts( *result.first );
            #endif
            vector< const Bound<Numeric>* >* boundVector = new vector< const Bound<Numeric>* >();
            boundVector->push_back( result.first );
            mConstraintToBound[_constraint->id()] = boundVector;
            #ifdef TLRA_SIMPLE_THEORY_PROPAGATION
            if( result.second.first != NULL && !result.second.first->isInfinite() )
            {
                Formula* deduction = new Formula( OR );
                deduction->addSubformula( new Formula( NOT ) );
                deduction->back()->addSubformula( result.second.first->pAsConstraint() );
                deduction->addSubformula( _constraint );
                addDeduction( deduction );
            }
            if( result.second.second != NULL && !result.second.second->isInfinite() )
            {
                Formula* deduction = new Formula( OR );
                deduction->addSubformula( new Formula( NOT ) );
                deduction->back()->addSubformula( _constraint );
                deduction->addSubformula( result.second.second->pAsConstraint() );
                addDeduction( deduction );
            }
            #endif
        }
    }

    #ifdef TLRA_SIMPLE_CONFLICT_SEARCH
    /**
     * Finds all conflicts between lower resp. upper bounds and the given upper
     * resp. lower bound and adds them to the deductions.
     *
     * @param _bound The bound to find conflicts for.
     */
    void TLRAModule::findSimpleConflicts( const Bound<Numeric>& _bound )
    {
        assert( !_bound.deduced() );
        if( _bound.isUpperBound() )
        {
            const Bound<Numeric>::BoundSet& lbounds = _bound.variable().lowerbounds();
            for( auto lbound = lbounds.rbegin(); lbound != --lbounds.rend(); ++lbound )
            {
                if( **lbound > _bound.limit() && (*lbound)->pAsConstraint() != NULL )
                {
                    Formula* deduction = new Formula( OR );
                    deduction->addSubformula( new Formula( NOT ) );
                    deduction->back()->addSubformula( _bound.pAsConstraint() );
                    deduction->addSubformula( new Formula( NOT ) );
                    deduction->back()->addSubformula( (*lbound)->pAsConstraint() );
                    addDeduction( deduction );
                }
                else
                {
                    break;
                }
            }
        }
        if( _bound.isLowerBound() )
        {
            const Bound<Numeric>::BoundSet& ubounds = _bound.variable().upperbounds();
            for( auto ubound = ubounds.begin(); ubound != --ubounds.end(); ++ubound )
            {
                if( **ubound < _bound.limit() && (*ubound)->pAsConstraint() != NULL )
                {
                    Formula* deduction = new Formula( OR );
                    deduction->addSubformula( new Formula( NOT ) );
                    deduction->back()->addSubformula( _bound.pAsConstraint() );
                    deduction->addSubformula( new Formula( NOT ) );
                    deduction->back()->addSubformula( (*ubound)->pAsConstraint() );
                    addDeduction( deduction );
                }
                else
                {
                    break;
                }
            }
        }
    }
    #endif

    /**
     * Initializes the tableau according to all linear constraints, of which this module has been informed.
     */
    void TLRAModule::initialize( const Constraint* const _pConstraint )
    {
        map<const string, numeric, strCmp> coeffs = _pConstraint->linearAndConstantCoefficients();
        assert( coeffs.size() > 1 );
        map<const string, numeric, strCmp>::iterator currentCoeff = coeffs.begin();
        ex*                                          linearPart   = new ex( _pConstraint->lhs() - currentCoeff->second );
        ++currentCoeff;

        // divide the linear Part and the constraint by the highest coefficient
        numeric highestCoeff = currentCoeff->second;
        --currentCoeff;
        while( currentCoeff != coeffs.end() )
        {
            currentCoeff->second /= highestCoeff;
            ++currentCoeff;
        }
        *linearPart /= highestCoeff;
        if( coeffs.size() == 2 )
        {
            // constraint has one variable
            ex* var = new ex( (*_pConstraint->variables().begin()).second );
            ExVariableMap::iterator basicIter = mOriginalVars.find( var );
            // constraint not found, add new nonbasic variable
            if( basicIter == mOriginalVars.end() )
            {
                Variable<Numeric>* nonBasic = mTableau.newNonbasicVariable( var );
                mOriginalVars.insert( pair<const ex*, Variable<Numeric>*>( var, nonBasic ) );
                setBound( *nonBasic, highestCoeff.is_negative(), -coeffs.begin()->second, _pConstraint );
            }
            else
            {
                delete var;
                Variable<Numeric>* nonBasic = basicIter->second;
                setBound( *nonBasic, highestCoeff.is_negative(), -coeffs.begin()->second, _pConstraint );
            }

        }
        else
        {
            ExVariableMap::iterator slackIter = mSlackVars.find( linearPart );
            if( slackIter == mSlackVars.end() )
            {
                vector< Variable<Numeric>* > nonbasics = vector< Variable<Numeric>* >();
                vector< Numeric > numCoeffs = vector< Numeric >();
                symtab::const_iterator varIt   = _pConstraint->variables().begin();
                map<const string, numeric, strCmp>::iterator coeffIt = coeffs.begin();
                ++coeffIt;
                while( varIt != _pConstraint->variables().end() )
                {
                    assert( coeffIt != coeffs.end() );
                    ex* var = new ex( varIt->second );
                    ExVariableMap::iterator nonBasicIter = mOriginalVars.find( var );
                    if( mOriginalVars.end() == nonBasicIter )
                    {
                        Variable<Numeric>* nonBasic = mTableau.newNonbasicVariable( var );
                        mOriginalVars.insert( pair<const ex*, Variable<Numeric>*>( var, nonBasic ) );
                        nonbasics.push_back( nonBasic );
                    }
                    else
                    {
                        delete var;
                        nonbasics.push_back( nonBasicIter->second );
                    }
                    numCoeffs.push_back( Numeric( coeffIt->second ) );
                    ++varIt;
                    ++coeffIt;
                }

                Variable<Numeric>* slackVar = mTableau.newBasicVariable( linearPart, nonbasics, numCoeffs );

                mSlackVars.insert( pair<const ex*, Variable<Numeric>*>( linearPart, slackVar ) );
                setBound( *slackVar, highestCoeff.is_negative(), -coeffs.begin()->second, _pConstraint );
            }
            else
            {
                delete linearPart;
                Variable<Numeric>* slackVar = slackIter->second;
                setBound( *slackVar, highestCoeff.is_negative(), -coeffs.begin()->second, _pConstraint );
            }
        }
    }

    /**
     * Initializes the tableau according to all linear constraints, of which this module has been informed.
     */
    void TLRAModule::initialize()
    {
        mInitialized = true;
        for( auto constraint = mLinearConstraints.begin(); constraint != mLinearConstraints.end(); ++constraint )
        {
            initialize( *constraint );
        }
        #ifdef TLRA_USE_PIVOTING_STRATEGY
        mTableau.setBlandsRuleStart( mTableau.columns().size() );
        #endif
    }

}    // namespace smtrat
