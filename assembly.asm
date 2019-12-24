.386
.model flat, c

extrn base:DWORD
extrn jmp_addr:DWORD
extrn speed:DWORD
extrn ret_speed:DWORD
extrn jump:DWORD
extrn ret_jump:DWORD
extrn gravity:DWORD
extrn ret_gravity:DWORD
extrn ret_tank_controls:DWORD
extrn addr_player_1:DWORD
extrn addr_player_2:DWORD
extrn pitch:DWORD
extrn ret_camera_pitch:DWORD

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

tank_controls proc export
  add   eax, [ecx + 20Ch]
  push  eax
  mov   eax, addr_player_1
  cmp   esi, [eax]
  je    adj
  mov   eax, addr_player_2
  cmp   esi, [eax]
  je    adj
  
  pop   eax
  jmp   ret_tank_controls
  
  adj:
  pop   eax
  add   eax, 8000
  jmp   ret_tank_controls
tank_controls endp

camera_pitch proc export
  push  eax
  mov   eax, 400000h + 0B71F84h
  cmp   ebp, [eax]
  je    pitch_orig
  
  pop   eax
  mov   ebx, pitch
  jmp   ret_camera_pitch
  
  pitch_orig:
  pop   eax
  mov   ebx, [ebp + 208h]
  jmp   ret_camera_pitch
camera_pitch endp

end