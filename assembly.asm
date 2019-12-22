.386
.model flat, c

extrn jmp_addr:DWORD

.code
my_jump proc export
  add DWORD PTR[esi+0001D5F8h], 7h
  jmp DWORD PTR[jmp_addr]
my_jump endp
end