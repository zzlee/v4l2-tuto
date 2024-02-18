/* akvcam, virtual camera for Linux.
 * Copyright (C) 2018  Gonzalo Exequiel Pedone
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef AKVCAM_RBUFFER_H
#define AKVCAM_RBUFFER_H

#include <linux/types.h>

#include "utils.h"

#define akvcam_rbuffer_tt(type) akvcam_rbuffer_t
#define akvcam_rbuffer_ctt(type) akvcam_rbuffer_ct

struct akvcam_rbuffer;
typedef struct akvcam_rbuffer *akvcam_rbuffer_t;
typedef const struct akvcam_rbuffer *akvcam_rbuffer_ct;

akvcam_rbuffer_t akvcam_rbuffer_new(void);
akvcam_rbuffer_t akvcam_rbuffer_new_copy(akvcam_rbuffer_ct other);
void akvcam_rbuffer_delete(akvcam_rbuffer_t self);
akvcam_rbuffer_t akvcam_rbuffer_ref(akvcam_rbuffer_t self);

void akvcam_rbuffer_copy(akvcam_rbuffer_t self, akvcam_rbuffer_ct other);
void akvcam_rbuffer_resize(akvcam_rbuffer_t self,
                           size_t n_elements,
                           size_t element_size,
                           AKVCAM_MEMORY_TYPE memory_type);
size_t akvcam_rbuffer_size(akvcam_rbuffer_ct self);
size_t akvcam_rbuffer_data_size(akvcam_rbuffer_ct self);
size_t akvcam_rbuffer_n_elements(akvcam_rbuffer_ct self);
size_t akvcam_rbuffer_element_size(akvcam_rbuffer_ct self);
size_t akvcam_rbuffer_n_data(akvcam_rbuffer_ct self);
ssize_t akvcam_rbuffer_available_data_size(akvcam_rbuffer_ct self);
bool akvcam_rbuffer_data_empty(akvcam_rbuffer_ct self);
bool akvcam_rbuffer_elements_empty(akvcam_rbuffer_ct self);
bool akvcam_rbuffer_data_full(akvcam_rbuffer_ct self);
bool akvcam_rbuffer_elements_full(akvcam_rbuffer_ct self);
void *akvcam_rbuffer_queue(akvcam_rbuffer_t self, const void *data);
void *akvcam_rbuffer_queue_bytes(akvcam_rbuffer_t self,
                                 const void *data,
                                 size_t size);
void *akvcam_rbuffer_dequeue(akvcam_rbuffer_t self,
                             void *data,
                             bool keep);
void *akvcam_rbuffer_dequeue_bytes(akvcam_rbuffer_t self,
                                   void *data,
                                   size_t *size,
                                   bool keep);
void akvcam_rbuffer_clear(akvcam_rbuffer_t self);
void *akvcam_rbuffer_ptr_at(akvcam_rbuffer_ct self, size_t i);
void *akvcam_rbuffer_ptr_front(akvcam_rbuffer_ct self);
void *akvcam_rbuffer_ptr_back(akvcam_rbuffer_ct self);
void *akvcam_rbuffer_find(akvcam_rbuffer_ct self,
                          const void *data,
                          const akvcam_are_equals_t equals,
                          ssize_t *offset);

#endif // AKVCAM_RBUFFER_H
