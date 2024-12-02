#include "Savgol.h"

 std::vector<double> Savgol::savgolCoeffs(size_t window_length, size_t polyorder, size_t deriv, double delta) {
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

std::vector<double> Savgol::convolve(const std::vector<double>& x, const std::vector<double>& coeffs, const SignalPadding& padding, double cval) {
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
			else if (padding == Savgol::SignalPadding::CONSTANT) {
				sum += cval * coeffs[j];
			}
			else if (padding == Savgol::SignalPadding::MIRROR) {
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

std::vector<double> Savgol::savgolFilter(const std::vector<double>& x, size_t window_length, size_t polyorder, size_t deriv, double delta, const SignalPadding& padding, double cval) {
	if (window_length > x.size()) {
		throw std::invalid_argument("window_length (received: " + std::to_string(window_length) + ") must be <= length of x (which is: " + std::to_string(x.size()) + ")");
	}

	if (polyorder >= window_length) {
		throw std::invalid_argument("polyorder (received: " + std::to_string(polyorder) + ") must be < than window_length (received: " + std::to_string(window_length) + ")");
	}

	std::vector<double> coeffs = savgolCoeffs(window_length, polyorder, deriv, delta);
	return convolve(x, coeffs, padding, cval);
}
