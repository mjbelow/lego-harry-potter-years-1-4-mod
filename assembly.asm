.386
.model flat, c

extrn jmp_addr:DWORD
extrn speed:DWORD
extrn ret_speed:DWORD
extrn jump:DWORD
extrn ret_jump:DWORD
extrn gravity:DWORD
extrn ret_gravity:DWORD

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

jump_hack proc export
  fld   dword ptr [jump]
  and   ebx, 101h
  jmp   dword ptr [ret_jump]
jump_hack endp

gravity_hack proc export
  fld   dword ptr [gravity]
  jmp   dword ptr [ret_gravity]
gravity_hack endp

end