stackTrace <- function() {
    local level = 1
    local output = ""
    local stackInfo = getstackinfos(level)

    while (stackInfo) {
        output += "\n" + stackInfo.func + "() in " + stackInfo.src + ":" + stackInfo.line

        if (level > 0) {
            output += ", "
        }

        level++
        stackInfo = getstackinfos(level)
    }

    return output
}

reporters <- {}

enum Outcome {
    PASSED,
    FAILED,
    SKIPPED
}

local env = getenv("NUTKIN_ENV")

testPattern  <- (vargv.len() > 0 ? vargv[0].tolower() : "")
class SpecBuilder {
    static function skip(specName, spec) {
        SpecBuilder(specName, spec, true)
    }

    static function only(specName, spec) {
        SpecBuilder(specName, spec, false, true)
    }

    constructor(specName, spec, skipped = false, only = false) {
        Spec(specName, spec, suiteStack.top(), skipped, only, testPattern)
    }
}
class Spec {
    name = null
    suite = null
    specBody = null
    skipped = null
    only = null
    _pattern = null

    constructor(specName, spec, parentSuite, isSkipped = false, isOnly = false, testPattern = "") {
        name = specName
        suite = parentSuite
        specBody = spec
        skipped = isSkipped
        only = isOnly
        _pattern = testPattern

        parentSuite.queue(this)

        if (only) {
            parentSuite.markOnly()
        }
    }

    function shouldBeSkipped() {
        return skipped || suite.shouldBeSkipped()
    }

    function isOnly() {
        return only
    }

    function reportTestFailed(reporter, e, stack) {
        reporter.testHasFailed(name, e, stack)
    }

    function run(reporter, onlyMode) {
        if (onlyMode && !only) return []

        if (shouldBeSkipped()) {
            reporter.testSkipped(name)
            return [Outcome.SKIPPED]
        } else {
            reporter.testStarted(name)
            try {
                specBody()
                reporter.testFinished(name)
                return [Outcome.PASSED]
            } catch (e) {
                // When an exception has been caught, the relevant stack has already been unwound, so stackTrace() doesn't help us
                local stack = ""
                if (typeof e == typeof "") {
                    e = Failure(e)
                    stack = "\nStack: " + ::stackTrace()
                }
                reporter.testHasFailed(name, e, stack)
                if (reporter.doNotReportIndividualTestFailures()) {
                    return [Outcome.PASSED]
                } else {
                    return [Outcome.FAILED]
                }
            }
        }
    }
}
class SuiteBuilder {
    static function skip(suiteName, suite) {
        SuiteBuilder(suiteName, suite, true)
    }

    static function only(suiteName, suite) {
        SuiteBuilder(suiteName, suite, false, true)
    }

    constructor(suiteName, suite, skipped = false, only = false) {
        Suite(suiteName, suite, suiteStack.top(), skipped, only, testPattern).parse()
    }
}
class NotApplicableSuite {
    static function skip(suiteName, suite) {
        NotApplicableSuite(suiteName, suite)
    }

    static function only(suiteName, suite) {
        NotApplicableSuite(suiteName, suite)
    }

    constructor(suiteName, suite) {
        reporter.print("Not applicable suite: "+ suiteName)
    }
}
class PredicateSuite {
    describe = null;

    constructor(condition) {
        if(condition) {
            describe = SuiteBuilder
        } else {
            describe = NotApplicableSuite
        }
    }
}
class Suite {
    name = null
    suiteBody = null
    parent = null
    beforeAllFunc = null
    afterAllFunc = null
    beforeEachFunc = null
    afterEachFunc = null
    skipped = null
    only = null
    _pattern = null
    runQueue = null
    it = SpecBuilder
    describe = SuiteBuilder
    onlyIf = PredicateSuite

    constructor(suiteName, suite, parentSuite = null, isSkipped = false, isOnly = false, pattern = "") {
        name = suiteName
        suiteBody = suite
        parent = parentSuite
        skipped = isSkipped
        only = isOnly
        _pattern = pattern
        runQueue = []

        if (parentSuite) {
            parentSuite.queue(this)
        }

        if (only) {
            markOnly()
        }
    }

    function beforeAll(beforeAllImpl) {
        if (typeof beforeAllImpl != "function") {
            throw "before() takes a function argument"
        }

        beforeAllFunc = beforeAllImpl
    }

    function afterAll(afterAllImpl) {
        if (typeof afterAllImpl != "function") {
            throw "after() takes a function argument"
        }

        afterAllFunc = afterAllImpl
    }

    function beforeEach(beforeEachImpl) {
        if (typeof beforeEachImpl != "function") {
            throw "beforeEach() takes a function argument"
        }

        beforeEachFunc = beforeEachImpl
    }

    function afterEach(afterEachImpl) {
        if (typeof afterEachImpl != "function") {
            throw "afterEach() takes a function argument"
        }

        afterEachFunc = afterEachImpl
    }

    function shouldBeSkipped() {
        return skipped || (parent ? parent.shouldBeSkipped() : false)
    }

    function markOnly() {
        only = true
        if (parent) {
            parent.markOnly()
        }
    }

    function autoQ(runnable) {
        runQueue.push(runnable)
    }

    function queue(runnable) {
        if (runnable.name.tolower().find(_pattern) != null || name.tolower().find(_pattern) != null) {
            runQueue.push(runnable)

            if (parent && parent.runQueue.find(this) == null) {
                parent.autoQ(this)
            }
        }
    }

    function parse() {
        suiteStack.push(this)
        suiteBody()
        suiteStack.pop()
    }

    function hasAnOnlyDescendant() {
        foreach(runnable in runQueue) {
            if (runnable.only) {
                return true
            }
        }
        return false
    }

    function runBeforeAlls() {
        if (beforeAllFunc) {
            beforeAllFunc()
        }
    }

    function runAfterAlls() {
        if (afterAllFunc) {
            afterAllFunc()
        }
    }

    function runBeforeEaches() {
        if (parent) {
            parent.runBeforeEaches()
        }

        if (beforeEachFunc) {
            beforeEachFunc()
        }
    }

    function runAfterEaches() {
        if (afterEachFunc) {
            afterEachFunc()
        }

        if (parent) {
            parent.runAfterEaches()
        }
    }

    function reportTestFailed(reporter, e, stack) {
        foreach(runnable in runQueue) {
            runnable.reportTestFailed(reporter, e, stack)
        }
    }

    function run(reporter, onlyMode) {
        if (onlyMode && !only) {
            return []
        }

        local explicitOnlyInChild = hasAnOnlyDescendant()
        local outcomes = []
        local suiteError = null
        local suiteStack = null

        reporter.suiteStarted(name)

        try {
            runBeforeAlls()

            foreach(runnable in runQueue) {
                try {
                    runBeforeEaches()
                    outcomes.extend(runnable.run(reporter, explicitOnlyInChild ? only : false))
                    runAfterEaches()
                } catch (e) {
                    suiteError = e
                    suiteStack = "\nStack: " + ::stackTrace()
                    runnable.reportTestFailed(reporter, e, suiteStack)
                }
            }

            runAfterAlls()
            reporter.suiteFinished(name)
        } catch (e) {
            suiteError = e
            suiteStack = "\nStack: " + ::stackTrace()
        }

        if (suiteError != null) {
            reporter.suiteErrorDetected(name, suiteError, suiteStack)
        }

        return outcomes
    }
}

