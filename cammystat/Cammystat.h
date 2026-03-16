#pragma once
#include <atomic>
#include <iostream>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <opencv2/opencv.hpp>
#include <optional>
#include <set>
#include <wx/wx.h>
#include "wx/setup.h"
#include <Eigen/Dense>
#include "Utils.h"

namespace Cammystat
{
    /** Used as default for abortFlag parameters (never aborts). */
    inline std::atomic<bool> kNoAbort{false};

    class ProcessingAbortedException : public std::runtime_error
    {
    public:
        ProcessingAbortedException() : std::runtime_error("Processing aborted") {}
    };

    // Klasa odpowiedzialna za kompresje danych (etap zero)

    class Compression {
    public:

        // Funkcja kompresji nagrania - 0.1
        //
        // Kompresja stanowi bardzo istotną część w procesie analizy. 
        //
        // Po pierwsze zmniejsza rozmiar analizowanej w kolejnych etapach macierzy znacznie podnosząc wydajność, 
        // a przy tym(dla nagrań wykonanych w większej rozdzielczości) nie powodując utraty kluczowej informacji.
        //
        // Po drugie, redukcja rozmiaru oparta na łączeniu kilku pikseli w jeden redukuje zakłócenia polegające 
        // na mikrodrganiach obrazu, które okazały się wyjątkowo uciążliwe w analizie angrań opartych na różnicowaniu macierzy

        static void resizeVideo(const std::filesystem::path& inputPath, const std::filesystem::path& outputPath, double scaleFactor);
    };

    // Klasa odpowiedzialna za przetwarzanie wstepne (etap pierwszy)

    class Preprocessing {
    public:

        enum BinarizationThresholdCalcProgress {
            STARTING,
            FINDING_MAX_BRIGHTNESS_DIFF_FRAMES,
            CALCULATING_XOR_SCORES
        };

        using BinarizationThresholdCalcProgressCallback = std::function<void(Cammystat::Preprocessing::BinarizationThresholdCalcProgress, std::optional<double>, std::optional<int>)>;

        // Binarization threshold
        // 
        // Calculates an automatic binarization threshold for the given video file using frames from the specified range, based on Marcin's algorithm design.

        static std::pair<int, std::vector<int>> calculateBinarizationThreshold(const std::filesystem::path& videoPath, const int startFrame, const int endFrame, const BinarizationThresholdCalcProgressCallback& progressCallback, const std::atomic<bool>& abortFlag);
        
        // 1.1 Heatmapa aktywności na filmie
        // 
        // Funkcja służy stworzeniu heatmapy aktywności przyjmując ścieżkę do filmu wideo. 
        //
        // Celem funkcji jest stworzenie heatmapy w postaci tablicy numpy, w oparciu o którą inne funkcje podejmują 
        // decyzję o tym jaki fragment obrazu poddać analizie.
        //
        // Funkcja przyjmuje cztery argumenty:
        // ścieżkę do filmu
        // Klatkę początkową
        // Klatkę końcową
        // Próg binaryzacji
        //
        // Funkcja działa poprzez tworzenie macierzy binarnych z kolejnych klatek i wykonywanie na nich funkcji 
        // XOR z sąsiadującymi obrazami.W ten sposób otrzymuje informację o zmianie pomiędzy klatkami odpowiadającej 
        // odpowiednim pikselom.Funkcja zlicza te zmiany dla każdego piksela i zwraca macierz odpowiadającą kształtem 
        // wideo z liczbą odnotowanych zmian dla każdego piksela.

        static cv::Mat createHeatmap(const std::filesystem::path& videoPath, const int startFrame = 0, const int endFrame = -1, const int threshold = 128, const std::filesystem::path& resultPath = "", const std::atomic<bool>& abortFlag = kNoAbort);

        // 1.2 Wyznaczanie obszaru zainteresowania
        // 
        // Funkcja do znajdowania najbardziej aktywnych koordynatow
        //
        // Funkcja find_max_sum_square_coordinates_with_percent stanowi rozwinięcie wersji V1.
        // W odróżnieniu od niej funkcja przyjmuje procent przeliczany na wielkość poszukiwanego kwadratu o największej aktywności.
        // Zaktualizowana funkcja posiada ponadto drugi stopień doboru koordynat do analizy.
        // 
        // Z wcześniej wyselekcjonowanej puli w ramach kwadratu wybiera określony zmienną "top_percent"
        // wycinek zbioru o największej aktywności.
        // Funkcja podobnie do poprzedniej wersji zwraca listę koordynat zakwalifikowanych do analizy

        static std::vector<std::pair<int, int>> findMaxSumSquareCoordinatesWithPercent(
            const cv::Mat& pixel_count_array,
            const double square_percent,
            const double top_percent,
            const std::filesystem::path& resultPath,
            const std::filesystem::path& imagePath,
            const std::atomic<bool>& abortFlag = kNoAbort
        );

