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
#ifndef THREADPOOL_WRAPPER
#define THREADPOOL_WRAPPER

#include "threadpool_c.h"
#include "thread_pool.h"
#include <string>

class ThreadPoolWrapper {
public:
    ThreadPoolWrapper() = default;
    
    ~ThreadPoolWrapper() = default;
    
    bool Start(int thread_num);
    
    void Stop();
    
    void AddTask(threadpool_task_func_t func, void* arg);

    void Wait();

private:
    OHOS::ThreadPool pool_;
};

#endif // THREADPOOL_WRAPPER

