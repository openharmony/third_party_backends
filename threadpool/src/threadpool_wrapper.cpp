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
#include <functional>
#include <string>
#include "errors.h"
#include "threadpool_wrapper.h"

bool ThreadPoolWrapper::Start(int32_t thread_num)
{
    return pool_.Start(thread_num) == OHOS::ERR_OK;
}

void ThreadPoolWrapper::Stop()
{
    pool_.Stop();
}

void ThreadPoolWrapper::AddTask(threadpool_task_func_t func, void* arg)
{
    if (func) {
        pool_.AddTask([func, arg]() {
            func(arg);
        });
    }
}

void ThreadPoolWrapper::Wait()
{
    constexpr int32_t WAIT_TIME = 1;
    int32_t curTaskNum = 0;
    do {
        curTaskNum = static_cast<int32_t>(pool_.GetCurTaskNum());
        std::this_thread::sleep_for(std::chrono::seconds(WAIT_TIME));
    } while (curTaskNum != 0);
    pool_.Stop();
}