class Printer {

    function prettyPrint(object) {
        //prettyPrinter.print(object);
    }

    function print(text) {
        ::print(text)
    }

    function println(text) {
        print(text + "\n")
    }
}
class Failure {
    message = ""
    description = ""

    constructor(failureMessage, descriptionMessage = "") {
        message = failureMessage
        description = descriptionMessage
    }

    _tostring = function() {
        local descriptionPart = ""
        if (description != "") {
            descriptionPart = ": " + description
        }
        return message + descriptionPart
    }
}
class Reporter {
    printer = null
    testFailures = null
    suiteErrors = null

    constructor(printerImpl = Printer()) {
        printer = printerImpl
        testFailures = []
        suiteErrors = []
    }

    function isFailure(thing) {
        return thing instanceof Failure
    }

    function safeGetErrorMessage(thing) {
        local message = thing
        if (isFailure(thing)) {
            message = thing.message
        }
        return message
    }

    function safeGetErrorDescription(thing) {
        local description = ""
        if (isFailure(thing)) {
            description = thing.description
        }
        return description
    }

    function print(message) {
        printer.println(message)
    }

    function testHasFailed(name, failure, stack = "") {
        testFailed(name, failure, stack)

        if (!doNotReportIndividualTestFailures()) {
            testFailures.append(name)
        }
    }

    function suiteErrorDetected(name, error, stack = "") {
        suiteErrors.append(name)
        suiteError(name, error, stack)
    }

    // Subclasses to implement these methods
    function suiteStarted(name) {}
    function suiteFinished(name) {}
    function suiteError(name, error, stack = "")
    function testStarted(name) {}
    function testFinished(name) {}
    function testFailed(name, failure, stack = "") {}
    function doNotReportIndividualTestFailures() { return false }
    function testSkipped(name) {}
    function begin() {}
    function end(passed, failed, skipped, timeTaken) {}
    function listFailures() {}
}
class TeamCityReporter extends Reporter {

    function escapeText(text) {
        // Team city needs messages to be escaped
        // escaped_value uses substitutions: '->|', [->|[, ]->|], |->||, newline->|n

        local subs = {
            "'": "|'",
            "[": "|[",
            "]": "|[",
            "|": "||",
            "\n": "|n"
        }

        local escaped = "";
        for (local index = 0; index < text.len(); index++) {
            // Can't use the char value as its literally a uint8 - not so good as a string
            local substr = text.slice(index, index + 1);
            if (substr in subs) {
                escaped += subs[substr];
            } else {
                escaped += substr;
            }
        }
        return escaped;
    }

    function suiteStarted(name) {
        print("##teamcity[testSuiteStarted name='" + escapeText(name) + "']")
    }

    function suiteFinished(name) {
        print("##teamcity[testSuiteFinished name='" + escapeText(name) + "']")
    }

    function suiteError(name, error, stack = "") {
        print("##teamcity[message text='suiteError: " + escapeText(name) + "' status='ERROR' errorDetails='" + escapeText(name) + "']")
        suiteFinished(name)
    }

    function testStarted(name) {
        print("##teamcity[testStarted name='" + escapeText(name) + "'] captureStandardOutput='true'")
    }

    function testFinished(name) {
        print("##teamcity[testFinished name='" + escapeText(name) + "']")
    }

    function testFailed(name, failure, stack = "") {
        print("##teamcity[testFailed name='" + escapeText(name) + "' message='" + escapeText(safeGetErrorMessage(failure)) + "' details='" + escapeText(safeGetErrorDescription(failure)) + "']")
        testFinished(name)
    }

    function doNotReportIndividualTestFailures() {
        return true
    }

    function testSkipped(name) {
        print("##teamcity[testIgnored name='" + escapeText(name) + "']")
    }
}

reporters["TEAM_CITY"] <- TeamCityReporter()
class ConsoleReporter extends Reporter {
    indent = 0
    bold = ""//"\x1B[1m"
    titleColour = ""//"\x1B[34m"
    testColour = ""//"\x1B[38;5;240m"
    passColour = ""//"\x1B[32m"
    failColour = ""//"\x1B[31m"
    skipColour = ""//"\x1B[33m"
    errorColour = ""//"\x1B[1;31m"
    logColour = ""//"\x1B[36m"
    reset = ""//"\x1B[0m"

    function padding() {
        local output = ""
        for (local i=0;i<indent;i+=1){
            output += "  "
        }
        return output
    }

    function print(message) {
        printer.println(padding() + message + reset + logColour)
    }

    function suiteStarted(name) {
        indent++
        print(titleColour + name)
    }

    function suiteFinished(name) {
        indent--
        print("")
    }

    function suiteError(name, error, stack = "") {
        print(failColour + bold + error)

        if(stack != "") {
            print(failColour + stack)
        }

        suiteFinished(name)
    }

    function testStarted(name) {
        indent++
    }

    function testFinished(name) {
        print(passColour + "✓ " + testColour + name)
        indent--
    }

    function testFailed(name, failure, stack = "") {
        print(failColour + "✗ " + name)
        print(failColour + bold + safeGetErrorMessage(failure))
        if (stack != "") {
            print(failColour + stack)
        }
        local desc = safeGetErrorDescription(failure)
        if (desc != "") {
            print(skipColour + desc)
        }
        indent--
    }

    function testSkipped(name) {
        indent++
        print(skipColour + "➾ " + name)
        indent--
    }

    function begin() {
        printer.println("")
    }

    function end(passed, failed, skipped, timeTaken) {
        indent++
        print(passColour + passed + " passing")
        if (failed > 0) {
            print(failColour + failed + " failing")
        }
        if (skipped > 0) {
            print(skipColour + skipped + " skipped")
        }
        printer.println(reset)
        listFailures()
        print("Took " + timeTaken)
        printer.print(reset)
        indent--
    }

    function listFailures() {
        if (testFailures.len() > 0) {
            print(failColour + "TEST FAILURES:")
            indent++
            foreach (failure in testFailures) {
                print(failColour + "✗ " + failure)
            }
            printer.println(reset)
            indent--
        }

        if(suiteErrors.len() > 0) {
            print(failColour + "SUITE ERRORS:")
            indent++
            foreach (failure in suiteErrors) {
                print(failColour + "✗ " + failure)
            }
            printer.println(reset)
            indent--
        }
    }
}

class Matcher {

    expected = null
    description = null

    constructor(expectedVal = null, matchDescription = "") {
        expected = expectedVal
        description = matchDescription
    }

    function isTable(x) {
        return typeof x == typeof {}
    }

    function isArray(x) {
        return typeof x == typeof []
    }

    function isString(x) {
        return typeof x == typeof ""
    }

    function prettify(x) {
        if (isArray(x) || isTable(x)) {
           return NutkinPrettyPrinter().format(x);
        } else if (x == null) {
            return "(null)"
        } else if (isString(x)) {
            return "'" + x + "'"
        } else {
            return x
        }
    }

    function arraysEqual(a, b) {
        if (a.len() == b.len()) {
            foreach (i, value in a) {
                if (!equal(value, b[i])) {
                    return false
                }
            }
            return true
        } else {
            return false
        }
    }

