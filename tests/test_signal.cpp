#include "test_base.hpp"
//#include <sywu/signal.hpp>
#include <sywu/impl/signal_impl.hpp>

template class sywu::Signal<>;
template class sywu::Signal<int>;
template class sywu::Signal<int&>;
template class sywu::Signal<int, std::string>;

class Object1 : public std::enable_shared_from_this<Object1>
{
public:
    void methodWithNoArg()
    {
        ++methodCallCount;
    }

    size_t methodCallCount = 0u;
};

// The application developer can connect a signal to a function.
TEST_F(SignalTest, connectToFunction)
{
    sywu::Signal<> signal;
    auto connection = signal.connect(&function);
    EXPECT_NE(nullptr, connection);

    EXPECT_EQ(1, signal());
    EXPECT_EQ(1u, functionCallCount);
}

// The application developer can connect a signal to a function with arguments.
TEST_F(SignalTest, connectToFunctionWithArgument)
{
    sywu::Signal<int> signal;
    auto connection = signal.connect(&functionWithIntArgument);
    EXPECT_NE(nullptr, connection);

    signal(10);
    EXPECT_EQ(10, intValue);
}
TEST_F(SignalTest, connectToFunctionWithTwoArguments)
{
    sywu::Signal<int, std::string> signal;
    auto connection = signal.connect(&functionWithIntAndStringArgument);
    EXPECT_NE(nullptr, connection);

    signal(15, "alpha");
    EXPECT_EQ(15, intValue);
    EXPECT_EQ("alpha", stringValue);
}

// The application developer can connect a signal to a function with reference arguments.
TEST_F(SignalTest, connectToFunctionWithRefArgument)
{
    sywu::Signal<int&> signal;
    auto connection = signal.connect(&functionWithIntRefArgument);
    EXPECT_NE(nullptr, connection);

    int ivalue = 10;
    signal(std::ref(ivalue));
    EXPECT_EQ(10, intValue);
    EXPECT_EQ(20, ivalue);
}

// The application developer can connect a signal to a method.
TEST_F(SignalTest, connectToMethod)
{
    sywu::Signal<> signal;
    auto object = std::make_shared<Object1>();
    auto connection = signal.connect(object, &Object1::methodWithNoArg);
    EXPECT_NE(nullptr, connection);

    signal();
    EXPECT_EQ(1u, object->methodCallCount);
}

// The application developer can connect a signal to a lambda.
TEST_F(SignalTest, connectToLambda)
{
    sywu::Signal<> signal;
    bool invoked = false;
    auto connection = signal.connect([&invoked]() { invoked = true; });
    EXPECT_NE(nullptr, connection);

    signal();
    EXPECT_TRUE(invoked);
}
\
// The application developer can connect a signal to an other signal.
TEST_F(SignalTest, connectToSignal)
{
    sywu::Signal<> signal1;
    sywu::Signal<> signal2;
    bool invoked = false;

    auto connection = signal1.connect(signal2);
    EXPECT_NE(nullptr, connection);

    signal2.connect([&invoked] () { invoked = true; });
    signal1();
    EXPECT_TRUE(invoked);
}

// When the application developer emits the signal from a slot that is connected to that signal, the signal emit shall not happen
TEST_F(SignalTest, emitSignalThatActivatedTheSlot)
{
    sywu::Signal<> signal;

    auto slot = [&signal]()
    {
        EXPECT_EQ(0u, signal());
    };
    signal.connect(slot);

    EXPECT_EQ(1u, signal());
}

// The application developer can disconnect a connection from a signal through its disconnect function.
TEST_F(SignalTest, disconnectWithConnection)
{
    sywu::Signal<> signal;
    sywu::ConnectionPtr connection;
    auto slot = []()
    {
        if (sywu::SignalConcept::currentConnection)
        {
            sywu::SignalConcept::currentConnection->disconnect();
        }
    };
    connection = signal.connect(slot);
    EXPECT_TRUE(connection->isValid());
    EXPECT_EQ(1u, signal());
    EXPECT_FALSE(connection->isValid());
    EXPECT_EQ(0u, signal());
}

// The application developer can disconnect a connection from a signal using the signal dicsonnect function.
TEST_F(SignalTest, disconnectWithSignal)
{
    using SignalType = sywu::Signal<>;
    SignalType signal;
    sywu::ConnectionPtr connection;
    auto slot = []()
    {
        if (sywu::SignalConcept::currentConnection)
        {
            sywu::SignalConcept::currentConnection->getSender<SignalType>()->disconnect(sywu::SignalConcept::currentConnection);
        }
    };
    connection = signal.connect(slot);
    EXPECT_TRUE(connection->isValid());
    EXPECT_EQ(1u, signal());
    EXPECT_FALSE(connection->isValid());
    EXPECT_EQ(0u, signal());
}

// The application developer can connect the same function multiple times.
TEST_F(SignalTest, connectFunctionManyTimes)
{
    sywu::Signal<> signal;
    signal.connect(&function);
    signal.connect(&function);
    signal.connect(&function);

    EXPECT_EQ(functionCallCount, signal());
}

