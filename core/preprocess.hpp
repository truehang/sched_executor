#ifndef DEMO_SCHEDULE_PREPROCESS_HPP_DEADER
#define DEMO_SCHEDULE_PREPROCESS_HPP_DEADER
#include <time.h>

#include <algorithm>  // std::sort
#include <cmath>
#include <condition_variable>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <set>
#include <stack>
#include <string> /* std::string */
#include <unordered_map>

#include "posix_shm.h"
namespace gstone {
namespace exec {
class StringOperation {
public:
  /*
   */
  /**
   * @brief Delete all the white space in given string, return the new string.
   *Input:
     A & B -> C
       A &C   & D   -> E
     D   -> A
        E
    Output:
    A&B->C
    A&C&D->E
    D->A
    E
   *
   * @param le Long expression string
   * @return std::string New not contain white space string.
   */
  static std::string rmWhiteSpace(const std::string &le);

  /**
   * @brief Delete all the white space in given string, and split this string
   * with "->", return the two parts.
   *
   * @param le Long expression string
   * @return std::pair<std::string, std::string> The part before "->", and the
   * part after "->". If le not contain "->", the second part is an empty
   * string.
   */
  static std::pair<std::string, std::string> splitWithDemiliter(
      const std::string &long_expression, const std::string &demiliter);
};

/*--------------1 done*/

class DirectedVertex {
public:
  /**
   * @brief Construct a new Directed Vertex object.
   *
   */
  DirectedVertex(){};

  /**
   * @brief Construct a new Directed Vertex object with name.
   *
   * @param abbr_name Vertex name.
   */
  DirectedVertex(const std::string &abbr_name)
      : processed_(false), name_(abbr_name) {}

  /**
   * @brief Get the Post Vertices of this vertex.
   *
   * @return std::set<std::string>  post-vertex name set.
   */
  const std::set<std::string> &getPostVertices() const {
    return post_vertices_;
  }

  /**
   * @brief Get the Pre Vertices of this vertex.
   *
   * @return std::set<std::string> pre-vertex name set.
   */
  const std::set<std::string> &getPreVertices() const { return pre_vertices_; }

  /**
   * @brief Add a post-vertex
   *
   * @param v post-vertex name
   */
  void addPostVertex(const std::string &v) { post_vertices_.emplace(v); }
  /**
   * @brief Set the Pre Vertices of this vertex.
   *
   * @param set The vertex name set that will be set.
   */
  void SetPreVertices(const std::set<std::string> &set) { pre_vertices_ = set; }

  /**
   * @brief Get the indegree of this vertex.
   *
   * @return size_t Indegree
   */
  size_t getIndegree() const { return pre_vertices_.size(); }

  void dependencyReadyReset() { dependencies_processed_ = 0; }

  /**
   * @brief Get this vertex name
   *
   * @return std::string vertex name.
   */
  const std::string &getName() const { return name_; }

  int dependencyReadyAddOne() { return ++dependencies_processed_; }

  bool isReady() {
    return static_cast<size_t>(dependencies_processed_) == getIndegree();
  }

  /**
   * @brief Get the outdegree of this vertex.
   *
   * @return size_t outdegree
   */
  size_t getOutdegree() const { return post_vertices_.size(); }

