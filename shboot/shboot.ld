SECTIONS
{
 . = 0x0;
 .text : {
 shboot.o(.text)
 *(.text)
 }
 .data : { *(.data) }
 .bss : { *(.bss COMMON) }
 stack_top = 0x2000000;
}
