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
