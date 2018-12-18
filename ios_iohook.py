import os
import lldb
import sys

#command script import /Users/alexander/Documents/workspace/programming/projects/osx/fuzz_ioctl/ios_iohook.py

CLANG="~/Documents/workspace/builds/llvm/build/bin/clang"
GOBJCOPY="~/Documents/workspace/bin/toolchains/aarch64-linux-android-4.9-master/bin/aarch64-linux-android-objcopy"

TMP_ASM_NAME="/tmp/test.S"
TMP_OBJ_NAME="/tmp/test.o"
TMP_BIN_NAME="/tmp/test.bin"

SELF_PATH=sys.path[1]

def MakeShellcode(addr=0):
    with open(SELF_PATH + '/' + 'temp_trampoline.S') as f:
        assembly = str.join('', f.readlines())
        HI_HI = addr >> 48
        HI_LO = (addr >> 32) & 0xffff
        LO_HI = (addr >> 16) & 0xffff
        LO_LO = addr & 0xffff

        assembly = assembly.replace("HI_HI", hex(HI_HI))
        assembly = assembly.replace("HI_LO", hex(HI_LO))
        assembly = assembly.replace("LO_HI", hex(LO_HI))
        assembly = assembly.replace("LO_LO", hex(LO_LO))

        with open("/tmp/test.S", "w") as o:
            o.write(assembly)
            o.flush()

            CMD=CLANG+" -march=armv8.3a -target aarch64--linux -c -o " + TMP_OBJ_NAME + " " + TMP_ASM_NAME
            print(CMD)
            os.system(CMD)

            CMD=GOBJCOPY+" -I elf64-littleaarch64 -O binary " + TMP_OBJ_NAME + " " + TMP_BIN_NAME
            print(CMD)
            os.system(CMD)

def DoHook(lldb_debugger):
    lldb_target = lldb_debugger.GetTargetAtIndex(0)
    lldb_process = lldb_target.GetProcess()

    # these are not initialized if the script is launched with "command script import"
    # and the user has not previously invoked "script"
    lldb.target = lldb_target
    lldb.process = lldb_process

    real_io_connect_method = lldb_target.FindFunctions("io_connect_method")[0].symbol.addr.load_addr
#>>> hex(i.symbol.addr.load_addr)
#'0x22e2be3b4'

    assign_orig_io_connect_method = "orig_io_connect_method=(typeof(orig_io_connect_method))" + hex(real_io_connect_method)
    lldb_target.EvaluateExpression(assign_orig_io_connect_method)

    hook_IOConnectCallMethod = lldb_target.FindSymbols("fake_IOConnectCallMethod")[0].symbol.addr.load_addr
#>>> hex(hook_IOConnectCallMethod)
#'0x102382f34'

    #TODO: why does load_addr sometimes return -1?
    #maybe move this to a function?
    orig_IOConnectCallMethods = lldb_target.FindSymbols("IOConnectCallMethod")
    for i in range(orig_IOConnectCallMethods.GetSize()):
        iocm = orig_IOConnectCallMethods[i]
        addr = iocm.symbol.addr.load_addr
        addr = int(addr)
        if (addr & 0xffff) == 0xffff:
            next
        else:
            orig_IOConnectCallMethod = addr
            break

#orig_IOConnectCallMethod = lldb_target.FindSymbols("IOConnectCallMethod")[0].symbol.addr.load_addr
#>>> hex(orig_IOConnectCallMethod)
#'0x22e254f18'

    MakeShellcode(hook_IOConnectCallMethod)
    expr_patch_IOConnectCallMethod = "memory write -i " + TMP_BIN_NAME + " " + hex(orig_IOConnectCallMethod)
    lldb_debugger.HandleCommand(expr_patch_IOConnectCallMethod)
    lldb_process.Continue()

def __lldb_init_module(debugger, internal_dict):
    print 'lldb init module'
    DoHook(debugger)

if __name__ == "__main__":
    DoHook(lldb.debugger)
