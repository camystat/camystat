#include <gtest/gtest.h>
#include <numeric>
#include "Savgol.h"

namespace
{
	void expectVectorNear(const std::vector<double>& actual, const std::vector<double>& expected, double tolerance = 1e-9)
	{
		ASSERT_EQ(actual.size(), expected.size());
		for (size_t i = 0; i < expected.size(); ++i) {
			EXPECT_NEAR(actual[i], expected[i], tolerance) << "at index " << i;
		}
	}

	double sum(const std::vector<double>& v)
	{
		return std::accumulate(v.begin(), v.end(), 0.0);
	}

	// Reference values below were generated with scipy.signal.savgol_coeffs(..., use='dot') and savgol_filter(..., mode='mirror')
	const std::vector<double> kSignal = { 1, 4, 2, 8, 5, 7, 3, 9, 6, 0, 2 };
}

TEST(SavgolCoeffs, Window5Order2MatchesReference)
{
	auto coeffs = Savgol::savgolCoeffs(5, 2);

	expectVectorNear(coeffs, { -3.0 / 35, 12.0 / 35, 17.0 / 35, 12.0 / 35, -3.0 / 35 });
	EXPECT_NEAR(sum(coeffs), 1.0, 1e-12);
}

TEST(SavgolCoeffs, Window7Order5MatchesReference)
{
	auto coeffs = Savgol::savgolCoeffs(7, 5);

	expectVectorNear(coeffs, { 5.0 / 231, -30.0 / 231, 75.0 / 231, 131.0 / 231, 75.0 / 231, -30.0 / 231, 5.0 / 231 });
	EXPECT_NEAR(sum(coeffs), 1.0, 1e-12);
}

TEST(SavgolCoeffs, FirstDerivativeMatchesReference)
{
	auto coeffs = Savgol::savgolCoeffs(7, 2, 1);

	expectVectorNear(coeffs, { -3.0 / 28, -2.0 / 28, -1.0 / 28, 0.0, 1.0 / 28, 2.0 / 28, 3.0 / 28 });
	EXPECT_NEAR(sum(coeffs), 0.0, 1e-12);
}

TEST(SavgolCoeffs, DerivativeScalesWithDelta)
{
	auto unit = Savgol::savgolCoeffs(5, 2, 1, 1.0);
	auto scaled = Savgol::savgolCoeffs(5, 2, 1, 0.5);

	ASSERT_EQ(unit.size(), scaled.size());
	for (size_t i = 0; i < unit.size(); ++i) {
		EXPECT_NEAR(scaled[i], unit[i] * 2.0, 1e-12);
	}
}

TEST(SavgolCoeffs, RejectsInvalidArguments)
{
	EXPECT_THROW(Savgol::savgolCoeffs(6, 2), std::invalid_argument);
	EXPECT_THROW(Savgol::savgolCoeffs(5, 5), std::invalid_argument);
	EXPECT_THROW(Savgol::savgolCoeffs(5, 2, 3), std::invalid_argument);
}

TEST(SavgolConvolve, MirrorPaddingIsSymmetricAtBothEdges)
{
	const std::vector<double> x = { 1, 2, 4, 8, 16 };

	// Kernel picking x[i - 2] exercises the left edge, x[i + 2] the right edge
	expectVectorNear(Savgol::convolve(x, { 1, 0, 0, 0, 0 }), { 4, 2, 1, 2, 4 });
	expectVectorNear(Savgol::convolve(x, { 0, 0, 0, 0, 1 }), { 4, 8, 16, 8, 4 });
	expectVectorNear(Savgol::convolve(x, { 1, 2, 3, 4, 5 }), { 39, 66, 129, 138, 120 });
}

TEST(SavgolConvolve, ConstantPaddingUsesFillValue)
{
	expectVectorNear(Savgol::convolve({ 1, 2, 3 }, { 1, 1, 1 }, Savgol::SignalPadding::CONSTANT, 10.0), { 13, 6, 15 });
}

TEST(SavgolFilter, MatchesScipyWindow5Order2)
{
	expectVectorNear(Savgol::savgolFilter(kSignal, 5, 2), {
		2.885714285714285, 1.9428571428571417, 4.571428571428569, 5.34285714285714, 7.14285714285714, 4.685714285714283,
		5.999999999999998, 6.857142857142853, 5.5714285714285685, 1.971428571428571, -0.05714285714285733
	});
}

TEST(SavgolFilter, MatchesScipyWindow7Order5)
{
	// Default parameters of the application
	expectVectorNear(Savgol::savgolFilter(kSignal, 7, 5), {
		2.99134199134199, 1.8354978354978333, 4.489177489177476, 5.467532467532456, 7.337662337662339, 4.53246753246754,
		5.6406926406926345, 7.2683982683982515, 5.826839826839856, 1.6233766233766338, -0.03463203463203568
	});
}

TEST(SavgolFilter, MatchesScipyFirstDerivative)
{
	expectVectorNear(Savgol::savgolFilter(kSignal, 5, 2, 1), { 0, 0.9, 1.2, 0.9, 0.1, 0, 0.4, -1.1, -1.1, -2.2, 0 });
}

TEST(SavgolFilter, PreservesConstantSignal)
{
	const std::vector<double> flat(20, 3.5);

	expectVectorNear(Savgol::savgolFilter(flat, 7, 5), flat);
}

TEST(SavgolFilter, PreservesPolynomialUpToOrderAwayFromEdges)
{
	std::vector<double> quadratic;
	for (int i = 0; i < 20; ++i) {
		quadratic.push_back(0.5 * i * i - 3.0 * i + 2.0);
	}

	auto filtered = Savgol::savgolFilter(quadratic, 5, 2);
	for (size_t i = 2; i + 2 < quadratic.size(); ++i) {
		EXPECT_NEAR(filtered[i], quadratic[i], 1e-9) << "at index " << i;
	}
}

TEST(SavgolFilter, RejectsWindowLongerThanSignal)
{
	EXPECT_THROW(Savgol::savgolFilter({ 1, 2, 3 }, 5, 2), std::invalid_argument);
}
