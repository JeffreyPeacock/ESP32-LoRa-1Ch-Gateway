// Host-side tests for the LoRa parameter arithmetic. No hardware required:
//
//   make test          (or: pio test -e native)

#include <unity.h>

#include "loramath.h"

void test_bandwidth_index_boundaries() {
    // The table is "less than or equal", so the named rates must land on their
    // own index and not the next one up.
    TEST_ASSERT_EQUAL_UINT8(7, loramath::bandwidthIndex(125000));
    TEST_ASSERT_EQUAL_UINT8(8, loramath::bandwidthIndex(250000));
    TEST_ASSERT_EQUAL_UINT8(9, loramath::bandwidthIndex(500000));
    TEST_ASSERT_EQUAL_UINT8(6, loramath::bandwidthIndex(62500));
    // One hertz over a boundary must move up.
    TEST_ASSERT_EQUAL_UINT8(8, loramath::bandwidthIndex(125001));
}

void test_frf_does_not_overflow_32_bit() {
    // 915 MHz: (915e6 << 19) / 32e6. Computed in 32-bit this wraps and tunes
    // the radio to nonsense, so pin the exact expected register value.
    TEST_ASSERT_EQUAL_UINT32(14991360UL, loramath::frfRegister(915000000UL));

    // 903.9 MHz -- this board's configured channel.
    // 903.9e6 / 32e6 = 28.246875, times 2^19, truncated.
    TEST_ASSERT_EQUAL_UINT32(14809497UL, loramath::frfRegister(903900000UL));

    // Monotonic across the band: a higher frequency must never produce a
    // smaller register value, which is what an overflow would look like.
    TEST_ASSERT_TRUE(loramath::frfRegister(902300000UL) <
                     loramath::frfRegister(927500000UL));
}

void test_low_data_rate_optimize_threshold() {
    // SF11 and SF12 at 125 kHz exceed 16 ms and need it; SF10 does not.
    TEST_ASSERT_FALSE(loramath::needsLowDataRateOptimize(10, 125000));
    TEST_ASSERT_TRUE (loramath::needsLowDataRateOptimize(11, 125000));
    TEST_ASSERT_TRUE (loramath::needsLowDataRateOptimize(12, 125000));

    // Widening the bandwidth shortens the symbol, so SF11 at 250 kHz does not.
    TEST_ASSERT_FALSE(loramath::needsLowDataRateOptimize(11, 250000));

    // SF7 on 125 kHz is about 1 ms, nowhere near the threshold.
    TEST_ASSERT_EQUAL_UINT32(1024, loramath::symbolDurationUs(7, 125000));
}

void test_us915_channel_plan() {
    // 903.9 is channel 8, NOT channel 0. This project's notes said channel 0
    // in three places and were wrong; channel 0 is 902.3 MHz.
    TEST_ASSERT_EQUAL_UINT32(903900000UL, loramath::us915UplinkHz(8));
    TEST_ASSERT_EQUAL_UINT32(902300000UL, loramath::us915UplinkHz(0));
    // SparkFun's device sketch enables channel 17 instead -- 1.8 MHz away
    // from the gateway it ships alongside.
    TEST_ASSERT_EQUAL_UINT32(905700000UL, loramath::us915UplinkHz(17));
    // Sub-band 2, what US networks use: channels 8-15.
    TEST_ASSERT_EQUAL_UINT32(905300000UL, loramath::us915UplinkHz(15));
}

void test_915_is_not_a_lorawan_channel() {
    // Our default is 915.0 MHz, and it is deliberately NOT in the plan --
    // (915.0 - 902.3)/0.2 = 63.5, between channels 63 and 64. Recorded as a
    // test so nobody later "corrects" board.h to a channel number.
    for (uint8_t n = 0; n < 64; ++n) {
        TEST_ASSERT_NOT_EQUAL_UINT32(915000000UL, loramath::us915UplinkHz(n));
    }
    TEST_ASSERT_EQUAL_UINT32(914900000UL, loramath::us915UplinkHz(63));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_bandwidth_index_boundaries);
    RUN_TEST(test_frf_does_not_overflow_32_bit);
    RUN_TEST(test_low_data_rate_optimize_threshold);
    RUN_TEST(test_us915_channel_plan);
    RUN_TEST(test_915_is_not_a_lorawan_channel);
    return UNITY_END();
}
