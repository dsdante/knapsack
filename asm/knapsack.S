.extern n
.extern limit
.extern items

# in/out
.set in_depth,  %dil
.set in_mask,   %rsi
.set sum,       %xmm0

# cached global variables
.set _n,        %cl
.set _n_wide,   %rcx
.set _limit,    %xmm2
.set _items,    %rdi

# locals
.set depth,     %bl
.set depth_wide,%rbx
.set mask,      %rax
.set sum_b,     %xmm1
.set mask_b,    %rdx


# Knapsack calculation
#   depth: current recursion depth, i.e. the number of the considered items
#   sum: the sum of the taken items among considered
#   mask: bit mask of the taken items
#   returns the best sum for this branch of recursion
# sysv_abi double knapsack(uint8 depth, double sum, uint64* mask)
.globl knapsack
knapsack:
    push    %rbx                # Prologue
    mov     (in_mask), mask
    xor     depth_wide, depth_wide
    mov     in_depth, depth
    xor     _n_wide, _n_wide
    mov     n(%rip), _n
    movsd   limit(%rip), _limit
    lea     items(%rip), _items
    call    knapsack_recursive
    mov     mask, (in_mask)     # Epilogue
    pop     %rbx
    ret                         # The result is already in xmm0.

knapsack_recursive:
    movsd   sum, sum_b          # sum_b = sum;
    addsd   (_items,depth_wide,8), sum_b  # sum_b += items[depth];
    comisd  _limit, sum_b       # if (sum_b <= limit)
    jbe     below_limit         #     no pruning
    sub     depth, _n           # else
    shl     _n, mask            #     mask <<= n - depth;
    add     depth, _n
    ret                         # return;

below_limit:
    inc     depth               # depth++;
    shl     $1, mask            # mask <<= 1;
    lea     1(mask), mask_b     # mask_b = mask | 0x1;

    cmp     _n, depth           # if (depth == n)
    je      select_better       #     select the better one

    sub     $32, %rsp
    mov     mask_b, (%rsp)      # push b
    movsd   sum_b, 8(%rsp)
    call    knapsack_recursive  # a = knapsack(depth, sum, mask);  // Try without items[depth]
    mov     mask, 16(%rsp)      # swap a and b in the stack; we don't care for the order
    movsd   sum, 24(%rsp)
    mov     (%rsp), mask
    movsd   8(%rsp), sum
    call    knapsack_recursive  # a = routine(depth, sum_b, mask_b);  // Try with items[depth]
    mov     16(%rsp), mask_b    # pop a as b
    movsd   24(%rsp), sum_b
    add     $32, %rsp

select_better:
    comisd  sum_b, sum          # if (sum >= sum_b)
    ja      return              #     return a;
    movsd   sum_b, sum          # else
    mov     mask_b, mask        #     return b;

return:
    dec     depth               # depth--;
    ret                         # return;
