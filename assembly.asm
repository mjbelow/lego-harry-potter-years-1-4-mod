.386
.xmm

.model flat, c

x_cam PROTO C rotate:WORD, x:DWORD
y_cam PROTO C rotate:WORD, y:DWORD

extrn base:DWORD
extrn jmp_addr:DWORD
extrn speed:DWORD
extrn ret_speed:DWORD
extrn jump:DWORD
extrn ret_jump:DWORD
extrn gravity:DWORD
extrn ret_gravity:DWORD
extrn tank:DWORD
extrn ret_tank_controls:DWORD
extrn addr_player_1:DWORD
extrn addr_player_2:DWORD
extrn pitch:DWORD
extrn ret_camera_pitch:DWORD
extrn yaw:DWORD
extrn ret_camera_yaw:DWORD
extrn ret_camera_position_x:DWORD
extrn ret_camera_position_y:DWORD
extrn ret_camera_position_z:DWORD

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
  add   eax, tank
  jmp   ret_tank_controls
tank_controls endp

camera_pitch proc export
  push  eax
  mov   eax, base
  add   eax, 0B71F84h
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

camera_yaw proc export
  mov   eax, base
  add   eax, 0B71F84h
  mov   eax, [eax]
  cmp   ebp, eax
  je    yaw_orig
  
  add   eax, 26Ch
  cmp   ebp, eax
  je    yaw_orig
  
  p1_yaw:
  mov   eax, addr_player_1
  mov   eax, [eax]
  mov   eax, [eax + 88h]
  sub   eax, yaw
  jmp   ret_camera_yaw

  p2_yaw:
  mov   eax, addr_player_2
  mov   eax, [eax]
  mov   eax, [eax + 88h]
  sub   eax, yaw
  jmp   ret_camera_yaw
  
  yaw_orig:
  mov   eax, [ebp + 20Ch]
  jmp   ret_camera_yaw
camera_yaw endp

camera_position_x proc export
  fstp  st(0)
  push  eax
  push  ebx
  push  ecx
  push  edx
  
  push  eax
  mov   eax, base
  add   eax, 0B71F80h
  mov   eax, [eax]
  add   eax, 26Ch
  cmp   ecx, eax
  pop   eax
  ; je    p2_x
  
  p1_x:
  push  ecx
  mov   eax, addr_player_1
  mov   eax, [eax]
  mov   eax, [eax + 1604h]
  movss xmm0, dword ptr [eax + 78h]
  movss dword ptr [esp], xmm0
  mov   eax, addr_player_1
  mov   eax, [eax]
  push  [eax + 88h]
  jmp   x_end
  
  p2_x:
  push  ecx
  mov   eax, addr_player_2
  mov   eax, [eax + 1604h]
  movss xmm0, dword ptr [eax + 78h]
  movss dword ptr [esp], xmm0
  mov   eax, addr_player_2
  mov   eax, [eax]
  push  [eax + 88h]
  
  x_end:
  call  x_cam
  add   esp, 8
  pop   edx
  pop   ecx
  pop   ebx
  pop   eax
  fstp  dword ptr [ecx + 124h]
  jmp   ret_camera_position_x
camera_position_x endp

camera_position_y proc export
  fstp  st(0)
  push  eax
  push  ebx
  push  ecx
  push  edx
  
  push  eax
  mov   eax, base
  add   eax, 0B71F80h
  mov   eax, [eax]
  add   eax, 26Ch
  cmp   ecx, eax
  pop   eax
  ; je    p2_y
  
  p1_y:
  push  ecx
  mov   eax, addr_player_1
  mov   eax, [eax]
  mov   eax, [eax + 1604h]
  movss xmm0, dword ptr [eax + 70h]
  movss dword ptr [esp], xmm0
  mov   eax, addr_player_1
  mov   eax, [eax]
  push  [eax + 88h]
  jmp   y_end
  
  p2_y:
  push  ecx
  mov   eax, addr_player_2
  mov   eax, [eax + 1604h]
  movss xmm0, dword ptr [eax + 70h]
  movss dword ptr [esp], xmm0
  mov   eax, addr_player_2
  mov   eax, [eax]
  push  [eax + 88h]
  
  y_end:
  call  y_cam
  add   esp, 8
  pop   edx
  pop   ecx
  pop   ebx
  pop   eax
  fstp  dword ptr [ecx + 11Ch]
  jmp   ret_camera_position_y
camera_position_y endp

camera_position_z proc export
  fstp  dword ptr [ecx + 120h]
  jmp   ret_camera_position_z
camera_position_z endp

end