    function tablesEqual(a, b) {
        if (a.len() == b.len()) {
            foreach (key, value in a) {
                if (!(key in b)) {
                    return false
                } else if (!equal(value, b[key])) {
                    return false
                }
            }
            return true
        } else {
            return false
        }
    }

    function equal(a, b) {
        if (isArray(a) && isArray(b)) {
            return arraysEqual(a, b)
        } else if (isTable(a) && isTable(b)) {
            return tablesEqual(a, b)
        } else {
            return a == b
        }
    }

    function contains(things, value) {
        if (isTable(value)) {
            local matcher = function(index, item) {
                return equal(value, item)
            }
            local filtered = things.filter(matcher.bindenv(this))

            return filtered.len() > 0
        }

        return things.find(value) != null
    }

    function test(actual) {
        return false
    }

    function negateIfRequired(text, isNegated) {
        local negatedText = isNegated ? "not " : ""
        return negatedText + text
    }

    function failureMessage(actual, isNegated) {
        return "Expected " + prettify(actual) + negateIfRequired(" to be " + prettify(expected), isNegated)
    }
}

class EqualsMatcher extends Matcher {

    function test(actual) {
        return equal(actual, expected)
    }

    function failureMessage(actual, isNegated) {
        return "Expected " + prettify(actual) + negateIfRequired(" to equal " + prettify(expected), isNegated)
    }
}

class TruthyMatcher extends Matcher {

    function test(actual) {
        return actual
    }

    function failureMessage(actual, isNegated) {
        return "Expected " + prettify(actual) + negateIfRequired(" to be truthy", isNegated)
    }
}

class FalsyMatcher extends Matcher {

    function test(actual) {
        return !actual
    }

    function failureMessage(actual, isNegated) {
        return "Expected " + prettify(actual) + negateIfRequired(" to be falsy", isNegated)
    }
}

class ContainsMatcher extends Matcher {

    function test(actual) {
        return contains(actual, expected)
    }

    function failureMessage(actual, isNegated) {
        return "Expected " + prettify(actual) + negateIfRequired(" to contain " + prettify(expected), isNegated)
    }
}

class RegexpMatcher extends Matcher {

    function test(actual) {
        return regexp(expected).match(actual)
    }

    function failureMessage(actual, isNegated) {
        return "Expected " + prettify(actual) + negateIfRequired(" to match: " + expected, isNegated)
    }
}

class LessThanMatcher extends Matcher {

    function test(actual) {
        return actual < expected
    }

    function failureMessage(actual, isNegated) {
        return "Expected " + prettify(actual) + negateIfRequired(" to be less than " + prettify(expected), isNegated)
    }
}

class GreaterThanMatcher extends Matcher {

    function test(actual) {
        return actual > expected
    }

    function failureMessage(actual, isNegated) {
        return "Expected " + prettify(actual) + negateIfRequired(" to be greater than " + prettify(expected), isNegated)
    }
}

class CloseToMatcher extends Matcher {

    precision = null

    constructor(expectedVal = null, precisionInDps = 1, matchDescription = "") {
        base.constructor(expectedVal, matchDescription)
        precision = precisionInDps
    }

    function round(val, decimalPoints) {
        local f = pow(10, decimalPoints) * 1.0;
        local newVal = val * f;
        newVal = (val >= 0) ? floor(newVal) : ceil(newVal)
        return newVal;
    }

    function test(actual) {
        return round(expected, precision) == round(actual, precision)
    }

    function failureMessage(actual, isNegated) {
        return "Expected " + prettify(actual) + negateIfRequired(" to be close to " + expected, isNegated)
    }
}

class TypeMatcher extends Matcher {

    function test(actual) {
        return typeof actual == expected
    }

    function failureMessage(actual, isNegated) {
        return "Expected " + prettify(actual) + negateIfRequired(" to be of type " + expected, isNegated)
    }
}

class InstanceOfMatcher extends Matcher {

    function test(actual) {
        return actual instanceof expected
    }

    function failureMessage(actual, isNegated) {
        return "Expected instance" + negateIfRequired(" to be an instance of specified class", isNegated)
    }
}

class NumberMatcher extends Matcher {

    function test(actual) {
        return typeof actual == "integer" || typeof actual == "float"
    }

    function failureMessage(actual, isNegated) {
        return "Expected " + prettify(actual) + negateIfRequired(" to be a number", isNegated)
    }
}

class ThrowsMatcher extends Matcher {

    function test(actual) {
        try {
            actual()
        } catch (error) {
            if (error == expected) {
                return true
            }
        }
        return false
    }

    function failureMessage(actual, isNegated) {
        if (isNegated) {
            return "Expected no exception but caught " + error
        }
        return "Expected " + expected + " but caught " + error
    }
}


/// This class implements a Matcher which compares the contents of two tables or arrays
/// It considers them to match if the contents are the same, regardless of ordering of the arrays
/// This is useful if you're generating arrays but don't care about the order (eg if they're generated from a foreach over a table)
class UnorderedObjectMatcher extends Matcher {

    _sortedExpected = null;
    _sortedActual = null;

    /// Sorts an object (array or table) recursively depth first based on the JSON representation of the members of the array
    /// \param obj The object (array or table) to be sorted. Any other type is ignored.
    function _sortObject(obj) {
        if (typeof(obj) == "table")
        {
            foreach (key, value in obj) {
                _sortObject(value);
            }
        } else if (typeof(obj) == "array") {
            foreach (entry in obj) {
                _sortObject(entry);
            }
            obj.sort( function( first, second ) {
                // We need to sort potentially any kind of data
                // So we convert it to JSON and then sort
                local jsonFirst = NutkinPrettyPrinter().format(first);
                local jsonSecond = NutkinPrettyPrinter().format(second);

                // Very crude - loop through the first JSON looking for differences with the second JSON
                // This algorithm just needs to be consistent, not sorted by any particular criteria
                foreach (index, ch in jsonFirst) {
                    if ((jsonSecond.len() > index) && (ch != jsonSecond[index])) {
                        return (ch <=> jsonSecond[index]);
                    }
                }

                // Here because they're the same
                return 0;

            }.bindenv(this));

        }
    }

    /// Performs a deep clone of a table or array container.
    /// \warning Container must not have circular references.
    /// \param  container the container/structure to deep clone.
    /// \return the cloned container.
    function _deepClone( container )
    {
        switch( typeof( container ) )
        {
            case "table":
                local result = clone container;
                foreach( k, v in container ) result[k] = _deepClone( v );
                return result;
            case "array":
                return container.map( _deepClone.bindenv(this) );
            default: return container;
        }
    }

    function test(actual) {
        _sortedExpected = _deepClone(expected);
        _sortObject(_sortedExpected);

        _sortedActual = _deepClone(actual);
        _sortObject(_sortedActual);

        return equal(_sortedActual, _sortedExpected);

    }

    function failureMessage(actual, isNegated) {
        return "Expected " + prettify(_sortedActual) + negateIfRequired(" to equal " + prettify(_sortedExpected), isNegated);
    }
}


class MockCallCountMatcher extends Matcher {
    function test(actual) {
        return (actual.hasCallCount(expected) == null);
    }

    function failureMessage(actual, isNegated) {
        if (isNegated)
        {
            return "Failed as function DID have " + expected + " calls";
        }

        return actual.hasCallCount(expected);
    }

}

class MockCalledWithAtIndexMatcher extends Matcher {
    _index = null;
    _argArray = null;

