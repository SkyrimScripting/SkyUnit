// This whole plugin intentionally implemented in 1 file (for a truly minimalistic v1 of SkyUnit)

#include <spdlog/sinks/basic_file_sink.h>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <format>
#include <functional>
#include <queue>
#include <string>

#define Log(...) SKSE::log::info(__VA_ARGS__)

namespace SkyUnit {

    class GameStartedEvent : public RE::BSTEventSink<RE::TESCellFullyLoadedEvent> {
    public:
        std::function<void()> callback;
        RE::BSEventNotifyControl ProcessEvent(const RE::TESCellFullyLoadedEvent*,
                                              RE::BSTEventSource<RE::TESCellFullyLoadedEvent>* source) override {
            callback();
            source->RemoveEventSink(this);
            return RE::BSEventNotifyControl::kContinue;
        }
    };

    class PapyrusCallbackHandler : public RE::BSScript::IStackCallbackFunctor {
    public:
        std::function<void()> callback;
        ~PapyrusCallbackHandler() override = default;
        void operator()(RE::BSScript::Variable) override { callback(); }
        bool CanSave() const override { return false; }
        void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}
    };

    constexpr auto LOG_FILE_NAME = "SkyUnit.TestResults.log";
    constexpr auto DATA_SCRIPTS_PATH = "Data/Scripts";
    constexpr auto UNIT_TEST_SCRIPT_NAME_SUFFIX = "UnitTest";

    GameStartedEvent GameStartedEventListener;
    PapyrusCallbackHandler PapyrusCallbackHandlerInstance;
    std::string CurrentlyRunning_UnitTestScriptName;
    std::string CurrentlyRunning_FunctionName;
    unsigned int TestsPassedCount = 0;
    unsigned int TestsFailedCount = 0;
    unsigned int CurrentTestAssertionErrorCount = 0;
    std::atomic<bool> UnitTestsRunStarted = false;
    std::queue<std::string> UnitTestScriptNames;
    std::queue<std::string> UnitTestScriptFunctionNames;

    std::string LowerCase(const std::string& text) {
        std::string copy = text;
        std::transform(copy.begin(), copy.end(), copy.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return copy;
    }

    void DiscoverUnitTestFunctions() {
        auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
        RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo> typeInfo;
        vm->GetScriptObjectType(CurrentlyRunning_UnitTestScriptName, typeInfo);
        auto functionCount = typeInfo->GetNumGlobalFuncs();
        auto* functions = typeInfo->GetGlobalFuncIter();
        for (unsigned int i = 0; i < functionCount; i++)
            UnitTestScriptFunctionNames.push(functions[i].func->GetName().c_str());
    }

    void DiscoverUnitTests() {
        auto scriptNameSuffix = std::format("{}.pex", LowerCase(UNIT_TEST_SCRIPT_NAME_SUFFIX));
        for (auto& file : std::filesystem::directory_iterator(DATA_SCRIPTS_PATH))
            if (LowerCase(file.path().string()).ends_with(scriptNameSuffix))
                UnitTestScriptNames.push(file.path().stem().string().c_str());
    }

    void TestsAreFinishedRunning() {
        if (TestsFailedCount > 0)
            Log("Tests failed. {} passed, {} failed.", TestsPassedCount, TestsFailedCount);
        else if (TestsPassedCount > 0)
            Log("Tests passed. {} passed, {} failed.", TestsPassedCount, TestsFailedCount);
        else
            Log("No tests ran");
        std::exit(0);
    }

    void RunNextUnitTestScript();  // because we're not bothering with headers in this minimalistic v1 implementation

    void RunUnitTestFunction() {
        auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
        RE::BSScript::ZeroFunctionArguments args;
        RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callbackPtr{&PapyrusCallbackHandlerInstance};
        vm->DispatchStaticCall(CurrentlyRunning_UnitTestScriptName, CurrentlyRunning_FunctionName, &args, callbackPtr);
    }

    void PrintUnitTestFunctionResult() {
        if (CurrentlyRunning_FunctionName.empty()) return;
        if (CurrentTestAssertionErrorCount > 0) {
            TestsFailedCount++;
            Log("[FAIL] {}", CurrentlyRunning_FunctionName);
        } else {
            TestsPassedCount++;
            Log("[PASS] {}", CurrentlyRunning_FunctionName);
        }
        CurrentTestAssertionErrorCount = 0;
    }

    void RunNextUnitTestFunction() {
        PrintUnitTestFunctionResult();
        if (UnitTestScriptFunctionNames.empty()) {
            RunNextUnitTestScript();
            return;
        }
        CurrentlyRunning_FunctionName = UnitTestScriptFunctionNames.front();
        UnitTestScriptFunctionNames.pop();
        RunUnitTestFunction();
    }

    void RunNextUnitTestScript() {
        if (UnitTestScriptNames.empty()) {
            TestsAreFinishedRunning();
            return;
        }
        CurrentlyRunning_UnitTestScriptName = UnitTestScriptNames.front();
        UnitTestScriptNames.pop();
        Log("{}", CurrentlyRunning_UnitTestScriptName);
        DiscoverUnitTestFunctions();
        RunNextUnitTestFunction();
    }

    void RunAllTestScripts() {
        if (!UnitTestsRunStarted.exchange(true)) RunNextUnitTestScript();
    }

    void SetupLog() {
        auto logsFolder = SKSE::log::log_directory();
        if (!logsFolder) {
            SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");
            return;
        }
        auto logFilePath = *logsFolder / LOG_FILE_NAME;
        auto fileLoggerPtr = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath.string(), true);
        auto loggerPtr = std::make_shared<spdlog::logger>("log", std::move(fileLoggerPtr));
        spdlog::set_default_logger(std::move(loggerPtr));
        spdlog::set_level(spdlog::level::trace);
        spdlog::flush_on(spdlog::level::info);
    }

    bool SkyUnit_Assert(RE::StaticFunctionTag*, bool result, std::string failureMessage) {
        if (!result) {
            CurrentTestAssertionErrorCount++;
            Log("FAIL! {}", failureMessage);
        }
        return result;
    }

    bool BindPapyrusFunctions(RE::BSScript::IVirtualMachine* vm) {
        vm->RegisterFunction("Assert", "SkyUnit", SkyUnit_Assert);
        return true;
    }

    SKSEPluginLoad(const SKSE::LoadInterface* skse) {
        SKSE::Init(skse);
        SetupLog();
        DiscoverUnitTests();
        if (UnitTestScriptNames.empty()) {
            Log("No unit tests found (Data\\Scripts\\*UnitTest.pex)");
        } else {
            Log("Found {} unit tests (Data\\Scripts\\*UnitTest.pex)", UnitTestScriptNames.size());
            SKSE::GetPapyrusInterface()->Register(BindPapyrusFunctions);
            GameStartedEventListener.callback = []() { RunAllTestScripts(); };
            PapyrusCallbackHandlerInstance.callback = []() { RunNextUnitTestFunction(); };
            RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESCellFullyLoadedEvent>(
                &GameStartedEventListener);
        }
        return true;
    }
}
