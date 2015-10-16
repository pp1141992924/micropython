/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Rob Fowler
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include "py/nlr.h"
#include "py/runtime.h"
#include "py/gc.h"

#include "osapi.h"
#include "os_type.h"
#include "utils.h"
#include "user_interface.h"

#include "mod_esp_mutex.h"


STATIC ICACHE_FLASH_ATTR void mod_esp_mutex_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    esp_mutex_obj_t *self = self_in;

    mp_printf(print, "mutex.value=%d", (unsigned int)&self->mutex);
}


STATIC ICACHE_FLASH_ATTR mp_obj_t mod_esp_mutex_make_new(mp_obj_t type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    esp_mutex_obj_t *self = m_new_obj(esp_mutex_obj_t);
    self->base.type = &esp_mutex_type;
    self->mutex = 1;
    return (mp_obj_t)self;
}

// Richard Antony Burton
// https://github.com/raburton/esp8266/tree/master/mutex
bool ICACHE_FLASH_ATTR GetMutex(mutex_t *mutex) {

    int iOld = 1, iNew = 0;

    asm volatile (
        "rsil a15, 1\n"    // read and set interrupt level to 1
        "l32i %0, %1, 0\n" // load value of mutex
        "bne %0, %2, 1f\n" // compare with iOld, branch if not equal
        "s32i %3, %1, 0\n" // store iNew in mutex
        "1:\n"             // branch target
        "wsr.ps a15\n"     // restore program state
        "rsync\n"
        : "=&r" (iOld)
        : "r" (mutex), "r" (iOld), "r" (iNew)
        : "a15", "memory"
    );
    return (bool)iOld;
}


STATIC ICACHE_FLASH_ATTR mp_obj_t mod_esp_mutex_acquire(mp_obj_t self_in, mp_obj_t len_in) {
    esp_mutex_obj_t *self = self_in;
    // TODO: raise on unable
    return mp_obj_new_int(GetMutex(&self->mutex));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_esp_mutex_acquire_obj, mod_esp_mutex_acquire);

STATIC mp_obj_t mod_esp_mutex_release(mp_obj_t self_in, mp_obj_t lambda_in) {
    esp_mutex_obj_t *self = self_in;
    self->mutex = 1;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_esp_mutex_release_obj, mod_esp_mutex_release);

STATIC const mp_map_elem_t mod_esp_mutex_locals_dict_table[] = {
//    { MP_OBJ_NEW_QSTR(MP_QSTR_cancel), (mp_obj_t)&esp_os_timer_cancel_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_acquire), (mp_obj_t)&mod_esp_mutex_acquire_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_release), (mp_obj_t)&mod_esp_mutex_release_obj },
};
STATIC MP_DEFINE_CONST_DICT(mod_esp_mutex_locals_dict, mod_esp_mutex_locals_dict_table);

const mp_obj_type_t esp_mutex_type = {
    { &mp_type_type },
    .name = MP_QSTR_mutex,
    .print = mod_esp_mutex_print,
    .make_new = mod_esp_mutex_make_new,
    .locals_dict = (mp_obj_t)&mod_esp_mutex_locals_dict,
};

