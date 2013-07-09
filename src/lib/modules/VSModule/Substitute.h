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
 * Class containing a method applying a virtual substitution.
 * @author Florian Corzilius
 * @since 2011-05-23
 * @version 2013-05-20
 */

#ifndef SMTRAT_VS_SUBSTITUTE_H
#define SMTRAT_VS_SUBSTITUTE_H

#include "Substitution.h"
#include "../../misc/VS_Tools.hpp"
#include <cmath>

namespace vs
{
    // Methods:
    bool substitute( const smtrat::Constraint*, const Substitution&, DisjunctionOfConstraintConjunctions&, bool, GiNaC::symtab&, const GiNaCRA::evaldoubleintervalmap& );
    void substituteNormal( const smtrat::Constraint*, const Substitution&, DisjunctionOfConstraintConjunctions&, bool, GiNaC::symtab&, const GiNaCRA::evaldoubleintervalmap& );
    void substituteNormalSqrt( const smtrat::Constraint*, const Substitution&, const GiNaC::ex&, DisjunctionOfConstraintConjunctions&, bool );
    void substituteNormalSqrtEq( const smtrat::Constraint*, const Substitution&, const GiNaC::ex&, const GiNaC::ex&, const GiNaC::ex&, const GiNaC::symtab&, DisjunctionOfConstraintConjunctions&, bool );
    void substituteNormalSqrtNeq( const smtrat::Constraint*, const Substitution&, const GiNaC::ex&, const GiNaC::ex&, const GiNaC::ex&, const GiNaC::symtab&, DisjunctionOfConstraintConjunctions&, bool );
    void substituteNormalSqrtLess( const smtrat::Constraint*, const Substitution&, const GiNaC::ex&, const GiNaC::ex&, const GiNaC::ex&, const GiNaC::ex&, const GiNaC::symtab&, DisjunctionOfConstraintConjunctions&, bool );
    void substituteNormalSqrtLeq( const smtrat::Constraint*, const Substitution&, const GiNaC::ex&, const GiNaC::ex&, const GiNaC::ex&, const GiNaC::ex&, const GiNaC::symtab&, DisjunctionOfConstraintConjunctions&, bool );
    bool substitutePlusEps( const smtrat::Constraint*, const Substitution&, DisjunctionOfConstraintConjunctions&, bool, GiNaC::symtab&, const GiNaCRA::evaldoubleintervalmap& );
    bool substituteEpsGradients( const smtrat::Constraint*, const Substitution&, const smtrat::Constraint_Relation, DisjunctionOfConstraintConjunctions&, bool, GiNaC::symtab&, const GiNaCRA::evaldoubleintervalmap& );
    void substituteMinusInf( const smtrat::Constraint*, const Substitution&, DisjunctionOfConstraintConjunctions&, GiNaC::symtab&, const GiNaCRA::evaldoubleintervalmap& );
    void substituteInfLessGreater( const smtrat::Constraint*, const Substitution&, DisjunctionOfConstraintConjunctions& );
    void substituteTrivialCase( const smtrat::Constraint*, const Substitution&, DisjunctionOfConstraintConjunctions& );
    void substituteNotTrivialCase( const smtrat::Constraint*, const Substitution&, DisjunctionOfConstraintConjunctions& );
}    // end namspace vs

#endif
