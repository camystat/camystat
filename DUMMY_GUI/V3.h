#pragma once
#include <iostream>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <opencv2/opencv.hpp>

// Klasa funkcjonujaca jako przestrzen nazw V2 -- implementujaca metody z kodu Pythona w jezyku C++
class V3
{
public:

    // Klasa odpowiedzialna za kompresje danych (etap zero)

    class Compression {
    public:

        // Funkcja kompresji nagrania - 0.1
        //
        // Kompresja stanowi bardzo istotn¹ czêœæ w procesie analizy. 
        //
        // Po pierwsze zmniejsza rozmiar analizowanej w kolejnych etapach macierzy znacznie podnosz¹c wydajnoœæ, 
        // a przy tym(dla nagrañ wykonanych w wiêkszej rozdzielczoœci) nie powoduj¹c utraty kluczowej informacji.
        //
        // Po drugie, redukcja rozmiaru oparta na ³¹czeniu kilku pikseli w jeden redukuje zak³ócenia polegaj¹ce 
        // na mikrodrganiach obrazu, które okaza³y siê wyj¹tkowo uci¹¿liwe w analizie angrañ opartych na ró¿nicowaniu macierzy

        static void resizeVideo(std::string inputPath, std::string outputPath, double scaleFactor);
    };

    // Klasa odpowiedzialna za przetwarzanie wstepne (etap pierwszy)

    class Preprocessing {
    public:
        
        // 1.1 Heatmapa aktywnoœci na filmie
        // 
        // Funkcja s³u¿y stworzeniu heatmapy aktywnoœci przyjmuj¹c œcie¿kê do filmu wideo. 
        //
        // Celem funkcji jest stworzenie heatmapy w postaci tablicy numpy, w oparciu o któr¹ inne funkcje podejmuj¹ 
        // decyzjê o tym jaki fragment obrazu poddaæ analizie.
        //
        // Funkcja przyjmuje cztery argumenty:
        // œcie¿kê do filmu
        // Klatkê pocz¹tkow¹
        // Klatkê koñcow¹
        // Próg binaryzacji
        //
        // Funkcja dzia³a poprzez tworzenie macierzy binarnych z kolejnych klatek i wykonywanie na nich funkcji 
        // XOR z s¹siaduj¹cymi obrazami.W ten sposób otrzymuje informacjê o zmianie pomiêdzy klatkami odpowiadaj¹cej 
        // odpowiednim pikselom.Funkcja zlicza te zmiany dla ka¿dego piksela i zwraca macierz odpowiadaj¹c¹ kszta³tem 
        // wideo z liczb¹ odnotowanych zmian dla ka¿dego piksela.

        static cv::Mat createHeatmap(std::string videoPath, int startFrame = 0, int endFrame = -1, int threshold = 128, std::string resultPath = "");

        // 1.2 Wyznaczanie obszaru zainteresowania
        // 
        // Funkcja do znajdowania najbardziej aktywnych koordynatow
        //
        // Funkcja find_max_sum_square_coordinates_with_percent stanowi rozwiniêcie wersji V1.
        // W odró¿nieniu od niej funkcja przyjmuje procent przeliczany na wielkoœæ poszukiwanego kwadratu o najwiêkszej aktywnoœci.
        // Zaktualizowana funkcja posiada ponadto drugi stopieñ doboru koordynat do analizy.
        // 
        // Z wczeœniej wyselekcjonowanej puli w ramach kwadratu wybiera okreœlony zmienn¹ "top_percent"
        // wycinek zbioru o najwiêkszej aktywnoœci.
        // Funkcja podobnie do poprzedniej wersji zwraca listê koordynat zakwalifikowanych do analizy

        static std::vector<std::pair<int, int>> findMaxSumSquareCoordinatesWithPercent(
            const cv::Mat& pixel_count_array,
            double square_percent,
            double top_percent,
            std::string resultPath,
            std::string imagePath
        );

        // 1.3 Analiza aktywnoœci na nagraniu
        // 
        // Funkcja count_ones_in_xor_at_coordinates jest kluczowym etapem analizy, s³u¿y 
        // dokonaniu analizy aktywnoœci na przestrzeni kolejnych klatek na nagraniu i 
        // zwraca listê zawieraj¹c¹ aktywnoœæ odpowiadaj¹cej ka¿dej klatce wyra¿on¹ jako 
        // stosunek procentowy liczby pikseli aktywnych do liczbie pikseli poddawanych analizie.
        //
        // Funkcja przyjmuje trzy zmienne :
        // video_path - stanowi¹c¹ œcie¿kê do analizowanego nagrania
        // coordinates - listê koordynat poddawanych analizie. Jeœli ta zmienna nie zostanie 
        // zdefiniowania, analizie bêd¹ poddane wszystkie piksele.
        // threshold - okreœlaj¹c¹ próg binaryzacji
        //
        // Analiza aktywnoœci :
        //
        // Proces analizy aktywnoœci jest podobny do tworzenia heatmapy aktywnoœci funkcji 1.1.
        // Funkcja binaryzacje parê klatek wed³ug progu wszystkie tworz¹c zestaw macierzy binarnych.
        // Kolejno funkcja porównuje pary macierzy binarnych przy pomocy funkcji XOR, tworz¹c macierze ró¿nicowe.
        // 
        // Nastêpnie funkcja zlicza ró¿nice dla analizowanych koordynat dla danej pary klatek i dodaje 
        // je do listy w postaci stosunku koordynat aktywnych do poddanych analizie.
        // Po iteracji przez wszystkie klatki filmu funkcja zwraca listê zawieraj¹c¹ kolejne ró¿nice 
        // odpowiadaj¹ce aktywnoœci na kolejnych klatkach filmu.

        static std::vector<double> countOnesInXorAtCoordinates(
            std::string videoPath,
            std::vector<std::pair<int, int>> coordinates = {},
            int threshold = 128,
            std::string resultPath = ""
        );
    };

    class Smoothing {
    public:

        static std::vector<double> applySavgolFilter(const std::vector<double>& data, size_t window_length = 7, size_t polyorder = 5);
        static std::vector<double> smoothValues(const std::vector<double>&, float percentile);
        static std::vector<double> modify_means(const std::vector<double>& input_list, size_t n = 2, size_t x = 1);
        static std::vector<double> clone_normalized_values(const std::vector<double>& values);
        static std::vector<double> clone_replace_zeros_values_below_threshold(const std::vector<double>& lst, double threshold, std::string resultPath);
    };

    class Detection {
    public:
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
        static std::vector<std::vector<double>> calculate_integrals_with_reference_points(const std::vector<double>& values);
        static std::vector<std::vector<double>> merge_events(const std::vector<std::vector<double>>& event_list, double distance_threshold);
        static std::vector<std::vector<double>> remove_events(const std::vector<std::vector<double>>& event_list, double threshold_value);
    };
};
