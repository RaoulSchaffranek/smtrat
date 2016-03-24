#pragma once

#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>

#include <carl/core/RealAlgebraicNumber.h>
#include <carl/core/RealAlgebraicNumberEvaluation.h>

#include "../../Common.h"
#include "Settings.h"
#include "utils/Bitset.h"
#include "utils/DynamicPriorityQueue.h"
#include "utils/IDPool.h"
#include "utils/Origin.h"

namespace smtrat {
namespace cad {
	using Variables = std::vector<carl::Variable>;
	using UPoly = carl::UnivariatePolynomial<Poly>;
	using RAN = carl::RealAlgebraicNumber<Rational>;
	using SampleLiftedWith = Bitset;
	using SampleRootOf = Bitset;
	using ConstraintSelection = Bitset;
	using OptionalPoly = boost::optional<const UPoly&>;
	using OptionalID = boost::optional<std::size_t>;
	using Assignment = std::map<carl::Variable, RAN>;
}
}
