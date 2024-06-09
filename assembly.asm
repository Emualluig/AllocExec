comiss xmm0, CONSTANT_(0)
jbe .L0
comiss xmm0, CONSTANT_(-3)
jbe .L1
mov eax, -1
ret
.L1
mov eax, 0
ret
.L0
comiss xmm0, CONSTANT_(1)
jbe .L2
mov eax, 1
ret
.L2
mov eax, -1
ret