    constructor(index, argArray)
    {
        _index = index;
        _argArray = argArray;

        // As we store the 2 expected values in this class, we don't need to pass them to the constructor
        base.constructor();
    }
    
    function test(actual) {
        return (actual.wasCalledWithAtIndex(_index, _argArray) == null);
    }

    function failureMessage(actual, isNegated) {
        if (isNegated)
        {
            local res = "Failed as function WAS called at index " + _index + " with args";
            foreach (arg in _argArray)
            {
                res += " " + arg;
            }
            return res;
        }

        return actual.wasCalledWithAtIndex(_index, _argArray);
    }
}

class MockLastCalledMatcher extends Matcher {
    
    function _makeCall(actual)
    {
        // Put the reference to the MockFunction into the first arg (essentially bindenv for acall)
        local expContext = clone expected;
        expContext.insert(0, actual);

        return actual.wasLastCalledWith.acall(expContext);
    }

    function test(actual) {
        return (_makeCall(actual) == null);
    }

    function failureMessage(actual, isNegated) {
        if (isNegated) {
            local err = "Last function call WAS with args: ";
            foreach (arg in expected)
            {
                err += arg + ", ";
            }
            return err;
        }
        return _makeCall(actual);
    }
}


class MockAnyCallMatcher extends Matcher {
    function _makeCall(actual)
    {
        // Put the reference to the MockFunction into the first arg (essentially bindenv for acall)
        local expContext = clone expected;
        expContext.insert(0, actual);

        return actual.anyCallWith.acall(expContext);
    }

    function test(actual) {
        return (_makeCall(actual) == null);
    }

    function failureMessage(actual, isNegated) {
        if (isNegated) {
            local err = "There WAS a call with args: ";
            foreach (arg in expected)
            {
                err += arg + ", ";
            }
            return err;
        }
        return _makeCall(actual);
    }
}
class Expectation {
    actual = null
    to = null
    be = null
    been = null
    a = null
    has = null
    have = null
    with = null
    that = null
    which = null
    and = null
    of = null
    is = null

    constructor(actualValue) {
        actual = actualValue
        to = this
        be = this
        been = this
        a = this
        has = this
        have = this
        with = this
        that = this
        which = this
        and = this
        of = this
        is = this

    }

    function isString(x) {
        return typeof x == typeof ""
    }

    function execMatcher(matcher) {
        if (!matcher.test(actual)) {
            throw Failure(matcher.failureMessage(actual, false), matcher.description)
        }
    }

    function equal(expected, description = "") {
        return execMatcher(EqualsMatcher(expected, description))
    }

    function truthy(description = "") {
        return execMatcher(TruthyMatcher(null, description))
    }

    function falsy(description = "") {
        return execMatcher(FalsyMatcher(null, description))
    }

    function contain(expected, description = "") {
        return execMatcher(ContainsMatcher(expected, description))
    }

    function contains(value, description = "") {
        return contain(value, description)
    }

    function match(expression, description = "") {
        return execMatcher(RegexpMatcher(expression, description))
    }

    function matches(expression, description = "") {
        return match(expression, description)
    }

    function lessThan(value, description = "") {
        return execMatcher(LessThanMatcher(value, description))
    }

    function greaterThan(value, description = "") {
        return execMatcher(GreaterThanMatcher(value, description))
    }

    function throws(exception, description = "") {
        return execMatcher(ThrowsMatcher(exception, description))
    }

    function toThrow(expected, description = "") {
        return throws(expected, description)
    }

    function number(description = "") {
        return execMatcher(NumberMatcher(null, description))
    }

    function ofType(type, description = "") {
        return execMatcher(TypeMatcher(type, description))
    }

    function ofClass(clazz, description = "") {
        return execMatcher(InstanceOfMatcher(clazz, description))
    }

    function closeTo(value, precision = 1, description = "") {
        return execMatcher(CloseToMatcher(value, precision, description))
    }

    function equalUnordered(value, description = "") {
        return execMatcher(UnorderedObjectMatcher(value, description));
    }

    function beCloseTo(value, precision = 1, description = "") {
        return closeTo(value, precision, description)
    }



    function toBe(expectedOrMatcher, description = "") {
        if (typeof expectedOrMatcher == "instance") {
            // Custom matcher
            return execMatcher(expectedOrMatcher)
        } else {
            // SquirrelJasmine compatability function
            return equal(expectedOrMatcher, description)
        }
    }

    function callCount(value, description = "")
    {
        execMatcher(MockCallCountMatcher(value, description));
        // We return this so we can chain this matcher
        return this;
    }

    function calledWithAtIndex(index, argArray)
    {
        return execMatcher(MockCalledWithAtIndexMatcher(index, argArray));
    }

    function lastCalledWith(...)
    {
        return execMatcher(MockLastCalledMatcher(vargv));
    }

    function anyCallWith(...)
    {
        return execMatcher(MockAnyCallMatcher(vargv));
    }

    // SquirrelJasmine compatability functions

    function toBeTruthy() {
        return truthy()
    }

    function toBeFalsy() {
        return falsy()
    }

    function toBeNull() {
        return equal(null)
    }

    function toContain(value) {
        return contain(value)
    }

    function toMatch(regex) {
        return match(regex)
    }

    function toBeLessThan(value) {
        return lessThan(value)
    }

    function toBeGreaterThan(value) {
        return greaterThan(value)
    }

    function toBeCloseTo(value, precision = 1) {
        return closeTo(value, precision)
    }

    function toBeEqualUnordered(value, description = "") {
        return equalUnordered(value, description);
    }

    function toHaveCallCount(value, description = "")
    {
        return callCount(value, description);
    }

    function toBeCalledWithAtIndex(index, argArray)
    {
        return calledWithAtIndex(index, argArray);
    }

    function toBeLastCalledWith(...)
    {
        return execMatcher(MockLastCalledMatcher(vargv));
    }

    function toHaveAnyCallWith(...)
    {
        return execMatcher(MockAnyCallMatcher(vargv));
    }

}

class NegatedExpectation extends Expectation {

    function execMatcher(matcher) {
        if (matcher.test(actual)) {
            throw Failure(matcher.failureMessage(actual, true), matcher.description)
        }
    }
}

class expect extends Expectation {
    not = null

    constructor(expectedValue) {
        base.constructor(expectedValue)
        not = NegatedExpectation(expectedValue)
    }
}

// SquirrelJasmine compatability function
function expectException(exception, func) {
    return expect(func).throws(exception)
}


/*

Squirrel Mocking Class
Author: Leo Rampen
Date: 20/04/2020

This class provides a generic, stable interface to build mocks with. It allows the logic of a mocks behaviour to be defined
as part of a unit test.

The functionality is broadly based on the Python unittest.Mock class. https://docs.python.org/3/library/unittest.mock.html#unittest.mock.Mock.

Interface names have been converted to match the Electric Imp Style Guide - generally the snake_case python names have been converted to camelCase. Not all features are implemented, or implemented fully.

The unit tests should give an idea of the spec this class was built against.

*/

class MockFunction {
    // The parent Mock type which created this
    _parentMock = null;

    // The function name called to create us
    _name = null;

    // A list of all the variables which have been added (with the <- operator)
    _attributes = null;

    _callCount = 0;
    _callArgs = null;
    _callMock = null;

