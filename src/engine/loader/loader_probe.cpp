#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "saex/engine/loader_observation.hpp"
#include "saex/engine/loader_policy.generated.hpp"
#include "saex/engine/entry_policy.generated.hpp"
#include "saex/engine/proxy_policy.generated.hpp"
#include "saex/engine/startup_policy.generated.hpp"
#include "saex/engine/codec_policy.generated.hpp"
#include "saex/engine/binding_policy.generated.hpp"
#include "saex/engine/asi_policy.generated.hpp"
#include "saex/engine/bootstrap_artifact.generated.hpp"
#include "saex/engine/bootstrap_lifecycle_policy.generated.hpp"
#include "saex/engine/observed_profile.generated.hpp"
#include "saex/engine/frame_target_policy.generated.hpp"
#include "saex/engine/startup_return_policy.generated.hpp"
#include "saex/engine/crt_startup_policy.generated.hpp"
#include "saex/engine/application_entry_policy.generated.hpp"
#include "saex/engine/cwd_acquire_policy.generated.hpp"
#include "saex/engine/windows_file_observation.hpp"
#include "saex/engine/launch_context.hpp"
#include <iostream>
#include <algorithm>
#include <string>

namespace {
template<std::size_t N> std::string hex(const std::array<std::byte, N>& bytes) {
    constexpr char alphabet[] = "0123456789abcdef";
    std::string result;
    for (auto byte : bytes) {
        const auto n = std::to_integer<unsigned>(byte);
        result += alphabet[n >> 4U]; result += alphabet[n & 15U];
    }
    return result;
}
std::string json_path(std::wstring_view value) {
    constexpr char digits[] = "0123456789abcdef";
    std::string result{"\""};
    for (const auto ch : value) {
        const auto code = static_cast<unsigned short>(ch);
        result += "\\u";
        for (int shift = 12; shift >= 0; shift -= 4) result += digits[(code >> shift) & 15];
    }
    return result + '"';
}
void print(const saex::engine::LoaderTrace& trace, std::uint32_t pid, std::string_view engine_hash,
    bool reviewed, std::string_view failed_module, const saex::engine::LaunchContext* context, bool entry_mode, bool proxy_mode, bool startup_mode, bool codec_mode, bool binding_mode, bool asi_mode, bool bootstrap_mode, bool frame_mode, bool tail_mode, bool crt_mode, bool application_mode, bool platform_mode, bool suppression_mode, bool instance_mode, bool dispatch_mode, bool routing_mode, bool prelude_mode, bool manager_mode, bool seh_mode, bool lock_mode, bool acquire_mode) {
    const auto policy = reviewed ? saex::engine::reviewed_loader_policy_id : "windows-loader-three-file-observation-v1";
    const auto digest = reviewed ? saex::engine::reviewed_loader_policy_digest : "";
    // A continued thread may fail before the next checkpoint; continuation alone is not execution proof.
    const char* body_execution = !trace.platform_startup.continued ? "false" : trace.platform_startup.call_reached ? "true" : "null";
    std::cout << std::boolalpha << "{\"scope\":\"" << (acquire_mode ? "bounded-cwd-acquire" : lock_mode ? "bounded-cwd-lock" : seh_mode ? "bounded-cwd-seh" : manager_mode ? "bounded-file-manager-entry" : prelude_mode ? "bounded-game-prelude" : routing_mode ? "bounded-application-routing" : dispatch_mode ? "bounded-event-dispatch" : instance_mode ? "bounded-instance-startup" : suppression_mode ? "bounded-platform-suppression" : platform_mode ? "bounded-platform-startup" : application_mode ? "bounded-application-entry" : crt_mode ? "bounded-crt-initializer-boundary" : tail_mode ? "bounded-natural-startup-return" : frame_mode ? "bounded-frame-target-observation" : bootstrap_mode ? "bounded-bootstrap-lifecycle-observation" : asi_mode ? "bounded-asi-return-observation" : binding_mode ? "bounded-codec-bindings-observation" : codec_mode ? "bounded-codec-return-observation" : startup_mode ? "bounded-startup-call-observation" : proxy_mode ? "bounded-proxy-return-observation" : entry_mode ? "bounded-entry-boundary-observation" : "bounded-loader-mapping-observation")
        << "\",\"schemaVersion\":1,\"canAttach\":false,"
        << "\"initializationVerified\":false,\"policy\":\"" << policy << "\",\"policySourceDigest\":\"" << digest
        << "\",\"failedPolicyModule\":\"" << failed_module << "\",\"engineSha256\":\"" << engine_hash
        << "\",\"childCreated\":" << (pid != 0) << ",\"childPid\":" << pid << ",\"childExitConfirmed\":" << trace.exit_confirmed
        << ",\"loaderAdvanced\":" << trace.advanced << ",\"breakpointCandidate\":" << trace.breakpoint_candidate
        << ",\"eventCount\":" << trace.event_count << ",\"threadCount\":" << trace.thread_count
        << ",\"lastEventCode\":" << trace.last_event_code << ",\"lastEventThreadId\":" << trace.last_event_thread_id
        << ",\"lastEventAddress\":" << trace.last_event_address << ",\"activeModuleCount\":" << trace.active_module_count
        << ",\"unloadCount\":" << trace.unload_count
        << ",\"chargedBytes\":" << trace.charged_bytes << ",\"exceptionCode\":" << trace.exception_code
        << ",\"exceptionAddress\":" << trace.exception_address << ",\"systemError\":" << trace.system_error
        << ",\"reason\":\"" << trace.reason << "\",\"launchContext\":";
    if (context) {
        std::cout << "{\"mode\":\"explicit-directory-environment-snapshot\",\"prepared\":" << context->valid()
            << ",\"directory\":" << json_path(context->directory()) << ",\"environmentSha256\":\""
            << (context->valid() ? hex(context->environment_hash()) : "") << "\",\"environmentEntries\":" << context->entry_count()
            << ",\"environmentCodeUnits\":" << context->environment().size() << '}';
    } else std::cout << "null";
    std::cout << ",\"cwdAcquireObservation\":";
    if(acquire_mode){
        const auto& s=trace.cwd_acquire;
        std::cout << "{\"policy\":\"" << saex::engine::reviewed_cwd_acquire_id << "\",\"policySourceDigest\":\"" << saex::engine::reviewed_cwd_acquire_digest
            << "\",\"directoryApiAllowed\":false,\"lazyInitializationAllowed\":false,\"unlockVerified\":false,\"cwdReturnVerified\":false"
            << ",\"stage\":" << s.stage << ",\"stopAddress\":" << s.stop_address << ",\"objectAddress\":" << s.object_address << ",\"functionAddress\":" << s.function_address << ",\"ownerThreadId\":" << s.thread_id
            << ",\"objectMemoryType\":" << s.object_memory_type << ",\"objectProtection\":" << s.object_protection
            << ",\"lockCountBefore\":" << s.object_before[1] << ",\"lockCountAfter\":" << s.object_after[1] << ",\"recursionBefore\":" << s.object_before[2] << ",\"recursionAfter\":" << s.object_after[2]
            << ",\"ownerBefore\":" << s.object_before[3] << ",\"ownerAfter\":" << s.object_after[3]
            << ",\"continued\":" << s.continued << ",\"branchReached\":" << s.branch_reached << ",\"callReached\":" << s.call_reached << ",\"functionEntered\":" << s.function_entered << ",\"functionReturned\":" << s.function_returned << ",\"selectorReturned\":" << s.selector_returned
            << ",\"shapeValid\":" << s.shape_valid << ",\"frameValid\":" << s.frame_valid << ",\"objectRead\":" << s.object_read << ",\"objectValid\":" << s.object_valid << ",\"slotPreserved\":" << s.slot_preserved
            << ",\"stackPreserved\":" << s.stack_preserved << ",\"sehPreserved\":" << s.seh_preserved << ",\"priorRecordPreserved\":" << s.prior_record_preserved << ",\"callerPreserved\":" << s.caller_preserved
            << ",\"bufferPreserved\":" << s.buffer_preserved << ",\"localisationPreserved\":" << s.localisation_preserved << ",\"acquired\":" << s.acquired << ",\"verified\":" << s.verified << '}';
    }else std::cout << "null";
    std::cout << ",\"cwdLockObservation\":";
    if (lock_mode) {
        const auto& s=trace.cwd_lock;
        std::cout << "{\"policy\":\"" << saex::engine::reviewed_cwd_lock_id << "\",\"policySourceDigest\":\"" << saex::engine::reviewed_cwd_lock_digest
            << "\",\"lockIndex\":7,\"branchAllowed\":" << acquire_mode << ",\"lazyInitializationAllowed\":false,\"criticalSectionCallAllowed\":" << acquire_mode << ",\"lockAcquiredVerified\":" << trace.cwd_acquire.acquired << ",\"cwdReturnVerified\":false"
            << ",\"stage\":" << s.stage << ",\"stopAddress\":" << s.stop_address << ",\"slotAddress\":" << s.slot_address << ",\"slotValue\":" << s.slot_value
            << ",\"slotPresent\":" << (s.slot_read && s.slot_value!=0) << ",\"continued\":" << s.continued << ",\"entryReached\":" << s.entry_reached << ",\"comparisonReached\":" << s.comparison_reached
            << ",\"shapeValid\":" << s.shape_valid << ",\"frameValid\":" << s.frame_valid << ",\"slotRead\":" << s.slot_read << ",\"slotPreserved\":" << s.slot_preserved
            << ",\"memoryValid\":" << s.memory_valid << ",\"sehPreserved\":" << s.seh_preserved << ",\"priorRecordPreserved\":" << s.prior_record_preserved
            << ",\"callerPreserved\":" << s.caller_preserved << ",\"bufferPreserved\":" << s.buffer_preserved << ",\"localisationPreserved\":" << s.localisation_preserved << ",\"verified\":" << s.verified << '}';
    } else std::cout << "null";
    std::cout << ",\"cwdSehObservation\":";
    if (seh_mode) {
        const auto& s=trace.cwd_seh;
        std::cout << "{\"policy\":\"" << saex::engine::reviewed_cwd_seh_id << "\",\"policySourceDigest\":\"" << saex::engine::reviewed_cwd_seh_digest
            << "\",\"lockPathAllowed\":" << lock_mode << ",\"directoryApiAllowed\":false,\"cwdReturnVerified\":false,\"unwindVerified\":false"
            << ",\"stage\":" << s.stage << ",\"stopAddress\":" << s.stop_address << ",\"recordAddress\":" << s.record_address
            << ",\"previousHead\":" << s.tib_before[0] << ",\"currentHead\":" << s.tib_after[0]
            << ",\"continued\":" << s.continued << ",\"wrapperEntryReached\":" << s.wrapper_entry_reached << ",\"prologueEntryReached\":" << s.prologue_entry_reached
            << ",\"prologueReturned\":" << s.prologue_returned << ",\"shapeValid\":" << s.shape_valid << ",\"frameValid\":" << s.frame_valid
            << ",\"memoryRead\":" << s.memory_read << ",\"memoryValid\":" << s.memory_valid << ",\"priorRecordPreserved\":" << s.prior_record_preserved
            << ",\"callerPreserved\":" << s.caller_preserved << ",\"bufferPreserved\":" << s.buffer_preserved << ",\"localisationPreserved\":" << s.localisation_preserved
            << ",\"verified\":" << s.verified << '}';
    } else std::cout << "null";
    std::cout << ",\"fileManagerEntryObservation\":";
    if (manager_mode) {
        const auto& s=trace.file_manager_entry;
        std::cout << "{\"policy\":\"" << saex::engine::reviewed_file_manager_id << "\",\"policySourceDigest\":\"" << saex::engine::reviewed_file_manager_digest
            << "\",\"cwdCallAllowed\":" << seh_mode << ",\"fileManagerReturnVerified\":false,\"bufferCapacity\":128,\"guardBytes\":8"
            << ",\"stage\":" << s.stage << ",\"stopAddress\":" << s.stop_address << ",\"bufferAddress\":" << s.buffer_address << ",\"cwdAddress\":" << s.cwd_address
            << ",\"continued\":" << s.continued << ",\"entryReached\":" << s.entry_reached << ",\"cwdCallReached\":" << s.cwd_call_reached
            << ",\"shapeValid\":" << s.shape_valid << ",\"frameValid\":" << s.frame_valid << ",\"stackPreserved\":" << s.stack_preserved
            << ",\"bufferRead\":" << s.buffer_read << ",\"bufferUnchanged\":" << s.buffer_unchanged << ",\"localisationPreserved\":" << s.localisation_preserved
            << ",\"verified\":" << s.verified << '}';
    } else std::cout << "null";
    std::cout << ",\"gamePreludeObservation\":";
    if (prelude_mode) {
        const auto& s=trace.game_prelude;
        std::cout << "{\"policy\":\"" << saex::engine::reviewed_prelude_id << "\",\"policySourceDigest\":\"" << saex::engine::reviewed_prelude_digest
            << "\",\"helperCallsAllowed\":2,\"fileManagerCallAllowed\":" << manager_mode << ",\"initializerReturnVerified\":false"
            << ",\"stage\":" << s.stage << ",\"stopAddress\":" << s.stop_address << ",\"flagsAddress\":" << s.flags_address
            << ",\"flagsBeforeHex\":\"" << hex(s.flags_before) << "\",\"flagsAfterHex\":\"" << hex(s.flags_after) << '"'
            << ",\"continued\":" << s.continued << ",\"initializerEntryReached\":" << s.initializer_entry_reached
            << ",\"emptyEntryReached\":" << s.empty_entry_reached << ",\"emptyReturned\":" << s.empty_returned
            << ",\"localisationEntryReached\":" << s.localisation_entry_reached << ",\"localisationReturned\":" << s.localisation_returned
            << ",\"shapeValid\":" << s.shape_valid << ",\"frameValid\":" << s.frame_valid << ",\"stackPreserved\":" << s.stack_preserved
            << ",\"flagsRead\":" << s.flags_read << ",\"flagsValid\":" << s.flags_valid << ",\"verified\":" << s.verified << '}';
    } else std::cout << "null";
    std::cout << ",\"applicationRoutingObservation\":";
    if (routing_mode) {
        const auto& s=trace.application_routing;
        std::cout << "{\"policy\":\"" << saex::engine::reviewed_routing_id << "\",\"policySourceDigest\":\"" << saex::engine::reviewed_routing_digest
            << "\",\"eventId\":24,\"initializerCallAllowed\":" << prelude_mode << ",\"rendererInitializationVerified\":false"
            << ",\"stage\":" << s.stage << ",\"stopAddress\":" << s.stop_address
            << ",\"selectedIndex\":" << s.selected_index << ",\"selectedTarget\":" << s.selected_target
            << ",\"continued\":" << s.continued << ",\"entryReached\":" << s.entry_reached
            << ",\"detourReached\":" << s.detour_reached << ",\"indirectReached\":" << s.indirect_reached
            << ",\"initializerCallReached\":" << s.initializer_call_reached
            << ",\"shapeValid\":" << s.shape_valid << ",\"frameValid\":" << s.frame_valid
            << ",\"stackPreserved\":" << s.stack_preserved << ",\"verified\":" << s.verified << '}';
    } else std::cout << "null";
    std::cout << ",\"eventDispatchObservation\":";
    if (dispatch_mode) {
        const auto& s=trace.event_dispatch;
        std::cout << "{\"policy\":\"" << saex::engine::reviewed_dispatch_id << "\",\"policySourceDigest\":\"" << saex::engine::reviewed_dispatch_digest
            << "\",\"applicationHandlerCallAllowed\":" << routing_mode << ",\"rendererInitializationVerified\":false"
            << ",\"applicationSampleHex\":\"" << hex(s.application_sample) << '"'
            << ",\"stage\":" << s.stage
            << ",\"stopAddress\":" << s.stop_address
            << ",\"callerStack\":" << s.caller_stack
            << ",\"continued\":" << s.continued
            << ",\"callReached\":" << s.call_reached
            << ",\"entryReached\":" << s.entry_reached
            << ",\"applicationCallReached\":" << s.application_call_reached
            << ",\"shapeValid\":" << s.shape_valid
            << ",\"frameValid\":" << s.frame_valid
            << ",\"stackPreserved\":" << s.stack_preserved
            << ",\"verified\":" << s.verified
            << '}';
    } else std::cout << "null";
    std::cout << ",\"instanceStartupObservation\":";
    if (instance_mode) {
        const auto& s=trace.instance_startup;
        std::cout << "{\"policy\":\"" << saex::engine::reviewed_instance_id << "\",\"policySourceDigest\":\"" << saex::engine::reviewed_instance_digest
            << "\",\"namedEventCallAllowed\":true,\"windowActivationAllowed\":false,\"contextRestoreAfterContinuation\":false"
            << ",\"stage\":" << s.stage
            << ",\"stopAddress\":" << s.stop_address
            << ",\"callerStack\":" << s.caller_stack
            << ",\"createStack\":" << s.create_stack
            << ",\"eventHandle\":" << s.event_handle
            << ",\"lastError\":" << s.last_error
            << ",\"continued\":" << s.continued
            << ",\"suppressionBoundaryValidated\":" << s.suppression_boundary_validated
            << ",\"createCallReached\":" << s.create_call_reached
            << ",\"createReturned\":" << s.create_returned
            << ",\"argumentsValid\":" << s.arguments_valid
            << ",\"eventIdentityValid\":" << s.event_identity_valid
            << ",\"getterReturned\":" << s.getter_returned
            << ",\"existingDetected\":" << s.existing_detected
            << ",\"callerReturned\":" << s.caller_returned
            << ",\"stackPreserved\":" << s.stack_preserved
            << ",\"verified\":" << s.verified
            << '}';
    } else std::cout << "null";
    std::cout << ",\"platformSuppressionObservation\":";
    if (suppression_mode) {
        const auto& s=trace.platform_suppression; const auto& t=s.transaction;
        std::cout << "{\"policy\":\"" << saex::engine::reviewed_suppression_id << "\",\"policySourceDigest\":\"" << saex::engine::reviewed_suppression_digest
            << "\",\"method\":\"owned-context-false-return\",\"contextWriteAllowed\":true,\"hostSettingCallExecuted\":false,\"naturalApiReturn\":false"
            << ",\"writeAttempted\":" << t.write_attempted << ",\"applied\":" << t.applied
            << ",\"applyFailure\":\"" << t.failure << '"'
            << ",\"continued\":" << s.continued << ",\"returnReached\":" << s.return_reached << ",\"returnAddress\":" << s.return_address
            << ",\"lastErrorRead\":" << s.last_error_read << ",\"lastErrorBefore\":" << s.last_error_before << ",\"lastErrorAfter\":" << s.last_error_after
            << ",\"lastErrorPreserved\":" << s.last_error_preserved << ",\"stackPreserved\":" << s.stack_preserved
            << ",\"restoreAttempted\":" << t.restore_attempted << ",\"restored\":" << t.restored
            << ",\"syntheticResult\":0,\"verified\":" << s.verified << '}';
    } else std::cout << "null";
    std::cout << ",\"platformStartupObservation\":";
    if (platform_mode) {
        const auto& s=trace.platform_startup;
        std::cout << "{\"policy\":\"" << saex::engine::reviewed_platform_id << "\",\"policySourceDigest\":\"" << saex::engine::reviewed_platform_digest
            << "\",\"prologueExecutionAllowed\":true,\"hostSettingCallExecuted\":false,\"hostSettingCallAllowed\":false"
            << ",\"continued\":" << s.continued << ",\"callReached\":" << s.call_reached << ",\"stopAddress\":" << s.stop_address
            << ",\"functionAddress\":" << s.function_address << ",\"shapeValid\":" << s.shape_valid
            << ",\"stackValid\":" << s.stack_valid << ",\"registersValid\":" << s.registers_valid
            << ",\"argumentsValid\":" << s.arguments_valid << ",\"arguments\":[" << s.arguments[0] << ',' << s.arguments[1] << ',' << s.arguments[2] << ',' << s.arguments[3]
            << "],\"verified\":" << s.verified << '}';
    } else std::cout << "null";
    std::cout << ",\"applicationEntryObservation\":";
    if (application_mode) {
        const auto& s=trace.application_entry;
        std::cout << "{\"policy\":\"" << saex::engine::reviewed_application_policy_id
            << "\",\"policySourceDigest\":\"" << saex::engine::reviewed_application_policy_digest
            << "\",\"initializerExecutionAllowed\":true,\"applicationBodyExecuted\":" << body_execution
            << ",\"stage\":" << s.stage << ",\"stopAddress\":" << s.stop_address
            << ",\"continued\":" << s.continued << ",\"initializerReturned\":" << s.initializer_returned
            << ",\"initializerResult\":" << s.initializer_result << ",\"initializerAbiValid\":" << s.initializer_abi_valid
            << ",\"secondCallReached\":" << s.second_call_reached << ",\"secondReturned\":" << s.second_returned
            << ",\"secondAbiValid\":" << s.second_abi_valid << ",\"onceValid\":" << s.once_valid
            << ",\"showCommand\":" << s.show_command << ",\"commandLineBytes\":" << s.command_line_bytes
            << ",\"argumentsValid\":" << s.arguments_valid << ",\"entryReached\":" << s.entry_reached
            << ",\"entryStackValid\":" << s.entry_stack_valid << ",\"verified\":" << s.verified << '}';
    } else std::cout << "null";
    std::cout << ",\"crtStartupObservation\":";
    if (crt_mode) {
        const auto& s=trace.crt_startup;
        std::cout << "{\"policy\":\"" << saex::engine::reviewed_crt_policy_id
            << "\",\"policySourceDigest\":\"" << saex::engine::reviewed_crt_policy_digest
            << "\",\"initializerExecutionAllowed\":" << application_mode << ",\"applicationEntryExecuted\":" << body_execution
            << ",\"stage\":" << s.stage << ",\"stopAddress\":" << s.stop_address
            << ",\"continued\":" << s.continued << ",\"ioReturnReached\":" << s.io_return_reached
            << ",\"returnCode\":" << s.return_code << ",\"stackValid\":" << s.stack_valid
            << ",\"registersValid\":" << s.registers_valid << ",\"checkedSlots\":" << s.checked_slots
            << ",\"nonzeroSlots\":" << s.nonzero_slots << ",\"samplesVerified\":" << s.samples_verified
            << ",\"failedSlotRva\":" << s.failed_slot_rva << ",\"actualTarget\":" << s.actual_target
            << ",\"initializerCallReached\":" << s.initializer_call_reached << ",\"verified\":" << s.verified << '}';
    } else std::cout << "null";
    std::cout << ",\"startupReturnObservation\":";
    if (tail_mode) {
        using namespace saex::engine;
        const auto& s=trace.startup_return;
        std::cout << "{\"policy\":\"" << reviewed_startup_return_id << "\",\"policySourceDigest\":\"" << reviewed_startup_return_digest
            << "\",\"loaderProtectionChangeAllowed\":true,\"bootstrapInvoked\":false,\"stage\":" << s.stage
            << ",\"continued\":" << s.continued << ",\"protectCallReached\":" << s.protect_call_reached
            << ",\"argumentsValid\":" << s.arguments_valid << ",\"protectContinued\":" << s.protect_continued
            << ",\"protectReturnReached\":" << s.protect_return_reached << ",\"returnCode\":" << s.return_code
            << ",\"protectionsVerified\":" << s.protections_verified << ",\"regionCount\":" << s.region_count
            << ",\"lastRegionRva\":" << s.last_region_rva << ",\"lastRegionProtect\":" << s.last_region_protect
            << ",\"lastRegionState\":" << s.last_region_state << ",\"lastRegionType\":" << s.last_region_type
            << ",\"copyOnWriteRegions\":" << s.copy_on_write_regions << ",\"readWriteRegions\":" << s.read_write_regions
            << ",\"oldProtect\":" << s.old_protect << ",\"firstPageProtect\":" << s.first_page_protect
            << ",\"startupReturnReached\":" << s.startup_return_reached << ",\"stackValid\":" << s.stack_valid
            << ",\"registersValid\":" << s.registers_valid << ",\"startupInfoBytes\":" << s.startup_info_bytes
            << ",\"protectTarget\":" << s.protect_target << ",\"startupTarget\":" << s.startup_target
            << ",\"verified\":" << s.verified << ",\"frameSamples\":[";
        for (std::size_t i=0;i<s.frame_samples.size();++i) {
            if (i) std::cout << ',';
            const auto& sample=s.frame_samples[i];
            std::cout << "{\"attempted\":" << sample.attempted << ",\"callRead\":" << sample.call_read
                << ",\"targetRead\":" << sample.target_read << ",\"match\":" << sample.match
                << ",\"threadId\":" << sample.thread_id << ",\"eventIndex\":" << sample.event_index
                << ",\"callHex\":\"" << hex(sample.call) << "\",\"targetPrefixHex\":\"" << hex(sample.target_prefix) << "\"}";
        }
        std::cout << "]}";
    } else std::cout << "null";
    std::cout << ",\"frameTargetObservation\":";
    if (frame_mode) {
        using namespace saex::engine;
        std::cout << "{\"policy\":\"" << reviewed_frame_policy_id
            << "\",\"policySourceDigest\":\"" << reviewed_frame_policy_digest
            << "\",\"nativeFunctionCalled\":false,\"hookInstalled\":false,\"runtimeAbiVerified\":false"
            << ",\"verified\":" << trace.frame_target.verified
            << ",\"callRva\":" << reviewed_frame_spec.call_rva << ",\"targetRva\":" << reviewed_frame_spec.target_rva
            << ",\"samples\":[";
        constexpr const char* phases[]{"create-process", "asi-return", "bootstrap-terminal"};
        for (std::size_t i = 0; i < trace.frame_target.samples.size(); ++i) {
            if (i) std::cout << ',';
            const auto& sample = trace.frame_target.samples[i];
            std::cout << "{\"phase\":\"" << phases[i] << "\",\"attempted\":" << sample.attempted
                << ",\"threadId\":" << sample.thread_id << ",\"eventIndex\":" << sample.event_index
                << ",\"callRead\":" << sample.call_read << ",\"targetRead\":" << sample.target_read
                << ",\"match\":" << sample.match << ",\"callHex\":\"" << hex(sample.call)
                << "\",\"targetPrefixHex\":\"" << hex(sample.target_prefix) << "\"}";
        }
        std::cout << "]}";
    } else std::cout << "null";
    std::cout << ",\"bootstrapObservation\":";
    if (bootstrap_mode) {
        using namespace saex::engine;
        const auto& state = trace.bootstrap;
        std::cout << "{\"executionPolicy\":\"" << reviewed_bootstrap_policy_id
            << "\",\"executionPolicySourceDigest\":\"" << reviewed_bootstrap_policy_digest
            << "\",\"bootstrapExecutionAllowed\":true,\"stackWriteAttempted\":" << state.stack_write_attempted
            << ",\"stackWritten\":" << state.stack_written
            << ",\"callsArmed\":" << state.calls_armed << ",\"callsReturned\":" << state.calls_returned
            << ",\"verified\":" << state.verified << ",\"observedUnverified\":" << state.observed_unverified
            << ",\"frameAddress\":" << state.frame_address << ",\"outputAddress\":" << state.output_address << ",\"calls\":[";
        for (std::uint32_t i = 0; i < state.calls_armed; ++i) {
            if (i) std::cout << ',';
            const auto& call = state.calls[i];
            std::array<std::byte, sizeof(SaexBootstrapStatus)> raw{};
            std::copy_n(reinterpret_cast<const std::byte*>(&call.status), raw.size(), raw.begin());
            std::cout << "{\"exportIndex\":" << call.export_index << ",\"target\":" << call.target
                << ",\"continued\":" << call.continued << ",\"returned\":" << call.returned
                << ",\"returnCode\":" << call.return_code << ",\"stackAfter\":" << call.stack_after
                << ",\"stackValid\":" << call.stack_valid << ",\"guardsValid\":" << call.guards_valid
                << ",\"statusValid\":" << call.status_valid << ",\"state\":" << call.status.state
                << ",\"reason\":" << call.status.reason << ",\"observationAttempts\":" << call.status.observation_attempts
                << ",\"statusBytesHex\":\"" << hex(raw) << "\"}";
        }
        std::cout << "]}";
    } else std::cout << "null";
    std::cout << ",\"asiObservation\":";
    if (asi_mode) {
        using namespace saex::engine;
        std::cout << "{\"executionPolicy\":\"" << reviewed_asi_policy_id
            << "\",\"executionPolicySourceDigest\":\"" << reviewed_asi_policy_digest
            << "\",\"artifactSha256\":\"" << bootstrap_artifact_digest
            << "\",\"artifactMapSha256\":\"" << bootstrap_artifact_map_digest
            << "\",\"asiExecutionAllowed\":true,\"bootstrapExportsCalled\":" << (trace.bootstrap.calls_armed != 0 && trace.bootstrap.calls[0].continued)
            << ",\"callArmed\":" << trace.asi_call_armed << ",\"scanContinued\":" << trace.asi_scan_continued
            << ",\"callReached\":" << trace.asi_call_reached << ",\"pathVerified\":" << trace.asi_path_verified
            << ",\"returnArmed\":" << trace.asi_return_armed << ",\"loadContinued\":" << trace.asi_load_continued
            << ",\"returnReached\":" << trace.asi_return_reached << ",\"verified\":" << trace.asi_verified
            << ",\"callAddress\":" << trace.asi_call_address << ",\"returnAddress\":" << trace.asi_return_address
            << ",\"moduleHandle\":" << trace.asi_handle << ",\"mappingId\":" << trace.asi_mapping_id
            << ",\"armEvent\":" << trace.asi_arm_event << ",\"stackBefore\":" << trace.asi_stack_before
            << ",\"stackAfter\":" << trace.asi_stack_after << '}';
    } else std::cout << "null";
    std::cout << ",\"bindingObservation\":";
    if (binding_mode) {
        std::cout << "{\"executionPolicy\":\"" << saex::engine::reviewed_binding_policy_id
            << "\",\"executionPolicySourceDigest\":\"" << saex::engine::reviewed_binding_policy_digest
            << "\",\"bindingExecutionAllowed\":true,\"breakpointArmed\":" << trace.binding_breakpoint_armed
            << ",\"continued\":" << trace.binding_continued << ",\"reached\":" << trace.binding_reached
            << ",\"verified\":" << trace.binding_verified << ",\"stopAddress\":" << trace.binding_stop_address << ",\"slots\":[";
        for (std::uint32_t i = 0; i < trace.binding_count; ++i) {
            if (i) std::cout << ',';
            const auto& slot = trace.bindings[i];
            std::cout << "{\"name\":\"" << slot.name.data() << "\",\"slotRva\":" << slot.slot_rva
                << ",\"targetRva\":" << slot.target_rva << ",\"before\":" << slot.before << ",\"after\":" << slot.after
                << ",\"targetStable\":" << slot.target_stable << ",\"match\":" << slot.match << '}';
        }
        std::cout << "]}";
    } else std::cout << "null";
    std::cout << ",\"codecObservation\":";
    if (codec_mode) {
        std::cout << "{\"executionPolicy\":\"" << saex::engine::reviewed_codec_policy_id
            << "\",\"executionPolicySourceDigest\":\"" << saex::engine::reviewed_codec_policy_digest
            << "\",\"startupBodyExecutionAllowed\":true,\"breakpointArmed\":" << trace.codec_breakpoint_armed
            << ",\"continued\":" << trace.codec_continued << ",\"reached\":" << trace.codec_reached
            << ",\"shapeVerified\":" << trace.codec_shape_verified << ",\"modulesVerified\":" << trace.codec_modules_verified
            << ",\"returnAddress\":" << trace.codec_return_address << ",\"moduleHandle\":" << trace.codec_handle
            << ",\"armEvent\":" << trace.codec_arm_event << ",\"moduleCount\":" << trace.codec_module_count << ",\"mappingIds\":[";
        for (std::size_t i = 0; i < trace.codec_mapping_ids.size(); ++i) {
            if (i) std::cout << ',';
            std::cout << trace.codec_mapping_ids[i];
        }
        std::cout << "]}";
    } else std::cout << "null";
    std::cout << ",\"entryObservation\":";
    if (entry_mode) {
        std::cout << "{\"executionPolicy\":\"" << saex::engine::reviewed_entry_policy_id
            << "\",\"executionPolicySourceDigest\":\"" << saex::engine::reviewed_entry_policy_digest << "\","
            << "\"dllInitializationAllowed\":true,\"breakpointArmed\":" << trace.entry_breakpoint_armed
            << ",\"initialBreakpointContinued\":" << trace.initial_breakpoint_continued
            << ",\"boundaryReached\":" << trace.entry_reached << ",\"address\":" << trace.entry_address
            << ",\"bytesRead\":" << trace.entry_bytes_read << ",\"bytesMatch\":" << trace.entry_bytes_match
            << ",\"beforeHex\":\"" << hex(trace.entry_before) << "\",\"afterHex\":\"" << hex(trace.entry_after) << "\"}";
    } else std::cout << "null";
    std::cout << ",\"proxyObservation\":";
    if (proxy_mode) {
        std::cout << "{\"executionPolicy\":\"" << saex::engine::reviewed_proxy_policy_id
            << "\",\"executionPolicySourceDigest\":\"" << saex::engine::reviewed_proxy_policy_digest
            << "\",\"proxyExecutionAllowed\":true,\"validated\":" << trace.proxy_validated
            << ",\"breakpointArmed\":" << trace.proxy_breakpoint_armed << ",\"continued\":" << trace.proxy_continued
            << ",\"returnReached\":" << trace.proxy_return_reached << ",\"entryRestored\":" << trace.proxy_entry_restored
            << ",\"iatVerified\":" << trace.proxy_iat_verified << ",\"moduleBase\":" << trace.proxy_base
            << ",\"mappingId\":" << trace.proxy_mapping_id << ",\"returnAddress\":" << trace.proxy_return_address
            << ",\"iatRva\":" << saex::engine::reviewed_proxy_spec.iat_rva << ",\"iatBefore\":" << trace.proxy_iat_before
            << ",\"iatAfter\":" << trace.proxy_iat_after << ",\"entryAfterHex\":\"" << hex(trace.proxy_entry_after) << "\"}";
    } else std::cout << "null";
    std::cout << ",\"startupObservation\":";
    if (startup_mode) {
        std::cout << "{\"executionPolicy\":\"" << saex::engine::reviewed_startup_policy_id
            << "\",\"executionPolicySourceDigest\":\"" << saex::engine::reviewed_startup_policy_digest
            << "\",\"entryExecutionAllowed\":true,\"breakpointArmed\":" << trace.startup_breakpoint_armed
            << ",\"continued\":" << trace.startup_continued << ",\"reached\":" << trace.startup_reached
            << ",\"iatWriteObserved\":" << trace.startup_iat_write_observed << ",\"targetStable\":" << trace.startup_target_stable
            << ",\"callsiteVerified\":" << trace.startup_callsite_verified << ",\"argumentValid\":" << trace.startup_argument_valid
            << ",\"address\":" << trace.startup_address << ",\"returnAddress\":" << trace.startup_return_address
            << ",\"argumentAddress\":" << trace.startup_argument_address << ",\"targetBeforeHex\":\"" << hex(trace.startup_target_before)
            << "\",\"targetAfterHex\":\"" << hex(trace.startup_target_after) << "\",\"samples\":[";
        for (std::uint32_t i = 0; i < trace.startup_sample_count; ++i) {
            if (i) std::cout << ',';
            const auto& sample = trace.startup_samples[i];
            std::cout << "{\"rva\":" << sample.rva << ",\"length\":" << sample.length << ",\"read\":" << sample.read
                << ",\"match\":" << sample.match << ",\"beforeHex\":\"" << hex(sample.before).substr(0, sample.length * 2)
                << "\",\"afterHex\":\"" << hex(sample.after).substr(0, sample.length * 2) << "\"}";
        }
        std::cout << "]}";
    } else std::cout << "null";
    std::cout << ",\"modules\":[";
    for (std::uint32_t i = 0; i < trace.module_count; ++i) {
        if (i) std::cout << ',';
        const auto& module = trace.modules[i];
        std::cout << "{\"name\":\"" << module.file.name.data() << "\",\"sha256\":\"" << hex(module.file.sha256)
            << "\",\"volume\":" << module.file.volume << ",\"fileId\":\"" << hex(module.file.file_id)
            << "\",\"bytes\":" << module.file.bytes << ",\"base\":" << module.base << ",\"eventIndex\":" << module.event_index
            << ",\"identityRead\":" << module.identity_read << ",\"admitted\":" << module.admitted
            << ",\"mappingId\":" << module.mapping_id << ",\"unloadEventIndex\":" << module.unload_event_index
            << ",\"activeAtObservationEnd\":" << (module.mapping_id != 0 && module.unload_event_index == 0) << '}';
    }
    std::cout << "]}\n";
}
}
int wmain(int argc, wchar_t** argv) {
    const bool acquire_mode = argc == 4 && std::wstring_view(argv[1]) == L"--observe-cwd-acquire";
    const bool lock_mode = acquire_mode || (argc == 4 && std::wstring_view(argv[1]) == L"--observe-cwd-lock");
    const bool seh_mode = lock_mode || (argc == 4 && std::wstring_view(argv[1]) == L"--observe-cwd-seh");
    const bool manager_mode = seh_mode || (argc == 4 && std::wstring_view(argv[1]) == L"--observe-file-manager-entry");
    const bool prelude_mode = manager_mode || (argc == 4 && std::wstring_view(argv[1]) == L"--observe-game-prelude");
    const bool routing_mode = prelude_mode || (argc == 4 && std::wstring_view(argv[1]) == L"--observe-application-routing");
    const bool dispatch_mode = routing_mode || (argc == 4 && std::wstring_view(argv[1]) == L"--observe-event-dispatch");
    const bool instance_mode = dispatch_mode || (argc == 4 && std::wstring_view(argv[1]) == L"--observe-instance-startup");
    const bool suppression_mode = instance_mode || (argc == 4 && std::wstring_view(argv[1]) == L"--observe-platform-suppression");
    const bool platform_mode = suppression_mode || (argc == 4 && std::wstring_view(argv[1]) == L"--observe-platform-startup");
    const bool application_mode = platform_mode || (argc == 4 && std::wstring_view(argv[1]) == L"--observe-application-entry");
    const bool crt_mode = application_mode || (argc == 4 && std::wstring_view(argv[1]) == L"--observe-crt-startup");
    const bool tail_mode = crt_mode || (argc == 4 && std::wstring_view(argv[1]) == L"--observe-startup-return");
    const bool frame_mode = argc == 4 && std::wstring_view(argv[1]) == L"--observe-frame-target";
    const bool bootstrap_mode = frame_mode || (argc == 4 && std::wstring_view(argv[1]) == L"--observe-bootstrap-lifecycle");
    const bool asi_mode = tail_mode || bootstrap_mode || (argc == 4 && std::wstring_view(argv[1]) == L"--observe-asi-return");
    const bool binding_mode = asi_mode || (argc == 4 && std::wstring_view(argv[1]) == L"--observe-codec-bindings");
    const bool codec_mode = binding_mode || (argc == 4 && std::wstring_view(argv[1]) == L"--observe-codec-return");
    const bool startup_mode = codec_mode || (argc == 4 && std::wstring_view(argv[1]) == L"--observe-startup-call");
    const bool proxy_mode = startup_mode || (argc == 4 && std::wstring_view(argv[1]) == L"--observe-proxy-return");
    const bool entry_mode = proxy_mode || (argc == 4 && std::wstring_view(argv[1]) == L"--observe-entry-boundary");
    const bool controlled = entry_mode || (argc == 4 && std::wstring_view(argv[1]) == L"--observe-context-loader");
    if (!controlled && (argc != 3 || (std::wstring_view(argv[1]) != L"--observe-loader" && std::wstring_view(argv[1]) != L"--observe-reviewed-loader"))) {
        std::cerr << "Usage: saex_engine_loader_probe --observe-loader|--observe-reviewed-loader <gta_sa.exe>\n"
            << "       saex_engine_loader_probe --observe-context-loader|--observe-entry-boundary|--observe-proxy-return|--observe-startup-call|--observe-codec-return|--observe-codec-bindings|--observe-asi-return|--observe-bootstrap-lifecycle|--observe-frame-target|--observe-startup-return|--observe-crt-startup|--observe-application-entry|--observe-platform-startup|--observe-platform-suppression|--observe-instance-startup|--observe-event-dispatch|--observe-application-routing|--observe-game-prelude|--observe-file-manager-entry|--observe-cwd-seh|--observe-cwd-lock|--observe-cwd-acquire <gta_sa.exe> <absolute-working-directory>\n"; return 2;
    }
    using namespace saex::engine;
    const bool reviewed = controlled || std::wstring_view(argv[1]) == L"--observe-reviewed-loader";
    std::uint32_t pid{};
    bool exit_confirmed{};
    std::unique_ptr<LaunchContext> context;
    const auto emit = [&](const LoaderTrace& trace, std::uint32_t child_pid, std::string_view hash, std::string_view failed = {}) {
        print(trace, child_pid, hash, reviewed, failed, context.get(), entry_mode, proxy_mode, startup_mode, codec_mode, binding_mode, asi_mode, bootstrap_mode, frame_mode, tail_mode, crt_mode, application_mode, platform_mode, suppression_mode, instance_mode, dispatch_mode, routing_mode, prelude_mode, manager_mode, seh_mode, lock_mode, acquire_mode);
    };
    try {
        if (controlled) {
            context = std::make_unique<LaunchContext>(argv[3]);
            if (!context->valid()) { LoaderTrace result{}; result.reason = context->error(); emit(result, 0, ""); return 1; }
        }
        std::array<wchar_t, 32768> buffer{};
        const auto absolute_size = GetFullPathNameW(argv[2], static_cast<DWORD>(buffer.size()), buffer.data(), nullptr);
        if (!absolute_size || absolute_size > 32764) { LoaderTrace result{}; result.reason = "loader_executable_path"; emit(result, 0, ""); return 1; }
        const std::wstring executable_path(buffer.data());
        WindowsFileObservation executable(executable_path.c_str());
        if (!executable.error().empty()) { LoaderTrace result{}; result.reason = executable.error(); emit(result, 0, ""); return 1; }
        const auto hash = hex(executable.hash());
        EntryStopSpec entry{};
        if (entry_mode) {
            entry.rva = executable.layout().entry_rva;
            const auto offset = raw_offset(executable.layout(), entry.rva, entry.expected.size());
            if (!offset) { LoaderTrace result{}; result.reason = "entry_file_range"; emit(result, 0, hash); return 1; }
            std::copy_n(executable.bytes().begin() + *offset, entry.expected.size(), entry.expected.begin());
        }
        const auto size = GetSystemDirectoryW(buffer.data(), static_cast<UINT>(buffer.size()));
        if (!size || size >= buffer.size()) { LoaderTrace result{}; result.reason = "loader_system_directory"; emit(result, 0, hash); return 1; }
        // In this x86 process Windows resolves System32 to the x86 system directory.
        // These three retained files permit mapping observation only. No local DLLs,
        // arbitrary OS directory wildcard, server-provided policy or mod allowlist.
        const std::wstring system(buffer.data());
        std::array<std::unique_ptr<LoaderFile>, 3> basic_files{};
        std::array<const LoaderFile*, 3> basic_pins{};
        std::unique_ptr<PreparedLoaderPolicy> prepared;
        std::span<const LoaderFile* const> pins;
        std::vector<LoaderPinSpec> asi_specs;
        std::string asi_path;
        if (reviewed) {
            const auto separator = executable_path.find_last_of(L"\\/");
            const auto game = executable_path.substr(0, separator == 2 ? 3 : separator);
            auto specs = codec_mode ? std::span<const LoaderPinSpec>(reviewed_codec_specs)
                : entry_mode ? std::span<const LoaderPinSpec>(reviewed_entry_specs) : std::span<const LoaderPinSpec>(reviewed_loader_specs);
            if (asi_mode) {
                if (CompareStringOrdinal(game.c_str(), -1, context->directory().c_str(), -1, TRUE) != CSTR_EQUAL) {
                    LoaderTrace result{}; result.reason = "asi_context_mismatch"; emit(result, 0, hash); return 1;
                }
                const auto path = game + L"\\saex_bootstrap.asi";
                if (path.size() >= 260 || std::any_of(path.begin(), path.end(), [](wchar_t c) { return c < 32 || c > 126; })) {
                    LoaderTrace result{}; result.reason = "asi_path_encoding_or_length"; emit(result, 0, hash); return 1;
                }
                for (const auto ch : path) asi_path.push_back(static_cast<char>(ch));
                asi_specs.assign(specs.begin(), specs.end());
                asi_specs.insert(asi_specs.end(), reviewed_asi_commonModules.begin(), reviewed_asi_commonModules.end());
#if defined(_DEBUG)
                asi_specs.insert(asi_specs.end(), reviewed_asi_debugModules.begin(), reviewed_asi_debugModules.end());
#endif
                asi_specs.push_back(bootstrap_artifact_pin);
                if (bootstrap_mode) asi_specs.push_back(reviewed_bootstrap_crypto_pin);
                specs = asi_specs;
            }
            prepared = std::make_unique<PreparedLoaderPolicy>(specs, reviewed_loader_engine_hash, executable.hash(), game, system, asi_mode);
            if (!prepared->error().empty()) {
                LoaderTrace result{}; result.reason = prepared->error(); emit(result, 0, hash, prepared->failed_module()); return 1;
            }
            pins = prepared->pins();
        } else {
            constexpr std::array names{L"ntdll.dll", L"kernel32.dll", L"kernelbase.dll"};
            for (std::size_t i = 0; i < names.size(); ++i) {
                basic_files[i] = std::make_unique<LoaderFile>((system + L"\\" + names[i]).c_str());
                if (!basic_files[i]->valid()) { LoaderTrace result{}; result.reason = "loader_system_pin_failed"; emit(result, 0, hash); return 1; }
                basic_pins[i] = basic_files[i].get();
            }
            pins = basic_pins;
        }
        auto proxy = reviewed_proxy_spec;
        if (proxy_mode) {
            for (const auto pin : pins)
                if (std::string_view(pin->identity().name.data()) == reviewed_proxy_module) proxy.module = pin;
            if (!proxy.module) { LoaderTrace result{}; result.reason = "proxy_policy_module_missing"; emit(result, 0, hash); return 1; }
        }
        auto codec = reviewed_codec_spec;
        auto asi = reviewed_asi_spec;
        const BootstrapLifecycleSpec bootstrap{bootstrap_artifact_exports, observed_profile.id, observed_profile_source_digest};
        if (asi_mode) {
            asi.requested_path = asi_path;
            for (const auto pin : pins) if (std::string_view(pin->identity().name.data()) == reviewed_asi_artifact_name) asi.module = pin;
            if (!asi.module) { LoaderTrace result{}; result.reason = "asi_policy_module_missing"; emit(result, 0, hash); return 1; }
        }
        std::array<const LoaderFile*, 3> codec_pins{};
        if (codec_mode) {
            for (std::size_t i = 0; i < reviewed_codec_modules.size(); ++i) {
                for (const auto pin : pins)
                    if (std::string_view(pin->identity().name.data()) == reviewed_codec_modules[i]) codec_pins[i] = pin;
                if (!codec_pins[i]) { LoaderTrace result{}; result.reason = "codec_policy_module_missing"; emit(result, 0, hash); return 1; }
            }
            codec.modules = codec_pins;
        }
        LoaderTrace trace{};
        {
            SuspendedImage child(executable_path.c_str(), executable.layout().image_size, context.get());
            pid = child.process_id();
            auto reason = child.error();
            if (reason.empty() && !child.same_file(executable.handle())) reason = "observer_file_identity";
            if (reason.empty() && child.image_base() != executable.layout().image_base) reason = "observer_image_base";
            if (reason.empty()) {
                const auto check = check_mapped_observation(child, executable.bytes(), executable.layout(), observed_profile);
                if (check != ProfileResult::matched_observation) reason = profile_result_name(check);
            }
            // One return slot avoids a large temporary per ternary branch in MSVC /Od.
            if (reason.empty()) trace = [&]() -> LoaderTrace {
                if (acquire_mode) return LoaderObservation::run_cwd_acquire(child,executable.handle(),pins,entry,proxy,reviewed_startup_spec,codec,reviewed_binding_spec,asi,reviewed_startup_return_spec,reviewed_crt_spec,reviewed_application_spec,reviewed_platform_spec,reviewed_suppression_spec,reviewed_instance_spec,reviewed_dispatch_spec,reviewed_routing_spec,reviewed_prelude_spec,reviewed_file_manager_spec,reviewed_cwd_seh_spec,reviewed_cwd_lock_spec,reviewed_cwd_acquire_spec);
                if (lock_mode) return LoaderObservation::run_cwd_lock(child,executable.handle(),pins,entry,proxy,reviewed_startup_spec,codec,reviewed_binding_spec,asi,reviewed_startup_return_spec,reviewed_crt_spec,reviewed_application_spec,reviewed_platform_spec,reviewed_suppression_spec,reviewed_instance_spec,reviewed_dispatch_spec,reviewed_routing_spec,reviewed_prelude_spec,reviewed_file_manager_spec,reviewed_cwd_seh_spec,reviewed_cwd_lock_spec);
                if (seh_mode) return LoaderObservation::run_cwd_seh(child,executable.handle(),pins,entry,proxy,reviewed_startup_spec,codec,reviewed_binding_spec,asi,reviewed_startup_return_spec,reviewed_crt_spec,reviewed_application_spec,reviewed_platform_spec,reviewed_suppression_spec,reviewed_instance_spec,reviewed_dispatch_spec,reviewed_routing_spec,reviewed_prelude_spec,reviewed_file_manager_spec,reviewed_cwd_seh_spec);
                if (manager_mode) return LoaderObservation::run_file_manager_entry(child,executable.handle(),pins,entry,proxy,reviewed_startup_spec,codec,reviewed_binding_spec,asi,reviewed_startup_return_spec,reviewed_crt_spec,reviewed_application_spec,reviewed_platform_spec,reviewed_suppression_spec,reviewed_instance_spec,reviewed_dispatch_spec,reviewed_routing_spec,reviewed_prelude_spec,reviewed_file_manager_spec);
                if (prelude_mode) return LoaderObservation::run_game_prelude(child,executable.handle(),pins,entry,proxy,reviewed_startup_spec,codec,reviewed_binding_spec,asi,reviewed_startup_return_spec,reviewed_crt_spec,reviewed_application_spec,reviewed_platform_spec,reviewed_suppression_spec,reviewed_instance_spec,reviewed_dispatch_spec,reviewed_routing_spec,reviewed_prelude_spec);
                if (routing_mode) return LoaderObservation::run_application_routing(child,executable.handle(),pins,entry,proxy,reviewed_startup_spec,codec,reviewed_binding_spec,asi,reviewed_startup_return_spec,reviewed_crt_spec,reviewed_application_spec,reviewed_platform_spec,reviewed_suppression_spec,reviewed_instance_spec,reviewed_dispatch_spec,reviewed_routing_spec);
                if (dispatch_mode) return LoaderObservation::run_event_dispatch(child,executable.handle(),pins,entry,proxy,reviewed_startup_spec,codec,reviewed_binding_spec,asi,reviewed_startup_return_spec,reviewed_crt_spec,reviewed_application_spec,reviewed_platform_spec,reviewed_suppression_spec,reviewed_instance_spec,reviewed_dispatch_spec);
                if (instance_mode) return LoaderObservation::run_instance_startup(child,executable.handle(),pins,entry,proxy,reviewed_startup_spec,codec,reviewed_binding_spec,asi,reviewed_startup_return_spec,reviewed_crt_spec,reviewed_application_spec,reviewed_platform_spec,reviewed_suppression_spec,reviewed_instance_spec);
                if (suppression_mode) return LoaderObservation::run_platform_suppression(child,executable.handle(),pins,entry,proxy,reviewed_startup_spec,codec,reviewed_binding_spec,asi,reviewed_startup_return_spec,reviewed_crt_spec,reviewed_application_spec,reviewed_platform_spec,reviewed_suppression_spec);
                if (platform_mode) return LoaderObservation::run_to_platform_startup(child,executable.handle(),pins,entry,proxy,reviewed_startup_spec,codec,reviewed_binding_spec,asi,reviewed_startup_return_spec,reviewed_crt_spec,reviewed_application_spec,reviewed_platform_spec);
                if (application_mode) return LoaderObservation::run_to_application_entry(child,executable.handle(),pins,entry,proxy,reviewed_startup_spec,codec,reviewed_binding_spec,asi,reviewed_startup_return_spec,reviewed_crt_spec,reviewed_application_spec);
                if (crt_mode) return LoaderObservation::run_to_crt_startup(child,executable.handle(),pins,entry,proxy,reviewed_startup_spec,codec,reviewed_binding_spec,asi,reviewed_startup_return_spec,reviewed_crt_spec);
                if (tail_mode) return LoaderObservation::run_to_startup_return(child,executable.handle(),pins,entry,proxy,reviewed_startup_spec,codec,reviewed_binding_spec,asi,reviewed_startup_return_spec);
                if (frame_mode) return LoaderObservation::run_frame_target_observation(child, executable.handle(), pins, entry, proxy, reviewed_startup_spec, codec, reviewed_binding_spec, asi, bootstrap, reviewed_frame_spec);
                if (bootstrap_mode) return LoaderObservation::run_bootstrap_lifecycle(child, executable.handle(), pins, entry, proxy, reviewed_startup_spec, codec, reviewed_binding_spec, asi, bootstrap);
                if (asi_mode) return LoaderObservation::run_to_asi_return(child, executable.handle(), pins, entry, proxy, reviewed_startup_spec, codec, reviewed_binding_spec, asi);
                if (binding_mode) return LoaderObservation::run_to_codec_bindings(child, executable.handle(), pins, entry, proxy, reviewed_startup_spec, codec, reviewed_binding_spec);
                if (codec_mode) return LoaderObservation::run_to_codec_return(child, executable.handle(), pins, entry, proxy, reviewed_startup_spec, codec);
                if (startup_mode) return LoaderObservation::run_to_startup_call(child, executable.handle(), pins, entry, proxy, reviewed_startup_spec);
                if (proxy_mode) return LoaderObservation::run_to_proxy_return(child, executable.handle(), pins, entry, proxy);
                if (entry_mode) return LoaderObservation::run_to_entry(child, executable.handle(), pins, entry);
                return LoaderObservation::run(child, executable.handle(), pins);
            }();
            else { trace.reason = reason; trace.exit_confirmed = child.created() && child.stop(); }
            exit_confirmed = trace.exit_confirmed;
        } // Cleanup and all borrowed handles remain valid before output allocation/IO.
        emit(trace, pid, hash);
        const bool complete = acquire_mode ? trace.reason == "cwd_acquire_verified" : lock_mode ? trace.reason == "cwd_lock_verified" : seh_mode ? trace.reason == "cwd_seh_verified" : manager_mode ? trace.reason == "file_manager_entry_verified" : prelude_mode ? trace.reason == "game_prelude_verified" : routing_mode ? trace.reason == "application_routing_boundary_verified" : dispatch_mode ? trace.reason == "event_dispatch_boundary_verified" : instance_mode ? trace.reason == "instance_startup_verified" : suppression_mode ? trace.reason == "platform_suppression_verified" : platform_mode ? trace.reason == "platform_startup_boundary_verified" : application_mode ? trace.reason == "application_entry_verified" : crt_mode ? trace.reason == "crt_initializer_boundary_verified" : tail_mode ? trace.reason == "startup_return_verified" : frame_mode ? trace.reason == "frame_target_samples_verified" : bootstrap_mode ? trace.reason == "bootstrap_lifecycle_verified" : asi_mode ? trace.reason == "asi_return_verified" : binding_mode ? trace.reason == "codec_bindings_verified" : codec_mode ? trace.reason == "codec_return_verified" : startup_mode ? trace.reason == "startup_call_verified" : proxy_mode ? trace.reason == "proxy_return_verified"
            : entry_mode ? (trace.entry_reached && trace.entry_bytes_read) : trace.breakpoint_candidate;
        return complete && trace.exit_confirmed ? 3 : 1;
    } catch (const std::exception&) {
        LoaderTrace result{}; result.reason = "loader_probe_exception"; result.exit_confirmed = exit_confirmed;
        emit(result, pid, ""); return 1;
    }
}