  struct DirectedVertexSupplementaryInformation {
    std::vector<DirectedVertex *> preVertices;
    std::vector<DirectedVertex *> postVertices;
  } supplementary_information_;

private:
  bool processed_;
  std::string name_;
  std::set<std::string> post_vertices_;
  std::set<std::string> pre_vertices_;
  int dependencies_processed_;
};

/*--------------2 done*/

class DirectedGraph
    : public std::map<std::string, std::shared_ptr<DirectedVertex>> {
public:
  /**
   * @brief Construct a new default Directed Graph object.
   *
   */
  DirectedGraph() = default;

  DirectedGraph(const std::vector<std::string> &relations);
  DirectedGraph(
      const struct preschedule_dependency_array *dependencies) noexcept;

public:
  /**
   * @brief Add a relation that gives a description about inter-task dependency.
   *
   * @param relation Relation string
   */
  void addRelation(const std::string &relation);
  void updateVertexPointer() noexcept;

  /**
   * @brief Given a relation, generate  current vertex name and its dependent
   * vertex name set in pair {dependency, current vertex}.
   *
   * @param relation Relation string
   * @return std::pair<std::set<std::string>, std::string> dependent vertex name
   * set and current vertex name
   */
  std::pair<std::set<std::string>, std::string> parseRelation(
      const std::string &relation);

  /**
   * @brief Add a vertex
   *
   * @param name Vertex name
   */
  void addVertex(const std::string &name);
  /**
   * @brief Add a post-vertex to current vertex.
   *
   * @param name Current vertex name
   * @param post_vertex Post vertex name
   */
  void addPostVertex(const std::string &name, const std::string &post_vertex);
  /**
   * @brief Set the Pre Vertices of current vertex.
   *
   * @param name Current vertex name
   * @param dependency Its dependent vertex name set
   */
  void setPreVertices(const std::string &name,
                      const std::set<std::string> &dependency);

  /**
   * @brief Get the Source Vertex name, source vertex is the vertex whose
   * indegree equals 0.
   *
   * @return std::set<std::string> The set of source vertex name.
   */
  std::set<std::string> getSourceVerticesName() const;

  void dependencyReadyReset();

  std::set<DirectedVertex *> getSourceVertices() const noexcept;

  /**
   * @brief Get the Vertex object with given vertex name.
   *
   * @param name Vertex name.
   * @return std::shared_ptr<DirectedVertex> The pointer of given vertex name.
   */
  DirectedVertex *getVertex(const std::string &name) const;

  /**
   * @brief Dump the DirectedGraph.
   *
   */
  void dump();

  /**
   * @brief Get the Indegree of given vertex.
   *
   * @param name Vertex name
   * @return size_t Indegree
   */
  size_t getIndegree(const std::string &name) const {
    return this->at(name)->getIndegree();
  }

  /**
   * @brief Get the Outdegree of given vertex.
   *
   * @param name Vertex name
   * @return size_t Outdegree
   */
  size_t getOutdegree(const std::string &name) const {
    return this->at(name)->getOutdegree();
  }

private:
  bool vertex_pointer_update_;
};

/*--------------3 done*/
struct ExecutionSegment {
  int executionSegmentId;
  int startTime;
  int endTime;
  int taskInstanceId;
  int cpuCoreId;
  ExecutionSegment(int executionSegmentId, int startTime, int endTime,
                   int taskInstanceId, int cpuCoreId)
      : executionSegmentId{executionSegmentId},
        startTime{startTime},
        endTime{endTime},
        taskInstanceId{taskInstanceId},
        cpuCoreId{cpuCoreId} {}
};

struct TaskInstance;

struct TaskBasicInformation {
  std::string name;  // required
  bool sourceTask;   /* not work, refer the value in TDGG*/
  int taskId;
  int period;     // source task required
  int burstTime;  // required
  int cumulativeBurstTime;
  int deadline;
  int latestStartExecutionTimeOffset;
  int preferredCpuCoreIndex;
  int firstArrivalTime;  // source task required
  std::vector<TaskInstance *> instanceArray;

  TaskBasicInformation(const std::string &name, int burstTime, int period = 0,
                       int firstArrivalTime = 0, int taskId = 0) noexcept
      : name(name),
        sourceTask(period != 0),
        taskId(taskId),
        period(period),
        burstTime(burstTime),
        cumulativeBurstTime(burstTime),
        deadline(period),
        latestStartExecutionTimeOffset(INT32_MAX),
        preferredCpuCoreIndex(0),
        firstArrivalTime(firstArrivalTime),
        instanceArray() {}

  static void dumpFieldName();
  void dumpValue() const noexcept;
};

struct TaskInstance {
  std::string name;  // required
  int instanceId;
  enum class TaskInstanceStatus : uint8_t {
    WAITING = 0,
    EXECUTING = 1,
    DONE = 2,
    FAILED = 3
  } status;
  int arrivalTime;
  int arrivalTimeOffset;
  int completionTime;
  int burstTime;
  int cumulativeExecutionTime;
  int turnaroundTime;
  int responseTime;
  int waitingTime;
  int waitingTimeBudget;
  int deadline;
  std::vector<ExecutionSegment *> executionSegmentArray;