    constructor(parentMock = null, name = null)
    {
        _parentMock = parentMock;
        _name = name;

        _resetMockFunction();
    }

    // _set and _newslot are just used to record values in this class
    function _newslot(key, value)
    {
        _attributes[key] <- value;
    }   

    function _set(key, val)
    {
        _attributes[key] = val;
    }

    function arraysEqual(a, b) {
        if (a.len() == b.len()) {
            foreach (i, value in a) {
                if (!equal(value, b[i])) {
                    return false
                }
            }
            return true
        } else {
            return false
        }
    }

    function tablesEqual(a, b) {
        if (a.len() == b.len()) {
            foreach (key, value in a) {
                if (!(key in b)) {
                    return false
                } else if (!equal(value, b[key])) {
                    return false
                }
            }
            return true
        } else {
            return false
        }
    }

    function equal(a, b) {
        if ((typeof(a) == "array") && (typeof(b) == "array")) {
            return arraysEqual(a, b)
        } else if ((typeof(a) == "table") && (typeof(b) == "table")) {
            return tablesEqual(a, b)
        } else {
            return a == b
        }
    }

    function bindenv( unused )
    {
        return this;
    }

    function _resetMockFunction()
    {
        _callCount = 0;
        _attributes = {};
        _callArgs = [];

        // We don't always return a mock (if the user sets a return_value, we return something else)
        // But we pre-emptively create it anyway
        _callMock = Mock();
    }

    // Check for a call with the defined count
    function _hasCallCount(callCount)
    {
        // Helper function to collate common arguments
        if (_callCount != callCount)
        {
            return "Call count doesn't match. Expected: " + callCount + ", Actual: " + _callCount;
        }

        return null;
    }

    function _wasCalledWithAtIndex(callToCheck, callArgs = [])
    {
        if (callToCheck >= _callCount)
        {
            return "Call to check doesn't exist. Checking " + callToCheck + " but only " + _callCount + " calls.";
        }

        // Check that each argument is present
        foreach (index, arg in callArgs)
        {
            if (_callArgs[callToCheck].len() <= index)
            {
                return "Argument " + index + " expected: " + arg + " but no argument given";
            }

            local actual = _callArgs[callToCheck][index];

            // Type check or value check
            if (arg instanceof Mock.Type)
            {
                local comparison = arg.compare(actual);
                if (comparison != null)
                {
                    return comparison;
                }
            } 
            else if (arg instanceof Mock.Ignore)
            {
                // an arg of Mock.Ignore should be ignored - continue to next argument
                continue;
            } 
            else if (arg instanceof Mock.Instance)
            {
                local comparison = arg.compare(actual);
                if (comparison != null)
                {
                    return comparison;
                }
            }
            else
            {
                if (!equal(actual, arg))
                {
                    return "Argument " + index + " expected: " + arg + " but actual was " + actual;
                }
            }
        }

        // And then check that there's no additional arguments
        if (_callArgs[callToCheck].len() > callArgs.len())
        {
            return "Function called with additional arguments. Expected: " + callArgs.len() + " but got: " + _callArgs[callToCheck].len() + " arguments";
        }

        return null;

    } 

    function _anyCallWith(...)
    {
        local callFound = false;

        // Check each call made in turn looking for this call
        for (local i = 0; i < _callCount; i++)
        {
            local result = _wasCalledWithAtIndex(i, vargv);

            // A null result means the call was matched
            if (result == null)
            {
                callFound = true;
                break;
            }
        }

        if (callFound)
        {
            return null;
        } else {
            return "Call not found";
        }
    }

    // Check to see if this function was last called with the provided arguments
    function _wasLastCalledWith(...)
    {
        if (_callArgs.len() > 0)
        {
            return _wasCalledWithAtIndex(_callArgs.len() - 1, vargv);
        } else {
            return "Was not called";
        }
    }

    function _get(key)
    {
        // Special cases to access details of the mock calls
        switch (key)
        {
            case "resetMockFunction":
                return _resetMockFunction;
            case "called":
                return (_callCount > 0);
            case "callCount":
                return _callCount;
            case "callArgs":
                if ((_callArgs != null) && (_callArgs.len() > 0))
                {
                    return _callArgs.top();
                } else {
                    return null;
                }
            case "callArgsList":
                return _callArgs;
            case "returnValue":
                if ("returnValue" in _attributes)
                {
                    return _attributes.returnValue;
                } else {
                    return _callMock;
                }
            case "hasCallCount":
                return _hasCallCount;
            case "wasLastCalledWith":
                return _wasLastCalledWith;
            case "anyCallWith":
                return _anyCallWith;
            case "wasCalledWithAtIndex":
                return _wasCalledWithAtIndex;
        }

        // If the user has set this attribute we'll return it (otherwise we have nothing)
        if (key in _attributes)
        {
            return _attributes[key];
        } else {
            throw null;
        }
    }

    function acall( arguments )
    {
        ++_callCount;

        local newCallArgs = arguments.slice( 1, arguments.len() );

        // Save the callers arguments for analysis later
        _callArgs.append( _deepClone(newCallArgs) );

        // Check if there's a sideEffect function or array
        if ("sideEffect" in _attributes)
        {
            if (typeof(_attributes.sideEffect) == "function")
            {
                // Call the function using acall to pass the original argument array as its arguments
                local result = _attributes.sideEffect.acall(arguments);

                // The tester can return DefaultReturn to ignore this sideEffect function
                if (!(result instanceof MockFunction.DefaultReturn))
                {
                    return result;
                }
            } 
            else if (typeof(_attributes.sideEffect) == "array")
            {
                // If sideEffect is an array, return each variable in turn
                if (_callCount <= _attributes.sideEffect.len())
                {
                    return _attributes.sideEffect[_callCount - 1];
                } else {
                    // Saturate at the last member of the array for all subsequent calls
                    return _attributes.sideEffect.top();
                }

            }
        }

        // If we've had a return value set, we return that
        if ("returnValue" in _attributes)
        {
            return _attributes.returnValue;
        }
        
        // If nothing else was set, we return a new Mock class
        return _callMock;
    }

    // Argument zero is documented as "original_this" in the squirrel2 docs, but not in squirrel3
    // There's definitely something there... Whatever it is we don't need it :)
    function _call(originalThis, ...)
    {
        local arguments = [originalThis];
        arguments.extend( vargv );
        return acall( arguments );
    }

    /// Performs a deep clone of a table or array container.
    /// \warning Container must not have circular references.
    /// \param  container the container/structure to deep clone.
    /// \return the cloned container.
    function _deepClone( container )
    {
        switch( typeof( container ) )
        {
            case "table":
                local result = clone container;
                foreach( k, v in container ) result[k] = _deepClone( v );
                return result;
            case "array":
                return container.map( _deepClone.bindenv(this) );
            default: return container;
        }
    }
} 

class MockFunction.DefaultReturn {
    // A default return type for sideEffect functions
}


class Mock {  
    // A list of all the variables which have been added (with the <- operator)
    _attributes = null;

    // A list of all the mocks we returned
    _calls = null;


    constructor(mockType = null)
    {
        _attributes = {};
        _calls = {};
    }

    function _call(...)
    {
    }

    function bindenv( unused )
    {
        return this;
    }

    function resetMock()
    {
        // Clear everything
        _attributes = {};
        _calls = {};

        // TODO: We're just clearing the lists, which means if someone took a reference to a call
        // it might still be hanging about. Should we reset all referenced calls as well?
    }


