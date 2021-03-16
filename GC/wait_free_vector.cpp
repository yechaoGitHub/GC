// ConsoleApplication2.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <assert.h>
#include <vector>
#include <atomic>
#include <thread>
#include <windows.h>

class lock_free_vector
{
    static const int INIT_SIZE = 3;

public:
    lock_free_vector()
    {
        _data = new volatile void*[INIT_SIZE];
        assert(_data);
        ::memset(_data, 0, sizeof(void*) * INIT_SIZE);
        _capacity = INIT_SIZE;
        _size = 0;
        _inserting = 0;
        _removing = 0;
        _getting = 0;
        _resizing = 0;
    }

    ~lock_free_vector()
    {
        delete _data;
    }

    void insert(void* v)
    {
        assert(v);

        int64_t old_size(0);
        int64_t new_size(0);

        while (true) 
        {
            MutexCheckWeak(_inserting, _resizing);
            old_size = _size;
            new_size = (std::min)(old_size + 1, static_cast<int64_t>(_capacity));
            if (old_size == new_size) 
            {
                _inserting--;
                continue;
            }
            else if(!_size.compare_exchange_strong(old_size, new_size))
            {
                _inserting--;
                continue;
            }
            else 
            {
                break;
            }
        }

        assert(new_size != old_size);

        while (InterlockedCompareExchange64(reinterpret_cast<volatile long long* >(&_data[old_size]), reinterpret_cast<long long>(v), 0))
        {
            std::this_thread::yield();
        }

        _inserting--;

        if (new_size == _capacity)
        {
            resize();
        }

    }

    bool remove(uint32_t index, void*& value)
    {
        int64_t old_size(0);
        int64_t new_size(0);

        MutexCheckWeak(_removing, _resizing, _getting, _inserting);

        do
        {
            old_size = _size;
            if (index < old_size)
            {
                new_size = (std::max)(old_size - 1, 0ll);
            }
            else
            {
                _removing--;
                return false;
            }
        }
        while (!_size.compare_exchange_strong(old_size, new_size));

        while (true)
        {
            volatile void* old_value = _data[index];
            if (old_value &&
                (InterlockedCompareExchange64(reinterpret_cast<volatile long long*>(&_data[index]), 0, reinterpret_cast<long long>(old_value)) != 0))
            {
                break;
            }
            else
            {
                std::this_thread::yield();
            }
        }

        if (index != old_size - 1)
        {
            while (true)
            {
                volatile void* old_value = _data[old_size - 1];
                if (old_value && 
                    InterlockedCompareExchange64(reinterpret_cast<volatile long long*>(&_data[index]), reinterpret_cast<long long>(_data[old_size - 1]), 0) != 0) 
                {
                    break;
                }
                else 
                {
                    std::this_thread::yield();
                }
            }
            
            while (true)
            {
                volatile void* old_value = _data[old_size - 1];
                if (old_value &&
                    (InterlockedCompareExchange64(reinterpret_cast<volatile long long*>(&_data[old_size - 1]), 0, reinterpret_cast<long long>(old_value)) != 0))
                {
                    break;
                }
                else
                {
                    std::this_thread::yield();
                }
            }
       }
       
        _removing--;

        return true;
    }

    bool get(uint32_t index, void*& value)
    {
        int64_t old_total(0);
        int64_t new_total(0);

        MutexCheckWeak(_getting, _resizing, _removing);

        if (index >= _size) 
        {
            _getting--;
            return false;
        }

        value = const_cast<void*&>(_data[index]);

        _getting--;

        return true;
    }

    uint64_t size()
    {
        return _size;
    }

private:
    volatile void**         _data;
    std::atomic<int64_t>    _size;
    std::atomic<int64_t>    _capacity;
    std::atomic<int64_t>    _inserting;
    std::atomic<int64_t>    _removing;
    std::atomic<int64_t>    _getting;
    std::atomic<int64_t>    _resizing;
   
    void resize()
    {
        int64_t old_value = MutexCheckStrong(_resizing, _inserting, _removing, _getting);

        if (old_value != 0) 
        {
            _resizing--;
            return;
        }

        assert(_size == _capacity);
        int32_t new_capacity = _size * 1.5;

        volatile void** new_place = new volatile void* [new_capacity] { 0 };
        assert(new_place);

        ::memcpy(const_cast<void**>(new_place), const_cast<void**>(_data), _size * sizeof(void*));
        
        delete[] _data;
        _data = new_place;

        _capacity = new_capacity;

        _resizing--;
    }

    template<typename TCount, typename ...TMutex>
    void MutexCheckWeak(TCount &count, TMutex &...mutex)
    {
        while (true) 
        {
            count++;

            TCount old_mutex_count = (0 + ... + mutex);
            if (old_mutex_count)
            {
                count--;

                while (true) 
                {
                    std::this_thread::yield();
                    TCount new_mutex_total = (0 + ... + mutex);
                    if (new_mutex_total < old_mutex_count) 
                    {
                        break;
                    }
                }
            }
            else 
            {
                break;
            }
        }
    }

    template<typename TCount, typename ...TMutex>
    TCount MutexCheckStrong(TCount& count, TMutex &...mutex)
    {   
        count++;
        while (true)
        {
            TCount old_mutex_count = (0 + ... + mutex);
            if (old_mutex_count)
            {
                std::this_thread::yield();
            }
            else
            {
                break;
            }
        }
    }

    template<typename TCount, typename ...TMutex>
    TCount MutexCheckStrong(std::atomic<TCount>& count, TMutex &...mutex)
    {
        TCount ret = count++;
        while (true)
        {
            TCount old_mutex_count = (0 + ... + mutex);
            if (old_mutex_count)
            {
                std::this_thread::yield();
            }
            else
            {
                break;
            }
        }

        return ret;
    }

}VEC;

uint64_t d = 1;

void InsertThread() 
{
    for(;;)
    //for(int i = 100; i > 0; i--)
    {
        VEC.insert((void*)d++);
    }  
}

void RemoveThread() 
{
    uint64_t v(0);

    while (true) 
    {
        VEC.remove(0, reinterpret_cast<void*&>(v));
    }
}

int main()
{
    std::thread th1(InsertThread);
    std::thread th2(InsertThread);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    std::thread th3(RemoveThread);
    std::thread th4(RemoveThread);

    th1.join();
    th2.join();
    th3.join();
    th4.join();

    return 0;
}

