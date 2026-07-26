# asm_on_update(asm_state* s, double dt)
# SysV: rdi = s, xmm0 = dt
# struct { double t, x, y, vx, vy; } @ 0,8,16,24,32

.global asm_on_update
.text
asm_on_update:
    # t += dt
    movsd (%rdi), %xmm1
    addsd %xmm0, %xmm1
    movsd %xmm1, (%rdi)

    # x += vx * dt
    movsd 24(%rdi), %xmm2
    mulsd %xmm0, %xmm2
    movsd 8(%rdi), %xmm3
    addsd %xmm2, %xmm3
    movsd %xmm3, 8(%rdi)

    # y += vy * dt
    movsd 32(%rdi), %xmm2
    mulsd %xmm0, %xmm2
    movsd 16(%rdi), %xmm3
    addsd %xmm2, %xmm3
    movsd %xmm3, 16(%rdi)
    ret

.section .note.GNU-stack,"",@progbits
