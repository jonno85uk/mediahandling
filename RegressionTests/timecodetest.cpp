/*
  Copyright (c) 2020, Jonathan Noble
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
      * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      * Neither the name of the <organization> nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "timecode.h"

using namespace media_handling;

#include <gtest/gtest.h>


TEST(TimeCodeTests, TimeCodeInit)
{
  Rational time_scale{1,1};
  Rational frame_rate{1,1};
  TimeCode tc(time_scale, frame_rate);
  ASSERT_EQ(tc.timeScale(), time_scale);
  ASSERT_EQ(tc.frameRate(), frame_rate);
  ASSERT_EQ(tc.timestamp(), 0);
  ASSERT_EQ(tc.toMillis(), 0);
  ASSERT_EQ(tc.toFrames(), 0);
  ASSERT_EQ(tc.toString(), "00:00:00:00");
}

TEST(TimeCodeTests, InitSpecifiedTimestamp)
{
  Rational time_scale{1,1};
  Rational frame_rate{1,1};
  TimeCode tc(time_scale, frame_rate, 100);
  ASSERT_EQ(tc.timestamp(), 100);
  ASSERT_EQ(tc.toMillis(), 100'000);
  ASSERT_EQ(tc.toFrames(), 100);
  ASSERT_EQ(tc.toString(), "00:01:40:00");
}


TEST(TimeCodeTests, InitPAL25)
{
  Rational time_scale{1,1000};
  Rational frame_rate{25,1};
  TimeCode tc(time_scale, frame_rate, 150); // 0.15s
  ASSERT_EQ(tc.timestamp(), 150);
  ASSERT_EQ(tc.toFrames(), 3);
  ASSERT_EQ(tc.toMillis(), 150);
  ASSERT_EQ(tc.toString(), "00:00:00:03");

  tc.setTimestamp(60'000); // 1min
  ASSERT_EQ(tc.timestamp(), 60'000);
  ASSERT_EQ(tc.toMillis(), 60'000);
  ASSERT_EQ(tc.toFrames(), 1'500);
  ASSERT_EQ(tc.toString(), "00:01:00:00");


  tc.setTimestamp(3'600'000); // 1hour
  ASSERT_EQ(tc.timestamp(), 3'600'000);
  ASSERT_EQ(tc.toMillis(), 3'600'000);
  ASSERT_EQ(tc.toFrames(), 90'000);
  ASSERT_EQ(tc.toString(), "01:00:00:00");


  tc.setTimestamp(100'000); // 1min
  ASSERT_EQ(tc.timestamp(), 100'000);
  ASSERT_EQ(tc.toMillis(), 100'000);
  ASSERT_EQ(tc.toFrames(), 2'500);
  ASSERT_EQ(tc.toString(), "00:01:40:00");
}


TEST(TimeCodeTests, InitNTSC24)
{
  Rational time_scale{1,1000};
  Rational frame_rate{24000, 1001};
  TimeCode tc(time_scale, frame_rate, 600'000); // 10 minutes
  ASSERT_EQ(tc.timestamp(), 600'000);
  ASSERT_EQ(tc.toMillis(), 600'000);
  ASSERT_EQ(tc.toFrames(), 14'385);
  ASSERT_EQ(tc.toString(), "00:09:59:09");
}

TEST(TimeCodeTests, InitNTSC30)
{
  Rational time_scale{1,1000};
  Rational frame_rate{30000,1001};
  TimeCode tc(time_scale, frame_rate);
  ASSERT_EQ(tc.timestamp(), 0);
  ASSERT_EQ(tc.toMillis(), 0);
  ASSERT_EQ(tc.toFrames(), 0);
  ASSERT_EQ(tc.toString(), "00:00:00;00");
  ASSERT_EQ(tc.toString(false), "00:00:00:00");

  tc.setTimestamp(59'966);
  ASSERT_EQ(tc.timestamp(), 59'966);
  ASSERT_EQ(tc.toMillis(), 59'966);
  ASSERT_EQ(tc.toFrames(), 1'797);
  ASSERT_EQ(tc.toString(), "00:00:59;27");
  ASSERT_EQ(tc.toString(false), "00:00:59:27");

  tc.setTimestamp(60'000);
  ASSERT_EQ(tc.timestamp(), 60'000);
  ASSERT_EQ(tc.toMillis(), 60'000);
  ASSERT_EQ(tc.toFrames(), 1'798);
  ASSERT_EQ(tc.toString(), "00:00:59;28");
  ASSERT_EQ(tc.toString(false), "00:00:59:28");

  tc.setTimestamp(60'033);
  ASSERT_EQ(tc.timestamp(), 60'033);
  ASSERT_EQ(tc.toMillis(), 60'033);
  ASSERT_EQ(tc.toFrames(), 1'799);
  ASSERT_EQ(tc.toString(), "00:00:59;29");
  ASSERT_EQ(tc.toString(false), "00:00:59:29");

  tc.setTimestamp(60'066);
  ASSERT_EQ(tc.timestamp(), 60'066);
  ASSERT_EQ(tc.toMillis(), 60'066);
  ASSERT_EQ(tc.toFrames(), 1'800);
  ASSERT_EQ(tc.toString(), "00:01:00;02");
  ASSERT_EQ(tc.toString(false), "00:01:00:00");

  tc.setTimestamp(60'099);
  ASSERT_EQ(tc.timestamp(), 60'099);
  ASSERT_EQ(tc.toMillis(), 60'099);
  ASSERT_EQ(tc.toFrames(), 1'801);
  ASSERT_EQ(tc.toString(), "00:01:00;03");
  ASSERT_EQ(tc.toString(false), "00:01:00:01");

  tc.setTimestamp(599'967); // 10minute - 1frame
  ASSERT_EQ(tc.timestamp(), 599'967);
  ASSERT_EQ(tc.toMillis(), 599'967);
  ASSERT_EQ(tc.toFrames(), 17'981);
  ASSERT_EQ(tc.toString(), "00:09:59;29");
  ASSERT_EQ(tc.toString(false), "00:09:59:11");

  tc.setTimestamp(600'000); // 10minute
  ASSERT_EQ(tc.timestamp(), 600'000);
  ASSERT_EQ(tc.toMillis(), 600'000);
  ASSERT_EQ(tc.toFrames(), 17'982);
  ASSERT_EQ(tc.toString(), "00:10:00;00");
  ASSERT_EQ(tc.toString(false), "00:09:59:12");

  tc.setTimestamp(600'033); // 10minute + 1frame
  ASSERT_EQ(tc.timestamp(), 600'033);
  ASSERT_EQ(tc.toMillis(), 600'033);
  ASSERT_EQ(tc.toFrames(), 17'983);
  ASSERT_EQ(tc.toString(), "00:10:00;01");
  ASSERT_EQ(tc.toString(false), "00:09:59:13");
}

// TEST(TimeCodeTests, NTSC30Frames)
// {
//   Rational time_scale{1,1};
//   Rational frame_rate{30000, 1001};
//   TimeCode tc(time_scale, frame_rate);
//   ASSERT_EQ(tc.framesToSMPTE(0), "00:00:00;00");
//   ASSERT_EQ(tc.framesToSMPTE(1), "00:00:00;01");
//   ASSERT_EQ(tc.framesToSMPTE(2), "00:00:00;02");

//   ASSERT_EQ(tc.framesToSMPTE(1797), "00:00:59;27");
//   ASSERT_EQ(tc.framesToSMPTE(1798), "00:00:59;28");
//   ASSERT_EQ(tc.framesToSMPTE(1799), "00:00:59;29");
//   ASSERT_EQ(tc.framesToSMPTE(1800), "00:01:00;02");
//   ASSERT_EQ(tc.framesToSMPTE(1801), "00:01:00;03");
//   ASSERT_EQ(tc.framesToSMPTE(1802), "00:01:00;04");

//   ASSERT_EQ(tc.framesToSMPTE(17979), "00:09:59;27");
//   ASSERT_EQ(tc.framesToSMPTE(17980), "00:09:59;28");
//   ASSERT_EQ(tc.framesToSMPTE(17981), "00:09:59;29");
//   ASSERT_EQ(tc.framesToSMPTE(17982), "00:10:00;00");
//   ASSERT_EQ(tc.framesToSMPTE(17983), "00:10:00;01");
//   ASSERT_EQ(tc.framesToSMPTE(17984), "00:10:00;02");
// }

TEST(TimeCodeTests, InitNTSC60)
{
  Rational time_scale{1,1000};
  Rational frame_rate{60000,1001};
  TimeCode tc(time_scale, frame_rate);

  tc.setTimestamp(60'046);
  ASSERT_EQ(tc.timestamp(), 60'046);
  ASSERT_EQ(tc.toMillis(), 60'046);
  ASSERT_EQ(tc.toFrames(), 3'599);
  ASSERT_EQ(tc.toString(), "00:00:59;59");
  ASSERT_EQ(tc.toString(false), "00:00:59:59");

  tc.setTimestamp(60'066);
  ASSERT_EQ(tc.timestamp(), 60'066);
  ASSERT_EQ(tc.toMillis(), 60'066);
  ASSERT_EQ(tc.toFrames(), 3'600);
  ASSERT_EQ(tc.toString(), "00:01:00;04");
  ASSERT_EQ(tc.toString(false), "00:01:00:00");

  tc.setTimestamp(60'081);
  ASSERT_EQ(tc.timestamp(), 60'081);
  ASSERT_EQ(tc.toMillis(), 60'081);
  ASSERT_EQ(tc.toFrames(), 3'601);
  ASSERT_EQ(tc.toString(), "00:01:00;05");
  ASSERT_EQ(tc.toString(false), "00:01:00:01");

  tc.setTimestamp(599'983); // 10minute - 1frame
  ASSERT_EQ(tc.timestamp(), 599'983);
  ASSERT_EQ(tc.toMillis(), 599'983);
  ASSERT_EQ(tc.toFrames(), 35'963);
  ASSERT_EQ(tc.toString(), "00:09:59;59");
  ASSERT_EQ(tc.toString(false), "00:09:59:23");

  tc.setTimestamp(600'000); // 10minute
  ASSERT_EQ(tc.timestamp(), 600'000);
  ASSERT_EQ(tc.toMillis(), 600'000);
  ASSERT_EQ(tc.toFrames(), 35'964);
  ASSERT_EQ(tc.toString(), "00:10:00;00");
  ASSERT_EQ(tc.toString(false), "00:09:59:24");

  tc.setTimestamp(600'017); // 10minute + 1frame
  ASSERT_EQ(tc.timestamp(), 600'017);
  ASSERT_EQ(tc.toMillis(), 600'017);
  ASSERT_EQ(tc.toFrames(), 35'965);
  ASSERT_EQ(tc.toString(), "00:10:00;01");
  ASSERT_EQ(tc.toString(false), "00:09:59:25");
}

TEST(TimeCodeTests, TCFromString)
{
  Rational time_scale{1, 1000};
  Rational frame_rate{30, 1};
  TimeCode tc(time_scale, frame_rate);
  ASSERT_FALSE(tc.setTimeCode(""));
  ASSERT_FALSE(tc.setTimeCode("12345678"));
  ASSERT_FALSE(tc.setTimeCode("12.34.56.78"));
  ASSERT_FALSE(tc.setTimeCode("12:34:56.78"));
  ASSERT_FALSE(tc.setTimeCode("12;34;56;78"));
  ASSERT_FALSE(tc.setTimeCode("12:34:56:78"));
  ASSERT_FALSE(tc.setTimeCode("12:34:56;00"));
  ASSERT_TRUE(tc.setTimeCode("12:34:56:00"));
  ASSERT_EQ(tc.toFrames(), 1'358'880);
  ASSERT_TRUE(tc.setTimeCode("12:34:56:01"));
  ASSERT_EQ(tc.toFrames(), 1'358'881);
  ASSERT_TRUE(tc.setTimeCode("00:00:00:01"));
  ASSERT_EQ(tc.toFrames(), 1);
  ASSERT_TRUE(tc.setTimeCode("00:00:00:00"));
  ASSERT_EQ(tc.toFrames(), 0);
  ASSERT_TRUE(tc.setTimeCode("23:59:59:29"));
  ASSERT_EQ(tc.toFrames(), 2'591'999);
}

TEST(TimeCodeTests, DropTCFromString)
{
  Rational time_scale{1, 1000};
  Rational frame_rate{30000, 1001};
  TimeCode tc(time_scale, frame_rate);
  ASSERT_TRUE(tc.setTimeCode("12:34:56:00"));
  ASSERT_EQ(tc.toFrames(), 1'358'880);
  ASSERT_TRUE(tc.setTimeCode("12:34:56;00"));
  ASSERT_EQ(tc.toFrames(), 1'357'522);
  ASSERT_TRUE(tc.setTimeCode("00:00:00;00"));
  ASSERT_EQ(tc.toFrames(), 0);
  ASSERT_TRUE(tc.setTimeCode("00:01:00;02"));
  ASSERT_EQ(tc.toFrames(), 1'800);
  ASSERT_TRUE(tc.setTimeCode("00:01:00:00"));
  ASSERT_EQ(tc.toFrames(), 1'800);
  ASSERT_TRUE(tc.setTimeCode("00:01:00;00"));
  ASSERT_EQ(tc.toFrames(), 1'798);
  ASSERT_TRUE(tc.setTimeCode("00:10:00:00"));
  ASSERT_EQ(tc.toFrames(), 18'000);
  ASSERT_TRUE(tc.setTimeCode("00:09:59;29"));
  ASSERT_EQ(tc.toFrames(), 17'981);
  ASSERT_TRUE(tc.setTimeCode("00:10:00;00"));
  ASSERT_EQ(tc.toFrames(), 17'982);
  ASSERT_TRUE(tc.setTimeCode("00:10:00;01"));
  ASSERT_EQ(tc.toFrames(), 17'983);
}
