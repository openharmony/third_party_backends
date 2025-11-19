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
#include "threadpool_c.h"
#include "threadpool_wrapper.h"
#include "sanei_debug.h"

#define LEVEL 1

struct threadpool_handle_t {
    ThreadPoolWrapper* wrapper;
};

threadpool_handle_t* threadpool_create(int32_t thread_num)
{
    constexpr int32_t maxThreadNum = 20;
    if (thread_num <= 0 || thread_num > maxThreadNum) {
        DBG(LEVEL, "threadpool_create: fail\n");
        return nullptr;
    }
    
    threadpool_handle_t* handle = new(std::nothrow) threadpool_handle_t();
    if (!handle) {
        DBG(LEVEL, "threadpool_create: handle is nullptr\n");
        return nullptr;
    }
    
    handle->wrapper = new(std::nothrow) ThreadPoolWrapper();
    if (!handle->wrapper) {
        delete handle;
        DBG(LEVEL, "threadpool_create: wrapper is nullptr\n");
        return nullptr;
    }
    
    if (!handle->wrapper->Start(thread_num)) {
        delete handle->wrapper;
        delete handle;
        DBG(LEVEL, "threadpool_create: Start fail\n");
        return nullptr;
    }
    
    return handle;
}

bool threadpool_add_task(threadpool_handle_t* pool, threadpool_task_func_t func, void* arg)
{
    if (!pool || !pool->wrapper || !func) {
        DBG(LEVEL, "threadpool_add_task: fail\n");
        return false;
    }
    
    pool->wrapper->AddTask(func, arg);
    return true;
}

void threadpool_destroy(threadpool_handle_t* pool)
{
    if (pool) {
        if (pool->wrapper) {
            delete pool->wrapper;
        }
        delete pool;
    }
}

void threadpool_wait(threadpool_handle_t* pool)
{
    if (pool && pool->wrapper) {
        pool->wrapper->Wait();
    }
}
