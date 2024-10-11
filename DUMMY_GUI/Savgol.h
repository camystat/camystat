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
    // Function to generate Savitzky-Golay filter coefficients
    static std::vector<double> savgol_coeffs(int window_length, int polyorder, int deriv = 0, double delta = 1.0) {
        if (polyorder >= window_length) {
            throw std::invalid_argument("polyorder must be less than window_length.");
        }

        int half_window = window_length / 2;
        std::vector<double> x(window_length);
        for (int i = -half_window; i <= half_window; ++i) {
            x[i + half_window] = i;
        }

        // Create the Vandermonde matrix
        std::vector<std::vector<double>> A(window_length, std::vector<double>(polyorder + 1));
        for (int i = 0; i < window_length; ++i) {
            for (int j = 0; j <= polyorder; ++j) {
                A[i][j] = std::pow(x[i], j);
            }
        }

        // Compute A^T * A
        std::vector<std::vector<double>> ATA(polyorder + 1, std::vector<double>(polyorder + 1, 0.0));
        for (int i = 0; i <= polyorder; ++i) {
            for (int j = 0; j <= polyorder; ++j) {
                for (int k = 0; k < window_length; ++k) {
                    ATA[i][j] += A[k][i] * A[k][j];
                }
            }
        }

        // Compute the pseudoinverse of ATA
        std::vector<double> b(window_length, 0.0);
        b[deriv] = 1.0;

        std::vector<double> coeffs(window_length);
        for (int i = 0; i < window_length; ++i) {
            coeffs[i] = 0.0;
            for (int j = 0; j <= polyorder; ++j) {
                for (int k = 0; k <= polyorder; ++k) {
                    coeffs[i] += A[i][j] * ATA[j][k] * b[k];
                }
            }
            coeffs[i] /= delta;
        }

        return coeffs;
    }

    // Function to perform convolution
    static std::vector<double> convolve(const std::vector<double>& x, const std::vector<double>& coeffs, const std::string& mode = "mirror", double cval = 0.0) {
        int n = x.size();
        int m = coeffs.size();
        int half_m = m / 2;
        std::vector<double> result(n, 0.0);

        for (int i = 0; i < n; ++i) {
            double sum = 0.0;
            for (int j = 0; j < m; ++j) {
                int index = i - half_m + j;
                if (index >= 0 && index < n) {
                    sum += x[index] * coeffs[j];
                }
                else if (mode == "constant") {
                    sum += cval * coeffs[j];
                }
                else if (mode == "mirror") {
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
    static std::vector<double> savgol_filter(const std::vector<double>& x, int window_length, int polyorder, int deriv = 0, double delta = 1.0, const std::string& mode = "mirror", double cval = 0.0) {
        if (mode != "mirror" && mode != "constant") {
            throw std::invalid_argument("mode must be 'mirror' or 'constant'");
        }

        std::vector<double> coeffs = savgol_coeffs(window_length, polyorder, deriv, delta);
        return convolve(x, coeffs, mode, cval);
    }
};

