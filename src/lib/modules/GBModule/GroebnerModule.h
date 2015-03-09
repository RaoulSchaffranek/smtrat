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


/**
 * @file   GroebnerModule.h
 *
 * @author Sebastian Junges
 *
 * The classes contained in here are
 * GroebnerModuleState
 * InequalitiesTable
 * GroebnerModule
 *
 * Since: 2012-01-18
 * Version: 2013-30-03
 */

#pragma once

// Datastructures from carl
#include "carl/groebner/groebner.h"

// General Module interface
#include "../../solver/Module.h"

// Compile time settings structures
#include "GBSettings.h"
// Runtime settings class
#include "GBRuntimeSettings.h"
#include "RewriteRules.h"

#include "VariableRewriteRule.h"
#ifdef SMTRAT_DEVOPTION_Statistics
#include "GBModuleStatistics.h"
#include "GBCalculationStatistics.h"
#endif

#include "GroebnerModuleState.h"
#include "InequalitiesTable.h"

namespace smtrat
{
/**
 * A solver module based on Groebner basis.
 * Details can be found in my Bachelor Thesis
 * "On Groebner Bases in SMT-Compliant Decision Procedures"
 * @author Sebastian Junges
 */
template<class Settings>
class GroebnerModule : public Module
{
    friend class InequalitiesTable<Settings>;
public:
    typedef typename Settings::Order Order;
    typedef typename Settings::PolynomialWithReasons GBPolynomial;
    typedef typename Settings::MultivariateIdeal Ideal;
protected:
    /// The current Groebner basis
    typename Settings::Groebner mBasis;
    /// The inequalities table for handling inequalities
    InequalitiesTable<Settings> mInequalities;
    /// The vector of backtrack points, which has pointers to received constraints.
    std::vector<ModuleInput::const_iterator> mBacktrackPoints;
    /// Saves the relevant history to support backtracking
    std::list<GroebnerModuleState<Settings> > mStateHistory;
    /// After popping in the history, it might be necessary to recalculate. This flag indicates this
    bool mRecalculateGB;
    /// A list of inequalities which were added after the last consistency check.
    std::list<typename InequalitiesTable<Settings>::Rows::iterator> mNewInequalities;
    /// An reference to the RuntimeSettings
    GBRuntimeSettings* mRuntimeSettings;
    /// The rewrite rules for the variables
    groebner::RewriteRules mRewriteRules;

    std::map<unsigned, carl::Variable> mAdditionalVarMap;
    
    /** A workaround to associate equalities in the passed formula originating from the gb
     * (in contrast to those which originate from simplified formulae)
     */
    FormulasT mGbEqualities;

public:
    GroebnerModule( ModuleType _type, const ModuleInput* const, RuntimeSettings*, Conditionals&, Manager* const = NULL );
    virtual ~GroebnerModule( );

    bool addCore( ModuleInput::const_iterator _formula );
    virtual Answer checkCore( bool _full = true );
    void removeCore( ModuleInput::const_iterator _formula );
	

protected:
    bool constraintByGB( carl::Relation cr );
    
    void pushBacktrackPoint( ModuleInput::const_iterator btpoint );
    void popBacktrackPoint( ModuleInput::const_iterator btpoint );
    bool saveState( );

    FormulasT generateReasons( const carl::BitVector& reasons );
    void passGB( );
    
    void knownConstraintDeduction( const std::list<std::pair<carl::BitVector, carl::BitVector> >& deductions );
    void newConstraintDeduction( );
    void factorisedConstraintDeduction( const std::list<GBPolynomial>& factorisation, const carl::BitVector& reasons );
    
    GBPolynomial transformIntoEquality( ModuleInput::const_iterator constraint );

    GBPolynomial callGroebnerToSDP( const Ideal& gb);
    
    bool iterativeVariableRewriting();
    bool findTrivialFactorisations();
    
    void processNewConstraint( ModuleInput::const_iterator _formula );
    void handleConstraintToGBQueue( ModuleInput::const_iterator _formula );
    void handleConstraintNotToGB( ModuleInput::const_iterator _formula );
    
    
    void removeReceivedFormulaFromNewInequalities( ModuleInput::const_iterator _formula );
    void removeSubformulaFromPassedFormula( ModuleInput::iterator _formula );

	GBPolynomial rewriteVariable(const GBPolynomial&, const carl::Variable&, const TermT&, const BitVector&);
    bool validityCheck( );
public:
    void printStateHistory( );
    void printRewriteRules( );


private:
    #ifdef SMTRAT_DEVOPTION_Statistics
    GroebnerModuleStats* mStats;
    GBCalculationStats* mGBStats;
    #endif //SMTRAT_DEVOPTION_Statistics

    typedef Module super;
};
} // namespace smtrat
#include "GroebnerModule.tpp"
#include "InequalitiesTable.tpp"