    // Store any data which is assigned
    function _newslot(key, value)
    {
       _attributes[key] <- value;
    }   

    function _set(key, value)
    {
        _attributes[key] = value;
    }

    function _get(key)
    {
        // Check if this slot has been created and if it has, return the value
        if (key in _attributes)
        {
            return _attributes[key];
        }

        // If we don't whitelist all external functions/objects/references
        // we end up with recursion. Uhoh. If you have an infinite loop or stack overflow, check this
        if ((key == "print") || (key=="Mock") || (key=="MockFunction"))
        {
            throw null;
        }

        if (key in _calls)
        {
            return _calls[key];
        }

        local newReturn = MockFunction(this, key);
        _calls[key] <- newReturn;
        return newReturn;
    }
}

class Mock.Type
{    
    _type = null;
    
    constructor(type)
    {
        local allowedTypes = ["bool", "string", "integer", "float", "blob", "instance", "array", "table", "function"];

        if (allowedTypes.find(type) == null)
        {
            throw("Type " + type + " not valid");
        }

        _type = type;
    }

    function compare(var)
    {
        if (typeof(var) != _type)
        {
            return "Var " + var + " is not of type " + _type;
        }

        return null;
    }
}

class Mock.Instance
{
    _classType = null;

    constructor(classType)
    {
        _classType = classType;
    }

    function compare(var)
    {
        if (var instanceof _classType)
        {
            return null;
        } else {
            return "Var " + var + " is not an instance of " + _classType;
        }
    }
}

class Mock.Ignore
{

}



/**
 * JSON Parser
 *
 * @author Mikhail Yurasov <mikhail@electricimp.com>
 * @package JSONParser
 * @version 1.0.1
 */

/**
 * JSON Parser
 * @package JSONParser
 */
class JSONParser {

  // should be the same for all components within JSONParser package
  static version = "1.0.1";

  /**
   * Parse JSON string into data structure
   *
   * @param {string} str
   * @param {function({string} value[, "number"|"string"])|null} converter
   * @return {*}
   */
  function parse(str, converter = null) {

    local state;
    local stack = []
    local container;
    local key;
    local value;

    // actions for string tokens
    local string = {
      go = function () {
        state = "ok";
      },
      firstokey = function () {
        key = value;
        state = "colon";
      },
      okey = function () {
        key = value;
        state = "colon";
      },
      ovalue = function () {
        value = this._convert(value, "string", converter);
        state = "ocomma";
      }.bindenv(this),
      firstavalue = function () {
        value = this._convert(value, "string", converter);
        state = "acomma";
      }.bindenv(this),
      avalue = function () {
        value = this._convert(value, "string", converter);
        state = "acomma";
      }.bindenv(this)
    };

    // the actions for number tokens
    local number = {
      go = function () {
        state = "ok";
      },
      ovalue = function () {
        value = this._convert(value, "number", converter);
        state = "ocomma";
      }.bindenv(this),
      firstavalue = function () {
        value = this._convert(value, "number", converter);
        state = "acomma";
      }.bindenv(this),
      avalue = function () {
        value = this._convert(value, "number", converter);
        state = "acomma";
      }.bindenv(this)
    };

    // action table
    // describes where the state machine will go from each given state
    local action = {

      "{": {
        go = function () {
          stack.push({state = "ok"});
          container = {};
          state = "firstokey";
        },
        ovalue = function () {
          stack.push({container = container, state = "ocomma", key = key});
          container = {};
          state = "firstokey";
        },
        firstavalue = function () {
          stack.push({container = container, state = "acomma"});
          container = {};
          state = "firstokey";
        },
        avalue = function () {
          stack.push({container = container, state = "acomma"});
          container = {};
          state = "firstokey";
        }
      },

      "}" : {
        firstokey = function () {
          local pop = stack.pop();
          value = container;
          container = ("container" in pop) ? pop.container : null;
          key = ("key" in pop) ? pop.key : null;
          state = pop.state;
        },
        ocomma = function () {
          local pop = stack.pop();
          container[key] <- value;
          value = container;
          container = ("container" in pop) ? pop.container : null;
          key = ("key" in pop) ? pop.key : null;
          state = pop.state;
        }
      },

      "[" : {
        go = function () {
          stack.push({state = "ok"});
          container = [];
          state = "firstavalue";
        },
        ovalue = function () {
          stack.push({container = container, state = "ocomma", key = key});
          container = [];
          state = "firstavalue";
        },
        firstavalue = function () {
          stack.push({container = container, state = "acomma"});
          container = [];
          state = "firstavalue";
        },
        avalue = function () {
          stack.push({container = container, state = "acomma"});
          container = [];
          state = "firstavalue";
        }
      },

      "]" : {
        firstavalue = function () {
          local pop = stack.pop();
          value = container;
          container = ("container" in pop) ? pop.container : null;
          key = ("key" in pop) ? pop.key : null;
          state = pop.state;
        },
        acomma = function () {
          local pop = stack.pop();
          container.push(value);
          value = container;
          container = ("container" in pop) ? pop.container : null;
          key = ("key" in pop) ? pop.key : null;
          state = pop.state;
        }
      },

      ":" : {
        colon = function () {
          // Check if the key already exists
          // NOTE previous code used 'if (key in container)...'
          //      but this finds table ('container') member methods too
          local err = false;
          foreach (akey, avalue in container) {
            if (akey == key) err = true;
            break
          }
          if (err) throw "Duplicate key \"" + key + "\"";
          state = "ovalue";
        }
      },

      "," : {
        ocomma = function () {
          container[key] <- value;
          state = "okey";
        },
        acomma = function () {
          container.push(value);
          state = "avalue";
        }
      },

      "true" : {
        go = function () {
          value = true;
          state = "ok";
        },
        ovalue = function () {
          value = true;
          state = "ocomma";
        },
        firstavalue = function () {
          value = true;
          state = "acomma";
        },
        avalue = function () {
          value = true;
          state = "acomma";
        }
      },

      "false" : {
        go = function () {
          value = false;
          state = "ok";
        },
        ovalue = function () {
          value = false;
          state = "ocomma";
        },
        firstavalue = function () {
          value = false;
          state = "acomma";
        },
        avalue = function () {
          value = false;
          state = "acomma";
        }
      },

      "null" : {
        go = function () {
          value = null;
          state = "ok";
        },
        ovalue = function () {
          value = null;
          state = "ocomma";
        },
        firstavalue = function () {
          value = null;
          state = "acomma";
        },
        avalue = function () {
          value = null;
          state = "acomma";
        }
      }
    };

    //

    state = "go";
    stack = [];

    // current tokenizeing position
    local start = 0;

    try {

      local
        result,
        token,
        tokenizer = _JSONTokenizer();

      while (token = tokenizer.nextToken(str, start)) {

        if ("ptfn" == token.type) {
          // punctuation/true/false/null
          action[token.value][state]();
        } else if ("number" == token.type) {
          // number
          value = token.value;
          number[state]();
        } else if ("string" == token.type) {
          // string
          value = tokenizer.unescape(token.value);
          string[state]();
        }

        start += token.length;
      }

    } catch (e) {
      state = e;
    }

    // check is the final state is not ok
    // or if there is somethign left in the str
    if (state != "ok" || regexp("[^\\s]").capture(str, start)) {
      local min = @(a, b) a < b ? a : b;
      local near = str.slice(start, min(str.len(), start + 10));
      throw "JSON Syntax Error near `" + near + "`";
    }

    return value;
  }