// The application developer can connect the same method multiple times.
TEST_F(SignalTest, connectMethodManyTimes)
{
    sywu::Signal<> signal;
    auto object = std::make_shared<Object1>();
    signal.connect(object, &Object1::methodWithNoArg);
    signal.connect(object, &Object1::methodWithNoArg);
    signal.connect(object, &Object1::methodWithNoArg);

    EXPECT_EQ(object->methodCallCount, signal());
}

// The application developer can connect the same lambda multiple times.
TEST_F(SignalTest, connectLambdaManyTimes)
{
    sywu::Signal<> signal;
    int invokeCount = 0;
    auto slot = [&invokeCount] { ++invokeCount; };
    signal.connect(slot);
    signal.connect(slot);
    signal.connect(slot);

    EXPECT_EQ(invokeCount, signal());
}

// The application developer can connect the activated signal to a slot from an activated slot.
TEST_F(SignalTest, connectToTheInvokingSignal)
{
    using SignalType = sywu::Signal<>;
    SignalType signal;

    auto slot = []()
    {
        sywu::SignalConcept::currentConnection->getSender<SignalType>()->connect(&function);
    };
    signal.connect(slot);
    EXPECT_EQ(1, signal());
    EXPECT_EQ(2, signal());
    EXPECT_EQ(3, signal());
}

// When a signal is blocked, it shall not activate its connections.
TEST_F(SignalTest, blockSignal)
{
    using SignalType = sywu::Signal<>;
    SignalType signal;
    signal.connect([](){});
    signal.connect([](){});
    signal.connect([](){});

    signal.setBlocked(true);
    EXPECT_EQ(0, signal());
    signal.setBlocked(false);
    EXPECT_EQ(3, signal());
}

// The application developer can block the signal from a slot.
TEST_F(SignalTest, blockSignalFromSlot)
{
    using SignalType = sywu::Signal<>;
    SignalType signal;
    signal.connect([](){});
    signal.connect([](){ sywu::SignalConcept::currentConnection->getSender()->setBlocked(true); });
    signal.connect([](){});

    EXPECT_EQ(2, signal());
    EXPECT_EQ(0, signal());
}

// When the application developer connects the activated signal to a slot from an activated slot,
// that connection is not activated in the same signal activation.
TEST_F(SignalTest, connectionFromSlotGetsActivatedNextTime)
{
    using SignalType = sywu::Signal<>;
    SignalType signal;

    auto slot = []()
    {
        sywu::SignalConcept::currentConnection->getSender<SignalType>()->connect(&function);
    };
    signal.connect(slot);
    EXPECT_EQ(1, signal());
    EXPECT_EQ(0, functionCallCount);

    EXPECT_EQ(2, signal());
    EXPECT_EQ(1, functionCallCount);
}

// When the application developer destroys the object of a method that is a slot of a signal connection,
// the connections in which the object is found are invalidated.
TEST_F(SignalTest, signalsConnectedToAnObjectThatGetsDeleted)
{
    using SignalType = sywu::Signal<>;
    SignalType signal1;
    SignalType signal2;
    SignalType signal3;

    auto object = std::make_shared<Object1>();
    auto connection1 = signal1.connect(object, &Object1::methodWithNoArg);
    auto connection2 = signal2.connect(object, &Object1::methodWithNoArg);
    auto connection3 = signal3.connect(object, &Object1::methodWithNoArg);
    auto objectDeleter = [&object]()
    {
        object.reset();
    };
    signal1.connect(objectDeleter);
    signal1();
    EXPECT_FALSE(connection1->isValid());
    EXPECT_FALSE(connection2->isValid());
    EXPECT_FALSE(connection3->isValid());
}
TEST_F(SignalTest, signalsConnectedToAnObjectThatGetsDeleted_noConenctionHolding)
{
    using SignalType = sywu::Signal<>;
    SignalType signal1;
    SignalType signal2;
    SignalType signal3;

    auto object = std::make_shared<Object1>();
    signal1.connect(object, &Object1::methodWithNoArg);
    signal2.connect(object, &Object1::methodWithNoArg);
    signal3.connect(object, &Object1::methodWithNoArg);
    auto objectDeleter = [&object]()
    {
        object.reset();
    };
    signal1.connect(objectDeleter);
    EXPECT_EQ(2, signal1());
    EXPECT_EQ(0, signal2());
    EXPECT_EQ(0, signal3());
}

// When the signal is destroyed in a slot connected to that signal, it invalidates the connections of the signal.
// The connections following the slot that destroys the signal are not processed.
TEST_F(SignalTest, deleteEmitterSignalFromSlot)
{
    using SignalType = sywu::Signal<>;
    auto signal = std::make_unique<SignalType>();

    auto killSignal = [&signal]()
    {
        signal.reset();
    };
    auto connection1 = signal->connect(killSignal);
    auto connection2 = signal->connect([](){});
    auto connection3 = signal->connect([](){});

    EXPECT_EQ(1, (*signal)());
    EXPECT_EQ(nullptr, signal);
    EXPECT_FALSE(connection1->isValid());
    EXPECT_FALSE(connection2->isValid());
    EXPECT_FALSE(connection3->isValid());
}
