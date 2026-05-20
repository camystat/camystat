#include "IterativeReportWriter.h"

static std::string serialize(const double value, const int precision = 20)
{
	std::ostringstream ss;
	ss.precision(precision);
	ss << value;

	std::string serialized = ss.str();
	// since Excel uses , as the decimal separator, replace all . with ,
	std::replace(serialized.begin(), serialized.end(), '.', ',');

	return serialized;
}

namespace Camystat {
	void IterativeReportWriterRow::reset() {
		this->videoName = "";
		this->videoDurationSeconds = 0;
		this->eventsDetected = 0;
		this->avgEventDurationSeconds = 0.0;
		this->avgRestDurationSeconds = 0.0;
		this->approvedEventsForContrRelaxAnalysis = 0;
		this->avgContractionDurationSeconds = 0.0;
		this->avgRelaxationDurationSeconds = 0.0;
	}

	std::string IterativeReportWriterRow::str() {
		std::ostringstream ss;
		ss << this->videoName << ";";
		ss << this->videoDurationSeconds << ";";
		ss << this->eventsDetected << ";";
		ss << serialize(this->getEventsPerMinute()) << ";";
		ss << serialize(this->avgEventDurationSeconds) << ";";
		ss << serialize(this->avgRestDurationSeconds);

		if (this->contractionRelaxationAnalysis) {
			ss << ";";
			ss << this->approvedEventsForContrRelaxAnalysis << ";";
			ss << serialize(this->avgContractionDurationSeconds) << ";";
			ss << serialize(this->avgRelaxationDurationSeconds);
		}

		return ss.str();
	}
}