  /**
   * Convert strings/numbers
   * Uses custom converter function
   *
   * @param {string} value
   * @param {string} type
   * @param {function|null} converter
   */
  function _convert(value, type, converter) {
    if ("function" == typeof converter) {

      // # of params for converter function

      local parametercCount = 2;

      // .getinfos() is missing on ei platform
      if ("getinfos" in converter) {
        parametercCount = converter.getinfos().parameters.len()
          - 1 /* "this" is also included */;
      }

      if (parametercCount == 1) {
        return converter(value);
      } else if (parametercCount == 2) {
        return converter(value, type);
      } else {
        throw "Error: converter function must take 1 or 2 parameters"
      }

    } else if ("number" == type) {
      return (value.find(".") == null && value.find("e") == null && value.find("E") == null) ? value.tointeger() : value.tofloat();
    } else {
      return value;
    }
  }
}

/**
 * JSON Tokenizer
 * @package JSONParser
 */
class _JSONTokenizer {

  _ptfnRegex = null;
  _numberRegex = null;
  _stringRegex = null;
  _ltrimRegex = null;
  _unescapeRegex = null;

  constructor() {
    // punctuation/true/false/null
    this._ptfnRegex = regexp("^(?:\\,|\\:|\\[|\\]|\\{|\\}|true|false|null)");

    // numbers
    this._numberRegex = regexp("^(?:\\-?\\d+(?:\\.\\d*)?(?:[eE][+\\-]?\\d+)?)");

    // strings
    this._stringRegex = regexp("^(?:\\\"((?:[^\\r\\n\\t\\\\\\\"]|\\\\(?:[\"\\\\\\/trnfb]|u[0-9a-fA-F]{4}))*)\\\")");

    // ltrim pattern
    this._ltrimRegex = regexp("^[\\s\\t\\n\\r]*");

    // string unescaper tokenizer pattern
    this._unescapeRegex = regexp("\\\\(?:(?:u\\d{4})|[\\\"\\\\/bfnrt])");
  }

  /**
   * Get next available token
   * @param {string} str
   * @param {integer} start
   * @return {{type,value,length}|null}
   */
  function nextToken(str, start = 0) {

    local
      m,
      type,
      token,
      value,
      length,
      whitespaces;

    // count # of left-side whitespace chars
    whitespaces = this._leadingWhitespaces(str, start);
    start += whitespaces;

    if (m = this._ptfnRegex.capture(str, start)) {
      // punctuation/true/false/null
      value = str.slice(m[0].begin, m[0].end);
      type = "ptfn";
    } else if (m = this._numberRegex.capture(str, start)) {
      // number
      value = str.slice(m[0].begin, m[0].end);
      type = "number";
    } else if (m = this._stringRegex.capture(str, start)) {
      // string
      value = str.slice(m[1].begin, m[1].end);
      type = "string";
    } else {
      return null;
    }

    token = {
      type = type,
      value = value,
      length = m[0].end - m[0].begin + whitespaces
    };

    return token;
  }

  /**
   * Count # of left-side whitespace chars
   * @param {string} str
   * @param {integer} start
   * @return {integer} number of leading spaces
   */
  function _leadingWhitespaces(str, start) {
    local r = this._ltrimRegex.capture(str, start);

    if (r) {
      return r[0].end - r[0].begin;
    } else {
      return 0;
    }
  }

  // unesacape() replacements table
  _unescapeReplacements = {
    "b": "\b",
    "f": "\f",
    "n": "\n",
    "r": "\r",
    "t": "\t"
  };

  /**
   * Unesacape string escaped per JSON standard
   * @param {string} str
   * @return {string}
   */
  function unescape(str) {

    local start = 0;
    local res = "";

    while (start < str.len()) {
      local m = this._unescapeRegex.capture(str, start);

      if (m) {
        local token = str.slice(m[0].begin, m[0].end);

        // append chars before match
        local pre = str.slice(start, m[0].begin);
        res += pre;

        if (token.len() == 6) {
          // unicode char in format \uhhhh, where hhhh is hex char code
          // todo: convert \uhhhh chars
          res += token;
        } else {
          // escaped char
          // @see http://www.json.org/
          local char = token.slice(1);

          if (char in this._unescapeReplacements) {
            res += this._unescapeReplacements[char];
          } else {
            res += char;
          }
        }

      } else {
        // append the rest of the source string
        res += str.slice(start);
        break;
      }

      start = m[0].end;
    }

    return res;
  }
}

suiteStack <- []

reporter <- ConsoleReporter()

try {
    if (env && env != "" && reporters[env]) {
        reporter <- reporters[env]
    }
} catch (e) {
    throw "ERROR: Invalid NUTKIN_ENV: " + env
}

class Nutkin {
    reporter = null
    rootSuite = null

    constructor(reporterInstance) {
        reporter = reporterInstance
    }

    function skipSuite(name, suite) {
        addSuite(name, suite, true)
    }

    function addSuite(name, suite, skipped = false) {
        rootSuite = Suite(name, suite, null, skipped, false, testPattern)
        rootSuite.parse()
    }

    function getTimeInMillis() {
        try {
            if (typeof clock == "function") {
                return clock() * 1000.0
            } else {
                return clock * 1.0 // To ensure it's a float
            }
        } catch (e) {
            // Target env has no clock (e.g. device stub)
            return -1
        }
    }

    function calculateTimeTaken(started) {
        if (started < 0) {
            // No clock
            return ""
        }

        local stopped = getTimeInMillis()
        local timeTaken = stopped - started

        if (timeTaken > 1000) {
            return (timeTaken / 1000) + "s"
        }

        return timeTaken + "ms"
    }

    function runTests() {
        local started = getTimeInMillis()
        reporter.begin()

        local outcomes = rootSuite.run(reporter, false)

        local passed = outcomes.filter(@(index, item) item == Outcome.PASSED).len()
        local failed = outcomes.filter(@(index, item) item == Outcome.FAILED).len()
        local skipped = outcomes.filter(@(index, item) item == Outcome.SKIPPED).len()

        reporter.end(passed, failed, skipped, calculateTimeTaken(started))
    }
}

nutkin <- Nutkin(reporter)

class it {

    static function skip(title, ...) {
        throw "it() must be used inside a describe()"
    }

    constructor(title, ...) {
        throw "it() must be used inside a describe()"
    }
}

class describe {

    static function skip(title, suite) {
        nutkin.skipSuite(title, suite)
        nutkin.runTests()
    }

    static function only(title, suite) {
        describe(title, suite)
    }

    constructor(title, suite) {
        nutkin.addSuite(title, suite)
        nutkin.runTests()
    }
}

