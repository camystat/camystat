#pragma once
#include <Eigen/Dense>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <cmath>

using Eigen::MatrixXd;
using Eigen::VectorXd;
using Eigen::Map;

namespace Savgol
{
	/// <summary>
	/// Function to generate Savitzky-Golay filter coefficients for a filter with window of length <paramref name="window_length"/>, local polynomial of order <paramref name="polyorder"/>,
	/// fitted using derivative of order <paramref name="derivative"/> and spacing between input data samples of <paramref name="delta"/>.
	/// </summary>
	/// <param name="window_length">Filter window length</param>
	/// <param name="polyorder">Order of the local polynomial to be fitted on signal subsets</param>
	/// <param name="deriv">Order of derivative to compute; defaults to 0</param>
	/// <param name="delta">Spacing of input data samples; defaults to 1.0</param>
	/// <returns>Filtered signal</returns>
	extern std::vector<double> savgolCoeffs(size_t window_length, size_t polyorder, size_t deriv = 0, double delta = 1.0);


	// Savitzky-Golay signal padding mode specifier
	// MIRROR uses the adjacent value for padding signal samples
	// CONSTANT uses a constant value (passed as separate argument) for padding signal samples
	enum SignalPadding {
		MIRROR,
		CONSTANT
	};

	/// <summary>
	/// Function to perform convolution of <paramref name="x"/> and <paramref name="coeffs"/> with specified signal <paramref name="padding"/>; if padded by SignalPadding::CONSTANT, the constant padding value will be <paramref name="cval"/>.
	/// </summary>
	/// <param name="x">the signal samples</param>
	/// <param name="coeffs">coefficients</param>
	/// <param name="mode">padding mode</param>
	/// <param name="cval">padding fill value, used ONLY if <paramref name="padding"/> is set to <code>SignalPadding::CONSTANT</code></param>
	/// <returns>Convoluted signal</returns>
	extern std::vector<double> convolve(const std::vector<double>& x, const std::vector<double>& coeffs, const SignalPadding& padding = SignalPadding::MIRROR, double cval = 0.0);

	// Function to apply the Savitzky-Golay filter to 1D data
	extern std::vector<double> savgolFilter(const std::vector<double>& x, size_t window_length, size_t polyorder, size_t deriv = 0, double delta = 1.0, const SignalPadding& padding = SignalPadding::MIRROR, double cval = 0.0);
};
