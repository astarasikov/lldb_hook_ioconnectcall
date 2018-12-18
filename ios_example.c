#import <UIKit/UIKit.h>
#import "AAPLAppDelegate.h"

volatile kern_return_t (*orig_io_connect_method)
(
 mach_port_t connection,
 uint32_t selector,
 uint64_t input,
 mach_msg_type_number_t inputCnt,
 uint64_t inband_input,
 mach_msg_type_number_t inband_inputCnt,
 mach_vm_address_t ool_input,
 mach_vm_size_t  ool_input_size __unused,
 uint64_t output,
 void *outputCnt,
 uint64_t inband_output,
 void *inband_outputCnt,
 mach_vm_address_t ool_output,
 void * ool_output_size __unused
 );

typedef char io_struct_inband_t[4096];

static inline kern_return_t
local_IOConnectCallMethod(
                          mach_port_t     connection,        // In
                          uint32_t     selector,        // In
                          const uint64_t    *input,            // In
                          uint32_t     inputCnt,        // In
                          const void    *inputStruct,        // In
                          size_t         inputStructCnt,    // In
                          uint64_t    *output,        // Out
                          uint32_t    *outputCnt,        // In/Out
                          void        *outputStruct,        // Out
                          size_t        *outputStructCntP)    // In/Out
{
    kern_return_t            rtn = -1U;
    
    void            *inb_input       = 0;
    mach_msg_type_number_t     inb_input_size  = 0;
    void            *inb_output      = 0;
    mach_msg_type_number_t     inb_output_size = 0;
    
    mach_vm_address_t         ool_input       = 0;
    mach_vm_size_t         ool_input_size  = 0;
    mach_vm_address_t         ool_output      = 0;
    mach_vm_size_t         ool_output_size = 0;
    void*                 var_output      = NULL;
    mach_msg_type_number_t       var_output_size = 0;
    
    if (inputStructCnt <= sizeof(io_struct_inband_t)) {
        inb_input      = (void *) inputStruct;
        inb_input_size = (mach_msg_type_number_t) inputStructCnt;
    }
    else {
        ool_input      = (mach_vm_address_t)(inputStruct);
        ool_input_size = inputStructCnt;
    }
    
    if (!outputCnt) {
        static uint32_t zero[10] = {};
        outputCnt = zero;
    }
    
    if (outputStructCntP) {
        size_t size = *outputStructCntP;
        //printf("%s: size=%ld\n", __func__, size);
        
#define kIOConnectMethodVarOutputSize -3
        
        if (size == (size_t) kIOConnectMethodVarOutputSize) {
            puts("YOLO");
            exit(-1);
        }
        else if (size <= sizeof(io_struct_inband_t)) {
            inb_output      = outputStruct;
            inb_output_size = (mach_msg_type_number_t) size;
        }
        else {
            ool_output      = (mach_vm_address_t)(outputStruct);
            ool_output_size = (mach_vm_size_t)    size;
        }
    }
    
    //printf("%s: before call connection=%x selector=%x\n", __func__, connection, selector);
    rtn = orig_io_connect_method(connection,         selector,
                                 (uint64_t) input, inputCnt,
                                 (uint64_t)inb_input,          inb_input_size,
                                 ool_input,          ool_input_size,
                                 (uint64_t)inb_output,         &inb_output_size,
                                 (uint64_t)output,             outputCnt,
                                 ool_output,         &ool_output_size);
    //printf("%s: done rtn=%x connection=%x selector=%x\n", __func__, rtn, connection, selector);
    
    if (outputStructCntP) {
        //printf("%s: outputStructCntP=%p\n", __func__, outputStructCntP);
        if (*outputStructCntP <= sizeof(io_struct_inband_t))
            *outputStructCntP = (size_t) inb_output_size;
        else
            *outputStructCntP = (size_t) ool_output_size;
    }
    
    return rtn;
}

extern kern_return_t
fake_IOConnectCallMethod(
                         mach_port_t connection, //rdi
                         uint32_t    selector, //rsi
                         uint64_t   *input, //rdx
                         uint32_t    inputCnt, //rcx
                         void       *inputStruct, //r8
                         size_t      inputStructCnt, //r9
                         uint64_t   *output,
                         uint32_t   *outputCnt,
                         void       *outputStruct,
                         size_t     *outputStructCntP)
{
    kern_return_t ret = 0;
    
    printf("%s: count=%d connection=%x, selector=%x, input=%p"
           ", inputCnt=%d, inputStruct=%p, inputStructCnt=%lx"
           ", output=%p outputCnt=%p outputStruct=%p outputStructCntP=%p\n",
           __func__,
           count,
           connection,
           selector,
           input,
           inputCnt,
           inputStruct,
           inputStructCnt,
           output,
           outputCnt,
           outputStruct,
           outputStructCntP);
    
    ret = local_IOConnectCallMethod(
                                    connection,
                                    selector,
                                    input,
                                    inputCnt,
                                    inputStruct,
                                    inputStructCnt,
                                    output,
                                    outputCnt,
                                    outputStruct,
                                    outputStructCntP);
    
    return ret;
}

void installHooks(void)
{
    //TODO: these are only needed to keep clang from optimizing the debugging symbols
    volatile void *a;
    a = &orig_io_connect_method;
    a = fake_IOConnectCallMethod;
}

/*
 overwrite IOConnectCallMethod entry with a jump to fake_IOConnectCallMethod
 
 fake_IOConnectCallMethod has to implement the call to
 io_connect_method itself sin ce we'll be trashing IOConnectCallMethod
 */

int main(int argc, char * argv[]) {
    //break here!!!
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AAPLAppDelegate class]));
    }
}