  TaskInstance(const int instanceId, TaskBasicInformation *taskInfo,
               int nowTime, int arrivalTimeOffset);

  void dump(bool from_cpu = false) const noexcept;
};

class TaskBasicInformationPool
    : public std::map<std::string, std::shared_ptr<TaskBasicInformation>> {
public:
  TaskBasicInformationPool() = default;
  bool newTask(const std::string &name, int burstTime, int period = 0,
               int firstArrivalTime = 0, int taskId = 0);
  bool hasTask(const std::string &name) const noexcept;
  int updateLetoWithTtsDeadline(const std::string &name,
                                int tts_deadline) noexcept;
  void dump() const noexcept;
  TaskBasicInformationPool(
      const struct preschedule_task_info_array *tasks) noexcept;
};

/*--------------4 done*/

class TtsDeadlineParser : public std::map<std::string, int> {
public:
  TtsDeadlineParser() = default;
  TtsDeadlineParser(const std::vector<std::string> &tts_deadlines) noexcept;
  bool addDeadline(const std::string &str) noexcept;
  TtsDeadlineParser(
      const struct preschedule_tts_deadline_array *tts_deadlines) noexcept;
};

/*--------------5 done*/

class ProcessQueue : public std::queue<DirectedVertex *> {
public:
  void addSet(const std::set<DirectedVertex *> &set);

  int DoneThenAddSubsequent(DirectedVertex *);
};

class TriggeredDirectedGraphGroup : public DirectedGraph {
public:
  TriggeredDirectedGraphGroup() = default;
  void generate(DirectedGraph &dg, TaskBasicInformationPool &taskPool);
  std::pair<bool, std::vector<std::string>> deadlinePropagation(
      const TtsDeadlineParser &tts_deadline,
      TaskBasicInformationPool &taskPool);
};

/*--------------6 done*/

class TaskInstancePool
    : public std::unordered_map<int, std::shared_ptr<TaskInstance>> {
public:
  TaskInstance *newTaskInstance(TaskBasicInformation *taskInfo, int nowTime,
                                int arrivalTimeOffset) noexcept;
};

inline TaskInstance *TaskInstancePool::newTaskInstance(
    TaskBasicInformation *taskInfo, int nowTime,
    int arrivalTimeOffset) noexcept {
  static int taskInstanceId = 0;
  ++taskInstanceId;
  auto aTaskInstance = std::make_shared<TaskInstance>(
      taskInstanceId, taskInfo, nowTime, arrivalTimeOffset);
  this->emplace(taskInstanceId, aTaskInstance);
  return aTaskInstance.get();
}
/*--------------7 done*/

struct CpuBindingInformation {
  int expectedYieldTime;
  int cpuCoreId;
  CpuBindingInformation(int expectedYieldTime, int cpuCoreId)
      : expectedYieldTime(expectedYieldTime), cpuCoreId(cpuCoreId) {}
};

class TaskInstanceQueue : public std::deque<TaskInstance *> {
public:
  TaskInstanceQueue() = default;
  void paramsReset() noexcept {
    globalLastTime = 0;
    globalSchedulable = true;
  }
  int sumWTB() const noexcept;
  void merge(const TaskInstanceQueue &another);
  void addNewTasks(const TaskInstanceQueue &another, int now);
  bool updateWaitingTimeBudeget(int now);
  void sort() noexcept;
  void dumpTimeOut() const noexcept;
  void dump(bool from_cpu = false) const noexcept;
  TaskInstanceQueue(
      const std::unordered_map<TaskInstance *, CpuBindingInformation> &deque);

