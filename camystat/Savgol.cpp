#include "Savgol.h"

 std::vector<double> Savgol::savgolCoeffs(size_t window_length, size_t polyorder, size_t deriv, double delta) {
	if (polyorder >= window_length) {
		throw std::invalid_argument("polyorder (received: " + std::to_string(polyorder) + ") must be < than window_length (received: " + std::to_string(window_length) + ")");
	}

	if (window_length % 2 == 0) {
		throw std::invalid_argument("window_length (received: " + std::to_string(window_length) + ") must be odd");
	}

	if (deriv > polyorder) {
		throw std::invalid_argument("deriv (received: " + std::to_string(deriv) + ") must be <= polyorder (received: " + std::to_string(polyorder) + ")");
	}

	int half_window = static_cast<int>(window_length / 2);

	// Create the Vandermonde matrix over sample positions -half_window..half_window
	MatrixXd A(window_length, polyorder + 1);
	for (size_t i = 0; i < window_length; ++i) {
		double x = static_cast<double>(static_cast<int>(i) - half_window);
		for (size_t j = 0; j <= polyorder; ++j) {
			A(i, j) = std::pow(x, j);
		}
	}

	// Least-squares fit: polynomial coefficients = pinv(A) * y, where pinv(A) = (A^T A)^-1 A^T.
	// Row `deriv` of pinv(A) yields the deriv-th polynomial coefficient at the window centre;
	// multiplying by deriv! gives the deriv-th derivative there.
	MatrixXd pinvA = A.completeOrthogonalDecomposition().pseudoInverse();

	double scale = std::tgamma(static_cast<double>(deriv) + 1.0) / std::pow(delta, static_cast<double>(deriv));

	// Coefficients are in correlation order, matching Savgol::convolve (y[i] = sum_j x[i - half + j] * c[j])
	std::vector<double> coeffs(window_length);
	for (size_t i = 0; i < window_length; ++i) {
		coeffs[i] = pinvA(deriv, i) * scale;
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
			long long index = static_cast<long long>(i) - static_cast<long long>(half_m) + static_cast<long long>(j);
			long long len = static_cast<long long>(n);
			if (index >= 0 && index < len) {
				sum += x[index] * coeffs[j];
			}
			else if (padding == Savgol::SignalPadding::CONSTANT) {
				sum += cval * coeffs[j];
			}
			else if (padding == Savgol::SignalPadding::MIRROR) {
				// Reflect about the edge sample without repeating it (x[-1] = x[1], x[n] = x[n-2])
				if (index < 0) {
					sum += x[-index] * coeffs[j];
				}
				else {
					sum += x[2 * (len - 1) - index] * coeffs[j];
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
