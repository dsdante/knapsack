.extern n
.extern limit
.extern items

/* in/out */
#define in_depth    %dil
#define in_mask     (%rsi)
#define sum         %xmm0

/* globals cached in registers */
#define _n          %cl
#define _n_wide     %rcx
#define _limit      %xmm2
#define _items      %rdi

/* locals */
#define depth       %bl
#define depth_wide  %rbx
#define mask        %rax
#define sum_b       %xmm1
#define mask_b      %rdx


# Knapsack calculation routine
#   depth: current recursion depth, i.e. the number of the considered items
#   sum: the sum of the taken items among considered
#   mask: the bit mask of the taken items
#   returns the best sum for this branch of recursion
# __sysv_abi double knapsack(u_int8_t depth, double sum, u_int64_t* mask)
.globl knapsack
knapsack:
    push    %rbx                # Prologue
    mov     in_mask, mask
    xor     depth_wide, depth_wide
    mov     in_depth, depth
    xor     _n_wide, _n_wide
    mov     n(%rip), _n
    movsd   limit(%rip), _limit
    lea     items(%rip), _items
    call    knapsack_recursive
    mov     mask, in_mask       # Epilogue
    pop     %rbx
    ret                         # The result is already in xmm0.

knapsack_recursive:
    movsd   sum, sum_b          # sum_b = sum;
    addsd   (_items,depth_wide,8), sum_b  # sum_b += items[depth];
    comisd  _limit, sum_b       # if (sum_b <= limit)
    jbe     below_limit         #     no pruning
    sub     depth, _n           # else
    rol     _n, mask            #     mask <<= n - depth;
    add     depth, _n
    ret                         # return;

below_limit:
    inc     depth               # depth++;
    shl     $1, mask            # mask <<= 1;
    lea     1(mask), mask_b     # mask_b = mask | 0x1;

    cmp     _n, depth           # if (depth == n)
    je      select_better       #     select the better one

    sub     $16, %rsp
    movsd   sum_b, 8(%rsp)      # push sum_b
    mov     mask_b, (%rsp)      # push mask_b
    call    knapsack_recursive  # a = knapsack(depth, sum, mask);  // Try without items[depth]
    mov     (%rsp), mask_b      # pop mask_b
    movsd   8(%rsp), sum_b      # pop sum_b
    add     $16, %rsp

    xchg    mask, mask_b        # swap a and b; we don't care for the order
    xorpd   sum, sum_b
    xorpd   sum_b, sum
    xorpd   sum, sum_b
    sub     $16, %rsp
    movsd   sum_b, 8(%rsp)      # push sum_b
    mov     mask_b, (%rsp)      # push mask_b
    call    knapsack_recursive  # a = routine(depth, sum, mask);  // try with items[depth]
    mov     (%rsp), mask_b      # pop mask_b
    movsd   8(%rsp), sum_b      # pop sum_b
    add     $16, %rsp

select_better:
    comisd  sum_b, sum          # if (sum >= sum_b)
    ja      return              #     return a;
    movsd   sum_b, sum          # else
    mov     mask_b, mask        #     return b;

return:
    dec     depth               # depth--;
    ret                         # return;
