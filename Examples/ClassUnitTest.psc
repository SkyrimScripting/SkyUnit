scriptName ClassUnitTest extends SkyUnitTest

function TestAnotherShouldPass() global
    SkyUnit.Assert(true, "This message would show only if this failed")
endFunction

function TestAnotherShouldFail() global
    SkyUnit.Assert(false, "This message should show because this failed")
    if SkyUnit.Assert(false, "This message should ALSO show because the test execution does not stop")
        SkyUnit.Assert(false, "But this won't show because of the if conditional we used on the Assert above")
    endIf
endFunction
