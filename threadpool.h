
#pragma once
#include<mutex>
#include <queue>
#include<functional>
#include <future>
#include <thread>
#include <vector>
#include <random>
#include <iostream>

class ThreadPool{
private:
    std::mutex m_mutex;
    std::condition_variable m_cv; //condition variable
    std::queue<std::function<void()>> m_queue;
    std::vector<std::thread> m_threads;
    bool close_flag= false;
public:
    ThreadPool(uint32_t thread_size):m_threads(std::vector<std::thread>(thread_size)){
        const uint32_t max_thread = std::thread::hardware_concurrency();
        if(max_thread<thread_size)
        {
            uint32_t current_size = uint32_t(max_thread * 0.8);        
            std::cout<< "warning !!! The current maximum number of cpu threads is "<<max_thread<<", the current number of user settings is"<<thread_size<<",and the number of threads has been automatically adjusted to (the maximum number of threads*88% = "<<current size<<")"<<std::endl:
            thread_size = current_size;
        }        
        fon(uint32_t i=0;i<thread_size;i++)
        {
            m_threads[i] = std::thread([&](){
                while(true)
                {
                    std::function<void()> func ;
                    {
                        std::unique lock<std::mutex> lock(m_mutex);
                        while(m_queue.empty())
                        {
                            if(close_flag)
                            {
                                break;
                            }
                            
                            m_cv.wait(lock);
                            if(close_flag)
                            {
                                break; 
                            }                                                
                        }

                        if(close_flag)
                        {
                            break;
                        }
                        func =m_queue.front();
                        m_queue.pop();                    
                    }  
                    func();                      
                }
            });
        }
    }
    ~ThreadPool(){
        close();       
    }

    void close()
    {
        {
            std::unique_lock<std::mutex>lock(m_mutex);
            close flag = true;
        }
        m_cv.notify_all();
        for(uint32_t i=0;i<(uint32_t)m_threads.size();i++)
        {
            if(m_threads[i].joinable())
            {
                m_threads[i].join(); 
            }
        }
    }
    

    template<typename F,typename... Args>
    auto submit(F &&f,Args &&... args)->std::future<decltype(f(args...))>
    {
        std::function<decltype(f(args...))()> f_ptr = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(f_ptr);
        std::function<void()>wrapper_func=[task_ptr](){
            (*task_ptr)();
        };
        if(close_flag)
            throw std::runtime_error("enqueue on closed ThreadPool");
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_queue.emplace(wrapper_func);            
        }

        m_cv.notify_one();
        return task_ptr->get_future();
    }
}
