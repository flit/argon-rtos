/*
    The licence text below is the boilerplate "MIT Licence" used from:
    http://www.opensource.org/licenses/mit-license.php

    Copyright (c) 2009, Brodie Thiesfield

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is furnished
    to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
    FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
    COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef CPPUNIT_JUnitTestResultCollector_H
#define CPPUNIT_JUnitTestResultCollector_H

#include <time.h>
#include <cppunit/Portability.h>

#if CPPUNIT_NEED_DLL_DECL
#pragma warning( push )
#pragma warning( disable: 4251 4660 )  // X needs to have dll-interface to be used by clients of class Z
#endif

#include <cppunit/TestResultCollector.h>
#include <cppunit/TestSuccessListener.h>
#include <cppunit/portability/CppUnitDeque.h>


CPPUNIT_NS_BEGIN

#if CPPUNIT_NEED_DLL_DECL
//  template class CPPUNIT_API std::deque<TestFailure *>;
//  template class CPPUNIT_API std::deque<Test *>;
#endif


/*! \brief Collects test result.
 * \ingroup WritingTestResult
 * \ingroup BrowsingCollectedTestResult
 * 
 * A JUnitTestResultCollector is a TestListener which collects the results of executing 
 * a test case. It is an instance of the Collecting Parameter pattern.
 *
 * The test framework distinguishes between failures and errors.
 * A failure is anticipated and checked for with assertions. Errors are
 * unanticipated problems signified by exceptions that are not generated
 * by the framework.
 * \see TestListener, TestFailure.
 */
class CPPUNIT_API JUnitTestResultCollector : public TestResultCollector
{
public:
    struct TimingResult {
        Test *  test;
        clock_t start;
        clock_t duration;

        TimingResult(Test * aTest) : test(aTest) {
            start = clock();
            duration = 0;
        }
        TimingResult(const TimingResult& rhs) : test(rhs.test), start(rhs.start), duration(rhs.duration) { }
        bool operator==(const TimingResult& rhs) { return test == rhs.test; }
    };
    typedef CppUnitDeque<TimingResult> TimingResults;


    /*! Constructs a JUnitTestResultCollector object.
    */
    JUnitTestResultCollector( SynchronizationObject *syncObject = 0 );

    /// Destructor.
    virtual ~JUnitTestResultCollector();

    virtual void startTestRun(Test * test, TestResult * eventManager );
    virtual void startTest( Test *test );
    virtual void endTest( Test *test );
    virtual void endTestRun(Test * test, TestResult * eventManager );
    virtual void reset();

    clock_t getTestDuration( Test * test );
    clock_t getAllTestsDuration() const { return m_durationAll; }

protected:
    TimingResults   m_timings;
    clock_t         m_startAll;
    clock_t         m_durationAll;

private:
  /// Prevents the use of the copy constructor.
  JUnitTestResultCollector( const JUnitTestResultCollector &copy );

  /// Prevents the use of the copy operator.
  void operator =( const JUnitTestResultCollector &copy );
};



CPPUNIT_NS_END

#if CPPUNIT_NEED_DLL_DECL
#pragma warning( pop )
#endif


#endif  // CPPUNIT_JUnitTestResultCollector_H
