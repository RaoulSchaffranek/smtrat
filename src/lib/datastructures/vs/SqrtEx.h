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
 * Class to create a square root expression object.
 * @author Florian Corzilius
 * @since 2011-05-26
 * @version 2013-10-22
 */

#ifndef SMTRAT_VS_SQRTEX_H
#define SMTRAT_VS_SQRTEX_H

#include <iostream>
#include <assert.h>
#include "../../Common.h"

namespace vs
{
    class SqrtEx
    {
        private:
            /// The constant part c of this square root expression (c + f * sqrt(r))/d.
            smtrat::Polynomial mConstantPart;
            /// The factor f of this square root expression (c + f * sqrt(r))/d.
            smtrat::Polynomial mFactor;
            /// The denominator d of this square root expression (c + f * sqrt(r))/d.
            smtrat::Polynomial mDenominator;
            /// The radicand r of this square root expression (c + f * sqrt(r))/d.
            smtrat::Polynomial mRadicand;

        public:
            /**
             * Default Constructor. ( constructs (0 + 0 * sqrt( 0 )) / 1 )
             */
            SqrtEx();
            
            /**
             * Constructs a square root expression from a polynomial p leading to (p + 0 * sqrt( 0 )) / 1
             * @param _poly The polynomial to construct a square root expression for.
             */
            SqrtEx( const smtrat::Polynomial& _poly );
            
            /**
             * Constructs a square root expression from given constant part, factor, denominator and radicand.
             * @param _constantPart The constant part of the square root expression to construct.
             * @param _factor The factor of the square root expression to construct.
             * @param _denominator The denominator of the square root expression to construct.
             * @param _radicand The radicand of the square root expression to construct.
             */
            SqrtEx( const smtrat::Polynomial& _constantPart , const smtrat::Polynomial& _factor, const smtrat::Polynomial& _denominator, const smtrat::Polynomial& _radicand );
            
            /**
             * Copy constructor.
             * @param _sqrtEx The square root expression to copy.
             */
            SqrtEx( const SqrtEx& _sqrtEx );

            /**
             * Destructor:
             */
            ~SqrtEx();

            /**
             * @return A constant reference to the constant part of this square root expression.
             */
            const smtrat::Polynomial& constantPart() const
            {
                return mConstantPart;
            }
            
            /**
             * @return A constant reference to the factor of this square root expression.
             */
            const smtrat::Polynomial& factor() const
            {
                return mFactor;
            }

            /**
             * @return A constant reference to the denominator of this square root expression.
             */
            const smtrat::Polynomial& denominator() const
            {
                return mDenominator;
            }

            /**
             * @return A constant reference to the radicand of this square root expression.
             */
            const smtrat::Polynomial& radicand() const
            {
                return mRadicand;
            }

            /**
             * @return true, if the square root expression has a non trivial radicand;
             *          false, otherwise.
             */
            bool hasSqrt() const
            {
                return mFactor != smtrat::Polynomial( smtrat::Rational( 0 ) );
            }

            /**
             * @return true, if there is no variable in this square root expression;
             *          false, otherwise.
             */
            bool isConstant() const
            {
                return mConstantPart.isConstant() && mDenominator.isConstant() && mFactor.isConstant() && mRadicand.isConstant();
            }

            
        private:
            
            /**
             * Normalizes this object, that is extracts as much as possible from the radicand into the factor
             * and cancels the enumerator and denominator afterwards.
             */
            void normalize();
            
        public:
            
            /**
             * @return true, if the this square root expression corresponds to an integer value;
             *         false, otherwise.
             */
            bool isInteger() const
            {
                return radicand().isZero() && denominator() == smtrat::ONE_POLYNOMIAL && 
                       (constantPart().isZero() || (constantPart().isConstant() && carl::isInteger( constantPart().lcoeff() ) ) );
            }
            
            /**
             * @param _sqrtEx Square root expression to compare with.
             * @return  true, if this square root expression and the given one are equal;
             *          false, otherwise.
             */
            bool operator==( const SqrtEx& _toCompareWith ) const;
            
            /**
             * @param _sqrtEx A square root expression, which gets the new content of this square root expression.
             * @return A reference to this object.
             */
            SqrtEx& operator=( const SqrtEx& _sqrtEx );
            
            /**
             * @param _poly A polynomial, which gets the new content of this square root expression.
             * @return A reference to this object.
             */
            SqrtEx& operator=( const smtrat::Polynomial& _poly );
            
            /**
             * @param _summandA  First summand.
             * @param _summandB  Second summand.
             * @return The sum of the given square root expressions.
             */
            friend SqrtEx operator+( const SqrtEx& _summandA, const SqrtEx& _summandB );
            
            /**
             * @param _minuend  Minuend.
             * @param _subtrahend  Subtrahend.
             * @return The difference of the given square root expressions.
             */
            friend SqrtEx operator-( const SqrtEx& _minuend, const SqrtEx& _subtrahend );
      
            /**
             * @param _factorA  First factor.
             * @param _factorB  Second factor.
             * @return The product of the given square root expressions.
             */
            friend SqrtEx operator*( const SqrtEx& _factorA, const SqrtEx& _factorB );
            
            /**
             * @param _dividend  Dividend.
             * @param _divisor  Divisor.
             * @return The result of the first given square root expression divided by the second one
             *          Note that the second argument is not allowed to contain a square root.
             */
            friend SqrtEx operator/( const SqrtEx& _dividend, const SqrtEx& _divisor );
            
            /**
             * Prints the given square root expression on the given stream.
             * @param _out The stream to print on.
             * @param _sqrtEx The square root expression to print.
             * @return The stream after printing the square root expression on it.
             */
            friend std::ostream& operator<<( std::ostream& _out, const SqrtEx& _sqrtEx );
            
            /**
             * @param _infix A string which is printed in the beginning of each row.
             * @param _friendlyNames A flag that indicates whether to print the variables with their internal representation (false)
             *                        or with their dedicated names.
             * @return The string representation of this square root expression.
             */
            std::string toString( bool _infix = false, bool _friendlyNames = true ) const;
            
            bool evaluate( smtrat::Rational& _result, const smtrat::EvalRationalMap& _evalMap, int _rounding = 0 ) const;
            
            /**
             * Substitutes a variable in an expression by a square root expression, which results in a square root expression.
             * @param _substituteIn The polynomial to substitute in.
             * @param _varToSubstitute The variable to substitute.
             * @param _substituteBy The square root expression by which the variable gets substituted.
             * @return The resulting square root expression.
             */
            static SqrtEx subBySqrtEx( const smtrat::Polynomial& _substituteIn, const carl::Variable& _varToSubstitute, const SqrtEx& _substituteBy );
    };
}    // end namspace vs

namespace std
{
    template<>
    struct hash<vs::SqrtEx>
    {
    public:
        size_t operator()( const vs::SqrtEx& _sqrtEx ) const 
        {
            return ((hash<smtrat::Polynomial>()(_sqrtEx.radicand()) ^ hash<smtrat::Polynomial>()(_sqrtEx.denominator())) ^ hash<smtrat::Polynomial>()(_sqrtEx.factor())) ^ hash<smtrat::Polynomial>()(_sqrtEx.constantPart());
        }
    };
} // namespace std

#endif