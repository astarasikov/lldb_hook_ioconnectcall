pacibsp 
sub    sp, sp, #0xe0
stp    x20, x19, [sp, #0xd0]
stp    x29, x30, [sp, #0xc0]
stp    x9,  x10, [sp, #0xb0]

add    x29, sp, #0xe0
ldp    x9,  x10, [x29]
stp    x9,  x10, [sp]

movz   w10, #HI_LO
movk   w10, #HI_HI, LSL #16

lsl    x9, x10, #32
movz   w10, #LO_LO
movk   w10, #LO_HI, LSL #16
orr    x9, x9, x10

blr     x9

ldp    x9,  x10, [sp, #0xb0]
ldp    x29, x30, [sp, #0xc0]
ldp    x20, x19, [sp, #0xd0]
add    sp, sp, #0xe0
retab
