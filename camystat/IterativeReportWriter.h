#pragma once
#include <sstream>
#include <fstream>
#include <string>
#include <algorithm>
#include <filesystem>

namespace Camystat {
	class IterativeReportWriterRow {
	public:
		bool contractionRelaxationAnalysis;

		std::string videoName;
		int videoDurationSeconds;
		int eventsDetected;
		double getEventsPerMinute() {
			return static_cast<double>(this->eventsDetected) / (static_cast<double>(this->videoDurationSeconds) / 60.0);
		};
		double avgEventDurationSeconds;
		double avgRestDurationSeconds;
		int approvedEventsForContrRelaxAnalysis;
		double avgContractionDurationSeconds;
		double avgRelaxationDurationSeconds;

		IterativeReportWriterRow(bool contractionRelaxationAnalysis) : contractionRelaxationAnalysis(contractionRelaxationAnalysis) {
			this->reset();
		}

		void reset();

		std::string str();
	};

	class IterativeReportWriter {
	protected:
		std::ofstream outFile;

	public:
		IterativeReportWriterRow rowBuffer;

		IterativeReportWriter() = delete;

		IterativeReportWriter(std::filesystem::path filePath, bool contractionRelaxationAnalysis) : outFile(filePath), rowBuffer(contractionRelaxationAnalysis) {
			// write header
			this->outFile << "Video name;Video duration;Events detected;Events/minute;Average event duration;Average rest duration";
			
			if (contractionRelaxationAnalysis) {
				this->outFile << ";Events approved for further analysis;Average contraction time;Average relaxation time";
			}
			
			this->outFile << std::endl;
		}

		~IterativeReportWriter() {
			this->outFile.close();
		}

		bool isOpen() {
			return this->outFile.is_open();
		}

		void finalizeRow() {
			this->outFile << this->rowBuffer.str() << std::endl;

			this->rowBuffer.reset();
		}
	};
}