/////////////////////////////////////////////////////
//Testing Start
/////////////////////////////////////////////////////
describe("MT3620 C APIs", function()
{
    local stackTop;

    beforeEach(function()
    {
        stackTop = getStackTop();
    })

    afterEach(function()
    {
        expect(stackTop).to.equal(getStackTop());
    })

    describe("JSON", function()
    {
        describe("Decode", function()
        {
            it("Can decode the simplest top-level JSON", function()
            {
                expect(json.decode("{}")).to.equal({});
                expect(json.decode("[]")).to.equal([]);
            })

            it("Can decode a null", function()
            {
                local result = json.decode("[null]")[0];
                expect(result).to.be.ofType("null");
                expect(result).to.equal(null);
            })

            it("Can decode a boolean", function()
            {
                local result = json.decode("[true]")[0];
                expect(result).to.be.ofType("bool");
                expect(result).to.equal(true);

                local result = json.decode("[false]")[0];
                expect(result).to.be.ofType("bool");
                expect(result).to.equal(false);
            })

            it("Can decode an integer", function()
            {
                local result = json.decode("[123]")[0];
                expect(result).to.be.ofType("integer");
                expect(result).to.equal(123);
            })

            it("Can decode a float", function()
            {
                local result = json.decode("[456.0]")[0];
                expect(result).to.be.ofType("float");
                expect(result).to.equal(456.0);

                local result = json.decode("[456.5]")[0];
                expect(result).to.be.ofType("float");
                expect(result).to.equal(456.5);

                local result = json.decode("[456.000001]")[0];
                expect(result).to.be.ofType("float");
                expect(result).to.equal(456.000001);
            })

            it("Can decode a float with exponent", function()
            {
                local result = json.decode("[1.1E+2]")[0];
                expect(result).to.be.ofType("float");
                expect(result).to.equal(110.0);
                
                local result = json.decode("[1E+2]")[0];
                expect(result).to.be.ofType("float");
                expect(result).to.equal(100.0);

                local result = json.decode("[1e3]")[0];
                expect(result).to.be.ofType("float");
                expect(result).to.equal(1000.0);

                local result = json.decode("[1.1E-2]")[0];
                expect(result).to.be.ofType("float");
                expect(result).to.equal(0.011);
            })

            it("Can decode a string", function()
            {
                local result = json.decode(@"[""aString""]")[0];
                expect(result).to.be.ofType("string");
                expect(result).to.equal("aString");
            })

            it("Can decode a string with a compliant escape sequence", function()
            {
                local result = json.decode(@"[""aString\""""]")[0];
                expect(result).to.be.ofType("string");
                expect(result).to.equal("aString\"");

                result = json.decode(@"[""aString\\""]")[0];
                expect(result).to.be.ofType("string");
                expect(result).to.equal("aString\\");

                result = json.decode(@"[""aString\/""]")[0];
                expect(result).to.be.ofType("string");
                expect(result).to.equal("aString/");

                result = json.decode(@"[""aString\b""]")[0];
                expect(result).to.be.ofType("string");
                expect(result).to.equal("aString\b");

                result = json.decode(@"[""aString\f""]")[0];
                expect(result).to.be.ofType("string");
                expect(result).to.equal("aString\f");

                result = json.decode(@"[""aString\n""]")[0];
                expect(result).to.be.ofType("string");
                expect(result).to.equal("aString\n");

                result = json.decode(@"[""aString\r""]")[0];
                expect(result).to.be.ofType("string");
                expect(result).to.equal("aString\r");

                result = json.decode(@"[""aString\t""]")[0];
                expect(result).to.be.ofType("string");
                expect(result).to.equal("aString\t");              
            })

            it("Can decode only one byte from a \\u escape sequence (no unicode support)", function()
            {
                local result = json.decode(@"[""aString\u0001""]")[0];
                expect(result).to.be.ofType("string");
                expect(result).to.equal("aString\x01");
            })
        })

        describe("Encode", function()
        {
            it("Can encode the simplest top-level JSON", function()
            {
                expect(json.encode({})).to.equal("{}");
                expect(json.encode([])).to.equal("[]");
            })

            it("Can encode a string", function()
            {
                expect(json.encode(["aTestString"])).to.equal("[\"aTestString\"]");
            })

            it("Can encode a string with an escape sequence", function()
            {
                expect(json.encode(["aTest\nString"])).to.equal("[\"aTest\\nString\"]");
                expect(json.encode(["aTest\0String"])).to.equal("[\"aTest\\u0000String\"]");
            })

            it("Can encode a number", function()
            {
                expect(json.encode([123])).to.equal("[123]");
                expect(json.encode([123.456])).to.equal("[123.456]");
                expect(json.encode([891.1011121314151617])).to.equal("[891.101]");
            })

            it("Can encode a boolean", function()
            {
                expect(json.encode([true])).to.equal("[true]");
                expect(json.encode([false])).to.equal("[false]");
            })

            it("Can encode a null", function()
            {
                expect(json.encode([null])).to.equal("[null]");
            })

            it("Can encode a table", function()
            {
                expect(json.encode({"aTestKey" : "aTestValue", "aTestKey2" : "aTestValue2"})).to.equal("{\"aTestKey2\":\"aTestValue2\",\"aTestKey\":\"aTestValue\"}");
            })

            it("Can encode an array", function()
            {
                expect(json.encode(["aTestValue", "aTestValue2"])).to.equal("[\"aTestValue\",\"aTestValue2\"]");
            })

            it("Can encode an instance", function()
            {
                local TestClass = class {
                    anAtrribute = 1;
                }

                local TestClassWithSerializeraw = class {
                    anAtrribute = 1;
                    _str = "testdata";
                    function _serializeRaw() { return _str; }
                }

                local TestClassWithSerialize = class {
                    anAtrribute = 1;
                    _str = "testdata";
                    function _serialize() { return {"raw" : 999}; }
                }

                local SomethingWithNexti = class {
                    anAtrribute = 1;
                    _str = "testdata";
                    function _nexti(previdx) {
                        if (_str.len() == 0) {
                            return null;
                        } else if (previdx == null) {
                            return 0;
                        } else if (previdx == _str.len() - 1) {
                            return null;
                        } else {
                            return previdx + 1;
                        }
                    }
                    function _get(idx) {
                        return _str[idx].tochar();
                    }
                }

                local anInstance = TestClass();
                expect(json.encode([anInstance])).to.equal("[{\"anAtrribute\":1}]");

                anInstance = TestClassWithSerializeraw();
                expect(json.encode([anInstance])).to.equal("[\"testdata\"]");

                anInstance = TestClassWithSerialize();
                expect(json.encode([anInstance])).to.equal("[{\"raw\":999}]");

                anInstance = SomethingWithNexti();
                expect(json.encode([anInstance])).to.equal("[{\"0\":\"t\",\"1\":\"e\",\"2\":\"s\",\"3\":\"t\",\"4\":\"d\",\"5\":\"a\",\"6\":\"t\",\"7\":\"a\"}]");
            })

            it("Can encode a blob", function()
            {
                local aBlob = blob();
                aBlob.writestring("testdata");
                expect(json.encode([aBlob])).to.equal("[\"testdata\"]");
            })

            it("Can encode a blob with an escape sequence", function()
            {
                local aBlob = blob();
                aBlob.writestring("test\tdata");
                expect(json.encode([aBlob])).to.equal("[\"test\\tdata\"]");

                aBlob = blob();
                aBlob.writestring("test\0data");
                expect(json.encode([aBlob])).to.equal("[\"test\\u0000data\"]");
            })
        })
    })

    describe("PrettyPrint", function()
    {
        it("Can PrettyPrint a table", function()
        {
            prettyPrint.print({"aKey" : "aValue"});
            // Not verifyable
        })
    })
})