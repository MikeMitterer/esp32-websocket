//
//

#include <regex>
#include <gtest/gtest.h>

typedef enum { WS_DISCONNECTED, WS_CONNECTED, WS_DISCONNECTING } AwsClientStatus;

TEST(ws_basic, simple_test) {
    EXPECT_TRUE(true);
}

TEST(ws_basic, test_enums) {
    AwsClientStatus status = WS_CONNECTED;
    EXPECT_EQ(status,1);
    EXPECT_EQ(!status,0);
    //EXPECT_TRUE(!status == WS_CONNECTED);

    status = WS_DISCONNECTED;
    EXPECT_EQ(status,0);
    EXPECT_EQ(!status,1);
    EXPECT_TRUE((!status) == WS_CONNECTED);

    status = WS_DISCONNECTING;
    EXPECT_EQ(status,2);
    EXPECT_EQ(!status,0);
    //EXPECT_TRUE(!status == WS_CONNECTED);

}
