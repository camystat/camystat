#pragma once
#include <Eigen/Dense>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <cmath>

using Eigen::MatrixXd;
using Eigen::VectorXd;
using Eigen::Map;
class Savgol
{
public:
	/// <summary>
	/// Function to generate Savitzky-Golay filter coefficients for a filter with window of length <paramref name="window_length"/>, local polynomial of order <paramref name="polyorder"/>,
	/// fitted using derivative of order <paramref name="derivative"/> and spacing between input data samples of <paramref name="delta"/>.
	/// </summary>
	/// <param name="window_length">Filter window length</param>
	/// <param name="polyorder">Order of the local polynomial to be fitted on signal subsets</param>
	/// <param name="deriv">Order of derivative to compute; defaults to 0</param>
	/// <param name="delta">Spacing of input data samples; defaults to 1.0</param>
	/// <returns>Filtered signal</returns>
	static std::vector<double> savgol_coeffs(size_t window_length, size_t polyorder, size_t deriv = 0, double delta = 1.0) {
		if (polyorder >= window_length) {
			throw std::invalid_argument("polyorder (received: " + std::to_string(polyorder) + ") must be < than window_length (received: " + std::to_string(window_length) + ")");
		}

		int half_window = window_length / 2;
		std::vector<double> x(window_length);
		for (int i = -half_window; i <= half_window; ++i) {
			x[i + half_window] = i;
		}

		// Create the Vandermonde matrix
		MatrixXd A(window_length, polyorder + 1);
		for (size_t i = 0; i < window_length; ++i) {
			for (size_t j = 0; j <= polyorder; ++j) {
				A(i, j) = std::pow(x[i], j);
			}
		}

		// Compute A^T * A
		MatrixXd ATA = A.transpose() * A;

		// Compute the pseudoinverse of ATA
		std::vector<double> coeffs(window_length);
		for (size_t i = 0; i < window_length; ++i) {
			coeffs[i] = 0.0;
			for (size_t j = 0; j <= polyorder; ++j) {
				for (size_t k = 0; k <= polyorder; ++k) {
					coeffs[i] += A(i, j) * ATA(j, k) * (k == deriv ? 1.0 : 0.0);
				}
			}
			coeffs[i] /= delta;
		}

		return coeffs;
	}


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
	static std::vector<double> convolve(const std::vector<double>& x, const std::vector<double>& coeffs, const SignalPadding& padding = SignalPadding::MIRROR, double cval = 0.0) {
		size_t n = x.size();
		size_t m = coeffs.size();
		size_t half_m = m / 2;
		std::vector<double> result(n, 0.0);

		for (size_t i = 0; i < n; ++i) {
			double sum = 0.0;
			for (size_t j = 0; j < m; ++j) {
				int index = i - half_m + j;
				if (index >= 0 && index < n) {
					sum += x[index] * coeffs[j];
				}
				else if (padding == SignalPadding::CONSTANT) {
					sum += cval * coeffs[j];
				}
				else if (padding == SignalPadding::MIRROR) {
					if (index < 0) {
						sum += x[-index] * coeffs[j];
					}
					else if (index >= n) {
						sum += x[2 * n - index - 1] * coeffs[j];
					}
				}
			}
			result[i] = sum;
		}

		return result;
	}

	// Function to apply the Savitzky-Golay filter to 1D data
	static std::vector<double> savgol_filter(const std::vector<double>& x, size_t window_length, size_t polyorder, size_t deriv = 0, double delta = 1.0, const SignalPadding& padding = SignalPadding::MIRROR, double cval = 0.0) {
		if (window_length > x.size()) {
			throw std::invalid_argument("window_length (received: " + std::to_string(window_length) + ") must be <= length of x (which is: " + std::to_string(x.size()) + ")");
		}

		if (polyorder >= window_length) {
			throw std::invalid_argument("polyorder (received: " + std::to_string(polyorder) + ") must be < than window_length (received: " + std::to_string(window_length) + ")");
		}

		std::vector<double> coeffs = savgol_coeffs(window_length, polyorder, deriv, delta);
		return convolve(x, coeffs, padding, cval);
	}
};
