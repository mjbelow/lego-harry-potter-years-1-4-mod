.686
.xmm

.model flat, c


.data
_all_access_code dw 0FFFFh

.code

_my_jump proc
  add   dword ptr [esi+0001D5F8h], 7h
  jmp   dword ptr [_jmp_addr]
_my_jump endp

_speed_hack proc
  fmul  dword ptr [_speed]
  fstp  dword ptr [esp + 74h]
  push  _ret_speed
  ret
_speed_hack endp

_jump_hack proc
  fld   dword ptr [_jump]
  and   ebx, 101h
  push _ret_jump
  ret
_jump_hack endp

_gravity_hack proc
  fld   dword ptr [_gravity]
  push  _ret_gravity
  ret
_gravity_hack endp

_tank_controls proc
  add   eax, [ecx + 20Ch]
  push  eax
  mov   eax, _addr_player_1
  cmp   esi, [eax]
  je    _adj
  ; mov   eax, _addr_player_2
  ; cmp   esi, [eax]
  ; je    adj
  
  pop   eax
  push  _ret_tank_controls
  ret
  
  _adj:
  pop   eax
  add   eax, _tank
  push  _ret_tank_controls
  ret
_tank_controls endp

_camera_pitch proc
  push  eax
  mov   eax, _base
  add   eax, 0B71F84h
  cmp   ebp, [eax]
  je    _pitch_orig
  
  pop   eax
  mov   ebx, _pitch
  push  _ret_camera_pitch
  ret
  
 _pitch_orig:
  pop   eax
  mov   ebx, [ebp + 208h]
  push  _ret_camera_pitch
  ret
_camera_pitch endp

_camera_yaw proc
  mov   eax, _base
  add   eax, 0B71F84h
  mov   eax, [eax]
  cmp   ebp, eax
  je    _yaw_orig
  
  add   eax, 26Ch
  cmp   ebp, eax
  je    _yaw_orig
  
  p1_yaw:
  mov   eax, _addr_player_1
  mov   eax, [eax]
  mov   eax, [eax + 88h]
  sub   eax, _yaw
  push  _ret_camera_yaw
  ret

  p2_yaw:
  mov   eax, _addr_player_2
  mov   eax, [eax]
  mov   eax, [eax + 88h]
  sub   eax, _yaw
  push  _ret_camera_yaw
  ret
  
 _yaw_orig:
  mov   eax, [ebp + 20Ch]
  push  _ret_camera_yaw
  ret
_camera_yaw endp

_camera_position_x proc
  fstp  st(0)
  push  eax
  push  ebx
  push  ecx
  push  edx
  
  push  eax
  mov   eax, _base
  add   eax, 0B71F80h
  mov   eax, [eax]
  add   eax, 26Ch
  cmp   ecx, eax
  pop   eax
  ; je    p2_x
  
  p1_x:
  push  ecx
  mov   eax, _addr_player_1
  mov   eax, [eax]
  mov   eax, [eax + 1604h]
  movss xmm0, dword ptr [eax + 78h]
  movss dword ptr [esp], xmm0
  mov   eax, _addr_player_1
  mov   eax, [eax]
  push  [eax + 88h]
  jmp   x_end
  
  p2_x:
  push  ecx
  mov   eax, _addr_player_2
  mov   eax, [eax + 1604h]
  movss xmm0, dword ptr [eax + 78h]
  movss dword ptr [esp], xmm0
  mov   eax, _addr_player_2
  mov   eax, [eax]
  push  [eax + 88h]
  
  x_end:
  call  _x_cam
  add   esp, 8
  pop   edx
  pop   ecx
  pop   ebx
  pop   eax
  fstp  dword ptr [ecx + 124h]
  push  _ret_camera_position_x
  ret
_camera_position_x endp

_camera_position_y proc
  fstp  st(0)
  push  eax
  push  ebx
  push  ecx
  push  edx
  
  push  eax
  mov   eax, _base
  add   eax, 0B71F80h
  mov   eax, [eax]
  add   eax, 26Ch
  cmp   ecx, eax
  pop   eax
  ; je    p2_y
  
  p1_y:
  push  ecx
  mov   eax, _addr_player_1
  mov   eax, [eax]
  mov   eax, [eax + 1604h]
  movss xmm0, dword ptr [eax + 70h]
  movss dword ptr [esp], xmm0
  mov   eax, _addr_player_1
  mov   eax, [eax]
  push  [eax + 88h]
  jmp   y_end
  
  p2_y:
  push  ecx
  mov   eax, _addr_player_2
  mov   eax, [eax + 1604h]
  movss xmm0, dword ptr [eax + 70h]
  movss dword ptr [esp], xmm0
  mov   eax, _addr_player_2
  mov   eax, [eax]
  push  [eax + 88h]
  
  y_end:
  call  _y_cam
  add   esp, 8
  pop   edx
  pop   ecx
  pop   ebx
  pop   eax
  fstp  dword ptr [ecx + 11Ch]
  push  _ret_camera_position_y
  ret
_camera_position_y endp

_camera_position_z proc
  fstp  st(0)
  push  eax
  
  push  eax
  mov   eax, _base
  add   eax, 0B71F80h
  mov   eax, [eax]
  add   eax, 26Ch
  cmp   ecx, eax
  pop   eax
  ; je    p2_z
  
  p1_z:
  mov   eax, _addr_player_1
  mov   eax, [eax]
  jmp   z_end
  
  p2_z:
  mov   eax, _addr_player_2
  mov   eax, [eax]
  
  z_end:
  mov   eax, [eax + 1604h]
  mov   eax, [eax + 74h]
  
  push  eax
  fld   dword ptr [esp]
  pop   eax
  pop   eax
  fadd  dword ptr [_adjust_height]
  
  fstp  dword ptr [ecx + 120h]
  push  _ret_camera_position_z
  ret
_camera_position_z endp

_all_access proc
  movzx eax, _all_access_code
  push  _ret_all_access
  ret
_all_access endp

_all_access_gog proc
  movzx eax, _all_access_code
  push  _ret_all_access_gog
  ret
_all_access_gog endp

end