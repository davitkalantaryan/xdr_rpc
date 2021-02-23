//
// file:			0000_initial.cpp
// created on:		2021 Feb 02
//

#include "gtest/gtest.h"

static int s_handler_execution_number = 2;

TEST(f_0000_initial, first)
{
	EXPECT_EQ(s_handler_execution_number, 2);

}
