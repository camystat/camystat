#include <gtest/gtest.h>
#include "Camystat.h"

using Camystat::Detection;

using Events = std::vector<std::vector<double>>;

TEST(MergeEvents, MergesChainOfCloseEvents)
{
	// Events are { number, integral, start, end }
	Events events = { { 1, 5, 0, 10 }, { 2, 3, 12, 20 }, { 3, 4, 22, 30 } };

	EXPECT_EQ(Detection::merge_events(events, 3), (Events{ { 1, 12, 0, 30 } }));
}

TEST(MergeEvents, KeepsDistantEventsAndRenumbers)
{
	Events events = { { 1, 5, 0, 10 }, { 2, 3, 12, 20 }, { 3, 4, 40, 50 }, { 4, 1, 52, 55 } };

	EXPECT_EQ(Detection::merge_events(events, 3), (Events{ { 1, 8, 0, 20 }, { 2, 5, 40, 55 } }));
}

TEST(MergeEvents, NoMergeBeyondThreshold)
{
	Events events = { { 1, 5, 0, 10 }, { 2, 3, 14, 20 } };

	EXPECT_EQ(Detection::merge_events(events, 3), events);
}

TEST(MergeEvents, EmptyInput)
{
	EXPECT_TRUE(Detection::merge_events({}, 3).empty());
}

using Phase = Detection::Phase;

TEST(PhaseStatistics, AveragesOverAllPhases)
{
	std::vector<Phase> phases = {
		{ Phase::CONTRACTION, 1, 0, 0, 9 },
		{ Phase::RELAXATION, 2, 0, 9, 16 },
		{ Phase::CONTRACTION, 3, 0, 20, 29 },
		{ Phase::RELAXATION, 4, 0, 29, 36 },
		{ Phase::CONTRACTION, 5, 0, 40, 43 },
		{ Phase::RELAXATION, 6, 0, 43, 48 },
	};

	auto stats = Detection::calculate_phase_statistics(phases);

	EXPECT_EQ(stats.contractionCount, 3);
	EXPECT_EQ(stats.relaxationCount, 3);
	EXPECT_DOUBLE_EQ(stats.avgContractionLengthFrames, (10.0 + 10.0 + 4.0) / 3.0);
	EXPECT_DOUBLE_EQ(stats.avgRelaxationLengthFrames, (8.0 + 8.0 + 6.0) / 3.0);
}

TEST(PhaseStatistics, NoPhases)
{
	auto stats = Detection::calculate_phase_statistics({});

	EXPECT_EQ(stats.contractionCount, 0);
	EXPECT_EQ(stats.relaxationCount, 0);
	EXPECT_DOUBLE_EQ(stats.avgContractionLengthFrames, 0.0);
	EXPECT_DOUBLE_EQ(stats.avgRelaxationLengthFrames, 0.0);
}
