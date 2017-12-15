/**
 * @file    circular_buffer.c
 * @brief   Implementation of a circular buffer
 *
 * DAPLink Interface Firmware
 * Copyright (c) 2016-2016, ARM Limited, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "platform.h"
#include "circ_buf.h"

void circ_buf_init(circ_buf_t *circ_buf, uint8_t *buffer, uint32_t size)
{
	vPortEnterCritical();
	{
		circ_buf->buf = buffer;
		circ_buf->size = size;
		circ_buf->head = 0;
		circ_buf->tail = 0;
	}
	vPortExitCritical();
}

static void push_do(circ_buf_t *circ_buf, uint8_t data)
{
	circ_buf->buf[circ_buf->tail] = data;
	circ_buf->tail += 1;
	if (circ_buf->tail >= circ_buf->size) {
        assert_param(circ_buf->tail == circ_buf->size);
		circ_buf->tail = 0;
	}
}

bool circ_buf_push_try(circ_buf_t *circ_buf, uint8_t data)
{
	bool success;
	vPortEnterCritical();
	{
		if (circ_buf_count_free(circ_buf) == 0) {
			success = false;
			goto quit;
		}

		push_do(circ_buf, data);
		success = true;
	}
quit:
	vPortExitCritical();
	return success;
}

void circ_buf_push(circ_buf_t *circ_buf, uint8_t data)
{
	vPortEnterCritical();
	{
		push_do(circ_buf, data);
		// Assert no overflow
        assert_param(circ_buf->head != circ_buf->tail);
	}
	vPortExitCritical();
}

static uint8_t pop_do(circ_buf_t *circ_buf)
{
	uint8_t data;
	data = circ_buf->buf[circ_buf->head];
	circ_buf->head += 1;
	if (circ_buf->head >= circ_buf->size) {
        assert_param(circ_buf->head == circ_buf->size);
		circ_buf->head = 0;
	}
	return data;
}

uint8_t circ_buf_pop(circ_buf_t *circ_buf)
{
	uint8_t data;

	vPortEnterCritical();
	{
		// Assert buffer isn't empty
        assert_param(circ_buf->head != circ_buf->tail);
		data = pop_do(circ_buf);
	}
	vPortExitCritical();

    return data;
}

bool circ_buf_pop_try(circ_buf_t *circ_buf, uint8_t *data)
{
	bool success;
	vPortEnterCritical();
	{
		if (circ_buf->head == circ_buf->tail) {
			success = false;
			goto quit;
		}

		*data = pop_do(circ_buf);
		success = true;
	}
quit:
	vPortExitCritical();
    return success;
}

uint32_t circ_buf_count_used(circ_buf_t *circ_buf)
{
    uint32_t cnt;

	vPortEnterCritical();
	{
		if (circ_buf->tail >= circ_buf->head) {
			cnt = circ_buf->tail - circ_buf->head;
		} else {
			cnt = circ_buf->tail + circ_buf->size - circ_buf->head;
		}
	}
	vPortExitCritical();

    return cnt;
}

uint32_t circ_buf_count_free(circ_buf_t *circ_buf)
{
    uint32_t cnt;

	vPortEnterCritical();
	{
		cnt = circ_buf->size - circ_buf_count_used(circ_buf) - 1;
	}
	vPortExitCritical();

    return cnt;
}

uint32_t circ_buf_read(circ_buf_t *circ_buf, uint8_t *data, uint32_t size)
{
    uint32_t cnt;
    uint32_t i;

    cnt = circ_buf_count_used(circ_buf);
    cnt = MIN(size, cnt);
    for (i = 0; i < cnt; i++) {
        data[i] = circ_buf_pop(circ_buf);
    }

    return cnt;
}

uint32_t circ_buf_write(circ_buf_t *circ_buf, const uint8_t *data, uint32_t size)
{
    uint32_t cnt;
    uint32_t i;

    cnt = circ_buf_count_free(circ_buf);
    cnt = MIN(size, cnt);
    for (i = 0; i < cnt; i++) {
        circ_buf_push(circ_buf, data[i]);
    }

    return cnt;
}
