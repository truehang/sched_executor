#ifndef DETERMINISTIC_SCHEDULER_HPP_HEADER
#define DETERMINISTIC_SCHEDULER_HPP_HEADER
#include <cstdlib>
#include <iostream>
#include <memory> /* std::unique_ptr*/
#include <string>
#include <tuple> /* std::tuple*/

extern "C" {
#include "deterministic_scheduler_user_signal.h" /* define user signal*/
/* function declaration */
int add_func(struct func_array* func_array, void* invoker);
}

class Invoker {
public:
  template <typename Callable, typename... Args>
  void loadFunc(Callable&& f, Args&&... args) {
    _S_make_state(std::tuple<Callable, Args...>{std::forward<Callable>(f),
                                                std::forward<Args>(args)...});
    ready_ = 1;
  }
  void runFunc() {
    if (ready_) invoker_->run();
  }

private:
  bool ready_;

  struct __Invoker_Base {
    virtual void run() = 0;
  };

  template <typename _Tuple>
  struct _Invoker : public __Invoker_Base {
    _Tuple _M_t;
    _Invoker(_Tuple&& tp) : _M_t{tp} {}
    template <size_t _Index>
    static std::__tuple_element_t<_Index, _Tuple>&& _S_declval();

    template <size_t... _Ind>
    auto _M_invoke(std::_Index_tuple<_Ind...>) noexcept(
        noexcept(std::__invoke(_S_declval<_Ind>()...)))
        -> decltype(std::__invoke(_S_declval<_Ind>()...)) {
      return std::__invoke(std::get<_Ind>(std::move(_M_t))...);
    }

    using _Indices = typename std::_Build_index_tuple<
        std::tuple_size<_Tuple>::value>::__type;

    virtual void run() override { _M_invoke(_Indices()); }
  };

  template <typename _Callable>
  void _S_make_state(_Callable&& __f) {
    invoker_ = std::unique_ptr<_Invoker<_Callable>>(
        new _Invoker<_Callable>(std::forward<_Callable>(__f)));
  }

  std::unique_ptr<__Invoker_Base> invoker_;
};

class ScheduleClient {
public:
  enum class HowTrig { not_specified, external_signal, timer };

  int allocateTaskArray(int size);

  /* return index in task array*/
  template <typename Callable, typename... Args>
  int addTask(Callable&& f, Args&&... args) {
    if (!func_arr_) {
      printf("addTask failed, please call allocateTaskArray first.\n");
      return -1;
    }
    Invoker* invoker =
        create_invoker(std::forward<Callable>(f), std::forward<Args>(args)...);
    return add_func(func_arr_, invoker);
  }

  /* time unit: ms*/
  int setTaskParam(int task_index, const std::string& short_name,
                   int burst_time_ms = 0, const std::string& dependency = "",
                   int period_ms = 0, int deadline_ms = 0,
                   const HowTrig& how = HowTrig::not_specified,
                   int external_sig = 0);

  int spin();
  int enableScheduler();
  int printShm(int tick);
  int testBurstTime();

private:
  struct func_array* func_arr_;
  template <typename Callable, typename... Args>
  Invoker* create_invoker(Callable&& f, Args&&... args) {
    Invoker* invoker = new Invoker;
    invoker->loadFunc(std::forward<Callable>(f), std::forward<Args>(args)...);
    return invoker;
  }
};

#endif