        // 1.3 Analiza aktywności na nagraniu
        // 
        // Funkcja count_ones_in_xor_at_coordinates jest kluczowym etapem analizy, służy 
        // dokonaniu analizy aktywności na przestrzeni kolejnych klatek na nagraniu i 
        // zwraca listę zawierającą aktywność odpowiadającej każdej klatce wyrażoną jako 
        // stosunek procentowy liczby pikseli aktywnych do liczbie pikseli poddawanych analizie.
        //
        // Funkcja przyjmuje trzy zmienne :
        // video_path - stanowiącą ścieżkę do analizowanego nagrania
        // coordinates - listę koordynat poddawanych analizie. Jeśli ta zmienna nie zostanie 
        // zdefiniowania, analizie będą poddane wszystkie piksele.
        // threshold - określającą próg binaryzacji
        //
        // Analiza aktywności :
        //
        // Proces analizy aktywności jest podobny do tworzenia heatmapy aktywności funkcji 1.1.
        // Funkcja binaryzacje parę klatek według progu wszystkie tworząc zestaw macierzy binarnych.
        // Kolejno funkcja porównuje pary macierzy binarnych przy pomocy funkcji XOR, tworząc macierze różnicowe.
        // 
        // Następnie funkcja zlicza różnice dla analizowanych koordynat dla danej pary klatek i dodaje 
        // je do listy w postaci stosunku koordynat aktywnych do poddanych analizie.
        // Po iteracji przez wszystkie klatki filmu funkcja zwraca listę zawierającą kolejne różnice 
        // odpowiadające aktywności na kolejnych klatkach filmu.

        static std::vector<double> countOnesInXorAtCoordinates(
            const std::filesystem::path& videoPath,
            const std::vector<std::pair<int, int>>& coordinates = {},
            const int threshold = 128,
            const std::filesystem::path& resultPath = "",
            const std::atomic<bool>& abortFlag = kNoAbort
        );
    };

    class Smoothing {
    public:

        static std::vector<double> applySavgolFilter(const std::vector<double>& data, size_t window_length = 7, size_t polyorder = 5);
        static std::vector<double> smoothValues(const std::vector<double>&, float percentile);
        static std::vector<double> modifyMeans(const std::vector<double>& input_list, size_t n = 2, size_t x = 1, const std::atomic<bool>& abortFlag = kNoAbort);
        static std::vector<double> cloneNormalizedValues(const std::vector<double>& values);
        static std::vector<double> cloneReplaceZerosValuesBelowThreshold(const std::vector<double>& lst, double threshold, std::filesystem::path resultPath);
    };

    class Detection {
    public:
        class Phase {
        public:
            enum PhaseType
            {
                CONTRACTION,
                RELAXATION
            };

            /// <summary>
            /// Represents a phase of a contraction or relaxation event.
            /// Corresponds to "contraction" or "relaxation".
            /// </summary>
            PhaseType phase_type;
            int phase_number;
            double phase_value;
            int start_index;
            int end_index;

            Phase(PhaseType phase_type, int phase_number, double phase_value, int start_index, int end_index) :
                phase_type(phase_type), phase_number(phase_number), phase_value(phase_value), start_index(start_index), end_index(end_index) {}

            std::string to_string() const {
                std::ostringstream oss;

                oss << (phase_type == PhaseType::CONTRACTION ? "contraction" : "relaxation") << "," << phase_number << "," << phase_value << "," << start_index << "," << end_index;
                return oss.str();
            }

            friend std::ostream& operator<< (std::ostream& stream, const Phase& phase) {
                stream << phase.to_string();
                return stream;
            }
        };

        static std::vector<double> trim_list(const std::vector<double>& lst, int n, int x) {
            /*
            Removes n elements from the beginning and x elements from the end of the list.

            Args:
            lst (std::vector<double>): The input list (vector).
            n (int): Number of elements to remove from the beginning.
            x (int): Number of elements to remove from the end.

            Returns:
            std::vector<double>: The trimmed list (vector).
            */

            if (n < 0 || x < 0) {
                throw std::invalid_argument("n and x must be non-negative integers.");
            }

            if (n + x > lst.size()) {
                throw std::out_of_range("Cannot remove more elements than the list contains.");
            }

            // Remove n elements from the beginning and x elements from the end
            std::vector<double> trimmed_list(lst.begin() + n, lst.end() - x);

            return trimmed_list;
        }
        static std::vector<double> clone_padded_with_zeros(const std::vector<double>& input_list);
        static std::vector<std::vector<double>> calculate_integrals_with_reference_points(const std::vector<double>& values, const std::atomic<bool>& abortFlag);
        static std::vector<std::vector<double>> merge_events(const std::vector<std::vector<double>>& event_list, double distance_threshold);
        static std::vector<std::vector<double>> remove_events(const std::vector<std::vector<double>>& event_list, double threshold_value);

        static std::vector<Cammystat::Detection::Phase> locate_contractions_and_relaxations(const std::vector<double>& values, const std::vector<std::vector<double>>& integrals_results, const std::atomic<bool>& abortFlag);
    };
};
