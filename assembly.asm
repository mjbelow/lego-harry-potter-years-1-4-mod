.386
.model flat, c

extrn jmp_addr:DWORD
extrn ret_speed:DWORD
extrn speed:DWORD

.code

my_jump proc export
  add   dword ptr [esi+0001D5F8h], 7h
  jmp   dword ptr [jmp_addr]
my_jump endp

speed_hack proc export
  fmul  dword ptr [speed]
  fstp  dword ptr [esp + 74h]
  jmp   dword ptr [ret_speed]
speed_hack endp

end