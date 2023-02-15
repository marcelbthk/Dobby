#include "platform_macro.h"
#if defined(TARGET_ARCH_ARM64)

#include "dobby_internal.h"

#include "core/assembler/assembler-arm64.h"
#include "core/codegen/codegen-arm64.h"

#include "MemoryAllocator/NearMemoryAllocator.h"
#include "InstructionRelocation/arm64/InstructionRelocationARM64.h"
#include "InterceptRouting/RoutingPlugin/RoutingPlugin.h"

using namespace zz::arm64;

extern bool g_inject_valid_prologue;

CodeBufferBase *GenerateNormalTrampolineBuffer(addr_t from, addr_t to) {
  TurboAssembler turbo_assembler_((void *)from);
#define _ turbo_assembler_.

  if (g_inject_valid_prologue) {
    uint8_t fake_prologue[] = {
        0xff, 0xc3, 0x00, 0xd1, // sub sp,sp,0x30
        0xff, 0xc3, 0x00, 0x91  // add sp,sp,0x30
    };

    for (auto b : fake_prologue) {
      turbo_assembler_.GetCodeBuffer()->Emit8(b);
    }
  }

  uint64_t distance = llabs((int64_t)(from - to));
  uint64_t adrp_range = ((uint64_t)1 << (2 + 19 + 12 - 1));
  if (distance < adrp_range) {
    // adrp, add, br
    _ AdrpAdd(TMP_REG_0, from, to);
    _ br(TMP_REG_0);
    DLOG(0, "[trampoline] use [adrp, add, br]");
  } else {
    // ldr, br, branch-address
    CodeGen codegen(&turbo_assembler_);
    codegen.LiteralLdrBranch((uint64_t)to);
    DLOG(0, "[trampoline] use [ldr, br, #label]");
  }
#undef _

  // Bind all labels
  turbo_assembler_.RelocBind();

  auto result = turbo_assembler_.GetCodeBuffer()->Copy();
  return result;
}

#endif
