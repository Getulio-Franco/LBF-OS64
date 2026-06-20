#include "apps.h"
#include "drivers/video.h"
#include "include/elf.h"
#include "drivers/proc.h"
#include <stdint.h>

void task_a() {
//nunca usar
}

void task_b() {
    while(1) {
        process_cleanup_zombies(); // Ceifador
        for(volatile int i = 0; i < 500000; i++); 
        __asm__ volatile("hlt"); 
    }
}

void task_c() {
    for(volatile int i = 0; i < 5000000; i++); 
    create_elf_process("EXPLORER.ELF");
    while(1) {
        __asm__ volatile("hlt");
    }
}

void task_d() {
    while(1) {
        process_spawn_pending(); // exec software.elf
        for(volatile int i = 0; i < 500000; i++); 
        __asm__ volatile("hlt"); 
    }
}
