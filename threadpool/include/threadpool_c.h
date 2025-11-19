/*
* Copyright (c) 2025 Huawei Device Co., Ltd.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef THREADPOOL_C_H
#define THREADPOOL_C_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*threadpool_task_func_t)(void* arg);

typedef struct threadpool_handle_t threadpool_handle_t;

threadpool_handle_t* threadpool_create(int thread_num);

bool threadpool_add_task(threadpool_handle_t* pool, threadpool_task_func_t func, void* arg);

void threadpool_destroy(threadpool_handle_t* pool);

void threadpool_wait(threadpool_handle_t* pool);

#ifdef __cplusplus
}
#endif

#endif // THREADPOOL_C_H