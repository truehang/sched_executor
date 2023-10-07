# Deterministic Scheduler

## 简介

Deterministic Scheduler想要解决的问题是：对于有时间要求的任务（可能是连续的多个任务），如何保证最应该得到CPU时间的任务得到时间并且不会被不知情地切换走（尽最大可能），由此满足在指定的deadline前完成任务 。

Deterministic Scheduler中的任务是用户函数，它会把每个用户函数装载到线程中，然后调度这些线程。现在Deterministic Scheduler具备让用户设置时间参数，调度器根据参数实现CPU时间的合理分配，时间最迫切的任务会优先获得CPU时间。用户可以配置一个或者连续多个任务的总截止时间，即从第一个任务开始到最后一个任务结束的总时间，在模型中记为TTS(Triggered Task Sequence) Deadline。用户也配置任务的依赖关系（若有）。

Deterministic Scheduler管理的任务分布在各个进程中，在使用时，需要进行一些标准的步骤添加任务，并且在所有进程的任务添加完成后激活调度器，这样就能让任务转起来。

任务的触发方式分为定时器触发和外部信号触发。假如是外部信号触发，用户需要指明是哪个信号，推荐用户在include/deterministic_scheduler_user_signal.h中集中配置信号。需要说明的是，SIGRTMIN ~ SIGRTMIN + 3（值为34 ~37）的信号已经被调度器占用，请避免使用这些信号，否则调度器不能正常工作。

## 注意事项

1. **所有进程都需要使用root权限运行**
2. **用户不要使用值为34 ~37的信号**

## 编译和安装

创建一个与deterministic_scheduler文件夹的平行的文件夹build，并且切换到build文件中，之后进行编译和安装：

```bash
mkdir build && cd build
# 需要输入密码，因为cmake文件中有sudo指令
cmake ..
sudo make install
```
之后文件被安装到默认的位置，在linux下，默认位置为"/usr/local/", 用户可以通过CMAKE_INSTALL_PREFIX变量改变安装位置。

## 使用

include deterministic_scheduler.hpp文件，如果用户有自己定义的外部信号，请在deterministic_scheduler_user_signal.h文件中添加。编译时链接libdeterministic_scheduler.so即可（该.so库由前面的编译步骤生成并存放于安装目录下）。CMakeLists.txt写法为  
    ` 
    find_package (deterministic_scheduler REQUIRED)
    include_directories(${deterministic_scheduler_INCLUDE_DIR})
    target_link_libraries(YOUR_TARGET deterministic_scheduler)
    `
编译example（假设现在在build目录下）：
```bash
rm -rf *
cmake ../examples/
make 
```

如何使用调度器的例子放在了examples文件夹下，这里做简要说明：
### 打印调度器信息（可选）

参见print_information目录中的代码。步骤是，创建一个ScheduleClient对象然后调用printShm()函数即可，需要指定每隔多少秒打印一次。

打印共享内存中保存的调度信息，请在一开始（注册任务之前）就运行（如果你不想看调度信息则可以不用运行）：
```bash
sudo deterministic_scheduler_tool -p
```
### 添加任务

参见add_task目录中的代码。步骤是，创建一个ScheduleClient对象，分配一个任务数组allocateTaskArray()，添加任务addTask()，设置任务参数setTaskParam()，setTaskParam()会用到addTask()返回的任务索引，所以这两个函数请连续执行。一个进程的所有任务添加完毕后，注册信息让调度器把任务运转起来spin()。

在设置任务参数时，有一个重要的参数：任务的执行时间Burst Time需要用户指定。如果用户不知道填多少，可以使用ScheduleClient的testBurstTime方法来测试这些任务的Burst time，参考test_burst_time_main.cc中的写法。步骤也是先添加任务，在设置任务参数时burst_time设置为0即可，最后调用testBurstTime()，这将运行10次这些任务并统计出它们的burst_time值。

（假设现在在build目录下）使用sudo 运行deterministic_scheduler_add_task：
```bash
sudo ./deterministic_scheduler_add_task
```

### 激活调度器

参见enable_scheduler目录中的代码。当所有进程的所有任务都注册完任务信息后，我们需要单独用一个进程把调度器激活，这样调度器才真正解析任务信息并接管任务。步骤是，创建一个ScheduleClient对象然后调用enableScheduler()函数即可。
打开一个新窗口，使用sudo权限来激活（enable）调度器，可以观察到原来的任务已经开始使用实时调度策略了：
```bash
sudo deterministic_scheduler_tool -e
```