  int globalLastTime = 0;
  bool globalSchedulable = true;
};

/*--------------8 done*/

class ExecutionSegmentPool
    : public std::unordered_map<int, std::shared_ptr<ExecutionSegment>> {
public:
  ExecutionSegment *newSeg(int startTime, int endTime, int instanceId,
                           int coreIndex) noexcept;
};

/*--------------9 done*/

class CpuCore {
public:
  /**
   * @brief Construct a new Cpu Core object with given parameter.
   *
   * @param index Core index in a cpu.
   * @param sim_time Simulation time for later simulation.
   */
  CpuCore(int index, int sim_time)
      : startTime_(0),
        expectedYieldTime_(0),
        currentTaskInstance_(nullptr),
        index_(index),
        simulated_time_(sim_time),
        arranged_(sim_time, -1),
        in_use_(false) {}

  /**
   * @brief Clean all the scheduling data in the core. Prepare for next
   * scheduling.
   *
   */
  void reset();

  int yield(int now, ExecutionSegmentPool &ExeSegs) noexcept;
  /**
   * @brief If current core is in use.
   *
   * @return true in use
   * @return false not have scheduling data.
   */
  bool inUse() const { return this->in_use_; }
  int take(TaskInstance *taskInstance, int now) noexcept;

public:
  int startTime_;
  int expectedYieldTime_;
  TaskInstance *currentTaskInstance_;

private:
  int index_;
  int simulated_time_;
  std::vector<int> arranged_;
  bool in_use_;
};

class Cpu : public std::vector<CpuCore> {
public:
  /**
   * @brief Construct a new Cpu object with give parameter.
   *
   * @param cores How many cores
   * @param sim_time Simulation time.
   */
  Cpu(int cores, int sim_time = 0);
  /**
   * @brief Reset every core.
   *
   */
  void reset();
  int nextYieldTime() const noexcept;
  std::vector<TaskInstance *> yield(int now, ExecutionSegmentPool &ExeSegs);
  bool updateWtb(int now) noexcept;
  TaskInstanceQueue giveUp(int now, ExecutionSegmentPool &ExeSegs);
  std::vector<int> available() const noexcept;
  bool swapOutIn(TaskInstanceQueue &out, TaskInstanceQueue &in,
                 ExecutionSegmentPool &ExeSegs, int timeNow) noexcept;
  /**
   * @brief Given index, find the corresponding core iterator.
   *
   * @param core_index core index
   * @return Cpu::iterator
   */
  Cpu::iterator findCore(int core_index);

public:
  std::unordered_map<TaskInstance *, CpuBindingInformation> inExecution_;

private:
  int core_num_;
  int globalLastTime = 0;
};

/*--------------10 done*/
class Simulator {
public:
  Simulator() : LCM_{0} {}

  void init(const TaskBasicInformationPool &taskPool,
            TriggeredDirectedGraphGroup &tddg);
  std::pair<int, TaskInstanceQueue> nextSourceTaskInstance(
      TaskInstancePool &taskInstancePool, TaskBasicInformationPool &taskPool);
  TaskInstanceQueue nextTaskInstance(
      const std::vector<TaskInstance *> &taskInstanceDone, int time,
      TaskInstancePool &taskInstancePool, TaskBasicInformationPool &taskPool,
      TriggeredDirectedGraphGroup &tddg);
  int LCM_;
  std::deque<std::pair<int, std::string>> glabalSourceTaskTime;
};

class LeastWaitingTimeBudgetFirst {
public:
  bool schedulable(TaskInstanceQueue &Queue, Cpu &Cpu,
                   TaskInstancePool &TaskInstances,
                   TaskBasicInformationPool &Tasks,
                   TriggeredDirectedGraphGroup &Tdgg,
                   ExecutionSegmentPool &ExeSegs);
  bool schedule(TaskInstanceQueue &Queue, Cpu &Cpu,
                ExecutionSegmentPool &ExeSegs, int now,
                bool preemptive = true) noexcept;
};
/*--------------11 done*/
bool preCheck(const DirectedGraph &dg,
              const TaskBasicInformationPool &taskPool);
DirectedVertex *maxFrom(DirectedVertex *vertex,
                        TaskBasicInformationPool &taskPool);
/*============================================split*/

}  // namespace exec
}  // namespace gstone
#endif