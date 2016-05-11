#ifndef TestUnitCompression_H
#define TestUnitCompression_H


class TestUnitCompression
{
public:
    TestUnitCompression();
public:
    bool finalResult;

    void testZlib();
    void testLz4();
    void testXZ();
};

#endif // TESTUNITCPP_H
